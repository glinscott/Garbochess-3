#define EVAL_FEATURE(featureName, value) const int (featureName) = (value);

extern int PsqTableOpening[16][64];
extern int PsqTableEndgame[16][64];

void InitializePsqTable();