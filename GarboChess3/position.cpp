#include "garbochess.h"
#include "position.h"
#include "mersenne.h"

#include <string>

static MTRand Random;
static u64 GetRand64()
{
	return u64(Random.randInt()) | (u64(Random.randInt()) << 32);
}

u64 Position::zobrist[2][8][64];
u64 Position::zobristEP[64];
u64 Position::zobristCastle[16];
u64 Position::zobristToMove;

void Position::StaticInitialize()
{
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 8; j++)
			for (int k = 0; k < 64; k++)
				Position::zobrist[i][j][k] = GetRand64();

	for (int i = 0; i < 64; i++)
		Position::zobristEP[i] = GetRand64();

	for (int i = 0; i < 16; i++)
		Position::zobristCastle[i] = GetRand64();

	Position::zobristToMove = GetRand64();
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

		EnPassent = MakeSquare(7 - (row - 'a'), column - '1');
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
			result ^= Position::zobrist[color][piece][square];
		}
	}

	result ^= Position::zobristCastle[CastleFlags];

	if (EnPassent != -1)
	{
		result ^= Position::zobristEP[EnPassent];
	}

	if (ToMove == BLACK)
	{
		result ^= Position::zobristToMove;
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
		result ^= Position::zobrist[color][PAWN][square];
	}

	return result;
}

void Position::MakeMove(const Move move)
{
	const Color us = ToMove;
	const Color them = FlipColor(us);

	const Square from = GetFrom(move);
	const Square to = GetTo(move);

	const PieceType piece = GetPieceType(Board[from]);
	const PieceType target = GetPieceType(Board[to]);

	ASSERT(GetPieceColor(Board[from]) == us);
	ASSERT(GetPieceColor(Board[to]) == them || Board[to] == PIECE_NONE);

	// Remove the piece from where it was standing
	XorClearBit(Pieces[piece], from);
	XorClearBit(Colors[us], from);

	const Move moveFlags = move & MoveTypeFlags;
	// No flags?
	if (!moveFlags)
	{
		// Then normal move
		Fifty++;

		if (target != PIECE_NONE)
		{
			ASSERT(target != KING);

			XorClearBit(Pieces[target], to);
			XorClearBit(Colors[them], to);
		}

		SetBit(Pieces[piece], to);
		SetBit(Colors[us], to);

		Board[to] = Board[from];
		Board[from] = PIECE_NONE;

		if (piece == KING)
		{
			KingPos[us] = to;
		}
	}
	else
	{
		// All special moves are non-reversible
		Fifty = 0;

		if (moveFlags == MoveTypePromotion)
		{
		}
		else if (moveFlags == MoveTypeCastle)
		{
		}
		else if (moveFlags == MoveTypeEnPassent)
		{
		}
	}
}

void Position::UnmakeMove(const Move move)
{
}