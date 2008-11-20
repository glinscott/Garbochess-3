#include "garbochess.h"
#include "position.h"

#include <string>

void Position::Initialize(const std::string &fen)
{
	Hash = PawnHash = 0;
	for (int i = 0; i < 8; i++) Pieces[i] = 0;
	for (int i = 0; i < 2; i++) Colors[i] = 0;

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
			
			Piece piece;
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
	return 0;
}

u64 Position::GetPawnHash() const
{
	return 0;
}

void Position::MakeMove(const Move move)
{
	
}

void Position::UnmakeMove(const Move move)
{
}