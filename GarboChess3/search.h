const int OnePly = 8;

int See(Position &position,  Move move);
int QSearch(Position &position, int alpha, int beta, int depth);
int QSearchCheck(Position &position, int alpha, int beta, int depth);