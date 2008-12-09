#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "evaluation.h"
#include "search.h"

#include <algorithm>
#include <vector>

void RunTests();

void GenerateLegalMoves(Position &position, std::vector<Move> &legalMoves)
{
	Move moves[256];
	int moveCount = GenerateQuietMoves(position, moves);
	moveCount += GenerateCaptureMoves(position, moves + moveCount);

	for (int i = 0; i < moveCount; i++)
	{
		MoveUndo moveUndo;
		position.MakeMove(moves[i], moveUndo);
		if (!position.CanCaptureKing())
		{
			legalMoves.push_back(moves[i]);
		}
		position.UnmakeMove(moves[i], moveUndo);
	}
}

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

		std::vector<Move> legalMoves;
		GenerateLegalMoves(position, legalMoves);

		// Do we need to disambiguate?
		bool dupe = false, rowDiff = true, columnDiff = true;
		for (int i = 0; i < (int)legalMoves.size(); i++)
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

int SuperBasicQsearchTest(Position &position, int alpha, int beta, Move &bestMove, int depth)
{
	std::vector<Move> moves;
	GenerateLegalMoves(position, moves);

	bestMove = 0;
	int bestScore = MinEval;
	for (int i = 0; i < (int)moves.size(); i++)
	{
//		const std::string fooString = GetNiceString(position, moves[i]);
		MoveUndo moveUndo;
		position.MakeMove(moves[i], moveUndo);

		int score;
		if (depth == 0)
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
			score = -SuperBasicQsearchTest(position, -beta, -alpha, tmp, depth - 1);
		}
		position.UnmakeMove(moves[i], moveUndo);

//		if (depth == 3)
//		std::printf("Searched %d.%s - > %d\n", i, fooString.c_str(), score);

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

void TestSuite()
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
				int score = SuperBasicQsearchTest(position, MinEval, MaxEval, bestMove, 2);
				std::string bestMoveString = GetMoveSAN(position, bestMove);

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
//	printf("Seconds taken: %.2lf\n", totalTime / 1000.0);
//	printf("NPS: %.2lf\n", (totalCount / (totalTime / 1000.0)));

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

	RunTests();

//	TestSuite();

/*	Position position;
	// TODO: test this with q-search checks on.
	//position.Initialize("7k/p7/1R5K/6r1/6p1/6P1/8/8 w - -");
	Move bestMove;
	int score = SuperBasicQsearchTest(position, MinEval, MaxEval, bestMove, 3);
	printf("%s,%d\n", GetNiceString(position, bestMove).c_str(),score);*/

	return 0;
}