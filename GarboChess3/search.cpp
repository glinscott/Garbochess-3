#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "search.h"

int evaluate(const Position &position)
{
}

int see(Position &position, Move move)
{

}

int qSearch(Position &position, int alpha, int beta, int depth)
{
	int eval = evaluate(position);
	// What do we want from our evaluation?

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