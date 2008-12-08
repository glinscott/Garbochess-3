#define EVAL_FEATURE(featureName, value) const int (featureName) = (value);
#define EVAL_CONST const

const int MinEval = -32767;
const int MaxEval = 32767;

extern int PsqTableOpening[16][64];
extern int PsqTableEndgame[16][64];

void InitializePsqTable();

int Evaluate(const Position &position);