#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "evaluation.h"
#include "search.h"
#include "hashtable.h"

#include <cstdio>
#include <vector>

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

	// Regression test for pawn moves that don't cause revealed check
	Move moves[256];
	Position position;
	position.Initialize("r1bq1B1r/p1pp1p1p/1pn1p3/4P3/2Pb3k/6R1/P3BPPP/3K2NR w - - 0 2");
	int moveCount = GenerateCheckingMoves(position, moves);
	ASSERT(moveCount == 4);

	// Regression test for search not detecting stalemates
	position.Initialize("1k6/5RP1/1P6/1K6/6r1/8/8/8 w - - 0 1");
	int score;
	Move move = IterativeDeepening(position, 1, score);
	ASSERT(score > 100 && score < 10000);
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
	ASSERT(IsMovePseudoLegal(position, captureMove));
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

	// Knight capturing pawn defended by pawn
	CheckSee("K7/8/2p5/3p4/8/4N3/8/7k w - - 0 1", "e3d5", false);

	// Knight capturing undefended pawn
	CheckSee("K7/8/8/3p4/8/4N3/8/7k w - - 0 1", "e3d5", true);

	// Rook capturing pawn defended by knight
	CheckSee("K7/4n3/8/3p4/8/3R4/3R4/7k w - - 0 1", "d3d5", false);

	// Rook capturing pawn defended by bishop
	CheckSee("K7/5b2/8/3p4/8/3R4/3R4/7k w - - 0 1", "d3d5", false);

	// Rook capturing knight defended by bishop
	CheckSee("K7/5b2/8/3n4/8/3R4/3R4/7k w - - 0 1", "d3d5", true);

	// Rook capturing rook defended by bishop
	CheckSee("K7/5b2/8/3r4/8/3R4/3R4/7k w - - 0 1", "d3d5", true);
}

void MoveSortingTest()
{
	// TODO: test move sorting (might have to factor it a bit better)
}

void DrawTests()
{
	Position position;
	position.Initialize("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	
	Move move[10];
	MoveUndo moveUndo[10];
	
	move[0] = MakeMoveFromUciString("g1f3");
	move[1] = MakeMoveFromUciString("g8f6");
	move[2] = MakeMoveFromUciString("f3g1");
	move[3] = MakeMoveFromUciString("f6g8");

	for (int i = 0; i < 4; i++)
	{
		ASSERT(!position.IsDraw());
		position.MakeMove(move[i], moveUndo[i]);
	}
	ASSERT(position.IsDraw());
	for (int i = 3; i >=0; i--)
	{
		position.UnmakeMove(move[i], moveUndo[i]);
		ASSERT(!position.IsDraw());
	}

	// TODO: maybe a few more tests here?
}

void HashTests()
{
	Position position;
	position.Initialize("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	
	Move move[10];
	MoveUndo moveUndo[10];
	
	move[0] = MakeMoveFromUciString("g1f3");
	move[1] = MakeMoveFromUciString("g8f6");
	move[2] = MakeMoveFromUciString("f3g1");
	move[3] = MakeMoveFromUciString("f6g8");

	ASSERT(HashTable != 0);
	ASSERT(HashMask == (0x3ff & ~3));

	const Move testMove = GenerateMove(1, 1);
	const int testDepth = 5;
	const int testScore = 500;
	const int testFlags = HashFlagsBeta;
	for (int i = 0; i < 4; i++)
	{
		StoreHash(position.Hash, testScore + i, testMove + i, testDepth + i, (testFlags + i) & HashFlagsMask);

		HashEntry *result;
		bool foundHash = ProbeHash(position.Hash, result);
		ASSERT(foundHash);
		ASSERT(result->Score == testScore + i);
		ASSERT(result->Move == testMove + i);
		ASSERT(result->Depth == testDepth + i);
		ASSERT(result->GetHashFlags() == ((testFlags + i) & HashFlagsMask));
		ASSERT(result->GetHashDate() == HashDate);

		position.MakeMove(move[i], moveUndo[i]);
	}

	for (int i = 3; i >= 0; i--)
	{
		position.UnmakeMove(move[i], moveUndo[i]);

		HashEntry *result;
		bool foundHash = ProbeHash(position.Hash, result);
		ASSERT(foundHash);
		ASSERT(result->Score == testScore + i);
		ASSERT(result->Move == testMove + i);
		ASSERT(result->Depth == testDepth + i);
		ASSERT(result->GetHashFlags() == ((testFlags + i) & HashFlagsMask));
		ASSERT(result->GetHashDate() == HashDate);
	}

	// TODO: test hash aging
	// TODO: test hash depth collisions
}

u64 perft(Position &position, int depth)
{
	const bool verifyCheckingMoves = false;

	if (verifyCheckingMoves)
	{
		Move moves[256];
		int moveCount = GenerateLegalMoves(position, moves);

		Move checkingMoves[64];
		int checkingMovesCount = GenerateCheckingMoves(position, checkingMoves);

		for (int i = 0; i < moveCount; i++)
		{
			bool exists = false;
			for (int j = 0; j < checkingMovesCount; j++)
			{
				if (checkingMoves[j] == moves[i])
				{
					exists = true;
					break;
				}
			}

			MoveUndo moveUndo;
			position.MakeMove(moves[i], moveUndo);

			bool isCheck = position.IsInCheck();
			position.UnmakeMove(moves[i], moveUndo);

			if (position.Board[GetTo(moves[i])] == PIECE_NONE &&
				GetMoveType(moves[i]) == MoveTypeNone)
			{
				if (isCheck && !exists)
				{
					std::string fen = position.GetFen();
					const std::string moveString = GetMoveSAN(position, moves[i]);
					ASSERT(false);
					int checkingMovesCount = GenerateCheckingMoves(position, checkingMoves);
				}
				else if (!isCheck && exists)
				{
					std::string fen = position.GetFen();
					const std::string moveString = GetMoveSAN(position, moves[i]);
					ASSERT(false);
					int checkingMovesCount = GenerateCheckingMoves(position, checkingMoves);
				}
			}
		}

		// Test pseudo-legality of checking moves
		for (int i = 0; i < checkingMovesCount; i++)
		{
			ASSERT(IsMovePseudoLegal(position, checkingMoves[i]));
		}
	}

	if (depth == 0) return 1;

	u64 result = 0;

	Move moves[256];
	if (!position.IsInCheck())
	{
		int moveCount = GenerateQuietMoves(position, moves);
		moveCount += GenerateCaptureMoves(position, moves + moveCount);

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
			vCount += GenerateCaptureMoves(position, vMoves + vCount);
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
	InitializeHash(16384);

	UnitTests();
	SeeTests();
	DrawTests();
	HashTests();

/*	Position position;
	position.Initialize("8/Pk6/8/8/8/8/6Kp/8 w - -");
	perft(position, 3);*/

//	RunPerftSuite(5);
}