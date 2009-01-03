#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "evaluation.h"
#include "search.h"

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

// Pawn features
EVAL_FEATURE(PassedPawnOpeningMin, 10 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnOpeningMax, 70 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnEndgameMin, 20 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnEndgameMax, 140 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnPushEndgame, 50 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnFriendlyKingDistanceEndgame, 5 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnEnemyKingDistanceEndgame, 20 * EvalFeatureScale);

EVAL_FEATURE(CandidatePawnOpeningMin, 5 * EvalFeatureScale);
EVAL_FEATURE(CandidatePawnOpeningMax, 55 * EvalFeatureScale);
EVAL_FEATURE(CandidatePawnEndgameMin, 10 * EvalFeatureScale);
EVAL_FEATURE(CandidatePawnEndgameMax, 110 * EvalFeatureScale);

EVAL_FEATURE(DoubledPawnOpening, -10 * EvalFeatureScale);
EVAL_FEATURE(DoubledPawnEndgame, -20 * EvalFeatureScale);
EVAL_FEATURE(IsolatedPawnOpening, -10 * EvalFeatureScale);
EVAL_FEATURE(IsolatedPawnEndgame, -20 * EvalFeatureScale);
EVAL_FEATURE(BackwardPawnOpening, -8 * EvalFeatureScale);
EVAL_FEATURE(BackwardPawnEndgame, -10 * EvalFeatureScale);

// Material scoring
EVAL_FEATURE(BishopPairOpening, 50 * EvalFeatureScale);
EVAL_FEATURE(BishopPairEndgame, 70 * EvalFeatureScale);

// King attack scoring
EVAL_FEATURE(KingAttackWeightKnight, 20 * EvalFeatureScale);
EVAL_FEATURE(KingAttackWeightBishop, 20 * EvalFeatureScale);
EVAL_FEATURE(KingAttackWeightRook, 40 * EvalFeatureScale);
EVAL_FEATURE(KingAttackWeightQueen, 80 * EvalFeatureScale);

static const int KingAttackWeightScale[16] =
{
	0, 16, 128, 192, 224, 240, 248, 252, 254, 255, 256, 256, 256, 256, 256, 256,
};

Bitboard PawnGreaterBitboards[64][2];
Bitboard PawnGreaterEqualBitboards[64][2];
Bitboard PawnLessBitboards[64][2];
Bitboard PawnLessEqualBitboards[64][2];
Bitboard PassedPawnBitboards[64][2];
Bitboard IsolatedPawnBitboards[64];

void InitializePsqTable();

int RowScoreMultiplier[8];

void InitializeEvaluation()
{
	InitializePsqTable();

	for (Square square = 0; square < 64; square++)
	{
		const int row = GetRow(square);
		const int column = GetColumn(square);
		Bitboard piece = 0;
		SetBit(piece, square);

		PawnLessBitboards[square][WHITE] = 0;
		PawnGreaterBitboards[square][WHITE] = 0;
		IsolatedPawnBitboards[square] = 0;
		for (int testRow = 0; testRow < 8; testRow++)
		{
			if (testRow > row)
			{
				SetBit(PawnLessBitboards[square][WHITE], MakeSquare(testRow, column));
			}
			else if (testRow < row)
			{
				SetBit(PawnGreaterBitboards[square][WHITE], MakeSquare(testRow, column));
			}
		}

		PawnLessEqualBitboards[square][WHITE] = PawnLessBitboards[square][WHITE] | piece;
		PawnGreaterEqualBitboards[square][WHITE] = PawnGreaterBitboards[square][WHITE] | piece;
	}

	for (Square square = 0; square < 64; square++)
	{
		// Passed pawns (no enemy pawns in front of us on our file, or the two neighboring files).
		const int row = GetRow(square);
		const int column = GetColumn(square);

		PassedPawnBitboards[square][WHITE] = PawnGreaterBitboards[square][WHITE];
		IsolatedPawnBitboards[square] = 0;
		if (column - 1 >= 0)
		{
			PassedPawnBitboards[square][WHITE] |= PawnGreaterBitboards[MakeSquare(row, column - 1)][WHITE];
			IsolatedPawnBitboards[square] |= PawnLessEqualBitboards[MakeSquare(0, column - 1)][WHITE];
		}
		if (column + 1 < 8)
		{
			PassedPawnBitboards[square][WHITE] |= PawnGreaterBitboards[MakeSquare(row, column + 1)][WHITE];
			IsolatedPawnBitboards[square] |= PawnLessEqualBitboards[MakeSquare(0, column + 1)][WHITE];
		}
	}

	for (Square square = 0; square < 64; square++)
	{
		PawnLessBitboards[FlipSquare(square)][BLACK] = FlipBitboard(PawnLessBitboards[square][WHITE]);
		PawnGreaterBitboards[FlipSquare(square)][BLACK] = FlipBitboard(PawnGreaterBitboards[square][WHITE]);
		PawnLessEqualBitboards[FlipSquare(square)][BLACK] = FlipBitboard(PawnLessEqualBitboards[square][WHITE]);
		PawnGreaterEqualBitboards[FlipSquare(square)][BLACK] = FlipBitboard(PawnGreaterEqualBitboards[square][WHITE]);
		PassedPawnBitboards[FlipSquare(square)][BLACK] = FlipBitboard(PassedPawnBitboards[square][WHITE]);
	}

	for (int row = 0; row < 8; row++)
	{
		RowScoreMultiplier[row] = 0;
	}

	// TODO: Make EVAL_SCORES
	RowScoreMultiplier[RANK_4] = 26;
	RowScoreMultiplier[RANK_5] = 77;
	RowScoreMultiplier[RANK_6] = 154;
	RowScoreMultiplier[RANK_7] = 256;
}

struct PawnHashInfo
{
	u64 Lock;
	int Opening;
	int Endgame;
	int Kingside[2], Center[2], Queenside[2];
	u8 Passed[2];
};

// TODO: need to make these thread independent
const int PawnHashMask = (1 << 10) - 1;
PawnHashInfo PawnHash[PawnHashMask + 1];

template<Color color>
int GetMultiplier()
{
	return color == WHITE ? 1 : -1;
}

int GetMultiplier(Color color)
{
	return color == WHITE ? GetMultiplier<WHITE>() : GetMultiplier<BLACK>();
}

int RowScoreScale(const int scoreMin, const int scoreMax, const int row)
{
	return scoreMin + ((scoreMax - scoreMin) * RowScoreMultiplier[row] + 128) / 256;
}

template<Color color>
inline int PawnRow(const int row)
{
	return color == WHITE ? row : 7 - row;
}

inline int ShelterPenalty(const int row)
{
	const int inverted = 7 - row;
	ASSERT(inverted >= 0 && inverted <= 6);
	return inverted * inverted;
}

template<Color color, int multiplier>
void EvalPawns(const Position &position, PawnHashInfo &pawnScores)
{
	const Color them = FlipColor(color);
	const Bitboard ourPieces = position.Colors[color];
	const Bitboard allPawns = position.Pieces[PAWN];
	const Bitboard theirPawns = allPawns & position.Colors[them];
	const Bitboard ourPawns = allPawns & ourPieces;

	pawnScores.Passed[color] = 0;

	int shelter[8];
	for (int i = 0; i < 8; i++)
	{
		shelter[i] = ShelterPenalty(RANK_7);
	}

	Bitboard b = ourPawns;
	while (b)
	{
		const Square square = PopFirstBit(b);
		const int row = PawnRow<color>(GetRow(square));

		if ((PawnGreaterBitboards[square][color] & ourPawns) != 0)
		{
			// Doubled pawn
			pawnScores.Opening += multiplier * DoubledPawnOpening;
			pawnScores.Endgame += multiplier * DoubledPawnEndgame;
		}

		if ((IsolatedPawnBitboards[square] & ourPawns) == 0)
		{
			// Isolated
			pawnScores.Opening += multiplier * IsolatedPawnOpening;
			pawnScores.Endgame += multiplier * IsolatedPawnEndgame;
		}

		// TODO: Backward pawns

		if ((PawnGreaterBitboards[square][color] & allPawns) == 0)
		{
			const Bitboard blockingPawns = PassedPawnBitboards[square][color] & theirPawns;
			if (blockingPawns == 0)
			{
				// Passed pawn
				pawnScores.Passed[color] |= 1 << GetColumn(square);

				pawnScores.Opening += multiplier * RowScoreScale(PassedPawnOpeningMin, PassedPawnOpeningMax, row);
			}
			else
			{
				// Candidate passer
				int count = CountBitsSetFew(blockingPawns);
				const Bitboard supportingPawns = PassedPawnBitboards[square][them] & ~PawnLessBitboards[square][color] & ourPawns;
				if (count <= CountBitsSetFew(supportingPawns))
				{
					// Potential candidate.  Now, check if it is being attacked
					if (CountBitsSetFew(GetPawnAttacks(square, them) & ourPawns) - CountBitsSetFew(GetPawnAttacks(square, color) & theirPawns) >= 0)
					{
						pawnScores.Opening += multiplier * RowScoreScale(CandidatePawnOpeningMin, CandidatePawnOpeningMax, row);
						pawnScores.Endgame += multiplier * RowScoreScale(CandidatePawnEndgameMin, CandidatePawnEndgameMax, row);
					}
				}
			}
		}

		if ((PawnLessBitboards[square][color] & ourPawns) == 0)
		{
			// Pawn shelter pawn, as its the closest to our home rank
			shelter[GetColumn(square)] = ShelterPenalty(row);
		}
	}

	// Kingside pawn shelter
	pawnScores.Kingside[color] = shelter[FILE_F] + (shelter[FILE_G] * 2) + shelter[FILE_H];

	// Center pawn shelter
	pawnScores.Center[color] = shelter[FILE_C] / 2 + (shelter[FILE_D] * 3 / 2) + (shelter[FILE_E] * 2) + shelter[FILE_F];

	// Queenside pawn shelter
	pawnScores.Queenside[color] = shelter[FILE_A] / 2 + shelter[FILE_B] + (shelter[FILE_C] * 2) + shelter[FILE_D];
}

void ProbePawnHash(const Position &position, PawnHashInfo *&pawnScores)
{
	int index = position.PawnHash & PawnHashMask;
	pawnScores = PawnHash + index;

	if (pawnScores->Lock == position.PawnHash)
	{
		// We are done
		return;
	}

	pawnScores->Lock = position.PawnHash;
	pawnScores->Opening = 0;
	pawnScores->Endgame = 0;

	EvalPawns<WHITE, 1>(position, *pawnScores);
	EvalPawns<BLACK, -1>(position, *pawnScores);
}

template<Color color, int multiplier>
void EvalPieces(const Position &position, int &openingResult, int &endgameResult, int &gamePhase)
{
	int opening = 0, endgame = 0;

	const Bitboard us = position.Colors[color];
	const Bitboard allPieces = position.GetAllPieces();

	const Bitboard kingMoves = GetKingAttacks(position.KingPos[FlipColor(color)]);

	int kingAttacks = 0;
	int kingAttackWeight = 0;

	// TODO: king attacks by pawns?

	// Knight evaluation
	Bitboard b = position.Pieces[KNIGHT] & us;
	while (b)
	{
		const Square square = PopFirstBit(b);

		// Mobility
		const Bitboard attacks = GetKnightAttacks(square);
		const int mobility = CountBitsSetFew(attacks) - 4;
		opening += mobility * KnightMobilityOpening;
		endgame += mobility * KnightMobilityEndgame;
		
		gamePhase += KnightPhaseScale;

		if (attacks & kingMoves)
		{
			kingAttacks++;
			kingAttackWeight += KingAttackWeightKnight;
		}
	}

	// Bishop evaluation
	b = position.Pieces[BISHOP] & us;
	int bishopCount = 0;
	while (b)
	{
		const Square square = PopFirstBit(b);

		// Mobility
		const Bitboard attacks = GetBishopAttacks(square, allPieces);
		const int mobility = CountBitsSet(attacks) - 6;
		opening += mobility * BishopMobilityOpening;
		endgame += mobility * BishopMobilityEndgame;
		
		gamePhase += BishopPhaseScale;
		bishopCount++;

		if (attacks & kingMoves)
		{
			kingAttacks++;
			kingAttackWeight += KingAttackWeightBishop;
		}
	}

	if (bishopCount >= 2)
	{
		opening += BishopPairOpening;
		endgame += BishopPairEndgame;
	}

	// Rook evaluation
	b = position.Pieces[ROOK] & us;
	while (b)
	{
		const Square square = PopFirstBit(b);

		// Mobility
		const Bitboard attacks = GetRookAttacks(square, allPieces);
		const int mobility = CountBitsSet(attacks) - 7;
		opening += mobility * RookMobilityOpening;
		endgame += mobility * RookMobilityEndgame;
		
		gamePhase += RookPhaseScale;

		if (attacks & kingMoves)
		{
			kingAttacks++;
			kingAttackWeight += KingAttackWeightRook;
		}
	}

	// Queen evaluation
	b = position.Pieces[QUEEN] & us;
	while (b)
	{
		const Square square = PopFirstBit(b);

		// Mobility
		const Bitboard attacks = GetQueenAttacks(square, allPieces);
		const int mobility = CountBitsSet(attacks) - 13;
		opening += mobility * QueenMobilityOpening;
		endgame += mobility * QueenMobilityEndgame;
		
		gamePhase += QueenPhaseScale;

		if (attacks & kingMoves)
		{
			kingAttacks++;
			kingAttackWeight += KingAttackWeightQueen;
		}
	}

	// Give ourselves a bonus for how many of our big pieces are attacking the king.  The more the better.
	opening += (kingAttackWeight * KingAttackWeightScale[kingAttacks]) / 256;

	openingResult += opening * multiplier;
	endgameResult += endgame * multiplier;
}

inline int GetKingDistance(const int from, const int to)
{
	return max(abs(GetColumn(from) - GetColumn(to)), abs(GetRow(from) - GetRow(to)));
}

template<Color color, int multiplier>
void EvalPassed(const Position &position, u8 passedFiles, int oppGamePhase, int &endgameResult)
{
	const Bitboard ourPawns = position.Pieces[PAWN] & position.Colors[color];
	const Bitboard theirPawns = position.Pieces[PAWN] & position.Colors[FlipColor(color)];

	while (passedFiles)
	{
		const int column = GetFirstBitIndex(passedFiles);
		passedFiles &= passedFiles - 1;
		
		Bitboard pawns = ColumnBitboard[column] & ourPawns;
		ASSERT(pawns != 0);

		while (pawns)
		{
			const Square square = PopFirstBit(pawns);
			if ((PawnGreaterBitboards[square][color] & ourPawns) == 0)
			{
				const int row = PawnRow<color>(GetRow(square));

				int scoreMax = PassedPawnEndgameMax;
				const Square pushSquare = GetFirstBitIndex(GetPawnMoves(square, color));

				if (oppGamePhase == 0)
				{
					// No pieces left, unstoppable bonus possible
					// TODO
				}
				else if (position.Board[pushSquare] == PIECE_NONE &&
					FastSee(position, GenerateMove(square, pushSquare), color))
				{
					// Can we safely push the pawn?
					scoreMax += PassedPawnPushEndgame;
				}

				scoreMax -= GetKingDistance(position.KingPos[color], pushSquare) * PassedPawnFriendlyKingDistanceEndgame;
				scoreMax += GetKingDistance(position.KingPos[FlipColor(color)], pushSquare) * PassedPawnEnemyKingDistanceEndgame;

				endgameResult += multiplier * RowScoreScale(PassedPawnEndgameMin, scoreMax, row);
			}
		}
	}
}

int Evaluate(const Position &position, EvalInfo &evalInfo)
{
	// TODO: Lazy evaluation?

	int opening = position.PsqEvalOpening * EvalFeatureScale;
	int endgame = position.PsqEvalEndgame * EvalFeatureScale;

	evalInfo.GamePhase[WHITE] = 0;
	evalInfo.GamePhase[BLACK] = 0;

	EvalPieces<WHITE, 1>(position, opening, endgame, evalInfo.GamePhase[WHITE]);
	EvalPieces<BLACK, -1>(position, opening, endgame, evalInfo.GamePhase[BLACK]);

	// Goes from gamePhaseMax at opening to 0 at endgame
	int gamePhase = evalInfo.GamePhase[WHITE] + evalInfo.GamePhase[BLACK];
	ASSERT(gamePhase ==
			(CountBitsSetFew(position.Pieces[KNIGHT]) * KnightPhaseScale) +
			(CountBitsSetFew(position.Pieces[BISHOP]) * BishopPhaseScale) +
			(CountBitsSetFew(position.Pieces[ROOK]) * RookPhaseScale) +
			(CountBitsSetFew(position.Pieces[QUEEN]) * QueenPhaseScale));

	if (gamePhase > gamePhaseMax) gamePhase = gamePhaseMax;

	PawnHashInfo *pawnScores;
	ProbePawnHash(position, pawnScores);

	opening += pawnScores->Opening;
	endgame += pawnScores->Endgame;

	// Passed pawns, using king relative terms
	EvalPassed<WHITE, 1>(position, pawnScores->Passed[WHITE], evalInfo.GamePhase[BLACK], endgame);
	EvalPassed<BLACK, -1>(position, pawnScores->Passed[BLACK], evalInfo.GamePhase[WHITE], endgame);

	// Score pawn shelter
	for (Color color = WHITE; color <= BLACK; color++)
	{
		const int kingColumn = GetColumn(position.KingPos[color]);
		int penalty = kingColumn < FILE_D ? pawnScores->Queenside[color] : (kingColumn > FILE_E ? pawnScores->Kingside[color] : pawnScores->Center[color]);

		int castlePenalty = penalty;
		int castleFlags = color == WHITE ? position.CastleFlags : position.CastleFlags >> 2;

		if (castleFlags & CastleFlagWhiteKing)
		{
			castlePenalty = min(castlePenalty, pawnScores->Kingside[color]);
		}

		if (castleFlags & CastleFlagWhiteQueen)
		{
			castlePenalty = min(castlePenalty, pawnScores->Queenside[color]);
		}

		opening -= GetMultiplier(color) * ((penalty + castlePenalty) / 2) * EvalFeatureScale;
	}

	// TODO: tempo bonus?

	// Linear interpolation between opening and endgame
	int result = ((opening * gamePhase) + (endgame * (gamePhaseMax - gamePhase))) / gamePhaseMax;

	// Back down to cp scoring
	result /= EvalFeatureScale;

	return position.ToMove == WHITE ? result : -result;
}