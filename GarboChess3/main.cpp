#include "garbochess.h"
#include "position.h"
#include "movegen.h"
#include "evaluation.h"
#include "search.h"

void RunTests();

void RunEngine()
{
}

int main()
{
	InitializeBitboards();
	Position::StaticInitialize();
	InitializePsqTable();

	RunTests();

	return 0;
}