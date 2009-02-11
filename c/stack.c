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

#include "appconst.h"
#include "stack.h"
#include <stdlib.h>

stackP sp_New(int Size)
{
stackP theStack;

     theStack = (stackP) malloc(sizeof(stack));

     if (theStack != NULL)
     {
         theStack->S = (int *) malloc(Size*sizeof(int));
         if (theStack->S == NULL)
         {
             free(theStack);
             theStack = NULL;
         }
     }

     if (theStack != NULL)
     {
         theStack->Size = Size;
         sp_ClearStack(theStack);
     }

     return theStack;
}

void sp_Free(stackP *pStack)
{
     if (pStack == NULL || *pStack == NULL) return;

     (*pStack)->Size = (*pStack)->Top = 0;

     if ((*pStack)->S != NULL)
          free((*pStack)->S);
     (*pStack)->S = NULL;
     free(*pStack);

     *pStack = NULL;
}

int  sp_GetCurrentSize(stackP theStack)
{
     return theStack->Top;
}

int  sp_CopyContent(stackP stackDst, stackP stackSrc)
{
     if (stackDst->Size < stackSrc->Top)
         return NOTOK;

     if (stackSrc->Top > 0)
         memcpy(stackDst->S, stackSrc->S, stackSrc->Top*sizeof(int));

     stackDst->Top = stackSrc->Top;
     return OK;
}

stackP sp_Duplicate(stackP theStack)
{
stackP newStack = sp_New(theStack->Size);

    if (newStack == NULL)
        return NULL;

    if (theStack->Top > 0)
        memcpy(newStack->S, theStack->S, theStack->Top*sizeof(int));

    return newStack;
}

int  sp_Copy(stackP stackDst, stackP stackSrc)
{
    if (sp_CopyContent(stackDst, stackSrc) != OK)
    {
    stackP newStack = sp_Duplicate(stackSrc);
    int  *p;

         if (newStack == NULL)
             return NOTOK;

         p = stackDst->S;
         stackDst->S = newStack->S;
         newStack->S = p;
         newStack->Size = stackDst->Size;
         sp_Free(&newStack);

         stackDst->Top = stackSrc->Top;
         stackDst->Size = stackSrc->Size;
    }

    return OK;
}

#ifndef SPEED_MACROS

int  sp_ClearStack(stackP theStack)
{
     theStack->Top = 0;
     return OK;
}

int  sp_IsEmpty(stackP theStack)
{
     return !theStack->Top;
}

int  sp_NonEmpty(stackP theStack)
{
     return theStack->Top;
}

int  sp_Push(stackP theStack, int a)
{
     if (theStack->Top >= theStack->Size)
         return NOTOK;

     theStack->S[theStack->Top++] = a;
     return OK;
}

int  sp_Push2(stackP theStack, int a, int b)
{
     if (theStack->Top + 1 >= theStack->Size)
         return NOTOK;

     theStack->S[theStack->Top++] = a;
     theStack->S[theStack->Top++] = b;
     return OK;
}

int  sp__Pop(stackP theStack, int *pA)
{
     if (theStack->Top <= 0)
         return NOTOK;

     *pA = theStack->S[--theStack->Top];
     return OK;
}

int  sp__Pop2(stackP theStack, int *pA, int *pB)
{
     if (theStack->Top <= 1)
         return NOTOK;

     *pB = theStack->S[--theStack->Top];
     *pA = theStack->S[--theStack->Top];

     return OK;
}

int  sp_Top(stackP theStack)
{
    return theStack->Top ? theStack->S[theStack->Top-1] : NIL;
}

#endif // not defined SPEED_MACROS
