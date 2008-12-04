#define EVAL_FEATURE(featureName, value) const int (featureName) = (value);
#define EVAL_CONST const

extern int PsqTableOpening[16][64];
extern int PsqTableEndgame[16][64];

void InitializePsqTable();

int Evaluate(const Position &position);