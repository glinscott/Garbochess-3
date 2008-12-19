#include <windows.h>
#include <conio.h>
#include "garbochess.h"

u64 GetCurrentMilliseconds()
{
	static bool TimerIsInitialized = false;
	static u64 TimerDivisor;

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

bool CheckForPendingInput()
{
	static bool IsCheckForPendingInputInitialized = false;
	static HANDLE readHandle;
	static int pipe;

	DWORD result;

	if (!IsCheckForPendingInputInitialized)
	{
		IsCheckForPendingInputInitialized = true;
		readHandle = GetStdHandle(STD_INPUT_HANDLE);
		pipe = !GetConsoleMode(readHandle, &result);
		if (!pipe)
		{
			SetConsoleMode(readHandle, result & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
			FlushConsoleInputBuffer(readHandle);
		}
	}
	if (pipe)
	{
		if (PeekNamedPipe(readHandle, NULL, 0, NULL, &result, NULL) == 0)
			return false;
		return result > 0;
	}
	else
	{
		return false;
		// This does not appear to work at all...
//		GetNumberOfConsoleInputEvents(readHandle, &result);
//		return result > 1;
	}
}
