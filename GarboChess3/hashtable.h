const int HashFlagsAlpha = 0;
const int HashFlagsBeta = 1;
const int HashFlagsExact = 2;
const int HashFlagsMask = 3;
const int HashFlagsEasyMove = 4;

#pragma pack(push, 1)
struct HashEntry
{
	u32 Lock;
	s16 Score;
	Move Move;
	u8 Depth;
	u8 Extra;

	int GetHashFlags() const
	{
		return Extra & HashFlagsMask;
	}

	int GetIsEasyMove() const
	{
		return Extra & HashFlagsEasyMove;
	}

	int GetHashDate() const
	{
		return Extra >> 4;
	}
};
#pragma pack(pop)

extern HashEntry *HashTable;
extern u64 HashMask;
extern int HashDate;

// Hash size in bytes
void InitializeHash(int hashSize);
void IncrementHashDate();

inline bool ProbeHash(const u64 hash, HashEntry *&result)
{
	const u64 base = hash & HashMask;
	ASSERT(base <= HashMask);

	const u32 lock = hash >> 32;
	for (u64 i = base; i < base + 4; i++)
	{
		if (HashTable[i].Lock == lock)
		{
			result = &(HashTable[i]);
			return true;
		}
	}

	return false;
}

inline void StoreHash(const u64 hash, const s16 score, const Move move, int depth, const int flags)
{
	const u64 base = hash & HashMask;
	ASSERT(base <= HashMask);

	const u32 lock = hash >> 32;
	int bestScore = 512;
	u64 best;

	depth /= OnePly;

	for (u64 i = base; i < base + 4; i++)
	{
		if (HashTable[i].Lock == lock)
		{
			if (depth >= HashTable[i].Depth)
			{
				best = i;
				break;
			}
			if (HashTable[i].Move == 0)
			{
				HashTable[i].Move = move;
			}
			return;
		}
		
		int matchScore;
		if (HashTable[i].GetHashDate() != HashDate)
		{
			// We want to always allow overwriting of hash entries not from our hash date
			matchScore = HashTable[i].Depth;
		}
		else
		{
			// Otherwise, choose the hash entry with the lowest depth for overwriting
			matchScore = 256 + HashTable[i].Depth;
		}

		if (matchScore < bestScore)
		{
			bestScore = matchScore;
			best = i;
		}
	}

	HashTable[best].Lock = lock;
	HashTable[best].Move = move;
	HashTable[best].Score = score;
	HashTable[best].Depth = depth;

	ASSERT(flags <= 0xf);
	ASSERT(HashDate <= 0xf);

	HashTable[best].Extra = flags | (HashDate << 4);
}