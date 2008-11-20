void InitializeBitboards();

int GenerateQuietMoves(const Position &position, Move *moves);
int GenerateCaptures(const Position &position, Move *moves);
int GenerateCheckingMoves(const Position &position);
int GenerateCheckEscapeMoves(const Position &position);

// Attack generation

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

// Move generation
inline Move GenerateMove(const Square from, const Square to)
{
	ASSERT(IsSquareValid(from));
	ASSERT(IsSquareValid(to));

	return from | (to << 5);
}

inline Move GeneratePromotionMove(const Square from, const Square to, const int promotionType)
{
	return GenerateMove(from, to) | promotionType | MoveTypePromotion;
}

inline Move GenerateCastleMove(const Square from, const Square to)
{
	return GenerateMove(from, to) | MoveTypeCastle;
}

inline Move GenerateEnPassentMove(const Square from, const Square to)
{
	return GenerateMove(from, to) | MoveTypeEnPassent;
}