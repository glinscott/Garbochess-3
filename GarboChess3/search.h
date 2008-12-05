const int OnePly = 8;

bool FastSee(const Position &position, const Move move);
int QSearch(Position &position, int alpha, const int beta, const int depth);
int QSearchCheck(Position &position, int alpha, const int beta, const int depth);