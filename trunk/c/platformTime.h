#ifndef PLATFORM_TIME
#define PLATFORM_TIME

#ifdef WIN32

#include <windows.h>
#include <winbase.h>
#define platform_time DWORD
#define platform_GetTime GetTickCount
#define platform_GetDuration(startTime, endTime) ((double) (endTime-startTime) / 1000.0)

#else

#include <time.h>
#define platform_time clock_t
#define platform_GetTime() clock()
#define platform_GetDuration(startTime, endTime) (((double) (endTime - startTime)) / CLOCKS_PER_SEC)
/*
#define platform_time time_t
#define platform_GetTime() time((time_t *)NULL)
#define platform_GetDuration(startTime, endTime) ((double) (endTime - startTime))
*/

#endif

#endif
