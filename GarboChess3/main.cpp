#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "evaluation.h"
#include "search.h"
#include "hashtable.h"
#include "utilities.h"

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

static int foo = 0;
std::string ReadLine()
{
	if (foo == 0)
	{
		foo++;
		return "position startpos moves e2e4 c7c6 d2d4 d7d5 e4d5 c6d5 c2c4 g8f6 b1c3 b8c6 c1g5 d5c4 d4d5 c6e5 d1d4 h7h6 g5e3 d8c7 g1f3 e5f3 g2f3 e7e5 d4c4 c7c4 f1c4 c8d7 c3e4 f8b4 e1d1 f6e4 f3e4 e8g8 h1g1 f8c8 a1c1 g8f8 a2a3 b4e7 c4d3 c8c1 d1c1 a8c8 c1d2 b7b6 d3a6 c8c7 f2f4 e7d6 f4f5 d6e7 a6d3 e7f6 d2e2 f6e7 e2e1 d7a4 e1d2 a4b3 g1c1 c7c1 d2c1 f8e8 d3b5 e8d8 c1d2 h6h5 b5e2 g7g6 f5g6 f7g6 e2b5 h5h4 e3f2 d8c7 h2h3 g6g5 f2g1 e7c5 g1c5 b6c5 d2e3 c7d6 b5e2 b3a4 e2g4 a7a5 g4f5 a4d1 f5e6 d1b3 e6g4 b3c2 g4c8 c2d1 c8f5 d6c7 e3d2 d1f3 d2e1 c7d6 e1f2 f3d1 f2g2 d6e7 f5e6 e7d6 g2f1 d1f3 e6f5 a5a4 f1e1 f3h5 e1f2 d6c7 f2e3 c7d6 f5e6 d6c7 e3f2 c7d6 e6c8 d6c7 c8f5 c7d6 f5e6 d6c7 f2g2 c7d6 e6c8 h5e2 g2f2 e2h5 f2e3 d6c7 c8a6 h5e8 a6e2 e8d7 e2f1 c7d6 e3f2 d7c8 f2g2 c8d7 f1c4 d6e7 c4e2 e7d6 e2d3 d6c7 d3a6 c7d6 g2h2 d6c7 a6f1 c7d6 f1d3 d6c7 h2g2 c7d6 d3a6 d6c7 a6c4 c7d6 c4e2 d6c7 g2h2 c7d6 e2c4 d6c7 c4d3 d7e8 h2g1 e8d7 d3f1 c5c4 g1h2 d7b5 f1g2 c7d6 g2f3 b5d7 h2g2 d6c5 f3h5 c5d6 h5f7 d6e7 f7g6 g5g4 h3g4 d7g4 g6h7 g4d7 h7g8 d7e8 g8e6 e8g6 g2f3 e7d6 e6g4 g6h7 f3e3 h7g6 g4c8 g6h7 c8e6 d6c7 e6g4 h7g6 g4h3 c7d6 e3f3 g6h5 h3g4 h5g6 g4e6 g6h7 e6h3 h7g6 f3e3 d6e7 h3f1 g6h5 f1c4 h5d1 c4e2 d1c2 e2g4 c2b3";
	}
	else if (foo == 1)
	{
		return "go wtime 255590 btime 12266 winc 500 binc 500";
	}

	char line[16384];
	gets_s(line, 16384);
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
			movetime = min(movetime, (wtime + winc) - 50);
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
	InitializeHash(16000000);

//	RunTests();

	RunEngine();

/*	Position position;
	position.Initialize("6k1/2bq3p/3p1p1B/2nPpNp1/rRQ1P3/5P1P/6PK/8 b - - 12 51");
	int score;
	Move move = IterativeDeepening(position, 12, score, -1, true);
	printf("%s -> %d\n", GetMoveSAN(position, move).c_str(), score);*/

	return 0;
}