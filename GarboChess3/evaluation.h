#define EVAL_FEATURE(featureName, value) const int (featureName) = (value);

extern int PsqTable[16][64][2];

void InitializePsqTable();