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
	
	static void StaticInitialize();
	void Initialize(const std::string &fen);
	void MakeMove(const Move move, MoveUndo &moveUndo);
	void UnmakeMove(const Move move, const MoveUndo &moveUndo);
	bool IsSquareAttacked(const Square square, const Color them) const;

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