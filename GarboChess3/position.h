#include <string>

struct MoveUndo
{
	u64 Hash;
	u64 PawnHash;
	PieceType Captured;
	int CastleFlags;
	Square EnPassent;
	int Fifty;
};

class Position
{
public:
	u64 Hash;
	u64 PawnHash;

	Bitboard Pieces[8];
	Bitboard Colors[2];
	Square KingPos[2];

	int PsqEvalOpening;
	int PsqEvalEndgame;

	int CastleFlags;
	int Fifty;
	Color ToMove;
	Square EnPassent;
	Piece Board[64];

	inline Bitboard GetAllPieces() const { return Colors[WHITE] | Colors[BLACK]; }
	
	static void StaticInitialize();
	void Initialize(const std::string &fen);
	std::string GetFen() const;

	void MakeMove(const Move move, MoveUndo &moveUndo);
	void UnmakeMove(const Move move, const MoveUndo &moveUndo);

	inline bool IsSquareAttacked(const Square square, const Color them) const { return IsSquareAttacked(square, them, GetAllPieces()); }
	bool IsSquareAttacked(const Square square, const Color them, const Bitboard allPieces) const;

	inline bool IsCheck() const { return IsSquareAttacked(KingPos[ToMove], FlipColor(ToMove)); }
	inline bool CanCaptureKing() const { return IsSquareAttacked(KingPos[FlipColor(ToMove)], ToMove); }

	Bitboard GetAttacksTo(const Square square) const;
	Bitboard GetPinnedPieces(const Square square, const Color us) const;

private:
	void VerifyBoard() const;
	u64 GetHash() const;
	u64 GetPawnHash() const;
	int GetPsqEval(int gameStage) const;

	static int RookCastleFlagMask[64];
	static u64 Zobrist[2][8][64];
	static u64 ZobristEP[64];
	static u64 ZobristCastle[16];
	static u64 ZobristToMove;
};