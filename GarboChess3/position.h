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

	Piece Board[64];
	Bitboard Pieces[8];
	Bitboard Colors[2];
	int KingPos[2];
	Color ToMove;
	int CastleFlags;
	Square EnPassent;
	int Fifty;

	inline Bitboard GetAllPieces() const { return Colors[WHITE] | Colors[BLACK]; }
	
	static void StaticInitialize();
	void Initialize(const std::string &fen);
	void MakeMove(const Move move, MoveUndo &moveUndo);
	void UnmakeMove(const Move move, const MoveUndo &moveUndo);

	inline bool IsSquareAttacked(const Square square, const Color them) const { return IsSquareAttacked(square, them, GetAllPieces()); }
	bool IsSquareAttacked(const Square square, const Color them, const Bitboard allPieces) const;
	Bitboard GetAttacksTo(const Square square) const;
//	inline Bitboard GetPinnedPieces(); // TODO

private:
	void VerifyBoard() const;
	u64 GetHash() const;
	u64 GetPawnHash() const;

	static int RookCastleFlagMask[64];
	static u64 Zobrist[2][8][64];
	static u64 ZobristEP[64];
	static u64 ZobristCastle[16];
	static u64 ZobristToMove;
};