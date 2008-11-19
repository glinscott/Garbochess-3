#include <cstdio>
#include <string>
#include <cassert>

#define ASSERT(a) (assert(a))

typedef signed short s16;
typedef unsigned short u16;

typedef signed long long s64;
typedef unsigned long long u64;

// White is at the "bottom" of the bitboard, in the bits 56-63 for Rank A for example.
// So, white pawns move by subtracting 1 from their row/rank
typedef u64 Bitboard;
typedef u16 Move;
typedef int Square;

typedef int Piece;
typedef int Color;

enum Piece_Type
{
	NONE,
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

inline Square MakeSquare(const int row, const int column)
{
	return (row << 3) | column;
}

inline Color FlipColor(const Color color)
{
	ASSERT(color == WHITE || color == BLACK);

	return color ^ 1;
}

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

inline u64 GetLowSetBit(const Bitboard &board)
{
	// TODO: asm
	return board & (-(s64)board);
}

static const int BitTable[64] = {
	0, 1, 2, 7, 3, 13, 8, 19, 4, 25, 14, 28, 9, 34, 20, 40, 5, 17, 26, 38, 15,
	46, 29, 48, 10, 31, 35, 54, 21, 50, 41, 57, 63, 6, 12, 18, 24, 27, 33, 39,
	16, 37, 45, 47, 30, 53, 49, 56, 62, 11, 23, 32, 36, 44, 52, 55, 61, 22, 43,
	51, 60, 42, 59, 58
};

Square GetFirstBitIndex(const Bitboard b)
{
	// TODO: asm
	return Square(BitTable[((b & -s64(b)) * 0x218a392cd3d5dbfULL) >> 58]);
}

Square PopFirstBit(Bitboard &b)
{
	// TODO: asm
	const Bitboard bb = b;
	b &= (b - 1);
	return Square(BitTable[((bb & -s64(bb)) * 0x218a392cd3d5dbfULL) >> 58]); 
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
}

class Position
{
public:
	Bitboard Pieces[8];
	Bitboard Colors[2];
	int KingPos[2];
	u64 Hash;
	u64 PawnHash;
	Color ToMove;
	int CastleFlags;
	
	void Initialize(const std::string &fen);
};

void Position::Initialize(const std::string &fen)
{
	
}

/*void Position::MakeMove(const Move move)
{

}*/

bool IsSquareValid(const Square square)
{
	return square >= 0 && square < 64;
}

Move GenerateMove(const Square from, const Square to)
{
	ASSERT(IsSquareValid(from));
	ASSERT(IsSquareValid(to));

	return from | (to << 5);
}

#define MoveGenerationLoop(func, piece)								\
	b = position.Pieces[(piece)] & ourPieces;						\
	while (b)														\
	{																\
		const Square from = PopFirstBit(b);							\
		Bitboard attacks = (func) & targets;						\
		while (attacks)												\
		{															\
			const Square to = PopFirstBit(b);						\
			moves[moveCount++] = GenerateMove(from, to);			\
		}															\
	}

int GenerateQuietMoves(const Position &position, Move *moves)
{
	const Bitboard ourPieces = position.Pieces[position.ToMove];
	const Bitboard allPieces = position.Pieces[0] | position.Pieces[1];
	const Bitboard targets = ~allPieces;
	
	int moveCount = 0;
	Bitboard b;

	// Pawn double hops
	// TODO

	// Castling moves
	// TODO

	// Normal piece moves
	MoveGenerationLoop(GetPawnMoves(from, position.ToMove), PAWN);
	MoveGenerationLoop(GetKnightAttacks(from), KNIGHT);
	MoveGenerationLoop(GetBishopAttacks(from, allPieces), BISHOP);
	MoveGenerationLoop(GetRookAttacks(from, allPieces), ROOK);
	MoveGenerationLoop(GetQueenAttacks(from, allPieces), QUEEN);
	MoveGenerationLoop(GetKingAttacks(from), KING);

	return moveCount;
}

int GenerateCaptures(const Position &position, Move *moves)
{
	const Bitboard ourPieces = position.Pieces[position.ToMove];
	const Bitboard allPieces = position.Pieces[0] | position.Pieces[1];
	const Bitboard targets = position.Pieces[FlipColor(position.ToMove)];
	
	int moveCount = 0;
	Bitboard b;

	// Normal piece moves
	MoveGenerationLoop(GetPawnAttacks(from, position.ToMove), PAWN);
	MoveGenerationLoop(GetKnightAttacks(from), KNIGHT);
	MoveGenerationLoop(GetBishopAttacks(from, allPieces), BISHOP);
	MoveGenerationLoop(GetRookAttacks(from, allPieces), ROOK);
	MoveGenerationLoop(GetQueenAttacks(from, allPieces), QUEEN);
	MoveGenerationLoop(GetKingAttacks(from), KING);

	return moveCount;
}

int GenerateCheckingMoves(const Position &position)
{
	int moveCount = 0;
	return moveCount;
}

int GenerateCheckEscapeMoves(const Position &position)
{
	int moveCount = 0;
	return moveCount;
}

void PrintBitboard(const Bitboard b)
{
	for (int row = 0; row < 8; row++)
	{
		for (int col = 0; col < 8; col++)
		{
			std::printf("%c", IsBitSet(b, MakeSquare(row, col)) ? 'X' : '.');
		}
		std::printf("\n");
	}
}

void UnitTests()
{
	Bitboard b = 0;
	SetBit(b, 32);
	ASSERT(b == 0x0000000100000000ULL);
	ASSERT(CountBitsSet(b) == 1);

	ClearBit(b, 33);
	ClearBit(b, 31);
	ASSERT(b == 0x0000000100000000ULL);

	ClearBit(b, 32);
	ASSERT(b == 0);
	ASSERT(CountBitsSet(b) == 0);

	b = 0xFFFF;
	ASSERT(GetLowSetBit(b) == 1);
	ASSERT(CountBitsSet(b) == 16);

	b = 0;
	SetBit(b, 15);
	ASSERT(GetLowSetBit(b) == 0x8000);
	ASSERT(GetFirstBitIndex(b) == 15);
	ASSERT(PopFirstBit(b) == 15);
	ASSERT(b == 0);

	b = 0x0001000100010001ULL;
	ASSERT(GetLowSetBit(b) == 1);
	ASSERT(GetFirstBitIndex(b) == 0);
	ASSERT(CountBitsSet(b) == 4);
	ASSERT(PopFirstBit(b) == 0);
	ASSERT(GetLowSetBit(b) == 0x10000);
	ASSERT(GetFirstBitIndex(b) == 16);
	ASSERT(CountBitsSet(b) == 3);

	// TODO: unit test rook attacks and bishop attacks
}

int main()
{
	InitializeBitboards();

	UnitTests();

	for (int i = 0; i < 64; i++)
	{
		PrintBitboard(GetPawnMoves(i, BLACK));
	}

	return 0;
}