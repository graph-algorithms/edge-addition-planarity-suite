/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "g6-api-utilities.h"

int _g6_GetMaxEdgeCount(int order)
{
    return (order * (order - 1)) / 2;
}

int _g6_GetNumCharsForEncoding(int order)
{
    int maxNumEdges = _g6_GetMaxEdgeCount(order);

    return (maxNumEdges / 6) + (maxNumEdges % 6 ? 1 : 0);
}

int _g6_GetNumCharsForOrder(int order)
{
    if (order > 0 && order < 63)
    {
        return 1;
    }
    else if (order >= 63 && order <= 100000)
    {
        return 4;
    }

    return -1;
}

int _g6_GetExpectedNumPaddingZeroes(const int order, const int numChars)
{
    int maxNumEdges = _g6_GetMaxEdgeCount(order);
    int expectedNumPaddingZeroes = numChars * 6 - maxNumEdges;

    return expectedNumPaddingZeroes;
}
