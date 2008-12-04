#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "search.h"
#include "evaluation.h"

int See(Position &position, Move move)
{
	return 0;
}

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
		// Don't even try captures that won't make it back up to alpha?
	}
	
	Move moves[64];
	int moveScore[64];
	int moveCount = GenerateCaptures(position, moves);

	for (int i = 0; i < moveCount; i++)
	{
		moveScore[i] = 0;
	}

	for (int i = 0; i < moveCount; i++)
	{
		for (int j = i + 1; j < moveCount; j++)
		{
			
		}
	}
}