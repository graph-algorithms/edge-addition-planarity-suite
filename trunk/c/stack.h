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

#ifndef STACK_H
#define STACK_H

typedef struct
{
        int *S;
        int Top, Size;
} stack;

typedef stack * stackP;

stackP sp_New(int);
void sp_Free(stackP *);

int  sp_Copy(stackP, stackP);

int  sp_GetCurrentSize(stackP theStack);
int  sp_CopyContent(stackP stackDst, stackP stackSrc);
stackP sp_Duplicate(stackP theStack);

#ifndef SPEED_MACROS

int  sp_ClearStack(stackP);

int  sp_IsEmpty(stackP);
int  sp_NonEmpty(stackP);

int  sp_Push(stackP, int);
int  sp_Push2(stackP, int, int);

#define sp_Pop(theStack, a) sp__Pop(theStack, &(a))
#define sp_Pop2(theStack, a, b) sp__Pop2(theStack, &(a), &(b))

int  sp__Pop(stackP, int *);
int  sp__Pop2(stackP, int *, int *);

int  sp_Top(stackP);

#else

#define sp_ClearStack(theStack) theStack->Top=0

#define sp_IsEmpty(theStack) !theStack->Top
#define sp_NonEmpty(theStack) theStack->Top

#define sp_Push(theStack, a) theStack->S[theStack->Top++] = a
#define sp_Push2(theStack, a, b) {sp_Push(theStack, a); sp_Push(theStack, b);}

#define sp_Pop(theStack, a) a=theStack->S[--theStack->Top]
#define sp_Pop2(theStack, a, b) {sp_Pop(theStack, b);sp_Pop(theStack, a);}

#define sp_Top(theStack) (theStack->S[theStack->Top])

#endif

#endif
