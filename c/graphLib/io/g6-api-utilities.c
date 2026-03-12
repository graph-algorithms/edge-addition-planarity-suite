/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "g6-api-utilities.h"

int _getMaxEdgeCount(int order)
{
    return (order * (order - 1)) / 2;
}

int _getNumCharsForGraphEncoding(int order)
{
    int maxNumEdges = _getMaxEdgeCount(order);

    return (maxNumEdges / 6) + (maxNumEdges % 6 ? 1 : 0);
}

int _getNumCharsForGraphOrder(int order)
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

int _getExpectedNumPaddingZeroes(const int order, const int numChars)
{
    int maxNumEdges = _getMaxEdgeCount(order);
    int expectedNumPaddingZeroes = numChars * 6 - maxNumEdges;

    return expectedNumPaddingZeroes;
}
