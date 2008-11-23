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

u64 perft(Position &position, int depth)
{
	if (depth == 0) return 1;

	u64 result = 0;

	Move moves[256];
	int moveCount = GenerateQuietMoves(position, moves);
	moveCount += GenerateCaptures(position, moves + moveCount);

	for (int i = 0; i < moveCount; i++)
	{
		MoveUndo moveUndo;
		position.MakeMove(moves[i], moveUndo);
		if (!position.IsSquareAttacked(position.KingPos[FlipColor(position.ToMove)], position.ToMove))
		{
			result += perft(position, depth - 1);
		}
		position.UnmakeMove(moves[i], moveUndo);
	}

	return result;
}

void RunPerftSuite(int depthToVerify)
{
	std::FILE* file;
	fopen_s(&file, "tests/perftsuite.epd", "rt");

	char line[500];
	while (std::fgets(line, 500, file) != NULL)
	{
		Position position;
		position.Initialize(line);

		bool hasPrinted = false;
		int first = -1;
		for (int i = 0; line[i] != 0; i++)
		{
			if (line[i] == ';')
			{
				if (first == -1) first = i;

				int depth;
				u64 expected;
				sscanf_s(line + i + 2, "%d %lld", &depth, &expected);
				if (depth <= depthToVerify)
				{
					u64 count = perft(position, depth);
					if (count != expected)
					{
						if (!hasPrinted)
						{
							std::string fen = line;
							std::printf("\n%s\n", fen.substr(0, first).c_str());
							hasPrinted = true;
						}
						std::printf("failed depth %d - expected %lld - got %lld\n", depth, expected, count);
					}
					else
					{
						std::printf("%d..", depth);
					}
				}
			}
		}
		printf("\n");
	}

	fclose(file);
}

int main()
{
	InitializeBitboards();
	Position::StaticInitialize();

	UnitTests();

	Position position;
	position.Initialize("4k2r/8/8/8/8/8/8/4K3 w k - 0 1");

	RunPerftSuite(6);

	return 0;
}