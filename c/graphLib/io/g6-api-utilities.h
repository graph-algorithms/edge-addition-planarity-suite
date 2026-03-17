/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef G6_API_UTILITIES
#define G6_API_UTILITIES

#ifdef __cplusplus
extern "C"
{
#endif

    // FIXME: Should these be declared in g6-api-utilities.c and then
    // use extern at the top of g6-(read|write)-iterator.c source files to use
    // them?
    int _g6_GetMaxEdgeCount(int);
    int _g6_GetNumCharsForEncoding(int);
    int _g6_GetNumCharsForOrder(int);
    int _g6_GetExpectedNumPaddingZeroes(const int, const int);

#ifdef __cplusplus
}
#endif

#endif /* G6_API_UTILITIES */
