/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdlib.h>

#include "graphPlanarity.h"

/****************************************************************************
 gp_ExtendWith_Planarity()

 This function is intended to extend the graph data structure by attaching
 the planar graph embedding and obstruction isolation feature.

 To activate this feature during gp_Embed(), use EMBEDFLAGS_PLANAR.

 In the current implementation, planarity is automatically available,
 so the method succeeds without doing work, other than returning.

 Returns OK for success, NOTOK for failure.
 ****************************************************************************/

int gp_ExtendWith_Planarity(graphP theGraph)
{
    return OK;
}

/********************************************************************
 gp_Detach_Planarity()

 This function is intended to disinherit the planar graph embedding and
 obstruction isolation feature by remove the extension from the graph.

 In the current implementation, planarity is automatically available,
 and it cannot be removed, so this method fails without doing work.

 Returns OK on success, NOTOK on failure
 ********************************************************************/

int gp_Detach_Planarity(graphP theGraph)
{
    return NOTOK;
}
