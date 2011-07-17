#pragma warning(disable:4530)

#ifdef _MSC_VER
#include <intrin.h>
#endif

#if _DEBUG
extern "C" {
void __declspec(dllimport) __stdcall DebugBreak(void);
}
#define ASSERT(a) (!(a) ? DebugBreak() : 0)
#else
#define ASSERT(a)
#endif

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

// White is at the "bottom" of the bitboard, in the bits 56-63 for Rank 1 for example.
// So, white pawns move by subtracting 1 from their row/rank
typedef u64 Bitboard;

// Move 0-5: from, 6-11: to, 12-13: promotion type, 14-15: flags
typedef u16 Move;
typedef int Square;

typedef int Piece;
typedef int PieceType;
typedef int Color;

const Move PromotionTypeKnight = 0 << 12;
const Move PromotionTypeBishop = 1 << 12;
const Move PromotionTypeRook = 2 << 12;
const Move PromotionTypeQueen = 3 << 12;
const Move PromotionTypeMask = 3 << 12;

const Move MoveTypeNone = 0 << 14;
const Move MoveTypePromotion = 1 << 14;
const Move MoveTypeCastle = 2 << 14;
const Move MoveTypeEnPassent = 3 << 14;
const Move MoveTypeMask = 3 << 14;

const int CastleFlagWhiteKing = 1;
const int CastleFlagWhiteQueen = 2;
const int CastleFlagBlackKing = 4;
const int CastleFlagBlackQueen = 8;
const int CastleFlagMask = 15;

enum Piece_Type
{
	PIECE_NONE,
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING
};

enum Color_Type
{
	WHITE,
	BLACK
};

// Files are columns
enum File_Type
{
	FILE_A,	FILE_B,	FILE_C,	FILE_D,	FILE_E,	FILE_F,	FILE_G,	FILE_H
};

// Ranks are rows.  Note that RANK_8 (Black home row) is index 0
enum Rank_Type
{
	RANK_8, RANK_7, RANK_6, RANK_5, RANK_4, RANK_3, RANK_2, RANK_1
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Basic operations (Piece, Row, Column)
////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IsSquareValid(const Square square)
{
	return square >= 0 && square < 64;
}

inline Square MakeSquare(const int row, const int column)
{
	ASSERT(row >= 0 && row < 8);
	ASSERT(column >= 0 && column < 8);

	return (row << 3) | column;
}

inline Square FlipSquare(const Square square)
{
	ASSERT(IsSquareValid(square));
	return square ^ 070;
}

inline int GetRow(const Square square)
{
	ASSERT(IsSquareValid(square));
	return square >> 3;
}

inline int GetColumn(const Square square)
{
	ASSERT(IsSquareValid(square));
	return square & 7;
}

inline Color FlipColor(const Color color)
{
	ASSERT(color == WHITE || color == BLACK);
	return color ^ 1;
}

inline Piece MakePiece(const Color color, const PieceType piece)
{
	return (color << 3) | piece;
}

inline PieceType GetPieceType(const Piece piece)
{
	return piece & 7;
}

inline Color GetPieceColor(const Piece piece)
{
	return piece >> 3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Bitboard operations
////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IsBitSet(const Bitboard board, const Square square)
{
	return (board >> square) & 1;
}

inline void SetBit(Bitboard &board, const Square square)
{
	board |= 1ULL << square;
}

inline void ClearBit(Bitboard &board, const Square square)
{
	board &= ~(1ULL << square);
}

inline void XorClearBit(Bitboard &board, const Square square)
{
	ASSERT(IsBitSet(board, square));
	board ^= 1ULL << square;
}

extern const int BitTable[64];

inline Square GetFirstBitIndex(const Bitboard b)
{
#ifndef X64
	return Square(BitTable[((b & -s64(b)) * 0x218a392cd3d5dbfULL) >> 58]); 
#else
	unsigned long index;
	_BitScanForward64(&index, b);
	return index;
#endif
}

inline Square PopFirstBit(Bitboard &b)
{
	const Bitboard bb = b;
	b &= (b - 1);
	return GetFirstBitIndex(bb);
}

inline int CountBitsSet(Bitboard b)
{
	b -= ((b >> 1) & 0x5555555555555555ULL);
	b = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
	b = ((b >> 4) + b) & 0x0F0F0F0F0F0F0F0FULL;
	b *= 0x0101010101010101ULL;
	return int(b >> 56);
}

inline int CountBitsSetMax15(Bitboard b)
{
	b -= (b >> 1) & 0x5555555555555555ULL;
	b = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
	b *= 0x1111111111111111ULL;
	return int(b >> 60);
}

inline int CountBitsSetFew(Bitboard b)
{
	int result = 0;
	while (b)
	{
		b &= (b - 1);
		result++;
	}
	return result;
}

inline Bitboard FlipBitboard(const Bitboard b)
{
	Bitboard result = 0;
	for (int row = 0; row < 8; row++)
	{
		result |= ((b >> (row * 8)) & 0xFF) << ((7 - row) * 8);
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Move operations
////////////////////////////////////////////////////////////////////////////////////////////////////
inline Square GetFrom(const Move move)
{
	return move & 0x3F;
}

inline Square GetTo(const Move move)
{
	return (move >> 6) & 0x3F;
}

inline Move GetMoveType(const Move move)
{
	return move & MoveTypeMask;
}

inline PieceType GetPromotionMoveType(const Move move)
{
	const int promotionMove = move & PromotionTypeMask;
	if (promotionMove == PromotionTypeQueen)
	{
		return QUEEN;
	}
	else if (promotionMove == PromotionTypeKnight)
	{
		return KNIGHT;
	}
	else if (promotionMove == PromotionTypeRook)
	{
		return ROOK;
	}
	else if (promotionMove == PromotionTypeBishop)
	{
		return BISHOP;
	}
	// Whoops
	ASSERT(false);
	return PIECE_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Attack generation
////////////////////////////////////////////////////////////////////////////////////////////////////

extern Bitboard RowBitboard[8];
extern Bitboard ColumnBitboard[8];

extern Bitboard PawnMoves[2][64];
extern Bitboard PawnAttacks[2][64];
extern Bitboard KnightAttacks[64];

extern Bitboard BMask[64];
extern int BAttackIndex[64];
extern Bitboard BAttacks[0x1480];

extern const u64 BMult[64];
extern const int BShift[64];

extern Bitboard RMask[64];
extern int RAttackIndex[64];
extern Bitboard RAttacks[0x19000];

extern const u64 RMult[64];
extern const int RShift[64];

extern Bitboard KingAttacks[64];

inline Bitboard GetPawnMoves(const Square square, const Color color)
{
	return PawnMoves[color][square];
}

inline Bitboard GetPawnAttacks(const Square square, const Color color)
{
	return PawnAttacks[color][square];
}

inline Bitboard GetKnightAttacks(const Square square)
{
	return KnightAttacks[square];
}

inline Bitboard GetBishopAttacks(const Square square, const Bitboard blockers)
{
	const Bitboard b = blockers & BMask[square];
	return BAttacks[BAttackIndex[square] + ((b * BMult[square]) >> BShift[square])];
}

inline Bitboard GetRookAttacks(const Square square, const Bitboard blockers)
{
	const Bitboard b = blockers & RMask[square];
	return RAttacks[RAttackIndex[square] + ((b * RMult[square]) >> RShift[square])];
}

inline Bitboard GetQueenAttacks(const Square square, const Bitboard blockers)
{
	return GetBishopAttacks(square, blockers) | GetRookAttacks(square, blockers);
}

inline Bitboard GetKingAttacks(const Square square)
{
	return KingAttacks[square];
}

// Misc. attack functions
extern Bitboard SquaresBetween[64][64];

inline Bitboard GetSquaresBetween(const Square from, const Square to)
{
	return SquaresBetween[from][to];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Timer
////////////////////////////////////////////////////////////////////////////////////////////////////
u64 GetCurrentMilliseconds();
bool CheckForPendingInput();