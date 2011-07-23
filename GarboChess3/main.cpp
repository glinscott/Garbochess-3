#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "evaluation.h"
#include "search.h"
#include "hashtable.h"
#include "utilities.h"

#include <cstdlib>

void RunTests();

// Hashtable definitions
HashEntry *HashTable = 0;
u64 HashMask = 0;
int HashDate = 0;

void InitializeHash(int hashSize)
{
	for (HashMask = 1; HashMask < (hashSize / sizeof(HashEntry)); HashMask *= 2);
	HashMask /= 2;
	HashMask--;

	if (HashTable)
	{
		free(HashTable);
	}
	size_t allocSize = (size_t)(HashMask * sizeof(HashEntry));
	HashTable = (HashEntry*)malloc(allocSize);
	memset(HashTable, 0, allocSize);

	// Minor speed optimization, so we don't need to mask this out when we access the hash-table
	HashMask &= ~3;
}

void IncrementHashDate()
{
	ASSERT(HashDate <= 0xf);
	HashDate = (HashDate + 1) & 0xf;
}

std::string ReadLine()
{
	char line[16384];
    std::gets(line);
	return std::string(line);
} 

Position GamePosition;

void ReadCommand()
{
	const std::string line = ReadLine();
	const std::vector<std::string> tokens = tokenize(line, " ");
	if (tokens.size() == 0)
	{
		return;
	}

	const std::string command = tokens[0];

	if (command == "uci")
	{
		printf("id name GarboChess 3");
#ifndef X64
		printf(" (32-bit)\n");
#else
		printf("\n");
#endif
		printf("id author Gary Linscott\n");
		printf("uciok\n");
	}
	else if (command == "isready")
	{
		printf("readyok\n");
	}
	else if (command == "ucinewgame")
	{
		// TODO: clear hash
	}
	else if (command == "position")
	{
		std::string fen;
		if (tokens[1] == "fen")
		{
			fen = tokens[2] + tokens[3] + tokens[4] + tokens[5] + tokens[6] + tokens[7];
		}
		else
		{
			fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		}

		GamePosition.Initialize(fen);

		for (int i = 2; i < (int)tokens.size(); i++)
		{
			if (tokens[i] == "moves")
			{
				for (int j = i + 1; j < (int)tokens.size(); j++)
				{
					MoveUndo moveUndo;
					GamePosition.MakeMove(MakeMoveFromUciString(GamePosition, tokens[j]), moveUndo);

					// Make sure to reset our move depth if we have made a non-reversible move.  This can only be done for
					// moves we know we won't undo though, which is why we have this special case outside of the search.
					if (GamePosition.Fifty == 0)
					{
						GamePosition.ResetMoveDepth();
					}
				}
				break;
			}
		}
	}
	else if (command == "go")
	{
		int wtime = -1, btime = -1, winc = -1, binc = -1, depth = -1, movetime = -1;
		bool infinite = false;
		for (int i = 1; i < (int)tokens.size(); i++)
		{
			if (tokens[i] == "wtime")
			{
				wtime = atoi(tokens[++i].c_str());
			}
			else if (tokens[i] == "btime")
			{
				btime = atoi(tokens[++i].c_str());
			}
			else if (tokens[i] == "winc")
			{
				winc = atoi(tokens[++i].c_str());
			}
			else if (tokens[i] == "binc")
			{
				binc = atoi(tokens[++i].c_str());
			}
			else if (tokens[i] == "depth")
			{
				// TODO: actually support depth
				depth = atoi(tokens[++i].c_str());
			}
			else if (tokens[i] == "movetime")
			{
				movetime = atoi(tokens[++i].c_str());
			}
			else if (tokens[i] == "infinite")
			{
				// TODO: actually support infinite
				infinite = true;
			}
		}

		// TODO: way better time management needed
		if (movetime == -1 && !infinite)
		{
			int time, inc;
			if (GamePosition.ToMove == WHITE)
			{
				time = wtime;
				inc = winc;
			}
			else
			{
				time = btime;
				inc = binc;
			}

			movetime = time;
			if (inc != -1) movetime += inc * 30;
			movetime = max(0, (movetime / 30) - 50);
			movetime = min(movetime, (time + inc) - 50);
		}

		// Begin the search
		IncrementHashDate();
		int score;
		Move move = IterativeDeepening(GamePosition, MaxPly, score, movetime, true);

		ASSERT(IsMovePseudoLegal(GamePosition, move));

		printf("bestmove %s\n", GetMoveUci(move).c_str());
	}
	else if (command == "stop")
	{
		KillSearch = true;
	}
	else if (command == "ponderhit")
	{
	}
	else if (command == "quit")
	{
		exit(0);
	}
}

void RunEngine()
{
	for (;;)
	{
		ReadCommand();
	}
}

int main()
{
	// Disable buffering on stdin/stdout
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	fflush(NULL);

	InitializeBitboards();
	Position::StaticInitialize();
	InitializeEvaluation();
	InitializeSearch();
	InitializeHash(129000000);

	RunTests();
    return 0;

//	RunEngine();

	Position position;
//	position.Initialize("4N3/8/1pkR4/3p4/1p2p3/r5P1/5PK1/8 b - - 1 53"); // Good test position that passed pawns own pieces sometimes.
//	position.Initialize("8/8/5p1k/1p1pp2P/pP3PK1/P1P5/8/8 b - f3"); // d4 wins

//	position.Initialize("1r4k1/4p3/3p2Pp/qp1n4/2pB1rnP/P1P5/2BQ4/R3R1K1 w - - 1 34"); // avoid Rf4
//	position.Initialize("8/6p1/8/5N1r/5K2/1b6/pk4P1/4R3 w - - 0 63 "); // Completely won for black...  Should be able to spot it.
//	position.Initialize("1r1qr1k1/3nppb1/b2p2p1/2pn4/8/2N1BNPB/PPQ1PP2/R3K2R w KQ -"); // Testing.
	position.Initialize("4r3/pp1brp2/4p1k1/4P3/5R2/P1R4P/1P3PP1/6K1 b - - 0 23"); // mate in 7 - q-search danger extensions
//	position.Initialize("4r1k1/5p1p/b1p1q1p1/p2pn3/1r5P/PPN2BP1/2QRPP2/5RK1 w - - 1 26"); // Bxd5 is noooo good - queen proximity to king term
//  position.Initialize("r1b1rqk1/pp3pbp/4p1p1/1Q6/4BBN1/R7/1P3PPP/2R3K1 w - -");
    position.Initialize("rnbqkbnr/pppp1Np1/4ppB1/4P2p/1P6/2N2Q2/PBPP1PPP/R4RK1 w - - 4 14"); // mate

	EvalInfo evalInfo;
	int score = Evaluate(position, evalInfo);
	Move move = IterativeDeepening(position, 99, score, -1, true);
	printf("%s -> %d\n", GetMoveSAN(position, move).c_str(), score);

	return 0;
}