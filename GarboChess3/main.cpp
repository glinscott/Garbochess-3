#include "garbochess.h"
#include "position.h"
#include "movegen.h"

#include <cstdio>

void PrintBitboard(const Bitboard b)
{
	for (int row = 0; row < 8; row++)
	{
		for (int col = 0; col < 8; col++)
		{
			std::printf("%c", IsBitSet(b, MakeSquare(row, col)) ? 'X' : '.');
		}
		std::printf("\n");
	}
}

void UnitTests()
{
	Bitboard b = 0;
	SetBit(b, 32);
	ASSERT(b == 0x0000000100000000ULL);
	ASSERT(CountBitsSet(b) == 1);

	ClearBit(b, 33);
	ClearBit(b, 31);
	ASSERT(b == 0x0000000100000000ULL);

	ClearBit(b, 32);
	ASSERT(b == 0);
	ASSERT(CountBitsSet(b) == 0);

	b = 0xFFFF;
	ASSERT(GetLowSetBit(b) == 1);
	ASSERT(CountBitsSet(b) == 16);

	b = 0;
	SetBit(b, 15);
	ASSERT(GetLowSetBit(b) == 0x8000);
	ASSERT(GetFirstBitIndex(b) == 15);
	ASSERT(PopFirstBit(b) == 15);
	ASSERT(b == 0);

	b = 0x0001000100010001ULL;
	ASSERT(GetLowSetBit(b) == 1);
	ASSERT(GetFirstBitIndex(b) == 0);
	ASSERT(CountBitsSet(b) == 4);
	ASSERT(PopFirstBit(b) == 0);
	ASSERT(GetLowSetBit(b) == 0x10000);
	ASSERT(GetFirstBitIndex(b) == 16);
	ASSERT(CountBitsSet(b) == 3);

	// TODO: unit test rook attacks and bishop attacks
}

int main()
{
	InitializeBitboards();

	UnitTests();

	Position position;
	position.Initialize("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

	Move moves[200];
	int moveCount = GenerateQuietMoves(position, moves);
	moveCount += GenerateCaptures(position, moves);

	return 0;
}