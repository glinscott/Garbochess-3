#include "garbochess.h"
#include "position.h"
#include "evaluation.h"

int PsqTable[16][64][2];

// Material weights (in centipawns, which are directly used by the evaluation value)
EVAL_FEATURE(PawnOpening,   85);
EVAL_FEATURE(PawnEndgame,   90);
EVAL_FEATURE(KnightOpening, 325);
EVAL_FEATURE(KnightEndgame, 320);
EVAL_FEATURE(BishopOpening, 325);
EVAL_FEATURE(BishopEndgame, 320);
EVAL_FEATURE(RookOpening,   500);
EVAL_FEATURE(RookEndgame,   500);
EVAL_FEATURE(QueenOpening,  975);
EVAL_FEATURE(QueenEndgame,  975);

EVAL_FEATURE(BishopPairOpening, 50);
EVAL_FEATURE(BishopPairEndgame, 60);

// Psq weights (in millipawns, all are divided by 10 before being used in the psq-table)
EVAL_FEATURE(PawnColumnOpening, 45);
EVAL_FEATURE(PawnColumnEndgame, 0);
EVAL_FEATURE(KnightCenterOpening, 52);
EVAL_FEATURE(KnightCenterEndgame, 52);
EVAL_FEATURE(KnightRowOpening, 52);
EVAL_FEATURE(KnightRowEndgame, 0);
EVAL_FEATURE(KnightBackRowOpeningPenalty, 5);
EVAL_FEATURE(BishopCenterOpening, 19);
EVAL_FEATURE(BishopCenterEndgame, 27);
EVAL_FEATURE(BishopBackRowOpeningPenalty, 95);
EVAL_FEATURE(BishopDiagonalOpening, 43);
EVAL_FEATURE(RookColumnOpening, 30);
EVAL_FEATURE(RookColumnEndgame, 0);
EVAL_FEATURE(QueenCenterOpening, 5);
EVAL_FEATURE(QueenCenterEndgame, 40);
EVAL_FEATURE(QueenBackRowOpeningPenalty, 45);
EVAL_FEATURE(KingCenterOpening, 0);
EVAL_FEATURE(KingCenterEndgame, 120);
EVAL_FEATURE(KingColumnOpening, 95);
EVAL_FEATURE(KingColumnEndgame, 0);
EVAL_FEATURE(KingRowOpening, 95);
EVAL_FEATURE(KingRowEndgame, 0);

// Feature multipliers - weights from Toga
const int EmptyFeature[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
const int PawnColumn[8] = { -3, -1, +0, +1, +1, +0, -1, -3 };
const int KnightLine[8] = { -4, -2, +0, +1, +1, +0, -2, -4 };
const int KnightRow[8] = { -2, -1, +0, +1, +2, +3, +2, +1 };
const int BishopLine[8] = { -3, -1, +0, +1, +1, +0, -1, -3 };
const int RookColumn[8] = { -2, -1, +0, +1, +1, +0, -1, -2 };
const int QueenLine[8] = { -3, -1, +0, +1, +1, +0, -1, -3 };
const int KingLine[8] = { -3, -1, +0, +1, +1, +0, -1, -3 };
const int KingColumn[8] = { +3, +4, +2, +0, +0, +2, +4, +3 };
const int KingRow[8] = { +1, +0, -2, -3, -4, -5, -6, -7 };

void InitPiece(Piece piece,
			   int weightOpening, int weightEndgame,
			   const int center[8], int centerWeightOpening, int centerWeightEndgame,
			   const int row[8], int rowWeightOpening, int rowWeightEndgame,
			   const int column[8], int columnWeightOpening, int columnWeightEndgame)
{
	ASSERT(piece < 16);

	for (Square square = 0; square < 64; square++)
	{
		PsqTable[piece][square][0] = weightOpening * 10; // Will be divided by 10 at the end

		PsqTable[piece][square][0] +=
			(row[GetRow(square)] * rowWeightOpening) +
			(column[GetColumn(square)] * columnWeightOpening) +
			(center[GetRow(square)] * centerWeightOpening) +
			(center[GetColumn(square)] * centerWeightOpening);

		PsqTable[piece][square][1] = weightEndgame * 10; // Will be divided by 10 at the end

		PsqTable[piece][square][1] +=
			(row[GetRow(square)] * rowWeightEndgame) +
			(column[GetColumn(square)] * columnWeightEndgame) +
			(center[GetRow(square)] * centerWeightEndgame) +
			(center[GetColumn(square)] * centerWeightEndgame);
	}
}

void InitializePsqTable()
{
	for (Piece piece = 0; piece < 14; piece++)
	{
		for (Square square = 0; square < 64; square++)
		{
			for (int gameStage = 0; gameStage < 2; gameStage++)
			{
				PsqTable[piece][square][gameStage] = 0;
			}
		}
	}

	// Piece-square tables are initialized for white first, then flipped for black
	InitPiece(MakePiece(WHITE, PAWN),
		PawnOpening, PawnEndgame,
		EmptyFeature, 0, 0, 
		EmptyFeature, 0, 0, 
		PawnColumn, PawnColumnOpening, PawnColumnEndgame);

	InitPiece(MakePiece(WHITE, KNIGHT), 
		KnightOpening, KnightEndgame,
		KnightLine, KnightCenterOpening, KnightCenterEndgame, 
		KnightRow, KnightRowOpening, KnightRowEndgame, 
		EmptyFeature, 0, 0);

	// Knight bank rank penalty
	for (Square square = MakeSquare(RANK_1, FILE_A); square <= MakeSquare(RANK_1, FILE_H); square++)
	{
		PsqTable[MakePiece(WHITE, KNIGHT)][square][0] -= KnightBackRowOpeningPenalty;
	}

	InitPiece(MakePiece(WHITE, BISHOP), 
		BishopOpening, BishopEndgame,
		BishopLine, BishopCenterOpening, BishopCenterEndgame, 
		EmptyFeature, 0, 0,
		EmptyFeature, 0, 0);

	// Bishop bank rank penalty
	for (Square square = MakeSquare(RANK_1, FILE_A); square <= MakeSquare(RANK_1, FILE_H); square++)
	{
		PsqTable[MakePiece(WHITE, BISHOP)][square][0] -= BishopBackRowOpeningPenalty;
	}

	InitPiece(MakePiece(WHITE, ROOK), 
		RookOpening, RookEndgame,
		EmptyFeature, 0, 0, 
		EmptyFeature, 0, 0,
		RookColumn, RookColumnOpening, RookColumnEndgame);

	InitPiece(MakePiece(WHITE, QUEEN), 
		QueenOpening, QueenEndgame,
		QueenLine, QueenCenterOpening, QueenCenterEndgame, 
		EmptyFeature, 0, 0,
		EmptyFeature, 0, 0);

	// Queen bank rank penalty
	for (Square square = MakeSquare(RANK_1, FILE_A); square <= MakeSquare(RANK_1, FILE_H); square++)
	{
		PsqTable[MakePiece(WHITE, QUEEN)][square][0] -= QueenBackRowOpeningPenalty;
	}

	InitPiece(MakePiece(WHITE, KING), 
		0, 0,	// Kings are not worth anything materially
		KingLine, KingCenterOpening, KingCenterEndgame, 
		KingRow, KingRowOpening, KingRowEndgame,
		KingColumn, KingColumnOpening, KingColumnEndgame);

	// Normalize, and copy the tables to Black
	for (PieceType pieceType = PAWN; pieceType <= KING; pieceType++)
	{
		for (Square square = 0; square < 64; square++)
		{
			for (int gameStage = 0; gameStage < 2; gameStage++)
			{
				PsqTable[MakePiece(WHITE, pieceType)][square][gameStage] /= 10;
				PsqTable[MakePiece(BLACK, pieceType)][FlipSquare(square)][gameStage] = -PsqTable[MakePiece(WHITE, pieceType)][square][gameStage];
			}
		}
	}
}