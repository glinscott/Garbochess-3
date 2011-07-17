#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "mersenne.h"
#include "evaluation.h"

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

	if (at < (int)fen.size() && isdigit(fen[at]))
	{
		// Fifty move clock
		Fifty = atoi(fen.c_str() + at);
	}
	else
	{
		Fifty = 0;
	}

	MoveDepth = 0;

	for (int i = 0; i < Fifty; i++)
	{
		DrawKeys[i] = 0;
	}

	Hash = GetHash();
	PawnHash = GetPawnHash();

	PsqEvalOpening = GetPsqEval(0);
	PsqEvalEndgame = GetPsqEval(1);
}

std::string Position::GetFen() const
{
	std::string result;

	for (int row = RANK_8; row <= RANK_1; row++)
	{
		if (row != RANK_8)
		{
			result += '/';
		}
		
		int empty = 0;
		for (int column = FILE_A; column <= FILE_H; column++)
		{
			Piece piece = Board[MakeSquare(row, column)];
			if (piece == PIECE_NONE)
			{
				empty++;
			}
			else
			{
				if (empty != 0)
				{
					result += (empty + '0');
				}
				empty = 0;

				char pieceChar;
				PieceType pieceType = GetPieceType(piece);
				switch (pieceType)
				{
				case PAWN: pieceChar = 'p'; break;
				case KNIGHT: pieceChar = 'n'; break;
				case BISHOP: pieceChar = 'b'; break;
				case ROOK: pieceChar = 'r'; break;
				case QUEEN: pieceChar = 'q'; break;
				case KING: pieceChar = 'k'; break;
				}

				if (GetPieceColor(piece) == WHITE) pieceChar += 'A' - 'a';

				result += pieceChar;
			}
		}

		if (empty != 0)
		{
			result += (empty + '0');
		}
	}

	// ToMove
	if (ToMove == WHITE) result += " w"; else result += " b";
	
	// TODO: castle flags, e.p.
	result += " - - 0 0";

	return result;
}

void Position::Clone(Position &other) const
{
	// This should probably be cleaner
	memcpy(&other, this, sizeof(Position));

#if _DEBUG
	other.VerifyBoard();
#endif
}

void Position::Flip()
{
	ToMove = FlipColor(ToMove);

	for (int row = 0; row < 4; row++)
	{
		for (int column = 0; column < 8; column++)
		{
			const Square square = MakeSquare(row, column);
			Piece tmpP = Board[square];
			Board[square] = Board[FlipSquare(square)];
			Board[FlipSquare(square)] = tmpP;
		}
	}

	for (int i = 0; i < 8; i++) Pieces[i] = 0;
	for (int i = 0; i < 2; i++) Colors[i] = 0;

	for (int row = 0; row < 8; row++)
	{
		for (int column = 0; column < 8; column++)
		{
			const Square square = MakeSquare(row, column);
			if (Board[square] != PIECE_NONE)
			{
				const Color color = FlipColor(GetPieceColor(Board[square]));
				const PieceType pieceType = GetPieceType(Board[square]);
				Board[square] = MakePiece(color, pieceType);
				SetBit(Colors[color], square);
				SetBit(Pieces[pieceType], square);

				if (pieceType == KING)
				{
					KingPos[color] = square;
				}
			}
		}
	}

	if (EnPassent != -1)
	{
		EnPassent = FlipSquare(EnPassent);
	}

	int castleTmp = CastleFlags & 0x3;
	CastleFlags = ((CastleFlags >> 2) & 3) | (castleTmp << 2);

	Hash = GetHash();
	PawnHash = GetPawnHash();

	PsqEvalOpening = GetPsqEval(0);
	PsqEvalEndgame = GetPsqEval(1);

#if _DEBUG
	VerifyBoard();
#endif
}

void Position::MakeMove(const Move move, MoveUndo &moveUndo)
{
	ASSERT(IsMovePseudoLegal((const Position&)*this, move));

	const Color us = ToMove;
	const Color them = FlipColor(us);

	const Square from = GetFrom(move);
	const Square to = GetTo(move);

	const Piece boardFrom = Board[from];
	const Piece boardTo = Board[to];

	const PieceType piece = GetPieceType(boardFrom);
	const PieceType target = GetPieceType(boardTo);

	ASSERT(GetPieceColor(Board[from]) == us);
	ASSERT(GetPieceColor(Board[to]) == them || Board[to] == PIECE_NONE);

	moveUndo.Hash = Hash;
	moveUndo.PawnHash = PawnHash;
	moveUndo.CastleFlags = CastleFlags;
	moveUndo.EnPassent = EnPassent;
	moveUndo.Fifty = Fifty;
	moveUndo.Captured = target;

	DrawKeys[MoveDepth++] = Hash;
	ASSERT(MoveDepth < 256);

	if (EnPassent != -1)
	{
		Hash ^= Position::ZobristEP[EnPassent];
		EnPassent = -1;
	}

	const Move moveFlags = GetMoveType(move);

	if (target != PIECE_NONE)
	{
		ASSERT(target != KING);
		ASSERT(IsBitSet(Pieces[target], to));
		ASSERT(IsBitSet(Colors[them], to));

		Fifty = 0;

		XorClearBit(Pieces[target], to);
		XorClearBit(Colors[them], to);

		Hash ^= Position::Zobrist[them][target][to];

		PsqEvalOpening -= PsqTableOpening[boardTo][to];
		PsqEvalEndgame -= PsqTableEndgame[boardTo][to];

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
	else
	{
		Fifty++;
	}

	// Remove the piece from where it was standing
	XorClearBit(Pieces[piece], from);
	XorClearBit(Colors[us], from);

	SetBit(Pieces[piece], to);
	SetBit(Colors[us], to);

	PsqEvalOpening += PsqTableOpening[boardFrom][to] - PsqTableOpening[boardFrom][from];
	PsqEvalEndgame += PsqTableEndgame[boardFrom][to] - PsqTableEndgame[boardFrom][from];

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

			ASSERT(Board[to] == MakePiece(us, PAWN));

			XorClearBit(Pieces[PAWN], to);
			SetBit(Pieces[promotionType], to);

			const Piece promotedPiece = MakePiece(us, promotionType);

			PsqEvalOpening += PsqTableOpening[promotedPiece][to] - PsqTableOpening[Board[to]][to];
			PsqEvalEndgame += PsqTableEndgame[promotedPiece][to] - PsqTableEndgame[Board[to]][to];

			Board[to] = promotedPiece;

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

			PsqEvalOpening -= PsqTableOpening[Board[epSquare]][epSquare];
			PsqEvalEndgame -= PsqTableEndgame[Board[epSquare]][epSquare];

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

				PsqEvalOpening += PsqTableOpening[Board[rookTo]][rookTo] - PsqTableOpening[Board[rookTo]][rookFrom];
				PsqEvalEndgame += PsqTableEndgame[Board[rookTo]][rookTo] - PsqTableEndgame[Board[rookTo]][rookFrom];
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

	MoveDepth--;

	ToMove = us;

	// Restore the piece to its original location
	XorClearBit(Pieces[piece], to);
	XorClearBit(Colors[us], to);

	SetBit(Pieces[piece], from);
	SetBit(Colors[us], from);

	Board[from] = Board[to];

	const Move moveFlags = GetMoveType(move);
	if (piece == KING)
	{
		KingPos[us] = from;

		if (moveFlags == MoveTypeCastle)
		{
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

			PsqEvalOpening += PsqTableOpening[Board[rookFrom]][rookFrom] - PsqTableOpening[Board[rookFrom]][rookTo];
			PsqEvalEndgame += PsqTableEndgame[Board[rookFrom]][rookFrom] - PsqTableEndgame[Board[rookFrom]][rookTo];
		}
	}
	else if (moveFlags != MoveTypeNone)
	{
		if (moveFlags == MoveTypePromotion)
		{
			const PieceType promotionType = GetPromotionMoveType(move);

			XorClearBit(Pieces[promotionType], from);
			SetBit(Pieces[PAWN], from);

			const Piece unpromotedPawn = MakePiece(us, PAWN);
			Board[from] = unpromotedPawn;

			PsqEvalOpening += PsqTableOpening[unpromotedPawn][to] - PsqTableOpening[Board[to]][to];
			PsqEvalEndgame += PsqTableEndgame[unpromotedPawn][to] - PsqTableEndgame[Board[to]][to];
		}
		else if (moveFlags == MoveTypeEnPassent)
		{
			const Square epSquare = (to - from < 0) ? to + 8 : to - 8;

			SetBit(Pieces[PAWN], epSquare);
			SetBit(Colors[them], epSquare);
			Board[epSquare] = MakePiece(them, PAWN);

			PsqEvalOpening += PsqTableOpening[Board[epSquare]][epSquare];
			PsqEvalEndgame += PsqTableEndgame[Board[epSquare]][epSquare];
		}
	}

	if (moveUndo.Captured != PIECE_NONE)
	{
		// Restore the rest of the captured pieces state
		SetBit(Pieces[moveUndo.Captured], to);
		SetBit(Colors[them], to);

		const Piece restoredPiece = MakePiece(them, moveUndo.Captured);
		Board[to] = restoredPiece;

		PsqEvalOpening += PsqTableOpening[restoredPiece][to];
		PsqEvalEndgame += PsqTableEndgame[restoredPiece][to];
	}
	else
	{
		Board[to] = PIECE_NONE;
	}

	const Piece boardFrom = Board[from];

	PsqEvalOpening += PsqTableOpening[boardFrom][from] - PsqTableOpening[boardFrom][to];
	PsqEvalEndgame += PsqTableEndgame[boardFrom][from] - PsqTableEndgame[boardFrom][to];

#if _DEBUG
	VerifyBoard();
#endif
}

void Position::MakeNullMove(MoveUndo &moveUndo)
{
	ASSERT(!IsInCheck());

	moveUndo.EnPassent = EnPassent;

	DrawKeys[MoveDepth++] = Hash;
	ASSERT(MoveDepth < 256);

	Fifty++;

	if (EnPassent != -1)
	{
		Hash ^= Position::ZobristEP[EnPassent];
		EnPassent = -1;
	}

	Hash ^= Position::ZobristToMove;
	ToMove = FlipColor(ToMove);

#if _DEBUG
	VerifyBoard();
#endif
}

void Position::UnmakeNullMove(MoveUndo &moveUndo)
{
	ASSERT(!IsInCheck());

	MoveDepth--;
	Fifty--;

	EnPassent = moveUndo.EnPassent;

	if (EnPassent != -1)
	{
		Hash ^= Position::ZobristEP[EnPassent];
	}

	Hash ^= Position::ZobristToMove;
	ToMove = FlipColor(ToMove);

#if _DEBUG
	VerifyBoard();
#endif
}

// Returns all of the pieces (from both colors) that attack the given square
// Can be used to determine pinned pieces, or checking pieces
Bitboard Position::GetAttacksTo(const Square square) const
{
	const Bitboard allPieces = GetAllPieces();
	return
		(((GetPawnAttacks(square, WHITE) & Colors[BLACK]) | (GetPawnAttacks(square, BLACK) & Colors[WHITE])) & Pieces[PAWN]) |
		(GetKnightAttacks(square) & Pieces[KNIGHT]) |
		(GetBishopAttacks(square, allPieces) & (Pieces[BISHOP] | Pieces[QUEEN])) |
		(GetRookAttacks(square, allPieces) & (Pieces[ROOK] | Pieces[QUEEN])) |
		(GetKingAttacks(square) & Pieces[KING]);
}

// Gets all the pieces that are pinned to a given square.  Pinned is defined as being of our color, and
// blocking an enemy piece from capturing the piece at the given square.
Bitboard Position::GetPinnedPieces(const Square square, const Color us) const
{
	// We do this by determining all the enemy pieces that can potentially be attacking us along the attacking
	// files, then intersecting their attacks with our "semi-pinned" pieces.  Semi-pinned is defined as being
	// the first friendly piece along the direction.
	Bitboard pinned = 0;

	const Bitboard allPieces = GetAllPieces();
	const Color them = FlipColor(us);

	const Bitboard semiRookPinned = GetRookAttacks(square, allPieces) & Colors[us];
	Bitboard b = GetRookAttacks(square, 0) & Colors[them] & (Pieces[ROOK] | Pieces[QUEEN]);
	while (b)
	{
		Square from = PopFirstBit(b);
		pinned |= GetRookAttacks(from, allPieces) & semiRookPinned;
	}

	const Bitboard semiBishopPinned = GetBishopAttacks(square, allPieces) & Colors[us];
	b = GetBishopAttacks(square, 0) & Colors[them] & (Pieces[BISHOP] | Pieces[QUEEN]);
	while (b)
	{
		Square from = PopFirstBit(b);
		pinned |= GetBishopAttacks(from, allPieces) & semiBishopPinned;
	}

	return pinned;
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

int Position::GetPsqEval(int gameStage) const
{
	int result = 0;
	for (Square square = 0; square < 64; square++)
	{
		if (Board[square] != PIECE_NONE)
		{
			result += gameStage == 0 ? PsqTableOpening[Board[square]][square] : PsqTableEndgame[Board[square]][square];
		}
	}

	return result;
}

void Position::VerifyBoard() const
{
	const bool verifyBoard = false;
	const bool verifyHash = false;
	const bool verifyPsqEval = false;

	if (verifyBoard)
	{
		for (int i = 0; i < 64; i++)
		{
			const PieceType pieceType = GetPieceType(Board[i]);
			const Color color = GetPieceColor(Board[i]);

			for (int j = PAWN; j <= QUEEN; j++)
			{
				if (pieceType == j)
				{
					ASSERT(IsBitSet(Pieces[j], i));
				}
				else
				{
					ASSERT(!IsBitSet(Pieces[j], i));
				}
			}

			if (pieceType == PIECE_NONE)
			{
				ASSERT(!IsBitSet(GetAllPieces(), i));
			}
			else
			{
				ASSERT(IsBitSet(Colors[color], i));
				ASSERT(!IsBitSet(Colors[FlipColor(color)], i));

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

	if (verifyPsqEval)
	{
		ASSERT(PsqEvalOpening == GetPsqEval(0));
		ASSERT(PsqEvalEndgame == GetPsqEval(1));
	}
}