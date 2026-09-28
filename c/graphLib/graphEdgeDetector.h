#ifndef GRAPH_EDGE_DETECTOR_H
#define GRAPH_EDGE_DETECTOR_H
/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        unsigned *edgeDetector;
        int edgeDetectorCapacity;
    } graphEdgeDetectorStruct;

    typedef graphEdgeDetectorStruct *graphEdgeDetectorP;

    graphEdgeDetectorP ged_New(int theCapacity);

    unsigned long long ged_Hash(graphEdgeDetectorP theDetector, int v, int w);
    int ged_Set(graphEdgeDetectorP theDetector, int v, int w);
    int ged_IsSet(graphEdgeDetectorP theDetector, int v, int w);
    graphEdgeDetectorP ged_Duplicate(graphEdgeDetectorP srcDetector);

    void ged_Free(graphEdgeDetectorP *pDetector);

#ifdef __cplusplus
}
#endif
#endif
