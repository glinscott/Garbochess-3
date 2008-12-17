const int OnePly = 8;

const int MaxPly = 99;
const int MaxThreads = 8;

struct SearchInfo
{
	u64 NodeCount;
	u64 QNodeCount;

	Move Killers[MaxPly][2];
};

// Set to true to stop the search as soon as possible
extern bool KillSearch;

SearchInfo &GetSearchInfo(int thread);
bool FastSee(const Position &position, const Move move);
int QSearch(Position &position, SearchInfo &searchInfo, int alpha, const int beta, const int depth);
int QSearchCheck(Position &position, SearchInfo &searchInfo, int alpha, const int beta, const int depth);
Move IterativeDeepening(Position &position, const int maxDepth, int &score, bool printSearch);

void InitializeSearch();