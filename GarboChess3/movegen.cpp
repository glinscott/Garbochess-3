#include "garbochess.h"
#include "position.h"
#include "movegen.h"

const u64 RMult[64] = {
  0xa8002c000108020ULL, 0x4440200140003000ULL, 0x8080200010011880ULL, 
  0x380180080141000ULL, 0x1a00060008211044ULL, 0x410001000a0c0008ULL, 
  0x9500060004008100ULL, 0x100024284a20700ULL, 0x802140008000ULL, 
  0x80c01002a00840ULL, 0x402004282011020ULL, 0x9862000820420050ULL, 
  0x1001448011100ULL, 0x6432800200800400ULL, 0x40100010002000cULL, 
  0x2800d0010c080ULL, 0x90c0008000803042ULL, 0x4010004000200041ULL, 
  0x3010010200040ULL, 0xa40828028001000ULL, 0x123010008000430ULL, 
  0x24008004020080ULL, 0x60040001104802ULL, 0x582200028400d1ULL, 
  0x4000802080044000ULL, 0x408208200420308ULL, 0x610038080102000ULL, 
  0x3601000900100020ULL, 0x80080040180ULL, 0xc2020080040080ULL, 
  0x80084400100102ULL, 0x4022408200014401ULL, 0x40052040800082ULL, 
  0xb08200280804000ULL, 0x8a80a008801000ULL, 0x4000480080801000ULL, 
  0x911808800801401ULL, 0x822a003002001894ULL, 0x401068091400108aULL, 
  0x4a10a00004cULL, 0x2000800640008024ULL, 0x1486408102020020ULL, 
  0x100a000d50041ULL, 0x810050020b0020ULL, 0x204000800808004ULL, 
  0x20048100a000cULL, 0x112000831020004ULL, 0x9000040810002ULL, 
  0x440490200208200ULL, 0x8910401000200040ULL, 0x6404200050008480ULL, 
  0x4b824a2010010100ULL, 0x4080801810c0080ULL, 0x400802a0080ULL, 
  0x8224080110026400ULL, 0x40002c4104088200ULL, 0x1002100104a0282ULL, 
  0x1208400811048021ULL, 0x3201014a40d02001ULL, 0x5100019200501ULL, 
  0x101000208001005ULL, 0x2008450080702ULL, 0x1002080301d00cULL, 
  0x410201ce5c030092ULL
};

const int RShift[64] = {
  52, 53, 53, 53, 53, 53, 53, 52, 53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53, 52, 53, 53, 53, 53, 53, 53, 52
};

const u64 BMult[64] = {
  0x440049104032280ULL, 0x1021023c82008040ULL, 0x404040082000048ULL, 
  0x48c4440084048090ULL, 0x2801104026490000ULL, 0x4100880442040800ULL, 
  0x181011002e06040ULL, 0x9101004104200e00ULL, 0x1240848848310401ULL, 
  0x2000142828050024ULL, 0x1004024d5000ULL, 0x102044400800200ULL, 
  0x8108108820112000ULL, 0xa880818210c00046ULL, 0x4008008801082000ULL, 
  0x60882404049400ULL, 0x104402004240810ULL, 0xa002084250200ULL, 
  0x100b0880801100ULL, 0x4080201220101ULL, 0x44008080a00000ULL, 
  0x202200842000ULL, 0x5006004882d00808ULL, 0x200045080802ULL, 
  0x86100020200601ULL, 0xa802080a20112c02ULL, 0x80411218080900ULL, 
  0x200a0880080a0ULL, 0x9a01010000104000ULL, 0x28008003100080ULL, 
  0x211021004480417ULL, 0x401004188220806ULL, 0x825051400c2006ULL, 
  0x140c0210943000ULL, 0x242800300080ULL, 0xc2208120080200ULL, 
  0x2430008200002200ULL, 0x1010100112008040ULL, 0x8141050100020842ULL, 
  0x822081014405ULL, 0x800c049e40400804ULL, 0x4a0404028a000820ULL, 
  0x22060201041200ULL, 0x360904200840801ULL, 0x881a08208800400ULL, 
  0x60202c00400420ULL, 0x1204440086061400ULL, 0x8184042804040ULL, 
  0x64040315300400ULL, 0xc01008801090a00ULL, 0x808010401140c00ULL, 
  0x4004830c2020040ULL, 0x80005002020054ULL, 0x40000c14481a0490ULL, 
  0x10500101042048ULL, 0x1010100200424000ULL, 0x640901901040ULL, 
  0xa0201014840ULL, 0x840082aa011002ULL, 0x10010840084240aULL, 
  0x420400810420608ULL, 0x8d40230408102100ULL, 0x4a00200612222409ULL, 
  0xa08520292120600ULL
};

const int BShift[64] = {
  58, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 58
};

const int BitTable[64] = {
	0, 1, 2, 7, 3, 13, 8, 19, 4, 25, 14, 28, 9, 34, 20, 40, 5, 17, 26, 38, 15,
	46, 29, 48, 10, 31, 35, 54, 21, 50, 41, 57, 63, 6, 12, 18, 24, 27, 33, 39,
	16, 37, 45, 47, 30, 53, 49, 56, 62, 11, 23, 32, 36, 44, 52, 55, 61, 22, 43,
	51, 60, 42, 59, 58
};

Bitboard RowBitboard[8];
Bitboard ColumnBitboard[8];

Bitboard PawnMoves[2][64];
Bitboard PawnAttacks[2][64];
Bitboard KnightAttacks[64];

Bitboard BMask[64];
int BAttackIndex[64];
Bitboard BAttacks[0x1480];

Bitboard RMask[64];
int RAttackIndex[64];
Bitboard RAttacks[0x19000];

Bitboard KingAttacks[64];

Bitboard SquaresBetween[64][64];

Bitboard SlidingAttacks(Square sq, Bitboard block, int dirs, int deltas[][2], int fmin=0, int fmax=7, int rmin=0, int rmax=7)
{
	Bitboard result = 0ULL;
	int rk = sq / 8, fl = sq % 8, r, f, i;
	for(i = 0; i < dirs; i++)
	{
		int dx = deltas[i][0], dy = deltas[i][1];
		for(f = fl + dx, r = rk + dy;
			(dx == 0 || (f >= fmin && f <= fmax)) && (dy == 0 || (r >= rmin && r <= rmax));
			f += dx, r += dy)
		{
			result |= (1ULL << (f + r * 8));
			if (block & (1ULL << (f + r * 8))) break;
		}
	}
	return result;
}

Bitboard IndexToBitboard(int index, Bitboard mask)
{
	int i, j, bits = CountBitsSet(mask);
	Bitboard result = 0ULL;
	for(i = 0; i < bits; i++)
	{
		j = PopFirstBit(mask);
		if(index & (1 << i)) result |= (1ULL << j);
	}
	return result;
}

void InitializeAttackTables(Bitboard attacks[], int attackIndex[], Bitboard mask[], const int shift[64], const Bitboard mult[], int deltas[][2])
{
	int i, j, k, index = 0;
	Bitboard b;
	for(i = 0; i < 64; i++)
	{
		attackIndex[i] = index;
		mask[i] = SlidingAttacks(i, 0ULL, 4, deltas, 1, 6, 1, 6);
		j = (1 << (64 - shift[i]));
		for(k = 0; k < j; k++)
		{
			b = 0ULL;
			b = IndexToBitboard(k, mask[i]);
			attacks[index + ((b * mult[i]) >> shift[i])] =
				SlidingAttacks(i, b, 4, deltas);
		}
		index += j;
	}
}

void SetIfInBounds(int row, int col, int delta[2], Bitboard &b)
{
	int nr = row + delta[0];
	int nc = col + delta[1];

	if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8)
	{
		SetBit(b, MakeSquare(nr, nc));
	}
}

void InitializeBitboards()
{
	for (int i = 0; i < 8; i++)
	{
		RowBitboard[i] = 0xFFULL << (i * 8);
		ColumnBitboard[i] = 0x0101010101010101ULL << i;
	}

	int rookDeltas[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
	int bishopDeltas[4][2] = {{1,1},{-1,1},{1,-1},{-1,-1}};

	InitializeAttackTables(RAttacks, RAttackIndex, RMask, RShift, RMult, rookDeltas);
	InitializeAttackTables(BAttacks, BAttackIndex, BMask, BShift, BMult, bishopDeltas);

	int pawnDeltas[2][2][2] = {{{-1,-1},{-1,1}},{{1,-1},{1,1}}};
	int pawnMoveDeltas[2][2] = {{-1,0},{1,0}};
	int knightDeltas[8][2] = {{-1,-2},{-1,2},{1,-2},{1,2},{-2,-1},{-2,1},{2,-1},{2,1}};
	int kingDeltas[8][2] = {{0,1},{0,-1},{1,0},{-1,0},{1,1},{-1,1},{1,-1},{-1,-1}};

	for (int row = 0; row < 8; row++)
	{
		for (int col = 0; col < 8; col++)
		{
			Square square = MakeSquare(row, col);
			
			for (Color color = WHITE; color <= BLACK; color++)
			{
				PawnAttacks[color][square] = 0;
				for (int i = 0; i < 2; i++)
				{
					SetIfInBounds(row, col, pawnDeltas[color][i], PawnAttacks[color][square]);
				}

				PawnMoves[color][square] = 0;
				SetIfInBounds(row, col, pawnMoveDeltas[color], PawnMoves[color][square]);
			}

			KnightAttacks[square] = 0;
			for (int i = 0; i < 8; i++)
			{
				SetIfInBounds(row, col, knightDeltas[i], KnightAttacks[square]);
			}

			KingAttacks[square] = 0;
			for (int i = 0; i < 8; i++)
			{
				SetIfInBounds(row, col, kingDeltas[i], KingAttacks[square]);
			}
		}
	}

	for (Square from = 0; from < 64; from++)
	{
		for (Square to = 0; to < 64; to++)
		{
			Bitboard usThem = 0;
			SetBit(usThem, from);
			SetBit(usThem, to);

			if (GetRow(from) == GetRow(to) || GetColumn(from) == GetColumn(to))
			{
				SquaresBetween[from][to] = GetRookAttacks(from, usThem) & GetRookAttacks(to, usThem);
			}
			else if (abs(GetRow(from) - GetRow(to)) == abs(GetColumn(from) - GetColumn(to)))
			{
				SquaresBetween[from][to] = GetBishopAttacks(from, usThem) & GetBishopAttacks(to, usThem);
			}
			else
			{
				SquaresBetween[from][to] = 0;
			}
		}
	}
}

int ScoreCaptureMove(const Move move, const PieceType fromPiece, const PieceType toPiece)
{
	// Currently scoring moves using MVV/LVA scoring.  ie. PxQ goes first, BxQ next, ... , KxP last
	const Move moveType = GetMoveType(move);
	if (moveType == MoveTypeNone)
	{
		return ScoreCaptureMove(fromPiece, toPiece);
	}
	else
	{
		if (moveType == MoveTypeEnPassent)
		{
			// e.p. captures
			return PAWN * 100 - PAWN;
		}
		else
		{
			// Promotions
			ASSERT(moveType == MoveTypePromotion);

			if (GetPromotionMoveType(move) == QUEEN)
			{
				// Goes first
				return QUEEN * 100;
			}
			else
			{
				// Other types of promotions, we don't really care about
				return 0;
			}
		}
	}
}

template<int homeRow, int targetRow, int promotionRow>
inline void GeneratePawnQuietMoves(Bitboard b, Move *moves, int &moveCount, const Color us, const Bitboard empty, const Bitboard them)
{
	while (b)
	{
		const Square from = PopFirstBit(b);

		Bitboard attacks = GetPawnMoves(from, us) & empty;

		const int row = GetRow(from);

		if (attacks)
		{
			if (row == homeRow)
			{
				// Double push
				const Square to = MakeSquare(targetRow, GetColumn(from));
				if (IsBitSet(empty, to))
				{
					moves[moveCount++] = GenerateMove(from, MakeSquare(targetRow, GetColumn(from)));
				}
			}

			if (row != promotionRow)
			{
				// Single push
				moves[moveCount++] = GenerateMove(from, GetFirstBitIndex(attacks));
			}
			else
			{
				// Promotions (non-queen)
				const Square to = GetFirstBitIndex(attacks);
				moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeKnight);
				moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeRook);
				moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeBishop);
			}
		}

		if (row == promotionRow)
		{
			// Cheat a bit, and generate capture promotions
			attacks = GetPawnAttacks(from, us) & them;

			while (attacks)
			{
				const Square to = PopFirstBit(attacks);
				moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeKnight);
				moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeRook);
				moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeBishop);
			}
		}
	}
}

template<int promotionRow>
inline void GeneratePawnCaptures(Bitboard b, Move *moves, s16 *moveScores, int &moveCount, const Color us, const Bitboard empty, const Bitboard them, const Position &position)
{
	while (b)
	{
		const Square from = PopFirstBit(b);
		const int row = GetRow(from);

		Bitboard attacks = GetPawnAttacks(from, us) & them;

		if (row != promotionRow)
		{
			// Normal pawn attacks
			while (attacks)
			{
				const Square to = PopFirstBit(attacks);
				moveScores[moveCount] = ScoreCaptureMove(PAWN, GetPieceType(position.Board[to]));
				moves[moveCount++] = GenerateMove(from, to);
			}
		}
		else
		{
			// Pawn promotions - attacks to queen, push to queen
			attacks |= GetPawnMoves(from, us) & empty;
			while (attacks)
			{
				const Square to = PopFirstBit(attacks);
				moveScores[moveCount] = QUEEN * 100;
				moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeQueen);
			}
		}
	}
}

#define MoveGenerationLoop(func, pieceBitboard)						\
	b = (pieceBitboard) & ourPieces;								\
	while (b)														\
	{																\
		const Square from = PopFirstBit(b);							\
		Bitboard attacks = (func) & targets;						\
		while (attacks)												\
		{															\
			const Square to = PopFirstBit(attacks);					\
			moves[moveCount++] = GenerateMove(from, to);			\
		}															\
	}

#define MoveGenerationLoopAttacks(func, pieceBitboard, pieceType)	\
	b = (pieceBitboard) & ourPieces;								\
	while (b)														\
	{																\
		const Square from = PopFirstBit(b);							\
		Bitboard attacks = (func) & targets;						\
		while (attacks)												\
		{															\
			const Square to = PopFirstBit(attacks);					\
			moveScores[moveCount] = ScoreCaptureMove((pieceType), GetPieceType(position.Board[to]));	\
			moves[moveCount++] = GenerateMove(from, to);			\
		}															\
	}

inline bool IsKingsideCastleLegal(const Position &position, const Bitboard allPieces, const Color us, const Color them)
{
	const int kingRow = GetRow(position.KingPos[us]);
	if (!IsBitSet(allPieces, MakeSquare(kingRow, FILE_F)) &&
		!IsBitSet(allPieces, MakeSquare(kingRow, FILE_G)))
	{
		// Verify that the king is not moving through check
		ASSERT(!position.IsInCheck());
		return !position.IsSquareAttacked(MakeSquare(kingRow, FILE_F), them);
	}
	return false;
}

inline bool IsQueensideCastleLegal(const Position &position, const Bitboard allPieces, const Color us, const Color them)
{
	const int kingRow = GetRow(position.KingPos[us]);
	if (!IsBitSet(allPieces, MakeSquare(kingRow, FILE_B)) &&
		!IsBitSet(allPieces, MakeSquare(kingRow, FILE_C)) &&
		!IsBitSet(allPieces, MakeSquare(kingRow, FILE_D)))
	{
		// Verify that the king is not moving through check
		ASSERT(!position.IsInCheck());
		return !position.IsSquareAttacked(MakeSquare(kingRow, FILE_D), them);
	}
	return false;
}

int GenerateSliderMoves(const Position &position, Move *moves)
{
	const Color us = position.ToMove;
	const Color them = FlipColor(position.ToMove);
	const Bitboard ourPieces = position.Colors[us];
	const Bitboard allPieces = position.GetAllPieces();
	const Bitboard targets = ~allPieces;

	int moveCount = 0;
	Bitboard b;

	// Normal piece moves
	MoveGenerationLoop(GetKnightAttacks(from), position.Pieces[KNIGHT]);
	MoveGenerationLoop(GetBishopAttacks(from, allPieces), position.Pieces[BISHOP] | position.Pieces[QUEEN]);
	MoveGenerationLoop(GetRookAttacks(from, allPieces), position.Pieces[ROOK] | position.Pieces[QUEEN]);
	MoveGenerationLoop(GetKingAttacks(from), position.Pieces[KING]);

	return moveCount;
}

int GenerateQuietMoves(const Position &position, Move *moves)
{
	const Color us = position.ToMove;
	const Color them = FlipColor(position.ToMove);
	const Bitboard ourPieces = position.Colors[us];
	const Bitboard allPieces = position.GetAllPieces();
	const Bitboard targets = ~allPieces;
	
	int moveCount = 0;
	Bitboard b;
	int castleFlags;

	// Generate pawn push, push promotions (non-queen), capture promotions (non-queen), pawn double hops
	if (us == WHITE)
	{
		GeneratePawnQuietMoves<RANK_2, RANK_4, RANK_7>(position.Pieces[PAWN] & ourPieces, moves, moveCount, us, targets, position.Colors[them]);

		castleFlags = position.CastleFlags;
	}
	else
	{
		GeneratePawnQuietMoves<RANK_7, RANK_5, RANK_2>(position.Pieces[PAWN] & ourPieces, moves, moveCount, us, targets, position.Colors[them]);

		castleFlags = position.CastleFlags >> 2;
	}

	// Castling (treated as though we are always white)
	if (castleFlags & CastleFlagWhiteKing)
	{
		if (IsKingsideCastleLegal(position, allPieces, us, them))
		{
			moves[moveCount++] = GenerateCastleMove(position.KingPos[us], MakeSquare(GetRow(position.KingPos[us]), FILE_G));
		}
	}
	if (castleFlags & CastleFlagWhiteQueen)
	{
		if (IsQueensideCastleLegal(position, allPieces, us, them))
		{
			moves[moveCount++] = GenerateCastleMove(position.KingPos[us], MakeSquare(GetRow(position.KingPos[us]), FILE_C));
		}
	}

	// Normal piece moves
	MoveGenerationLoop(GetKnightAttacks(from), position.Pieces[KNIGHT]);
	MoveGenerationLoop(GetBishopAttacks(from, allPieces), position.Pieces[BISHOP] | position.Pieces[QUEEN]);
	MoveGenerationLoop(GetRookAttacks(from, allPieces), position.Pieces[ROOK] | position.Pieces[QUEEN]);
	MoveGenerationLoop(GetKingAttacks(from), position.Pieces[KING]);

	return moveCount;
}

int GenerateCaptureMoves(const Position &position, Move *moves, s16 *moveScores)
{
	const Color us = position.ToMove;
	const Color them = FlipColor(us);
	const Bitboard ourPieces = position.Colors[us];
	const Bitboard allPieces = position.GetAllPieces();
	const Bitboard targets = position.Colors[them];
	
	int moveCount = 0;
	Bitboard b;

	// Pawn attacks, pawn promotions (queen only)
	if (us == WHITE)
	{
		GeneratePawnCaptures<RANK_7>(position.Pieces[PAWN] & ourPieces, moves, moveScores, moveCount, us, ~allPieces, targets, position);
	}
	else
	{
		GeneratePawnCaptures<RANK_2>(position.Pieces[PAWN] & ourPieces, moves, moveScores, moveCount, us, ~allPieces, targets, position);
	}

	// En Passent
	if (position.EnPassent != -1)
	{
		b = position.Pieces[PAWN] & ourPieces & GetPawnAttacks(position.EnPassent, them);
		while (b)
		{
			Square from = PopFirstBit(b);
			moveScores[moveCount] = ScoreCaptureMove(PAWN, PAWN);
			moves[moveCount++] = GenerateEnPassentMove(from, position.EnPassent);
		}
	}

	// Normal piece captures
	MoveGenerationLoopAttacks(GetKnightAttacks(from), position.Pieces[KNIGHT], KNIGHT);
	MoveGenerationLoopAttacks(GetBishopAttacks(from, allPieces), position.Pieces[BISHOP], BISHOP);
	MoveGenerationLoopAttacks(GetRookAttacks(from, allPieces), position.Pieces[ROOK], ROOK);
	MoveGenerationLoopAttacks(GetQueenAttacks(from, allPieces), position.Pieces[QUEEN], QUEEN);
	MoveGenerationLoopAttacks(GetKingAttacks(from), position.Pieces[KING], KING);

#if _DEBUG
	for (int i = 0; i < moveCount; i++)
	{
		ASSERT(moveScores[i] == ScoreCaptureMove(moves[i], GetPieceType(position.Board[GetFrom(moves[i])]), GetPieceType(position.Board[GetTo(moves[i])])));
	}
#endif

	return moveCount;
}

// Generates pseudo-legal quiet moves that give check to the opponent king.  This includes both direct and revealed checks.
// For simplicity, we don't generate castling, promotion or e.p. checks
int GenerateCheckingMoves(const Position &position, Move *moves)
{
	int moveCount = 0;

	const Color us = position.ToMove;
	const Color them = FlipColor(us);
	const Square kingSquare = position.KingPos[them];

	const Bitboard ourPieces = position.Colors[us];
	const Bitboard allPieces = position.GetAllPieces();
	const Bitboard emptySquares = ~allPieces;

	// Direct pawn checks
	Bitboard targets = GetPawnAttacks(kingSquare, them) & emptySquares;
	while (targets)
	{
		const Square to = PopFirstBit(targets);
		const Bitboard potentialPawns = GetPawnMoves(to, them);
		if (potentialPawns & emptySquares)
		{
			// This is a bit funky, but if the target checking square is on rank 4, we can do a double pawn push, and check the king.
			if (GetRow(to) == RANK_4 && us == WHITE)
			{
				const Square from = MakeSquare(RANK_2, GetColumn(to));
				if (IsBitSet(ourPieces & position.Pieces[PAWN], from))
				{
					moves[moveCount++] = GenerateMove(from, to);
				}
			}
			else if (GetRow(to) == RANK_5 && us == BLACK)
			{
				const Square from = MakeSquare(RANK_7, GetColumn(to));
				if (IsBitSet(ourPieces & position.Pieces[PAWN], from))
				{
					moves[moveCount++] = GenerateMove(from, to);
				}
			}
		}

		// Single pawn push
		if (potentialPawns & position.Pieces[PAWN] & ourPieces)
		{
			moves[moveCount++] = GenerateMove(GetFirstBitIndex(potentialPawns), to);
		}
	}

	// Direct knight checks
	Bitboard b;
	targets = GetKnightAttacks(kingSquare) & emptySquares;
	MoveGenerationLoop(GetKnightAttacks(from), position.Pieces[KNIGHT]);

	// Do combined direct/revealed bishop/rook/queen checks
	const Bitboard bishopCheckingLines = GetBishopAttacks(kingSquare, allPieces);
	const Bitboard rookCheckingLines = GetRookAttacks(kingSquare, allPieces);
	b = (position.Pieces[BISHOP] | position.Pieces[ROOK] | position.Pieces[QUEEN]) & ourPieces;
	while (b)
	{
		const Square from = PopFirstBit(b);

		// Build the set of attackable squares that can give check to the king.  This includes friendly pieces that by moving would
		// cause a revealed check.
		Bitboard attacks;
		const PieceType ourPiece = GetPieceType(position.Board[from]);
		if (ourPiece == QUEEN)
		{
			attacks = (GetBishopAttacks(from, allPieces) | GetRookAttacks(from, allPieces)) & (bishopCheckingLines | rookCheckingLines);
		}
		else if (ourPiece == ROOK)
		{
			attacks = GetRookAttacks(from, allPieces) & rookCheckingLines;
		}
		else
		{
			attacks = GetBishopAttacks(from, allPieces) & bishopCheckingLines;
		}

		while (attacks)
		{
			const Square to = PopFirstBit(attacks);
			const PieceType at = GetPieceType(position.Board[to]);
			if (at == PIECE_NONE)
			{
				// A direct check
				moves[moveCount++] = GenerateMove(from, to);
			}
			else if (GetPieceColor(position.Board[to]) == us &&
				IsBitSet(GetSquaresBetween(from, kingSquare), to))
			{
				// Revealed checks (it is not possible for us to move a queen to reveal check here, as they
				// would already be giving check.  bishops/rooks are the same when a bishop/rook is moving)
				Bitboard revealedMoves;
				switch (at)
				{
				case PAWN:
					{
						const int row = GetRow(to);
						if ((row == RANK_7 && us == WHITE) || (row == RANK_2 && us == BLACK))
						{
							// Don't generate promotion moves
							revealedMoves = 0;
							break;
						}
						revealedMoves = GetPawnMoves(to, us) & emptySquares;
						if (revealedMoves && (row == RANK_2 || row == RANK_7))
						{
							// Double pawn push
							revealedMoves |= GetPawnMoves(MakeSquare(row == RANK_2 ? RANK_3 : RANK_6, GetColumn(to)), us) & emptySquares;
						}

						revealedMoves &= ~GetSquaresBetween(from, kingSquare);
					}
					break;
				case KNIGHT:
					revealedMoves = GetKnightAttacks(to) & emptySquares;
					break;
				case BISHOP:
					if (ourPiece == ROOK)
					{
						revealedMoves = GetBishopAttacks(to, allPieces) & emptySquares;
					}
					else
					{
						ASSERT(ourPiece == QUEEN);
						revealedMoves = 0;
					}
					break;
				case ROOK:
					if (ourPiece == BISHOP)
					{
						revealedMoves = GetRookAttacks(to, allPieces) & emptySquares;
					}
					else
					{
						ASSERT(ourPiece == QUEEN);
						revealedMoves = 0;
					}
					break;
				case QUEEN:
					ASSERT(false);
					break;
				case KING:
					revealedMoves = GetKingAttacks(to) & emptySquares & ~GetSquaresBetween(from, kingSquare);
					break;
				}

				while (revealedMoves)
				{
					Square revealedTo = PopFirstBit(revealedMoves);
					moves[moveCount++] = GenerateMove(to, revealedTo);
				}
			}
		}
	}

	return moveCount;
}

// Generates legal moves to escape from check.  If no legal moves are generated, we are in checkmate.
int GenerateCheckEscapeMoves(const Position &position, Move *moves)
{
	const Color us = position.ToMove;
	const Color them = FlipColor(us);
	const Square kingSquare = position.KingPos[us];

	int moveCount = 0;

	// King moves
	const Bitboard allPieces = position.GetAllPieces();
	Bitboard allPiecesMinusKing = allPieces;
	XorClearBit(allPiecesMinusKing, kingSquare);
	Bitboard b = GetKingAttacks(kingSquare) & ~position.Colors[us];
	while (b)
	{
		const Square to = PopFirstBit(b);
		if (!position.IsSquareAttacked(to, them, allPiecesMinusKing))
		{
			moves[moveCount++] = GenerateMove(kingSquare, to);
		}
	}

	// Check for number of checking pieces
	const Bitboard attacksToKing = position.GetAttacksTo(kingSquare);
	const Bitboard checkingPieces = attacksToKing & position.Colors[them];
	ASSERT(checkingPieces != 0);	// We should have some checking pieces if we are in check
	
	// If there are multiple checking pieces, the only way to escape check is by moving our poor king.
	if (!(checkingPieces & (checkingPieces - 1)))
	{
		// Only one checking piece.  We have two possibilities to save our king:
		// 1. Block the checking piece
		// 2. Capture the checking piece
		const Square checkingSquare = GetFirstBitIndex(checkingPieces);
		const Bitboard notPinnedPieces = ~position.GetPinnedPieces(kingSquare, us);

		// First, try to capture the checking piece, without using a pinned piece
		const Bitboard ourPieces = position.Colors[us] & notPinnedPieces;

		// Pawn captures are as usual, special, and annoying
		b = GetPawnAttacks(checkingSquare, them) & (position.Pieces[PAWN] & ourPieces);
		while (b)
		{
			Square from = PopFirstBit(b);
			if (GetRow(checkingSquare) == RANK_1 || GetRow(checkingSquare) == RANK_8)
			{
				moves[moveCount++] = GeneratePromotionMove(from, checkingSquare, PromotionTypeQueen);
				moves[moveCount++] = GeneratePromotionMove(from, checkingSquare, PromotionTypeKnight);
				moves[moveCount++] = GeneratePromotionMove(from, checkingSquare, PromotionTypeRook);
				moves[moveCount++] = GeneratePromotionMove(from, checkingSquare, PromotionTypeBishop);
			}
			else
			{
				moves[moveCount++] = GenerateMove(from, checkingSquare);
			}
		}

		// Initialize the targets to be the checking piece
		Bitboard targets = checkingPieces;

		// Try to block the checking piece, without moving a pinned piece.  But only if it's a slider
		if (GetPieceType(position.Board[checkingSquare]) >= BISHOP)
		{
			// Try to block with a pawn
			const Bitboard squaresBetween = GetSquaresBetween(kingSquare, checkingSquare);

			// Evil code to determine if a pawn can block the checking piece
			b = position.Pieces[PAWN] & ourPieces;
			while (b)
			{
				const Square from = PopFirstBit(b);

				Bitboard pawnMoves = GetPawnMoves(from, us) & ~allPieces;
				if (pawnMoves)
				{
					const int row = GetRow(from);
 					// Double moves
					if (us == WHITE && row == RANK_2)
					{
						SetBit(pawnMoves, MakeSquare(RANK_4, GetColumn(from)));
					}
					else if (us == BLACK && row == RANK_7)
					{
						SetBit(pawnMoves, MakeSquare(RANK_5, GetColumn(from)));
					}

					// We need to land on an in-between square, and it has to be empty.
					pawnMoves &= squaresBetween & ~allPieces;

					while (pawnMoves)
					{
						const Square to = PopFirstBit(pawnMoves);
						if (GetRow(to) == RANK_1 || GetRow(to) == RANK_8)
						{
							// Promotions
							moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeQueen);
							moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeKnight);
							moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeRook);
							moves[moveCount++] = GeneratePromotionMove(from, to, PromotionTypeBishop);
						}
						else
						{
							moves[moveCount++] = GenerateMove(from, to);
						}
					}
				}
			}

			// Allow the other pieces to try to land on the between squares
			targets |= squaresBetween;
		}

		// Now, actually generate the captures (and potentially blocking moves) - this correctly respects pinned pieces
		MoveGenerationLoop(GetKnightAttacks(from), position.Pieces[KNIGHT]);
		MoveGenerationLoop(GetBishopAttacks(from, allPieces), position.Pieces[BISHOP] | position.Pieces[QUEEN]);
		MoveGenerationLoop(GetRookAttacks(from, allPieces), position.Pieces[ROOK] | position.Pieces[QUEEN]);

		if (position.EnPassent != -1)
		{
			// We could potentially be checked by this piece.  Discovered checks will be handled elsewhere.  So, the only
			// move that could save us here is an e.p. capture.
			b = GetPawnAttacks(position.EnPassent, them) & ourPieces & position.Pieces[PAWN];
			while (b)
			{
				Square from = PopFirstBit(b);
				// Unfortunately, the capture has to be a legal move.  So, check if by moving our pawn, and removing the other
				// pawn causes a revealed check.
				Bitboard allPiecesMinusEp = allPieces;
				XorClearBit(allPiecesMinusEp, from);

				const Square epSquare = (position.EnPassent - from < 0) ? position.EnPassent + 8 : position.EnPassent - 8;
				XorClearBit(allPiecesMinusEp, epSquare);

				if (!position.IsSquareAttacked(kingSquare, them, allPiecesMinusEp))
				{
					moves[moveCount++] = GenerateEnPassentMove(from, position.EnPassent);
				}
			}
		}
	}

	return moveCount;
}

bool IsMovePseudoLegal(const Position &position, const Move move)
{
	const Square from = GetFrom(move);
	const Square to = GetTo(move);

	const Color us = position.ToMove;
	const Color them = FlipColor(us);
	
	const Piece piece = position.Board[from];
	const Piece targetPiece = position.Board[to];
	if (piece == PIECE_NONE ||
		// Have to move our own piece
		GetPieceColor(piece) != us ||
		// Can't be capturing our own pieces
		(targetPiece != PIECE_NONE && GetPieceColor(position.Board[to]) == us))
	{
		return false;
	}

	const Move moveType = GetMoveType(move);
	if (moveType == MoveTypeNone)
	{
		// Verify that the given piece type can actually make it to the the target square
		switch (GetPieceType(piece))
		{
		case PAWN: break;
		case KNIGHT: return IsBitSet(GetKnightAttacks(from), to);
		case BISHOP: return IsBitSet(GetBishopAttacks(from, position.GetAllPieces()), to);
		case ROOK: return IsBitSet(GetRookAttacks(from, position.GetAllPieces()), to);
		case QUEEN: return IsBitSet(GetBishopAttacks(from, position.GetAllPieces()) | GetRookAttacks(from, position.GetAllPieces()), to);
		case KING: return IsBitSet(GetKingAttacks(from), to);
		default: return false;
		}
	}

	if (moveType == MoveTypeNone)
	{
		if (position.Board[to] == PIECE_NONE)
		{
			// Single pawn moves
			if (IsBitSet(GetPawnMoves(from, us) & ~(RowBitboard[RANK_1] | RowBitboard[RANK_8]), to))
			{
				return true;
			}

			// Double pawn push, intermediate square and final square must be
			// empty, and we must be moving up two squares
			const Square nextSquare = GetFirstBitIndex(GetPawnMoves(from, us));
			if (position.Board[nextSquare] == PIECE_NONE &&
				IsBitSet(GetPawnMoves(nextSquare, us), to) &&
				((us == WHITE && GetRow(from) == RANK_2) || (us == BLACK && GetRow(from) == RANK_7)))
			{
				return true;
			}
		}
		else
		{
			// Has to be a pawn capture
			return IsBitSet(GetPawnAttacks(from, us) & ~(RowBitboard[RANK_1] | RowBitboard[RANK_8]), to);
		}
	}
	else if (moveType == MoveTypePromotion)
	{
		if (GetPieceType(piece) == PAWN)
		{
			if (position.Board[to] == PIECE_NONE)
			{
				// Single pawn promotions
				if (IsBitSet(GetPawnMoves(from, us) & (RowBitboard[RANK_1] | RowBitboard[RANK_8]), to))
				{
					return true;
				}
			}
			else
			{
				// Has to be a pawn capture
				return IsBitSet(GetPawnAttacks(from, us) & (RowBitboard[RANK_1] | RowBitboard[RANK_8]), to);
			}
		}
	}
	else if (moveType == MoveTypeCastle)
	{
		// We can't be in check to allow a castle move
		if (GetPieceType(piece) == KING && !position.IsInCheck())
		{
			// Castling is a bit tricky, as we don't do any MakeMove validation of castling moves
			const int castleFlags = us == WHITE ? position.CastleFlags : position.CastleFlags >> 2;

			if (GetColumn(to) == FILE_G &&
				castleFlags & CastleFlagWhiteKing)
			{
				return IsKingsideCastleLegal(position, position.GetAllPieces(), us, them);
			}
			if (GetColumn(to) == FILE_C &&
				castleFlags & CastleFlagWhiteQueen)
			{
				return IsQueensideCastleLegal(position, position.GetAllPieces(), us, them);
			}
		}
	}
	else if (moveType == MoveTypeEnPassent)
	{
		if (GetPieceType(piece) == PAWN)
		{
			return IsBitSet(GetPawnAttacks(from, us), to) &&
				position.EnPassent == to;
		}
	}

	return false;
}

int GenerateLegalMoves(Position &position, Move *legalMoves)
{
	Move moves[256];
	s16 moveScores[256];
	int moveCount;
	if (position.IsInCheck())
	{
		moveCount = GenerateCheckEscapeMoves(position, moves);
	}
	else
	{
		moveCount = GenerateQuietMoves(position, moves);
		moveCount += GenerateCaptureMoves(position, moves + moveCount, moveScores);
	}

	int legalCount = 0;
	for (int i = 0; i < moveCount; i++)
	{
		MoveUndo moveUndo;
		position.MakeMove(moves[i], moveUndo);
		if (!position.CanCaptureKing())
		{
			legalMoves[legalCount++] = moves[i];
		}
		position.UnmakeMove(moves[i], moveUndo);
	}

	return legalCount;
}