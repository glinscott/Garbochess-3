#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "evaluation.h"
#include "search.h"
#include "hashtable.h"

#include <algorithm>
#include <vector>

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

// Random move helpers
std::string GetSquareSAN(const Square square)
{
	std::string result;
	result += GetColumn(square) + 'a';
	result += (RANK_1 - GetRow(square)) + '1';
	return result;
}

std::string GetMoveSAN(Position &position, const Move move)
{
	const Square from = GetFrom(move);
	const Square to = GetTo(move);

	const Move moveType = GetMoveType(move);

	std::string result;
	if (moveType == MoveTypeCastle)
	{
		if (GetColumn(to) > FILE_E)
		{
			result = "O-O";
		}
		else
		{
			result = "O-O-O";
		}
	}
	else
	{
		// Piece that is moving
		const PieceType fromPieceType = GetPieceType(position.Board[from]);
		switch (fromPieceType)
		{
		case PAWN: break;
		case KNIGHT: result += "N"; break;
		case BISHOP: result += "B"; break;
		case ROOK: result += "R"; break;
		case QUEEN: result += "Q"; break;
		case KING: result += "K"; break;
		}

		Move legalMoves[256];
		int legalMoveCount = GenerateLegalMoves(position, legalMoves);

		// Do we need to disambiguate?
		bool dupe = false, rowDiff = true, columnDiff = true;
		for (int i = 0; i < legalMoveCount; i++)
		{
			if (GetFrom(legalMoves[i]) != from &&
				GetTo(legalMoves[i]) == to &&
				GetPieceType(position.Board[GetFrom(legalMoves[i])]) == fromPieceType)
			{
				dupe = true;
				if (GetRow(GetFrom(legalMoves[i])) == GetRow(from))
				{
					rowDiff = false;
				}
				if (GetColumn(GetFrom(legalMoves[i])) == GetColumn(from))
				{
					columnDiff = false;
				}
			}
		}

		if (dupe)
		{
			if (rowDiff)
			{
				result += GetSquareSAN(from)[0];
			}
			else if (columnDiff)
			{
				result += GetSquareSAN(from)[1];
			}
			else
			{
				result += GetSquareSAN(from);
			}
		}
		else if (fromPieceType == PAWN && position.Board[to] != PIECE_NONE)
		{
			// Pawn captures need a row
			result += GetSquareSAN(from)[0];
		}
		
		// Capture?
		if (position.Board[to] != PIECE_NONE ||
			moveType == MoveTypeEnPassent)
		{
			result += "x";
		}

		// Target square
		result += GetSquareSAN(to);
	}

	if (moveType == MoveTypePromotion)
	{
		switch (GetPromotionMoveType(move))
		{
		case KNIGHT: result += "=N"; break;
		case BISHOP: result += "=B"; break;
		case ROOK: result += "=R"; break;
		case QUEEN: result += "=Q"; break;
		}
	}

	MoveUndo moveUndo;
	position.MakeMove(move, moveUndo);

	if (position.IsInCheck())
	{
		Move checkEscapes[64];
		result += GenerateCheckEscapeMoves(position, checkEscapes) == 0 ? "#" : "+";
	}

	position.UnmakeMove(move, moveUndo);

	return result;
}

int AlphaBetaTest(Position &position, int alpha, int beta, Move &bestMove, int depth)
{
	if (position.IsDraw())
	{
		return 0;
	}

	Move moves[256];
	int moveCount = GenerateLegalMoves(position, moves);
	bool wasInCheck = position.IsInCheck();

	bestMove = 0;
	int bestScore = MinEval;
	for (int i = 0; i < moveCount; i++)
	{
//		const std::string fooString = GetMoveSAN(position, moves[i]);
		MoveUndo moveUndo;
		position.MakeMove(moves[i], moveUndo);

		int score;
		int newDepth = (position.IsInCheck() || (wasInCheck && moveCount == 1)) ? depth : depth - 1;
		if (newDepth <= 0)
		{
			if (position.IsInCheck())
			{
				score = -QSearchCheck(position, -beta, -alpha, 0);
			}
			else
			{
				score = -QSearch(position, -beta, -alpha, 0);
			}
		}
		else
		{
			Move tmp;
			score = -AlphaBetaTest(position, -beta, -alpha, tmp, newDepth);
		}
		position.UnmakeMove(moves[i], moveUndo);

/*		if (depth == 3)
		std::printf("Searched %d.%s - > %d\n", i, fooString.c_str(), score);*/

		if (score > bestScore || bestMove == 0)
		{
			bestMove = moves[i];
			bestScore = score;
			if (score > alpha)
			{
				alpha = score;
				if (score >= beta)
				{
					return score;
				}
			}
		}
	}

	return bestScore;
}

std::vector<std::string> tokenize(std::string &in, const std::string &tok)
{
	std::string::iterator cp = in.begin();
	std::vector<std::string> res;
	while (cp != in.end())
	{ 
		while (cp != in.end() && std::count(tok.begin(), tok.end(), *cp)) cp++;
		std::string::iterator tmp = std::find_first_of(cp, in.end(), tok.begin(), tok.end()); 
		if (cp != in.end()) res.push_back(std::string(cp, tmp)); 
		cp = tmp;
	}
	return res;
}

void TestSuite(int depth)
{
	std::FILE* file;
	fopen_s(&file, "tests/wac.epd", "rt");

	char line[500];
	u64 startTime = GetCurrentMilliseconds();
	int test = 0, passed = 0;
	while (std::fgets(line, 500, file) != NULL)
	{
		Position position;
		position.Initialize(line);

		for (int i = 0; line[i] != 0; i++)
		{
			if (line[i] == 'b' && line[i + 1] == 'm')
			{
				std::string fen = line;
				fen = fen.substr(i + 3);

				std::vector<std::string> moves = tokenize(fen, " ;");
				Move bestMove;
//				int score = AlphaBetaTest(position, MinEval, MaxEval, bestMove, depth);

				int iterScore;
				Move iterMove = IterativeDeepening(position, depth, iterScore);
				bestMove = iterMove;

				std::string bestMoveString = GetMoveSAN(position, bestMove);
/*				if (iterScore != score)
				{
					printf("score bad! %d. %s, %s\n", test, bestMoveString.c_str(), GetMoveSAN(position, iterMove).c_str());
				}
				else if (iterMove != bestMove)
				{
					printf("%d. %s, %s\n", test, bestMoveString.c_str(), GetMoveSAN(position, iterMove).c_str());
				}*/

				bool match = false;
				for (int j = 0; j < (int)moves.size(); j++)
				{
					if (moves[j] == bestMoveString)
					{
						match = true;
						break;
					}
				}
				
				if (!match)
				{
					std::printf("%d. %s -> expected %s\n", test, bestMoveString.c_str(), moves[0].c_str());
				}
				else
				{
					passed++;
				}

				test++;

				break;
			}
		}
	}

	u64 totalTime = GetCurrentMilliseconds() - startTime;
	printf("Passed: %d/%d\n", passed, test);

	fclose(file);
}

void RunEngine()
{
}

int main()
{
	InitializeBitboards();
	Position::StaticInitialize();
	InitializePsqTable();
	InitializeSearch();

	RunTests();

	InitializeHash(16000000);

	/*
	Position position;
	position.Initialize("1R2r3/1KP1p1p1/pP1Pb1Pp/pB1PB1pn/nN2pqr1/P4pNk/P3P3/2b1QR2 w - - 0 1");
	int score;
	Move move = IterativeDeepening(position, 0, score);
	printf("%s -> %d\n", GetMoveSAN(position, move).c_str(), score);
	*/

	TestSuite(7);

	return 0;
}