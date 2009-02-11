#ifndef GRAPH_EXTENSIONS_H
#define GRAPH_EXTENSIONS_H

/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2009, John M. Boyer
All rights reserved. Includes a reference implementation of the following:
John M. Boyer and Wendy J. Myrvold, "On the Cutting Edge: Simplified O(n)
Planarity by Edge Addition,"  Journal of Graph Algorithms and Applications,
Vol. 8, No. 3, pp. 241-273, 2004.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

* Neither the name of the Planarity-Related Graph Algorithms Project nor the names
  of its contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "graphStructures.h"

int gp_AddExtension(graphP theGraph,
                    char *module,
                    void *context,
                    void *(*dupContext)(void *, void *),
                    void (*freeContext)(void *),
                    graphFunctionTableP overloadTable);

int gp_FindExtension(graphP theGraph, char *module, void **pContext);

int gp_RemoveExtension(graphP theGraph, char *module);

int gp_CopyExtensions(graphP dstGraph, graphP srcGraph);

void gp_FreeExtensions(graphP theGraph);

#ifdef __cplusplus
}
#endif

#endif