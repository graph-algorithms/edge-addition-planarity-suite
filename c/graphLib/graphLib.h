#ifndef GRAPHLIB_H
#define GRAPHLIB_H

/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "c/graphLib/graph.h"
#include "c/graphLib/lowLevelUtils/platformTime.h"

#include "c/graphLib/homeomorphSearch/graphK23Search.h"
#include "c/graphLib/homeomorphSearch/graphK33Search.h"
#include "c/graphLib/homeomorphSearch/graphK4Search.h"
#include "c/graphLib/planarityRelated/graphDrawPlanar.h"

#ifdef __cplusplus
}
#endif

#endif