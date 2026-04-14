/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdlib.h>

#include "graphOuterplanarity.h"

/****************************************************************************
 gp_ExtendWith_Outerplanarity()

 This function is intended to extend the graph data structure by attaching
 the outerplanar graph embedding and obstruction isolation feature.

 To activate this feature during gp_Embed(), use EMBEDFLAGS_OUTERPLANAR.

 In the current implementation, outerplanarity is automatically available,
 so the method succeeds without doing work, other than returning.

 Returns OK for success, NOTOK for failure.
 ****************************************************************************/

int gp_ExtendWith_Outerplanarity(graphP theGraph)
{
    return OK;
}

/********************************************************************
 gp_Detach_Ouerplanarity()

 This function is intended to disinherit the outerplanar graph embedding
 and obstruction isolation feature by remove the extension from the graph.

 In the current implementation, outerplanarity is automatically available,
 and it cannot be removed, so this method fails without doing work.

 Returns OK on success, NOTOK on failure
 ********************************************************************/

int gp_Detach_Outerplanarity(graphP theGraph)
{
    return NOTOK;
}
