/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdlib.h>
#include <string.h>

#include "strOrFile.h"

#include "gp-read-iterator.h"
#include "g6-read-iterator.h"
#include "s6-read-iterator.h"

#include "../lowLevelUtils/apiutils.h"

/* Imported functions */
extern int _g6_InitReaderWithStrOrFile(G6ReadIteratorP theG6ReadIterator, strOrFileP *pInputContainer);
extern int _s6_InitReaderWithStrOrFile(S6ReadIteratorP theS6ReadIterator, strOrFileP *pInputContainer);

/* Private functions */
int _gp_IsReaderInitialized(GPReadIteratorP theGPReadIterator);
int _gp_SetFileType(GPReadIteratorP theGPReadIterator, char const *const firstLine);
int _gp_NewChildReader(GPReadIteratorP theGPReadIterator);
void _gp_FreeChildReader(GPReadIteratorP theGPReadIterator);

/********************************************************************
 Package private structure declaration for the read iterator

 A file of graphs is in one format throughout, and the first line
 says which one, so this iterator decides the type once, when it is
 initialized, and hands each later call to the read iterator of that
 type. The graph is the caller's, as it is for those readers.
 ********************************************************************/
struct GPReadIteratorStruct
{
    graphP currGraph;

    // One of the GP_FILE_TYPE_ macros; UNKNOWN until initialization
    // determines the type from the first line of the input
    int fileType;

    G6ReadIteratorP g6ReadIterator;
    S6ReadIteratorP s6ReadIterator;
};

/********************************************************************
 Public and package private method implementations for read iterator
 ********************************************************************/

int gp_NewReader(GPReadIteratorP *pGPReadIterator, graphP theGraph)
{
    if (pGPReadIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: pGPReadIterator must be non-NULL.");
        return NOTOK;
    }

    if ((*pGPReadIterator) != NULL)
    {
        gp_ErrorMessage("The read iterator is not NULL and therefore can't be "
                        "allocated.");
        return NOTOK;
    }

    if (theGraph == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theGraph must be non-NULL.");
        return NOTOK;
    }

    (*pGPReadIterator) = (GPReadIteratorP)calloc(1, sizeof(GPReadIteratorStruct));

    if ((*pGPReadIterator) == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for the read iterator.");
        return NOTOK;
    }

    (*pGPReadIterator)->currGraph = theGraph;
    (*pGPReadIterator)->fileType = GP_FILE_TYPE_UNKNOWN;
    (*pGPReadIterator)->g6ReadIterator = NULL;
    (*pGPReadIterator)->s6ReadIterator = NULL;

    return OK;
}

int _gp_IsReaderInitialized(GPReadIteratorP theGPReadIterator)
{
    return (theGPReadIterator->fileType != GP_FILE_TYPE_UNKNOWN) ? TRUE : FALSE;
}

/********************************************************************
 _gp_SetFileType()

 Decides the format from the first line of the input and stores it,
 so that the other methods dispatch on the type rather than on which
 read iterator happens to exist.
 ********************************************************************/
int _gp_SetFileType(GPReadIteratorP theGPReadIterator, char const *const firstLine)
{
    if (g6_IsGraph6Input(firstLine))
        theGPReadIterator->fileType = GP_FILE_TYPE_G6;
    else if (s6_IsSparse6Input(firstLine))
        theGPReadIterator->fileType = GP_FILE_TYPE_S6;
    else
    {
        gp_ErrorMessage("The input is in none of the formats that contain "
                        "multiple graphs, i.e. it is neither graph6 nor "
                        "sparse6.");
        return NOTOK;
    }

    return OK;
}

/********************************************************************
 _gp_NewChildReader()

 Creates the read iterator of the type already decided, which the
 initializers then initialize with the input.
 ********************************************************************/
int _gp_NewChildReader(GPReadIteratorP theGPReadIterator)
{
    switch (theGPReadIterator->fileType)
    {
    case GP_FILE_TYPE_G6:
        return g6_NewReader((&theGPReadIterator->g6ReadIterator), theGPReadIterator->currGraph);

    case GP_FILE_TYPE_S6:
        return s6_NewReader((&theGPReadIterator->s6ReadIterator), theGPReadIterator->currGraph);

    default:
        return NOTOK;
    }
}

/********************************************************************
 _gp_FreeChildReader()

 Frees the read iterator of the decided type, if one was created, and
 returns this iterator to the state it had before initialization. The
 child read iterator owns the input container once it has been
 initialized with one, so freeing the child frees that too.
 ********************************************************************/
void _gp_FreeChildReader(GPReadIteratorP theGPReadIterator)
{
    if (theGPReadIterator->g6ReadIterator != NULL)
        g6_FreeReader((&theGPReadIterator->g6ReadIterator));

    if (theGPReadIterator->s6ReadIterator != NULL)
        s6_FreeReader((&theGPReadIterator->s6ReadIterator));

    theGPReadIterator->fileType = GP_FILE_TYPE_UNKNOWN;
}

int gp_InitReaderWithString(GPReadIteratorP theGPReadIterator, char *inputString)
{
    int Result = OK;

    if (theGPReadIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theGPReadIterator must be non-NULL.");
        return NOTOK;
    }

    if (_gp_IsReaderInitialized(theGPReadIterator))
    {
        gp_ErrorMessage("Unable to initialize reader, as it was already "
                        "previously initialized.");
        return NOTOK;
    }

    if (inputString == NULL || strlen(inputString) == 0)
    {
        gp_ErrorMessage("Unable to initialize reader with empty input string.");
        return NOTOK;
    }

    // The string is the caller's and is not consumed by looking at it, so the
    // type is decided from it directly and the read iterator of that type is
    // then initialized with the whole string.
    if (_gp_SetFileType(theGPReadIterator, inputString) != OK ||
        _gp_NewChildReader(theGPReadIterator) != OK)
    {
        _gp_FreeChildReader(theGPReadIterator);
        return NOTOK;
    }

    switch (theGPReadIterator->fileType)
    {
    case GP_FILE_TYPE_G6:
        Result = g6_InitReaderWithString(theGPReadIterator->g6ReadIterator, inputString);
        break;

    case GP_FILE_TYPE_S6:
        Result = s6_InitReaderWithString(theGPReadIterator->s6ReadIterator, inputString);
        break;

    default:
        Result = NOTOK;
        break;
    }

    if (Result != OK)
    {
        _gp_FreeChildReader(theGPReadIterator);
        return NOTOK;
    }

    return OK;
}

int gp_InitReaderWithFileName(GPReadIteratorP theGPReadIterator, char const *const infileName)
{
    strOrFileP inputContainer = NULL;
    char lineBuff[MAXLINE + 1];
    int Result = OK;

    if (theGPReadIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theGPReadIterator must be non-NULL.");
        return NOTOK;
    }

    if (_gp_IsReaderInitialized(theGPReadIterator))
    {
        gp_ErrorMessage("Unable to initialize reader, as it was already "
                        "previously initialized.");
        return NOTOK;
    }

    if (infileName == NULL || strlen(infileName) == 0)
    {
        gp_ErrorMessage("Unable to initialize reader with empty infile name.");
        return NOTOK;
    }

    if ((inputContainer = sf_NewInputContainer(NULL, infileName)) == NULL)
    {
        gp_ErrorMessage("Unable to initialize reader with file name, as we "
                        "failed to allocate the inputContainer.");
        return NOTOK;
    }

    // The input is opened once: its first line is read to decide the type and
    // pushed back, and the same container is then handed to the read iterator
    // of that type. An input that cannot be reopened, such as stdin, is
    // therefore read from its first byte exactly once.
    memset(lineBuff, '\0', (MAXLINE + 1));

    if (sf_fgets(lineBuff, MAXLINE, inputContainer) != NULL &&
        sf_ungets(lineBuff, inputContainer) != OK)
    {
        gp_ErrorMessage("Unable to push back the first line of the input.");
        sf_Free(&inputContainer);
        return NOTOK;
    }

    if (_gp_SetFileType(theGPReadIterator, lineBuff) != OK ||
        _gp_NewChildReader(theGPReadIterator) != OK)
    {
        _gp_FreeChildReader(theGPReadIterator);
        sf_Free(&inputContainer);
        return NOTOK;
    }

    switch (theGPReadIterator->fileType)
    {
    case GP_FILE_TYPE_G6:
        Result = _g6_InitReaderWithStrOrFile(theGPReadIterator->g6ReadIterator, (&inputContainer));
        break;

    case GP_FILE_TYPE_S6:
        Result = _s6_InitReaderWithStrOrFile(theGPReadIterator->s6ReadIterator, (&inputContainer));
        break;

    default:
        Result = NOTOK;
        break;
    }

    if (Result != OK)
    {
        _gp_FreeChildReader(theGPReadIterator);

        // Frees the container only if no read iterator took ownership of it,
        // since the initializers set the pointer to NULL when they do.
        sf_Free(&inputContainer);
        return NOTOK;
    }

    return OK;
}

int gp_ReadGraph(GPReadIteratorP theGPReadIterator)
{
    if (theGPReadIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theGPReadIterator must be non-NULL.");
        return NOTOK;
    }

    switch (theGPReadIterator->fileType)
    {
    case GP_FILE_TYPE_G6:
        return g6_ReadGraph(theGPReadIterator->g6ReadIterator);

    case GP_FILE_TYPE_S6:
        return s6_ReadGraph(theGPReadIterator->s6ReadIterator);

    default:
        gp_ErrorMessage("Unable to read a graph, as the reader has not been "
                        "initialized with an input.");
        return NOTOK;
    }
}

int gp_EndReached(GPReadIteratorP theGPReadIterator)
{
    if (theGPReadIterator == NULL)
        return TRUE;

    switch (theGPReadIterator->fileType)
    {
    case GP_FILE_TYPE_G6:
        return g6_EndReached(theGPReadIterator->g6ReadIterator);

    case GP_FILE_TYPE_S6:
        return s6_EndReached(theGPReadIterator->s6ReadIterator);

    default:
        return TRUE;
    }
}

void gp_FreeReader(GPReadIteratorP *pGPReadIterator)
{
    if (pGPReadIterator == NULL || (*pGPReadIterator) == NULL)
        return;

    _gp_FreeChildReader((*pGPReadIterator));

    (*pGPReadIterator)->currGraph = NULL;

    free((*pGPReadIterator));
    (*pGPReadIterator) = NULL;
}
