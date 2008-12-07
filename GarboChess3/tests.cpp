#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "evaluation.h"
#include "search.h"

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
	ASSERT(CountBitsSet(b) == 16);

	b = 0;
	SetBit(b, 15);
	ASSERT(GetFirstBitIndex(b) == 15);
	ASSERT(PopFirstBit(b) == 15);
	ASSERT(b == 0);

	b = 0x0001000100010001ULL;
	ASSERT(GetFirstBitIndex(b) == 0);
	ASSERT(CountBitsSet(b) == 4);
	ASSERT(PopFirstBit(b) == 0);
	ASSERT(GetFirstBitIndex(b) == 16);
	ASSERT(CountBitsSet(b) == 3);

	// TODO: unit test rook attacks and bishop attacks
	// TODO: unit test move utility functions
	// TODO: unit test square utility functions
	// TODO: unit test Position::GetAttacksTo and Position::GetPinnedPieces
}

Move MakeMoveFromUciString(const std::string &moveString)
{
	const Square from = MakeSquare(RANK_1 - (moveString[1] - '1'), moveString[0] - 'a');
	const Square to = MakeSquare(RANK_1 - (moveString[3] - '1'), moveString[2] - 'a');

	if (moveString.length() == 5)
	{
		int promotionType;
		switch (tolower(moveString[4]))
		{
		case 'n': promotionType = PromotionTypeKnight; break;
		case 'b': promotionType = PromotionTypeBishop; break;
		case 'r': promotionType = PromotionTypeRook; break;
		case 'q': promotionType = PromotionTypeQueen; break;
		}
		return GeneratePromotionMove(from, to, promotionType);
	}

	return GenerateMove(from, to);
}

void CheckSee(const std::string &fen, const std::string &move, bool expected)
{
	Position position;
	
	position.Initialize(fen);
	Move captureMove = MakeMoveFromUciString(move);
	ASSERT(FastSee(position, captureMove) == expected);

	// TODO: implement position.Flip(), and verify that flipped move also gives correct SEE value
}

void SeeTests()
{
	// Winning pawn capture on rook
	CheckSee("2r3k1/2r4p/1PNqb1p1/3p1p2/4p3/2Q1P2P/5PP1/1R4K1 w - - 0 37", "b6c7", true);

	// Winning rook/queen capture on pawn
	CheckSee("2r3k1/2P4p/2Nqb1p1/3p1p2/4p3/2Q1P2P/5PP1/1R4K1 b - - 0 37", "c8c7", true);
	CheckSee("2r3k1/2P4p/2Nqb1p1/3p1p2/4p3/2Q1P2P/5PP1/1R4K1 b - - 0 37", "d6c7", true);

	// Winning rook/queen capture on knight
	CheckSee("6k1/2r4p/2Nqb1p1/3p1p2/4p3/2Q1P2P/5PP1/1R4K1 b - - 0 38", "c7c6", true);
	CheckSee("6k1/2r4p/2Nqb1p1/3p1p2/4p3/2Q1P2P/5PP1/1R4K1 b - - 0 38", "d6c6", true);

	// Losing rook/queen capture on knight (revealed rook attack)
	CheckSee("6k1/2r4p/2Nqb1p1/3p1p2/4p3/2Q1P2P/5PP1/2R3K1 b - - 0 38", "c7c6", false);
	CheckSee("6k1/2r4p/2Nqb1p1/3p1p2/4p3/2Q1P2P/5PP1/2R3K1 b - - 0 38", "d6c6", false);

	// Winning rook/queen capture on knight (revealed bishop attack)
	CheckSee("4b1k1/2rq3p/2N3p1/3p1p2/4p3/2Q1P2P/5PP1/2R3K1 b - - 0 38", "c7c6", true);
	CheckSee("4b1k1/2rq3p/2N3p1/3p1p2/4p3/2Q1P2P/5PP1/2R3K1 b - - 0 38", "d7c6", true);

	// Winning pawn capture on pawn
	CheckSee("2r3k1/2pq3p/3P2p1/b4p2/4p3/2R1P2P/5PP1/2R3K1 w - - 0 38", "d6c7", true);

	// Losing rook capture on pawn
	CheckSee("2r3k1/2pq3p/3P2p1/b4p2/4p3/2R1P2P/5PP1/2R3K1 w - - 0 38", "c3c7", false);

	// Losing queen capture on rook
	CheckSee("2r3k1/2p4p/3P2p1/q4p2/4p3/2R1P2P/5PP1/2R3K1 b - - 0 38", "a5c3", false);

	// Losing rook capture on pawn
	CheckSee("1br3k1/2p4p/3P2p1/q4p2/4p3/2R1P2P/5PP1/2R3K1 w - - 0 38", "c3c7", false);

	// Winning Q promotion (non-capture)
	CheckSee("4rrk1/2P4p/6p1/5p2/4p3/2R1P2P/5PP1/2R3K1 w - - 0 38", "c7c8q", true);

	// Losing Q promotion (non-capture)
	CheckSee("r3rrk1/2P4p/6p1/5p2/4p3/2R1P2P/5PP1/2R3K1 w - - 0 38", "c7c8q", false);
}

void MoveSortingTest()
{
	// TODO: test move sorting (might have to factor it a bit better)
}

u64 perft(Position &position, int depth)
{
	if (depth == 0) return 1;

	u64 result = 0;

	Move moves[256];
	if (!position.IsCheck())
	{
		int moveCount = GenerateQuietMoves(position, moves);
		moveCount += GenerateCaptures(position, moves + moveCount);

		for (int i = 0; i < moveCount; i++)
		{
			MoveUndo moveUndo;
			position.MakeMove(moves[i], moveUndo);
			if (!position.CanCaptureKing())
			{
				result += perft(position, depth - 1);
			}
			position.UnmakeMove(moves[i], moveUndo);
		}
	}
	else
	{
		int moveCount = GenerateCheckEscapeMoves(position, moves);

		const bool verifyCheckEscape = false;
		if (verifyCheckEscape)
		{

			Move vMoves[256];
			int vCount = GenerateQuietMoves(position, vMoves);
			vCount += GenerateCaptures(position, vMoves + vCount);
			for (int i = 0; i < vCount; i++)
			{
				MoveUndo moveUndo;
				position.MakeMove(vMoves[i], moveUndo);
				bool bad = false;
				if (position.CanCaptureKing()) bad = true;
				position.UnmakeMove(vMoves[i], moveUndo);
				if (bad) { vCount--; i--; for (int j = i + 1; j < vCount; j++) vMoves[j] = vMoves[j + 1]; }
			}

			if (vCount != moveCount)
			{
				printf("foobar %d,%d\n%s\n", vCount, moveCount, position.GetFen().c_str());
			}
		}

		for (int i = 0; i < moveCount; i++)
		{
			MoveUndo moveUndo;
			position.MakeMove(moves[i], moveUndo);
			// We shouldn't leave our king hanging
			ASSERT(!position.CanCaptureKing());
			result += perft(position, depth - 1);
			position.UnmakeMove(moves[i], moveUndo);
		}
	}

	return result;
}

void RunPerftSuite(int depthToVerify)
{
	std::FILE* file;
	fopen_s(&file, "tests/perftsuite.epd", "rt");

	char line[500];
	u64 startTime = GetCurrentMilliseconds();
	u64 totalCount = 0;
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
					totalCount += count;
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

	u64 totalTime = GetCurrentMilliseconds() - startTime;
	printf("Total positions: %lld\n", totalCount);
	printf("Seconds taken: %.2lf\n", totalTime / 1000.0);
	printf("NPS: %.2lf\n", (totalCount / (totalTime / 1000.0)));

	fclose(file);
}

void RunTests()
{
	UnitTests();
	SeeTests();

	Position position;
	position.Initialize("r2k3r/p1ppqNb1/bn2pQp1/3P4/1p2P3/2N4p/PPPBBPPP/R3K2R b - - 0 0");
	perft(position, 2);

	RunPerftSuite(5);
}