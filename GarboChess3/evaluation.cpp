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

EVAL_FEATURE(TempoOpening, 20 * EvalFeatureScale);
EVAL_FEATURE(TempoEndgame, 10 * EvalFeatureScale);

// Mobility features
EVAL_FEATURE(KnightMobilityOpening, 150);
EVAL_FEATURE(KnightMobilityEndgame, 115);
EVAL_FEATURE(BishopMobilityOpening, 130);
EVAL_FEATURE(BishopMobilityEndgame, 130);
EVAL_FEATURE(RookMobilityOpening, 50);
EVAL_FEATURE(RookMobilityEndgame, 135);
EVAL_FEATURE(QueenMobilityOpening, 25);
EVAL_FEATURE(QueenMobilityEndgame, 45);

EVAL_FEATURE(RookSemiOpenFile, 8 * EvalFeatureScale);
EVAL_FEATURE(RookOpenFile, 17 * EvalFeatureScale);

// Pawn features
EVAL_FEATURE(PassedPawnOpeningMin, 10 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnOpeningMax, 71 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnEndgameMin, 20 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnEndgameMax, 160 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnPushEndgame, 65 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnFriendlyKingDistanceEndgame, 5 * EvalFeatureScale);
EVAL_FEATURE(PassedPawnEnemyKingDistanceEndgame, 23 * EvalFeatureScale);
EVAL_FEATURE(UnstoppablePawnEndgame, 800 * EvalFeatureScale);

EVAL_FEATURE(CandidatePawnOpeningMin, 5 * EvalFeatureScale);
EVAL_FEATURE(CandidatePawnOpeningMax, 55 * EvalFeatureScale);
EVAL_FEATURE(CandidatePawnEndgameMin, 10 * EvalFeatureScale);
EVAL_FEATURE(CandidatePawnEndgameMax, 110 * EvalFeatureScale);

EVAL_FEATURE(DoubledPawnOpening, -8 * EvalFeatureScale);
EVAL_FEATURE(DoubledPawnEndgame, -20 * EvalFeatureScale);
EVAL_FEATURE(IsolatedPawnOpening, -15 * EvalFeatureScale);
EVAL_FEATURE(IsolatedOpenFilePawnOpening, -20 * EvalFeatureScale);
EVAL_FEATURE(IsolatedPawnEndgame, -14 * EvalFeatureScale);
EVAL_FEATURE(BackwardPawnOpening, -12 * EvalFeatureScale);
EVAL_FEATURE(BackwardOpenFilePawnOpening, -12 * EvalFeatureScale);
EVAL_FEATURE(BackwardPawnEndgame, -8 * EvalFeatureScale);

// Material scoring
EVAL_FEATURE(BishopPairOpening, 50 * EvalFeatureScale);
EVAL_FEATURE(BishopPairEndgame, 70 * EvalFeatureScale);

EVAL_FEATURE(ExchangePenalty, 30 * EvalFeatureScale);

// King attack scoring
EVAL_FEATURE(KingAttackWeightPawn, 20 * EvalFeatureScale);
EVAL_FEATURE(KingAttackWeightKnight, 35 * EvalFeatureScale);
EVAL_FEATURE(KingAttackWeightBishop, 30 * EvalFeatureScale);
EVAL_FEATURE(KingAttackWeightRook, 60 * EvalFeatureScale);
EVAL_FEATURE(KingAttackWeightQueen, 120 * EvalFeatureScale);

static const int KingAttackWeightScale[16] =
{
	0, 24, 136, 210, 232, 246, 252, 253, 254, 255, 256, 256, 256, 256, 256, 256,
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
	RowScoreMultiplier[RANK_3] = 10;
	RowScoreMultiplier[RANK_4] = 28;
	RowScoreMultiplier[RANK_5] = 88;
	RowScoreMultiplier[RANK_6] = 167;
	RowScoreMultiplier[RANK_7] = 256;
}

struct PawnHashInfo
{
	u64 Lock;
	int Opening;
	int Endgame;
	int Kingside[2], Center[2], Queenside[2];
	u8 Passed[2];
	u8 Count[2];
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
	pawnScores.Count[color] = 0;

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
		const Square pushSquare = GetFirstBitIndex(GetPawnMoves(square, color));

		bool openFile = false;

		pawnScores.Count[color]++;

		if ((PawnGreaterBitboards[square][color] & allPawns) == 0)
		{
			openFile = true;
		}

		const Bitboard blockingPawns = PassedPawnBitboards[square][color] & theirPawns;
		if (blockingPawns == 0)
		{
			// Passed pawn
			pawnScores.Passed[color] |= 1 << GetColumn(square);

			int score = RowScoreScale(PassedPawnOpeningMin, PassedPawnOpeningMax, row);
			if (!openFile) score /= 2;

			pawnScores.Opening += multiplier * score;
		}
		else if (openFile)
		{
			// Candidate passer
			int count = CountBitsSetFew(blockingPawns);
			const Bitboard supportingPawns = PassedPawnBitboards[pushSquare][them] & ~PawnLessBitboards[pushSquare][color] & ourPawns;
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

		if ((PawnGreaterBitboards[square][color] & ourPawns) != 0)
		{
			// Doubled pawn
			pawnScores.Opening += multiplier * DoubledPawnOpening;
			pawnScores.Endgame += multiplier * DoubledPawnEndgame;
		}

		if ((IsolatedPawnBitboards[square] & ourPawns) == 0)
		{
			// Isolated
			pawnScores.Opening += multiplier * (openFile ? IsolatedOpenFilePawnOpening : IsolatedPawnOpening);
			pawnScores.Endgame += multiplier * IsolatedPawnEndgame;
		}

		// Backward pawns
		Bitboard supportingPawns = PassedPawnBitboards[pushSquare][them] & ourPawns;
		XorClearBit(supportingPawns, square);
		if (supportingPawns == 0)
		{
			// Only backward if there is a pawn in a neigboring file ahead of us.
			if (PassedPawnBitboards[square][color] & ourPawns)
			{
				pawnScores.Opening += multiplier * (openFile ? BackwardOpenFilePawnOpening : BackwardPawnOpening);
				pawnScores.Endgame += multiplier * BackwardPawnEndgame;
			}
		}

		if ((PawnLessBitboards[square][color] & ourPawns) == 0)
		{
			// Pawn shelter pawn, as its the closest to our home rank
			shelter[GetColumn(square)] = ShelterPenalty(row);
		}
	}

	// Kingside pawn shelter
	pawnScores.Kingside[color] = shelter[FILE_F] + (shelter[FILE_G] * 3 / 2) + shelter[FILE_H];

	// Center pawn shelter
	pawnScores.Center[color] = shelter[FILE_C] / 2 + (shelter[FILE_D] * 3 / 2) + (shelter[FILE_E] * 3 / 2) + shelter[FILE_F];

	// Queenside pawn shelter
	pawnScores.Queenside[color] = shelter[FILE_A] / 2 + shelter[FILE_B] + (shelter[FILE_C] * 3 / 2) + shelter[FILE_D];
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
void EvalPieces(const Position &position, int &openingResult, int &endgameResult, int &gamePhase, bool &kingDanger)
{
	int opening = 0, endgame = 0;

	const Bitboard us = position.Colors[color];
	const Bitboard allPieces = position.GetAllPieces();

	const Bitboard kingMoves = GetKingAttacks(position.KingPos[FlipColor(color)]);

	int kingAttacks = 0;
	int kingAttackWeight = 0;

	const Bitboard pawns = position.Pieces[PAWN] & us;
	if ((color == BLACK && (((pawns << 7) | (pawns << 9)) & kingMoves)) ||
		(color == WHITE && (((pawns >> 7) | (pawns >> 9)) & kingMoves)))
	{
		kingAttacks++;
		kingAttackWeight += KingAttackWeightPawn;
	}

	// Knight evaluation
	Bitboard b = position.Pieces[KNIGHT] & us;
	while (b)
	{
		const Square square = PopFirstBit(b);

		// Mobility
		const Bitboard attacks = GetKnightAttacks(square);
		const int mobility = CountBitsSetFew(attacks) - 3;
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
		const int mobility = CountBitsSet(attacks) - 2;
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
		const int mobility = CountBitsSet(attacks) - 4;
		opening += mobility * RookMobilityOpening;
		endgame += mobility * RookMobilityEndgame;
		
		gamePhase += RookPhaseScale;

		if (attacks & kingMoves)
		{
			kingAttacks++;
			kingAttackWeight += KingAttackWeightRook;
		}

		// Open file
		const Bitboard pawnFile = ColumnBitboard[GetColumn(square)] & position.Pieces[PAWN];
		opening -= (RookSemiOpenFile + RookOpenFile) / 2;
		endgame -= (RookSemiOpenFile + RookOpenFile) / 2;
		if ((pawnFile & us) == 0)
		{
			opening += RookSemiOpenFile;
			endgame += RookSemiOpenFile;
			if (pawnFile == 0)
			{
				opening += RookOpenFile;
				endgame += RookOpenFile;
			}
		}

		// TODO: rook on 7th
		// TODO: penalize rooks trapped inside king
	}

	// Queen evaluation
	b = position.Pieces[QUEEN] & us;
	while (b)
	{
		const Square square = PopFirstBit(b);

		// Mobility
		const Bitboard attacks = GetQueenAttacks(square, allPieces);
		const int mobility = CountBitsSet(attacks) - 5;
		opening += mobility * QueenMobilityOpening;
		endgame += mobility * QueenMobilityEndgame;
		
		gamePhase += QueenPhaseScale;

		if (attacks & kingMoves)
		{
			kingAttacks++;
			kingAttackWeight += KingAttackWeightQueen;

			if (kingMoves & GetKingAttacks(square))
			{
				// Danger!  Queen close to king
				kingAttacks++;
				kingAttackWeight += KingAttackWeightQueen;
			}
		}

		// TODO: queen on 7th
	}

	// Give ourselves a bonus for how many of our big pieces are attacking the king.  The more the better.
	opening += (kingAttackWeight * KingAttackWeightScale[kingAttacks]) / 256;

	kingDanger = kingAttacks >= 2;

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
			const int row = PawnRow<color>(GetRow(square));

			int scoreMax = PassedPawnEndgameMax;
			const Square pushSquare = GetFirstBitIndex(GetPawnMoves(square, color));

			if (oppGamePhase == 0)
			{
				// No pieces left, unstoppable bonus possible
				const int pawnRow = row == RANK_2 ? RANK_3 : row;
				const Square promotionSquare = MakeSquare(color == WHITE ? RANK_8 : RANK_1, GetColumn(square));
				int stepsToPromote = GetKingDistance(square, promotionSquare);
				if (position.ToMove != color) stepsToPromote++;

				if (GetKingDistance(position.KingPos[FlipColor(color)], promotionSquare) > stepsToPromote)
				{
					scoreMax += UnstoppablePawnEndgame;
				}
			}
			else if (position.Board[pushSquare] == PIECE_NONE &&
				FastSee(position, GenerateMove(square, pushSquare), color))
			{
				// Can we safely push the pawn?
				scoreMax += PassedPawnPushEndgame;
			}

			scoreMax -= GetKingDistance(position.KingPos[color], pushSquare) * PassedPawnFriendlyKingDistanceEndgame;
			scoreMax += GetKingDistance(position.KingPos[FlipColor(color)], pushSquare) * PassedPawnEnemyKingDistanceEndgame;

			int score = RowScoreScale(PassedPawnEndgameMin, scoreMax, row);
			if ((PawnGreaterBitboards[square][color] & ourPawns) != 0) score /= 2;

			endgameResult += multiplier * score;
		}
	}
}

int Evaluate(const Position &position, EvalInfo &evalInfo)
{
	// TODO: Lazy evaluation?

	int opening = position.PsqEvalOpening * EvalFeatureScale;
	int endgame = position.PsqEvalEndgame * EvalFeatureScale;

	if (position.ToMove == WHITE)
	{
		opening += TempoOpening;
		endgame += TempoEndgame;
	}
	else
	{
		opening -= TempoOpening;
		endgame -= TempoEndgame;
	}

	evalInfo.GamePhase[WHITE] = 0;
	evalInfo.GamePhase[BLACK] = 0;

	EvalPieces<WHITE, 1>(position, opening, endgame, evalInfo.GamePhase[WHITE], evalInfo.KingDanger[BLACK]);
	EvalPieces<BLACK, -1>(position, opening, endgame, evalInfo.GamePhase[BLACK], evalInfo.KingDanger[WHITE]);

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

	// Exchange penalty
	if (evalInfo.GamePhase[WHITE] > evalInfo.GamePhase[BLACK] &&
		pawnScores->Count[BLACK] > pawnScores->Count[WHITE])
	{
		opening += ExchangePenalty;
		endgame += ExchangePenalty;
	}
	else if (evalInfo.GamePhase[BLACK] > evalInfo.GamePhase[WHITE] &&
		pawnScores->Count[WHITE] > pawnScores->Count[BLACK])
	{
		opening -= ExchangePenalty;
		endgame -= ExchangePenalty;
	}

	// Passed pawns, using king relative terms
	EvalPassed<WHITE, 1>(position, pawnScores->Passed[WHITE], evalInfo.GamePhase[BLACK], endgame);
	EvalPassed<BLACK, -1>(position, pawnScores->Passed[BLACK], evalInfo.GamePhase[WHITE], endgame);

	// Score pawn shelter
	for (Color color = WHITE; color <= BLACK; color++)
	{
		const int kingColumn = GetColumn(position.KingPos[color]);
		int penalty;
		
		switch (kingColumn)
		{
		case FILE_A:
		case FILE_B:
			penalty = pawnScores->Queenside[color];
			break;

		case FILE_C:
			penalty = (pawnScores->Queenside[color] + pawnScores->Center[color]) / 2;
			break;

		case FILE_D:
		case FILE_E:
			penalty = pawnScores->Center[color];
			break;

		case FILE_F:
			penalty = (pawnScores->Center[color] + pawnScores->Kingside[color]) / 2;
			break;

		case FILE_G:
		case FILE_H:
			penalty = pawnScores->Kingside[color];
			break;
		}

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

	// Linear interpolation between opening and endgame
	int result = ((opening * gamePhase) + (endgame * (gamePhaseMax - gamePhase))) / gamePhaseMax;

	// Back down to cp scoring
	result /= EvalFeatureScale;

	return position.ToMove == WHITE ? result : -result;
}