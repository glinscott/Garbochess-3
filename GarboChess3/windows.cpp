#include <stdlib.h>

#if defined (_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#include <windows.h>
#include <conio.h>
#include <sys/timeb.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#endif

#include "garbochess.h"


#ifdef _MSC_VER

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


#else

u64 GetCurrentMilliseconds()
{
    
#if defined(__MINGW32__) || defined(__MINGW64__)
    struct _timeb t;
    _ftime(&t);
    return int(t.time*1000 + t.millitm);
    
#else
    
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec*1000 + t.tv_usec/1000;
    
#endif
    
}


#endif



#if defined (_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

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

#else


/* Non-windows version */
bool CheckForPendingInput()
{
    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(fileno(stdin), &readfds);
    /* Set to timeout immediately */
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    select(16, &readfds, 0, 0, &timeout);
    
    return (FD_ISSET(fileno(stdin), &readfds));
}

#endif
