#include <windows.h>
#include "garbochess.h"

static bool TimerIsInitialized = false;
static u64 TimerDivisor;

u64 GetCurrentMilliseconds()
{
	if (!TimerIsInitialized)
	{
		TimerIsInitialized = true;

		// Gets the number of ticks/second
		LARGE_INTEGER timerFrequency;
		QueryPerformanceFrequency(&timerFrequency);
		TimerDivisor = timerFrequency.QuadPart / 1000;
	}

	LARGE_INTEGER ticks;
	QueryPerformanceCounter(&ticks);

	return u64(ticks.QuadPart / TimerDivisor);
}