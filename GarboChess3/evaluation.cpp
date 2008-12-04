#include "garbochess.h"
#include "position.h"
#include "evaluation.h"

// Scaling for game phase (opening -> endgame transition)
EVAL_FEATURE(KnightPhaseScale, 1);
EVAL_FEATURE(BishopPhaseScale, 1);
EVAL_FEATURE(RookPhaseScale, 2);
EVAL_FEATURE(QueenPhaseScale, 4);

int Evaluate(const Position &position)
{
	// TODO: Lazy evaluation?

	int opening = position.PsqEvalOpening;
	int endgame = position.PsqEvalEndgame;

	// TODO: this should go away once we actually loop over the pieces
	const int knightCount = CountBitsSetFew(position.Pieces[KNIGHT]);
	const int bishopCount = CountBitsSetFew(position.Pieces[BISHOP]);
	const int rookCount = CountBitsSetFew(position.Pieces[ROOK]);
	const int queenCount = CountBitsSetFew(position.Pieces[QUEEN]);
	
	EVAL_CONST int gamePhaseMax = (4 * KnightPhaseScale) + (4 * BishopPhaseScale) + (4 * RookPhaseScale) + (2 * QueenPhaseScale);

	// Goes from gamePhaseMax at opening to 0 at endgame
	int gamePhase = 
		(knightCount * KnightPhaseScale) +
		(bishopCount * BishopPhaseScale) +
		(rookCount * RookPhaseScale) +
		(queenCount * QueenPhaseScale);

	// Linear interpolation between opening and endgame
	int result = ((opening * gamePhase) + (endgame * (gamePhaseMax - gamePhase))) / gamePhaseMax;

	return result;
}