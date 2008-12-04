#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "search.h"
#include "evaluation.h"

int See(Position &position, Move move)
{
	return 0;
}

// depth == 0 is normally what is called for q-search.
// Checks are searched when depth >= -(OnePly / 2).  Depth is decreased by 1 for checks
int QSearch(Position &position, int alpha, int beta, int depth)
{
	// TODO: check for draws here?

	// What do we want from our evaluation? - this needs to be decided (mobility/threat information?)
	int eval = Evaluate(position);

	if (eval > alpha)
	{
		alpha = eval;
		if (alpha >= beta)
		{
			return eval;
		}
	}
	else
	{
		// TODO: Don't even try captures that won't make it back up to alpha?
	}
	
	Move moves[64];
	int moveScore[64];
	int moveCount = GenerateCaptures(position, moves);

	for (int i = 0; i < moveCount; i++)
	{
		// TODO: score moves
		moveScore[i] = 0;
	}

	for (int i = 0; i < moveCount; i++)
	{
		for (int j = i + 1; j < moveCount; j++)
		{
			// TODO: sort moves (nice move sorter?)
		}

		MoveUndo moveUndo;
		position.MakeMove(moves[i], moveUndo);

		if (!position.CanCaptureKing())
		{
			int value;

			// Search this move
			if (position.IsCheck())
			{
				value = -QSearchCheck(position, -beta, -alpha, depth - 1);
			}
			else
			{
				value = -QSearch(position, -beta, -alpha, depth - OnePly);
			}

			if (value > eval)
			{
				eval = value;
				if (value > alpha)
				{
					alpha = value;
					if (value > beta)
					{
						return value;
					}
				}
			}
		}

		position.UnmakeMove(moves[i], moveUndo);
	}

	if (depth < -(OnePly / 2))
	{
		// Don't bother searching checking moves
		return eval;
	}

	// TODO: Generate and search quiet checks
	return eval;
}

int QSearchCheck(Position &position, int alpha, int beta, int depth)
{
	ASSERT(false);

	return 0;
}