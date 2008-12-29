/* Copyright (c) 1997-2003, 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced in whole or in part without
        the written permission of the author. */
        
#ifndef _LISTCOLL_H
#define _LISTCOLL_H

/* This include is needed for memset and memcpy */
#include <string.h>

typedef struct
{
        int prev, next;
} lcnode;

typedef struct
{
        int N;
        lcnode *List;
} listCollectionRec;

typedef listCollectionRec * listCollectionP;

listCollectionP LCNew(int N);
void LCFree(listCollectionP *pListColl);

void LCInsertAfter(listCollectionP listColl, int theAnchor, int theNewNode);
void LCInsertBefore(listCollectionP listColl, int theAnchor, int theNewNode);

#ifndef SPEED_MACROS

void LCReset(listCollectionP listColl);
void LCCopy(listCollectionP dst, listCollectionP src);

int  LCGetNext(listCollectionP listColl, int theList, int theNode); 
int  LCGetPrev(listCollectionP listColl, int theList, int theNode); 

int  LCPrepend(listCollectionP listColl, int theList, int theNode);
int  LCAppend(listCollectionP listColl, int theList, int theNode);
int  LCDelete(listCollectionP listColl, int theList, int theNode); 

#else

/* void LCReset(listCollectionP listColl); */

#define LCReset(listColl) memset(listColl->List, NIL_CHAR, listColl->N*sizeof(lcnode))

/* void LCCopy(listCollectionP dst, listCollectionP src) */

#define LCCopy(dst, src) memcpy(dst->List, src->List, src->N*sizeof(lcnode))

/* int  LCGetNext(listCollectionP listColl, int theList, int theNode); 
	Return theNode's successor, unless it is theList head pointer */
	
#define LCGetNext(listColl, theList, theNode) listColl->List[theNode].next==theList ? NIL : listColl->List[theNode].next

/* int  LCGetPrev(listCollectionP listColl, int theList, int theNode); 
	Return theNode's predecessor unless theNode is theList head.
	To start going backwards, use NIL for theNode, which returns theList head's predecessor 
	Usage: Obtain last node, loop while NIL not returned, process node then get predecessor.
		After theList head processed, get predecessor returns NIL because we started with
		theList head's predecessor. */

#define LCGetPrev(listColl, theList, theNode) \
        (theNode==NIL \
	 ? listColl->List[theList].prev \
         : theNode==theList ? NIL : listColl->List[theNode].prev)

/* int  LCPrepend(listCollectionP listColl, int theList, int theNode);
    After an append, theNode is last, which in a circular list is the direct predecessor
	of the list head node, so we just back up one. For singletons, this has no effect.*/

#define LCPrepend(listColl, theList, theNode) listColl->List[LCAppend(listColl, theList, theNode)].prev

/* int  LCAppend(listCollectionP listColl, int theList, int theNode);
	If theList is empty, then theNode becomes its only member and is returned.
	Otherwise, theNode is placed before theList head, which is returned. */

#define LCAppend(listColl, theList, theNode) \
        (theList==NIL \
         ? (listColl->List[theNode].prev = listColl->List[theNode].next = theNode) \
         : (listColl->List[theNode].next = theList, \
            listColl->List[theNode].prev = listColl->List[theList].prev, \
            listColl->List[listColl->List[theNode].prev].next = theNode, \
            listColl->List[theList].prev = theNode, \
	    theList))

/* int  LCDelete(listCollectionP listColl, int theList, int theNode); 
	If theList contains only one node, then NIL it out and return NIL meaning empty list 
	Otherwise, join the predecessor and successor, then 
	return either the list head or its successor if the deleted node is the list head
	(in that case, the caller makes the successor become the new list head).*/


#define LCDelete(listColl, theList, theNode) \
        listColl->List[theList].next == theList \
	? (listColl->List[theList].prev = listColl->List[theList].next = NIL) \
        : (listColl->List[listColl->List[theNode].prev].next = listColl->List[theNode].next, \
           listColl->List[listColl->List[theNode].next].prev = listColl->List[theNode].prev, \
	   (theList==theNode ? listColl->List[theNode].next : theList))

#endif

#endif