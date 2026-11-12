/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../graph.h"

#include "graphIO.h"
#include "gp-write-iterator.h"
#include "g6-write-iterator.h"
#include "s6-write-iterator.h"
#include "d6-write-iterator.h"

/********************************************************************
 Package private structure declaration for the write iterator

 The format is chosen when the iterator is created, and each later
 call is handed to the write iterator of that format, which holds the
 graph, as it does when it is used on its own. So the contracts of
 those writers apply: the graph must have vertices when the writer is
 created, every graph written must have the order the graph had at
 initialization, the format header is written at initialization, and
 a writer whose initialization failed is to be freed, not initialized
 again.
 ********************************************************************/
struct GPWriteIteratorStruct
{
    // WRITE_G6, WRITE_SPARSE6 or WRITE_DIGRAPH6, from graphIO.h
    int writeMode;

    G6WriteIteratorP g6WriteIterator;
    S6WriteIteratorP s6WriteIterator;
    D6WriteIteratorP d6WriteIterator;
};

/********************************************************************
 Public method implementations for the write iterator
 ********************************************************************/

/********************************************************************
 gp_NewWriter()

 Creates a write iterator for theGraph in the format given by
 writeMode, which must be one of the formats that hold more than one
 graph: WRITE_G6, WRITE_SPARSE6 or WRITE_DIGRAPH6.

 Returns OK on success. Returns NOTOK on an invalid parameter or when
 the write iterator of the format cannot be created, in which case
 nothing is allocated and *pGPWriteIterator is left NULL.
 ********************************************************************/

int gp_NewWriter(GPWriteIteratorP *pGPWriteIterator, graphP theGraph, int writeMode)
{
    int Result = OK;

    if (pGPWriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: pGPWriteIterator must be non-NULL.");
        return NOTOK;
    }

    if ((*pGPWriteIterator) != NULL)
    {
        gp_ErrorMessage("The write iterator is not NULL and therefore can't be "
                        "allocated.");
        return NOTOK;
    }

    if (theGraph == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theGraph must be non-NULL.");
        return NOTOK;
    }

    if (writeMode != WRITE_G6 && writeMode != WRITE_SPARSE6 && writeMode != WRITE_DIGRAPH6)
    {
        gp_ErrorMessage("Invalid parameter: writeMode must be one of the formats "
                        "that contain multiple graphs, i.e. WRITE_G6, "
                        "WRITE_SPARSE6 or WRITE_DIGRAPH6.");
        return NOTOK;
    }

    (*pGPWriteIterator) = (GPWriteIteratorP)calloc(1, sizeof(GPWriteIteratorStruct));

    if ((*pGPWriteIterator) == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for the write iterator.");
        return NOTOK;
    }

    (*pGPWriteIterator)->writeMode = writeMode;

    switch (writeMode)
    {
    case WRITE_G6:
        Result = g6_NewWriter((&(*pGPWriteIterator)->g6WriteIterator), theGraph);
        break;

    case WRITE_SPARSE6:
        Result = s6_NewWriter((&(*pGPWriteIterator)->s6WriteIterator), theGraph);
        break;

    case WRITE_DIGRAPH6:
        Result = d6_NewWriter((&(*pGPWriteIterator)->d6WriteIterator), theGraph);
        break;

    default:
        Result = NOTOK;
        break;
    }

    if (Result != OK)
    {
        gp_FreeWriter(pGPWriteIterator);
        return NOTOK;
    }

    return OK;
}

int gp_InitWriterWithString(GPWriteIteratorP theGPWriteIterator, char **pOutputString)
{
    if (theGPWriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theGPWriteIterator must be non-NULL.");
        return NOTOK;
    }

    switch (theGPWriteIterator->writeMode)
    {
    case WRITE_G6:
        return g6_InitWriterWithString(theGPWriteIterator->g6WriteIterator, pOutputString);

    case WRITE_SPARSE6:
        return s6_InitWriterWithString(theGPWriteIterator->s6WriteIterator, pOutputString);

    case WRITE_DIGRAPH6:
        return d6_InitWriterWithString(theGPWriteIterator->d6WriteIterator, pOutputString);

    default:
        gp_ErrorMessage("Unable to initialize the writer, as its write mode is "
                        "invalid.");
        return NOTOK;
    }
}

int gp_InitWriterWithFileName(GPWriteIteratorP theGPWriteIterator, char *outputFileName)
{
    if (theGPWriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theGPWriteIterator must be non-NULL.");
        return NOTOK;
    }

    switch (theGPWriteIterator->writeMode)
    {
    case WRITE_G6:
        return g6_InitWriterWithFileName(theGPWriteIterator->g6WriteIterator, outputFileName);

    case WRITE_SPARSE6:
        return s6_InitWriterWithFileName(theGPWriteIterator->s6WriteIterator, outputFileName);

    case WRITE_DIGRAPH6:
        return d6_InitWriterWithFileName(theGPWriteIterator->d6WriteIterator, outputFileName);

    default:
        gp_ErrorMessage("Unable to initialize the writer, as its write mode is "
                        "invalid.");
        return NOTOK;
    }
}

void gp_SetOutputErrorFlag(GPWriteIteratorP theGPWriteIterator)
{
    if (theGPWriteIterator == NULL)
        return;

    switch (theGPWriteIterator->writeMode)
    {
    case WRITE_G6:
        g6_SetOutputErrorFlag(theGPWriteIterator->g6WriteIterator);
        break;

    case WRITE_SPARSE6:
        s6_SetOutputErrorFlag(theGPWriteIterator->s6WriteIterator);
        break;

    case WRITE_DIGRAPH6:
        d6_SetOutputErrorFlag(theGPWriteIterator->d6WriteIterator);
        break;

    default:
        break;
    }
}

/********************************************************************
 gp_StoreGraphChange()

 Stores one change to be written by the next gp_WriteGraph() as part
 of an incremental line, as s6_StoreGraphChange() does, to which the
 call is handed. Only sparse6 has incremental lines, so for the other
 formats the call is refused.

 Returns OK on success, NOTOK otherwise.
 ********************************************************************/

int gp_StoreGraphChange(GPWriteIteratorP theGPWriteIterator, int e, int u, int v)
{
    if (theGPWriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theGPWriteIterator must be non-NULL.");
        return NOTOK;
    }

    if (theGPWriteIterator->writeMode != WRITE_SPARSE6)
    {
        gp_ErrorMessage("Unable to store a graph change, as only sparse6 can "
                        "write incremental changes.");
        return NOTOK;
    }

    return s6_StoreGraphChange(theGPWriteIterator->s6WriteIterator, e, u, v);
}

int gp_WriteGraph(GPWriteIteratorP theGPWriteIterator)
{
    if (theGPWriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theGPWriteIterator must be non-NULL.");
        return NOTOK;
    }

    switch (theGPWriteIterator->writeMode)
    {
    case WRITE_G6:
        return g6_WriteGraph(theGPWriteIterator->g6WriteIterator);

    case WRITE_SPARSE6:
        return s6_WriteGraph(theGPWriteIterator->s6WriteIterator);

    case WRITE_DIGRAPH6:
        return d6_WriteGraph(theGPWriteIterator->d6WriteIterator);

    default:
        gp_ErrorMessage("Unable to write a graph, as the write mode of the "
                        "writer is invalid.");
        return NOTOK;
    }
}

/********************************************************************
 gp_FreeWriter()

 Frees the write iterator of the format, which finishes the output and
 hands an output string over to the caller, then frees this iterator
 and sets *pGPWriteIterator to NULL.

 NOTE: As with the writers of the formats, the output string is handed
 over only if no operation failed; after a failure, or after
 gp_SetOutputErrorFlag(), the caller's string pointer is left NULL and
 a file is removed, so the caller must check the string for NULL.
 ********************************************************************/

void gp_FreeWriter(GPWriteIteratorP *pGPWriteIterator)
{
    if (pGPWriteIterator == NULL || (*pGPWriteIterator) == NULL)
        return;

    g6_FreeWriter((&(*pGPWriteIterator)->g6WriteIterator));
    s6_FreeWriter((&(*pGPWriteIterator)->s6WriteIterator));
    d6_FreeWriter((&(*pGPWriteIterator)->d6WriteIterator));

    free((*pGPWriteIterator));
    (*pGPWriteIterator) = NULL;
}
