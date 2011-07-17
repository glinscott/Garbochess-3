#define EVAL_FEATURE(featureName, value) const int (featureName) = (value);
#define EVAL_CONST const

const int MinEval = -32767;
const int MaxEval = 32767;

extern int PsqTableOpening[16][64];
extern int PsqTableEndgame[16][64];

void InitializeEvaluation();

struct EvalInfo
{
	int GamePhase[2];
	bool KingDanger[2];
};

int Evaluate(const Position &position, EvalInfo &evalInfo);

template<class T>
inline const T& min(const T &a, const T &b) { return a < b ? a : b; }

template<class T>
inline const T& max(const T &a, const T &b) { return a > b ? a : b; }