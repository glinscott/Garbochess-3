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
	void Clone(Position &other) const;

	// Debug only!
	void Flip();

	void MakeMove(const Move move, MoveUndo &moveUndo);
	void UnmakeMove(const Move move, const MoveUndo &moveUndo);

	void MakeNullMove(MoveUndo &moveUndo);
	void UnmakeNullMove(MoveUndo &moveUndo);

	inline bool IsSquareAttacked(const Square square, const Color them) const
	{
		return IsSquareAttacked(square, them, GetAllPieces());
	}
	inline bool IsSquareAttacked(const Square square, const Color them, const Bitboard allPieces) const
	{
		const Bitboard enemyPieces = Colors[them] & allPieces;

		if ((GetPawnAttacks(square, FlipColor(them)) & Pieces[PAWN] & enemyPieces) ||
			(GetKnightAttacks(square) & Pieces[KNIGHT] & enemyPieces))
		{
			return true;
		}

		const Bitboard bishopQueen = Pieces[BISHOP] | Pieces[QUEEN];
		if (GetBishopAttacks(square, allPieces) & bishopQueen & enemyPieces)
		{
			return true;
		}

		const Bitboard rookQueen = Pieces[ROOK] | Pieces[QUEEN];
		if (GetRookAttacks(square, allPieces) & rookQueen & enemyPieces)
		{
			return true;
		}

		if (GetKingAttacks(square) & Pieces[KING] & enemyPieces)
		{
			return true;
		}

		return false;
	}

	inline bool IsInCheck() const { return IsSquareAttacked(KingPos[ToMove], FlipColor(ToMove)); }
	inline bool CanCaptureKing() const { return IsSquareAttacked(KingPos[FlipColor(ToMove)], ToMove); }

	Bitboard GetAttacksTo(const Square square) const;
	Bitboard GetPinnedPieces(const Square square, const Color us) const;

	inline bool IsDraw() const
	{
		if (Fifty >= 100)
		{
			return true;
		}

		// Check our previous positions.  If the hash key matches, it is a draw.
		const int end = MoveDepth - Fifty;
		for (int i = MoveDepth - 4; i >= end; i -= 2)
		{
			if (DrawKeys[i] == Hash)
			{
				return true;
			}
		}
		return false;
	}

	inline void ResetMoveDepth()
	{
		MoveDepth = 0;
	}

private:
	int MoveDepth;		// used for tracking repitition draws
	u64 DrawKeys[256];

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

std::string GetMoveSAN(Position &position, const Move move);
