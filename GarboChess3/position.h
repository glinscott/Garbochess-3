#include <string>

class Position
{
public:
	u64 Hash;
	u64 PawnHash;

	Bitboard Pieces[8];
	Bitboard Colors[2];
	int KingPos[2];
	Color ToMove;
	int CastleFlags;
	Square EnPassent;
	int Fifty;
	
	static void StaticInitialize();
	void Initialize(const std::string &fen);
	void MakeMove(const Move move);
	void UnmakeMove(const Move move);

private:
	u64 GetHash() const;
	u64 GetPawnHash() const;

	static u64 zobrist[2][8][64];
	static u64 zobristEP[64];
	static u64 zobristCastle[16];
	static u64 zobristToMove;
};