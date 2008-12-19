#include "garbochess.h"
#include "position.h"
#include "evaluation.h"

const int EvalFeatureScale = 32;

// Scaling for game phase (opening -> endgame transition)
EVAL_FEATURE(KnightPhaseScale, 1);
EVAL_FEATURE(BishopPhaseScale, 1);
EVAL_FEATURE(RookPhaseScale, 2);
EVAL_FEATURE(QueenPhaseScale, 4);

EVAL_CONST int gamePhaseMax = (4 * KnightPhaseScale) + (4 * BishopPhaseScale) + (4 * RookPhaseScale) + (2 * QueenPhaseScale);

// Mobility features
EVAL_FEATURE(KnightMobilityOpening, 4 * EvalFeatureScale);
EVAL_FEATURE(KnightMobilityEndgame, 4 * EvalFeatureScale);
EVAL_FEATURE(BishopMobilityOpening, 5 * EvalFeatureScale);
EVAL_FEATURE(BishopMobilityEndgame, 5 * EvalFeatureScale);
EVAL_FEATURE(RookMobilityOpening, 2 * EvalFeatureScale);
EVAL_FEATURE(RookMobilityEndgame, 4 * EvalFeatureScale);
EVAL_FEATURE(QueenMobilityOpening, 1 * EvalFeatureScale);
EVAL_FEATURE(QueenMobilityEndgame, 2 * EvalFeatureScale);

template<Color color, int multiplier>
void EvalPieces(const Position &position, int &openingResult, int &endgameResult, int &gamePhase)
{
	int opening = 0, endgame = 0;

	const Bitboard us = position.Colors[color];
	const Bitboard allPieces = position.GetAllPieces();

	// Knight evaluation
	Bitboard b = position.Pieces[KNIGHT] & us;
	while (b)
	{
		const Square square = PopFirstBit(b);

		// Mobility
		const int mobility = CountBitsSetFew(GetKnightAttacks(square));
		opening += mobility * KnightMobilityOpening;
		endgame += mobility * KnightMobilityEndgame;
		
		gamePhase += KnightPhaseScale;
	}

	// Bishop evaluation
	b = position.Pieces[BISHOP] & us;
	while (b)
	{
		const Square square = PopFirstBit(b);

		// Mobility
		const int mobility = CountBitsSet(GetBishopAttacks(square, allPieces));
		opening += mobility * BishopMobilityOpening;
		endgame += mobility * BishopMobilityEndgame;
		
		gamePhase += BishopPhaseScale;
	}

	// Rook evaluation
	b = position.Pieces[ROOK] & us;
	while (b)
	{
		const Square square = PopFirstBit(b);

		// Mobility
		const int mobility = CountBitsSet(GetRookAttacks(square, allPieces));
		opening += mobility * RookMobilityOpening;
		endgame += mobility * RookMobilityEndgame;
		
		gamePhase += RookPhaseScale;
	}

	// Queen evaluation
	b = position.Pieces[QUEEN] & us;
	while (b)
	{
		const Square square = PopFirstBit(b);

		// Mobility
		const int mobility = CountBitsSet(GetQueenAttacks(square, allPieces));
		opening += mobility * QueenMobilityOpening;
		endgame += mobility * QueenMobilityEndgame;
		
		gamePhase += QueenPhaseScale;
	}

	openingResult += opening * multiplier;
	endgameResult += endgame * multiplier;
}

int Evaluate(const Position &position)
{
	// TODO: Lazy evaluation?

	int opening = position.PsqEvalOpening * EvalFeatureScale;
	int endgame = position.PsqEvalEndgame * EvalFeatureScale;

	// Goes from gamePhaseMax at opening to 0 at endgame
	int gamePhase = 0;

	EvalPieces<WHITE, 1>(position, opening, endgame, gamePhase);
	EvalPieces<BLACK, -1>(position, opening, endgame, gamePhase);

	ASSERT(gamePhase ==
			(CountBitsSetFew(position.Pieces[KNIGHT]) * KnightPhaseScale) +
			(CountBitsSetFew(position.Pieces[BISHOP]) * BishopPhaseScale) +
			(CountBitsSetFew(position.Pieces[ROOK]) * RookPhaseScale) +
			(CountBitsSetFew(position.Pieces[QUEEN]) * QueenPhaseScale));

	if (gamePhase > gamePhaseMax) gamePhase = gamePhaseMax;

	// TODO: tempo bonus?

	// Linear interpolation between opening and endgame
	int result = ((opening * gamePhase) + (endgame * (gamePhaseMax - gamePhase))) / gamePhaseMax;

	// Back down to cp scoring
	result /= EvalFeatureScale;

	return position.ToMove == WHITE ? result : -result;
}