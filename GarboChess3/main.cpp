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

std::string ReadLine()
{
//	fprintf(stderr, "WARNING: Reading input during search (search will be paused until new-line)...\n");
	char line[1024];
	gets_s(line, 1024);
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
	InitializePsqTable();
	InitializeSearch();
	InitializeHash(16000000);

//	RunTests();

	RunEngine();

/*	Position position;
	position.Initialize("2rq1rk1/1b1nbppp/p2p1n2/1ppNp3/4P3/1P1B1P1P/P1PP2PN/R1BQ1RK1 b - - 0 12");
	int score;
	Move move = IterativeDeepening(position, 12, score, -1, true);
	printf("%s -> %d\n", GetMoveSAN(position, move).c_str(), score);*/

	return 0;
}