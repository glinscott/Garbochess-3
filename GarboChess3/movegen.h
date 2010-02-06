void InitializeBitboards();

inline int ScoreCaptureMove(const PieceType fromPiece, const PieceType toPiece)
{
	ASSERT(fromPiece != PIECE_NONE);
	ASSERT(toPiece != PIECE_NONE);

	return toPiece * 100 - fromPiece;
}
int ScoreCaptureMove(const Move move, const PieceType fromPiece, const PieceType toPiece);

int GenerateSliderMoves(const Position &position, Move *moves);
int GenerateQuietMoves(const Position &position, Move *moves);
int GenerateCaptureMoves(const Position &position, Move *moves, s16 *moveScores);
int GenerateCheckingMoves(const Position &position, Move *moves);
int GenerateCheckEscapeMoves(const Position &position, Move *moves);
bool IsMovePseudoLegal(const Position &position, const Move move);

// Slow, and should not be used.
int GenerateLegalMoves(Position &position, Move *legalMoves);

// Move generation
inline Move GenerateMove(const Square from, const Square to)
{
	ASSERT(IsSquareValid(from));
	ASSERT(IsSquareValid(to));

	return from | (to << 6);
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