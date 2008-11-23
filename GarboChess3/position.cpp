#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "mersenne.h"

#include <string>

static MTRand Random;
static u64 GetRand64()
{
	return u64(Random.randInt()) | (u64(Random.randInt()) << 32);
}

int Position::RookCastleFlagMask[64];
u64 Position::Zobrist[2][8][64];
u64 Position::ZobristEP[64];
u64 Position::ZobristCastle[16];
u64 Position::ZobristToMove;

void Position::StaticInitialize()
{
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 8; j++)
			for (int k = 0; k < 64; k++)
				Position::Zobrist[i][j][k] = GetRand64();

	for (int i = 0; i < 64; i++)
	{
		Position::ZobristEP[i] = GetRand64();
		RookCastleFlagMask[i] = CastleFlagMask;
	}

	RookCastleFlagMask[MakeSquare(RANK_1, FILE_A)] ^= CastleFlagWhiteQueen;
	RookCastleFlagMask[MakeSquare(RANK_1, FILE_H)] ^= CastleFlagWhiteKing;
	RookCastleFlagMask[MakeSquare(RANK_8, FILE_A)] ^= CastleFlagBlackQueen;
	RookCastleFlagMask[MakeSquare(RANK_8, FILE_H)] ^= CastleFlagBlackKing;

	for (int i = 0; i < 16; i++)
		Position::ZobristCastle[i] = GetRand64();

	Position::ZobristToMove = GetRand64();
}

void Position::Initialize(const std::string &fen)
{
	Hash = PawnHash = 0;
	for (int i = 0; i < 8; i++) Pieces[i] = 0;
	for (int i = 0; i < 2; i++) Colors[i] = 0;
	for (int i = 0; i < 64; i++) Board[i] = 0;

	// Board
	int at, row = RANK_8, column = FILE_A;
	for (at = 0; fen[at] != ' '; at++)
	{
		if (fen[at] == '/')
		{
			row++;
			column = FILE_A;
		}
		else if (isdigit(fen[at]))
		{
			column += fen[at] - '0';
		}
		else
		{
			Color color = islower(fen[at]) ? BLACK : WHITE;
			
			PieceType piece;
			switch (tolower(fen[at]))
			{
			case 'p': piece = PAWN; break;
			case 'n': piece = KNIGHT; break;
			case 'b': piece = BISHOP; break;
			case 'r': piece = ROOK; break;
			case 'q': piece = QUEEN; break;
			case 'k': piece = KING; break;
			}

			Square square = MakeSquare(row, column);
			SetBit(Pieces[piece], square);
			SetBit(Colors[color], square);
			Board[square] = MakePiece(color, piece);

			if (piece == KING)
			{
				KingPos[color] = MakeSquare(row, column);
			}

			column++;
		}
	}
	at++;

	// To move
	ToMove = fen[at++] == 'w' ? WHITE : BLACK;
	at++;

	// Castle rights
	CastleFlags = 0;
	for (; fen[at] != ' '; at++)
	{
		switch (fen[at])
		{
		case 'K': CastleFlags |= CastleFlagWhiteKing; break;
		case 'Q': CastleFlags |= CastleFlagWhiteQueen; break;
		case 'k': CastleFlags |= CastleFlagBlackKing; break;
		case 'q': CastleFlags |= CastleFlagBlackQueen; break;
		}
	}
	at++;

	// E.P.
	EnPassent = -1;
	if (fen[at++] != '-')
	{
		char column = fen[at - 1];
		char row = fen[at++];

		EnPassent = MakeSquare(7 - (row - '1'), column - 'a');
	}
	at++;

	// Fifty move clock
	Fifty = atoi(fen.c_str() + at);

	Hash = GetHash();
	PawnHash = GetPawnHash();
}

u64 Position::GetHash() const
{
	u64 result = 0;

	for (PieceType piece = PAWN; piece <= KING; piece++)
	{
		Bitboard b = Pieces[piece];
		while (b)
		{
			const Square square = PopFirstBit(b);
			const Color color = IsBitSet(Colors[WHITE], square) ? WHITE : BLACK;
			result ^= Position::Zobrist[color][piece][square];
		}
	}

	result ^= Position::ZobristCastle[CastleFlags];

	if (EnPassent != -1)
	{
		result ^= Position::ZobristEP[EnPassent];
	}

	if (ToMove == BLACK)
	{
		result ^= Position::ZobristToMove;
	}

	return result;
}

u64 Position::GetPawnHash() const
{
	u64 result = 0;

	Bitboard b = Pieces[PAWN];
	while (b)
	{
		const Square square = PopFirstBit(b);
		const Color color = IsBitSet(Colors[WHITE], square) ? WHITE : BLACK;
		result ^= Position::Zobrist[color][PAWN][square];
	}

	return result;
}

void Position::MakeMove(const Move move, MoveUndo &moveUndo)
{
	const Color us = ToMove;
	const Color them = FlipColor(us);

	const Square from = GetFrom(move);
	const Square to = GetTo(move);

	const PieceType piece = GetPieceType(Board[from]);
	const PieceType target = GetPieceType(Board[to]);

	ASSERT(GetPieceColor(Board[from]) == us);
	ASSERT(GetPieceColor(Board[to]) == them || Board[to] == PIECE_NONE);

	moveUndo.Hash = Hash;
	moveUndo.PawnHash = PawnHash;
	moveUndo.CastleFlags = CastleFlags;
	moveUndo.EnPassent = EnPassent;
	moveUndo.Fifty = Fifty;
	moveUndo.Captured = target;

	if (EnPassent != -1)
	{
		Hash ^= Position::ZobristEP[EnPassent];
		EnPassent = -1;
	}

	const Move moveFlags = move & MoveTypeMask;
	Fifty++;

	if (target != PIECE_NONE)
	{
		ASSERT(target != KING);
		ASSERT(IsBitSet(Pieces[target], to));
		ASSERT(IsBitSet(Colors[them], to));

		Fifty = 0;

		XorClearBit(Pieces[target], to);
		XorClearBit(Colors[them], to);

		Hash ^= Position::Zobrist[them][target][to];
		if (target == PAWN)
		{
			PawnHash ^= Position::Zobrist[them][PAWN][to];
		}
		else if (target == ROOK && CastleFlags)
		{
			Hash ^= Position::ZobristCastle[CastleFlags];
			CastleFlags &= RookCastleFlagMask[to];
			Hash ^= Position::ZobristCastle[CastleFlags];
		}
	}

	// Remove the piece from where it was standing
	XorClearBit(Pieces[piece], from);
	XorClearBit(Colors[us], from);

	SetBit(Pieces[piece], to);
	SetBit(Colors[us], to);

	Board[to] = Board[from];
	Board[from] = PIECE_NONE;

	Hash ^= Position::Zobrist[us][piece][from] ^ Position::Zobrist[us][piece][to];

	if (piece == PAWN)
	{
		Fifty = 0;

		if (!moveFlags)
		{
			PawnHash ^= Position::Zobrist[us][PAWN][from] ^ Position::Zobrist[us][PAWN][to];

			if (abs(to - from) == 16)
			{
				ASSERT((us == WHITE && GetRow(from) == RANK_2) || (us == BLACK && GetRow(from) == RANK_7));

				EnPassent = (to - from < 0) ? from - 8 : from + 8;
				Hash ^= Position::ZobristEP[EnPassent];
			}
		}
		else if (moveFlags == MoveTypePromotion)
		{
			// Promotion is a bit interesting, as we've already assumed the pawn moved to the new square.  So, we have to replace
			// the pawn at the destination square with the promoted piece.
			const PieceType promotionType = GetPromotionMoveType(move);

			XorClearBit(Pieces[PAWN], to);
			SetBit(Pieces[promotionType], to);

			Board[to] = MakePiece(us, promotionType);

			PawnHash ^= Position::Zobrist[us][PAWN][from];
			Hash ^= Position::Zobrist[us][PAWN][to] ^ Position::Zobrist[us][promotionType][to];
		}
		else if (moveFlags == MoveTypeEnPassent)
		{
			const Square epSquare = (to - from < 0) ? to + 8 : to - 8;

			ASSERT(Board[epSquare] == MakePiece(them, PAWN));
			ASSERT(IsBitSet(Pieces[PAWN] & Colors[them], epSquare));

			XorClearBit(Pieces[PAWN], epSquare);
			XorClearBit(Colors[them], epSquare);
			Board[epSquare] = PIECE_NONE;

			PawnHash ^= Position::Zobrist[us][PAWN][from] ^ Position::Zobrist[us][PAWN][to] ^ Position::Zobrist[them][PAWN][epSquare];
			Hash ^= Position::Zobrist[them][PAWN][epSquare];
		}
	}
	else if (piece == KING)
	{
		KingPos[us] = to;

		if (CastleFlags)
		{
			Hash ^= Position::ZobristCastle[CastleFlags];
			CastleFlags &= ~(us == WHITE ? (CastleFlagWhiteKing | CastleFlagWhiteQueen) : (CastleFlagBlackKing | CastleFlagBlackQueen));
			Hash ^= Position::ZobristCastle[CastleFlags];

			if (moveFlags == MoveTypeCastle)
			{
				Fifty = 0;

				const int kingRow = GetRow(from);
				ASSERT((us == WHITE && kingRow == RANK_1) || (us == BLACK && kingRow == RANK_8));

				Square rookFrom, rookTo;
				if (GetColumn(to) == FILE_G)
				{
					rookFrom = MakeSquare(kingRow, FILE_H);
					rookTo = MakeSquare(kingRow, FILE_F);
				}
				else
				{
					ASSERT(GetColumn(to) == FILE_C);
					rookFrom = MakeSquare(kingRow, FILE_A);
					rookTo = MakeSquare(kingRow, FILE_D);
				}

				XorClearBit(Pieces[ROOK], rookFrom);
				XorClearBit(Colors[us], rookFrom);

				SetBit(Pieces[ROOK], rookTo);
				SetBit(Colors[us], rookTo);

				Board[rookTo] = Board[rookFrom];
				Board[rookFrom] = PIECE_NONE;

				Hash ^= Position::Zobrist[us][ROOK][rookFrom] ^ Position::Zobrist[us][ROOK][rookTo];
			}
		}
	}
	else if (piece == ROOK && CastleFlags)
	{
		Hash ^= Position::ZobristCastle[CastleFlags];
		CastleFlags &= RookCastleFlagMask[from];
		Hash ^= Position::ZobristCastle[CastleFlags];
	}

	// Flip side to move
	Hash ^= Position::ZobristToMove;
	ToMove = them;

#if _DEBUG
	VerifyBoard();
#endif
}

void Position::UnmakeMove(const Move move, const MoveUndo &moveUndo)
{
	const Color them = ToMove;
	const Color us = FlipColor(them);

	const Square from = GetFrom(move);
	const Square to = GetTo(move);

	const PieceType piece = GetPieceType(Board[to]);

	ASSERT(GetPieceColor(Board[to]) == us);
	ASSERT(Board[from] == PIECE_NONE);

	Hash = moveUndo.Hash;
	PawnHash = moveUndo.PawnHash;
	CastleFlags = moveUndo.CastleFlags;
	EnPassent = moveUndo.EnPassent;
	Fifty = moveUndo.Fifty;

	ToMove = us;

	// Restore the piece to its original location
	XorClearBit(Pieces[piece], to);
	XorClearBit(Colors[us], to);

	SetBit(Pieces[piece], from);
	SetBit(Colors[us], from);

	Board[from] = Board[to];

	const Move moveFlags = move & MoveTypeMask;
	if (piece == KING)
	{
		KingPos[us] = from;

		if (moveFlags == MoveTypeCastle)
		{
			Fifty = 0;

			const int kingRow = GetRow(from);
			ASSERT((us == WHITE && kingRow == RANK_1) || (us == BLACK && kingRow == RANK_8));

			Square rookFrom, rookTo;
			if (GetColumn(to) == FILE_G)
			{
				rookFrom = MakeSquare(kingRow, FILE_H);
				rookTo = MakeSquare(kingRow, FILE_F);
			}
			else
			{
				ASSERT(GetColumn(to) == FILE_C);
				rookFrom = MakeSquare(kingRow, FILE_A);
				rookTo = MakeSquare(kingRow, FILE_D);
			}

			SetBit(Pieces[ROOK], rookFrom);
			SetBit(Colors[us], rookFrom);

			XorClearBit(Pieces[ROOK], rookTo);
			XorClearBit(Colors[us], rookTo);

			Board[rookFrom] = Board[rookTo];
			Board[rookTo] = PIECE_NONE;
		}
	}
	else if (moveFlags == MoveTypePromotion)
	{
		const PieceType promotionType = GetPromotionMoveType(move);

		XorClearBit(Pieces[promotionType], from);
		SetBit(Pieces[PAWN], from);

		Board[from] = MakePiece(us, PAWN);
	}
	else if (moveFlags == MoveTypeEnPassent)
	{
		const Square epSquare = (to - from < 0) ? to + 8 : to - 8;

		SetBit(Pieces[PAWN], epSquare);
		SetBit(Colors[them], epSquare);
		Board[epSquare] = MakePiece(them, PAWN);
	}

	if (moveUndo.Captured != PIECE_NONE)
	{
		// Restore the rest of the captured pieces state
		SetBit(Pieces[moveUndo.Captured], to);
		SetBit(Colors[them], to);
	
		Board[to] = MakePiece(them, moveUndo.Captured);
	}
	else
	{
		Board[to] = PIECE_NONE;
	}

#if _DEBUG
	VerifyBoard();
#endif
}

bool Position::IsSquareAttacked(const Square square, const Color them) const
{
	if ((GetPawnAttacks(square, FlipColor(them)) & Pieces[PAWN] & Colors[them]) ||
		(GetKnightAttacks(square) & Pieces[KNIGHT] & Colors[them]))
	{
		return true;
	}

	const Bitboard allPieces = Colors[WHITE] | Colors[BLACK];
	const Bitboard bishopQueen = Pieces[BISHOP] | Pieces[QUEEN];
	if (GetBishopAttacks(square, allPieces) & bishopQueen & Colors[them])
	{
		return true;
	}

	const Bitboard rookQueen = Pieces[ROOK] | Pieces[QUEEN];
	if (GetRookAttacks(square, allPieces) & rookQueen & Colors[them])
	{
		return true;
	}

	if (GetKingAttacks(square) & Pieces[KING] & Colors[them])
	{
		return true;
	}

	return false;
}

void Position::VerifyBoard() const
{
	const bool verifyBoard = true;
	const bool verifyHash = true;

	if (verifyBoard)
	{
		for (int i = 0; i < 64; i++)
		{
			const PieceType pieceType = GetPieceType(Board[i]);
			const Color color = GetPieceColor(Board[i]);

			if (pieceType == PIECE_NONE)
			{
				ASSERT(!IsBitSet(Pieces[pieceType], i));
				ASSERT(!IsBitSet(Colors[WHITE] | Colors[BLACK], i));
			}
			else
			{
				ASSERT(IsBitSet(Pieces[pieceType], i));
				ASSERT(IsBitSet(Colors[color], i));

				if (pieceType == KING)
				{
					ASSERT(KingPos[color] == i);
				}
			}
		}
	}

	if (verifyHash)
	{
		ASSERT(Hash == GetHash());
		ASSERT(PawnHash == GetPawnHash());
	}
}