/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"
#include "../test/testMain.h"

// TODO: These need to be removed as part of moving the gp iterator testing
//       to a separate source file in the /test directory
#include "../graphLib/io/strOrFile.h"
#include "../graphLib/io/g6-read-iterator.h"
#include "../graphLib/io/g6-write-iterator.h"
#include "../graphLib/io/s6-read-iterator.h"
#include "../graphLib/io/s6-write-iterator.h"
#include "../graphLib/io/d6-read-iterator.h"
#include "../graphLib/io/d6-write-iterator.h"

#if defined(_MSC_VER) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
// MSVC under Windows doesn't have unistd.h, but does define functions like getcwd and chdir
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#else
#include <unistd.h>
#endif

int callRandomGraphs(int argc, char *argv[]);
int callSpecificGraph(int argc, char *argv[]);
int callRandomMaxPlanarGraph(int argc, char *argv[]);
int callRandomNonplanarGraph(int argc, char *argv[]);
int callTestAllGraphs(int argc, char *argv[]);
int callTransformGraph(int argc, char *argv[]);

int runFaceListTest(void);
int runAddInsertEdgeTests(void);

int runHideRestoreTests(void);
int runIdentifyContractTests(void);
int runHideRestoreTest(graphP theGraph);
int runIdentifyContractTest(graphP theGraph);

int runReadWithExtensionAtEofTest(void);
int runHighByteRoundTripTest(void);
int runCapacityLimitTests(void);

int runStringInputLineTests(void);
int runGeneralReadIteratorTests(void);
int runGeneralWriteIteratorTests(void);
extern char *copySparse6TestString(char const *s6Str);

/****************************************************************************
 Command Line Processor
 ****************************************************************************/

int commandLine(int argc, char *argv[])
{
    int Result = OK;

#ifdef DEBUG
    char lineBuff[MAXLINE + 1];
#endif

    if (argc >= 3 && strcmp(argv[2], "-q") == 0)
        gp_SetQuietMode(QUIETMODE_ALL);

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0)
    {
        Result = helpMessage(argc >= 3 ? argv[2] : NULL);
    }

    else if (strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "-info") == 0)
    {
        Result = helpMessage(argv[1]);
    }

    else if (strcmp(argv[1], "-test") == 0)
        Result = runTestMain(argc, argv);

    else if (strcmp(argv[1], "-r") == 0)
        Result = callRandomGraphs(argc, argv);

    else if (strcmp(argv[1], "-s") == 0)
        Result = callSpecificGraph(argc, argv);

    else if (strcmp(argv[1], "-rm") == 0)
        Result = callRandomMaxPlanarGraph(argc, argv);

    else if (strcmp(argv[1], "-rn") == 0)
        Result = callRandomNonplanarGraph(argc, argv);

    else if (strncmp(argv[1], "-x", 2) == 0)
        Result = callTransformGraph(argc, argv);

    else if (strncmp(argv[1], "-t", 2) == 0)
        Result = callTestAllGraphs(argc, argv);

    else
    {
        gp_ErrorMessage("Unsupported command line.  Here is the help for this "
                        "program.");
        helpMessage(NULL);
        Result = NOTOK;
    }

#ifdef DEBUG
    // When one builds and runs the executable in an external console from an IDE
    // such as VSCode, the external console window will close immediately upon
    // exit 0 being returned. This means that one may miss gp_Message() and
    // gp_ErrorMessage() outputs that are crucial to the debugging process.
    // Hence, if we compile with the DDEBUG flag, this means that in appconst.h
    // we #define DEBUG. That way, this prompt will appear only for debug builds,
    // and will ensure the console window stays open until the user proceeds.
    printf("\n\tPress return key to exit...\n");
    fflush(stdout);
    if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
    {
        gp_ErrorMessage("Unable to fetch from stdin; exiting.");
        Result = NOTOK;
    }
#endif

    // NOTE: Translates internal planarity codes to appropriate exit codes
    return Result == OK ? 0 : (Result == NONEMBEDDABLE ? 1 : -1);
}

/****************************************************************************
 Legacy Command Line Processor from version 1.x
 ****************************************************************************/

int legacyCommandLine(int argc, char *argv[])
{
    int Result = OK;

    graphP theGraph = gp_New();

    if (theGraph == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for theGraph.");
        Result = NOTOK;
    }

    if (Result == OK)
    {
        Result = gp_Read(theGraph, argv[1]);
        if (Result != OK)
        {
            gp_ErrorMessage("Failed to read graph \"%.*s\"",
                            FILENAME_MAX, argv[1]);
            Result = NOTOK;
        }
    }

    if (Result == OK)
    {
        Result = gp_Embed(theGraph, EMBEDFLAGS_PLANAR);
        if (Result == OK)
        {
            if ((Result = gp_SortVertices(theGraph)) != OK)
                gp_ErrorMessage("Failed to restore original vertex labelling.");

            if (Result == OK && (Result = gp_Write(theGraph, argv[2], WRITE_ADJLIST)) != OK)
                gp_ErrorMessage("Failed to write embedding.");
        }

        else if (Result == NONEMBEDDABLE)
        {
            if (argc >= 5 && strcmp(argv[3], "-n") == 0)
            {
                if ((Result = gp_SortVertices(theGraph)) != OK)
                    gp_ErrorMessage("Failed to restore original vertex "
                                    "labelling.");

                if (Result == OK && (Result = gp_Write(theGraph, argv[4], WRITE_ADJLIST)) != OK)
                    gp_ErrorMessage("Failed to write obstruction.");
            }
        }
        else
            Result = NOTOK;
    }

    gp_Free(&theGraph);

    // In the legacy 1.x versions, OK/NONEMBEDDABLE was 0 and NOTOK was -2
    return Result == OK || Result == NONEMBEDDABLE ? 0 : -2;
}

int runAddInsertEdgeTests(void)
{
    graphP theGraph = NULL;
    int initialEdgeCapacity;
    int edgeToDelete;
    int v;

    gp_Message("Starting Add/Insert Edge Tests");

    if ((theGraph = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph for add/insert edge tests.");
        return NOTOK;
    }

    if (gp_EnsureEdgeCapacity(theGraph, 2) != OK ||
        gp_EnsureVertexCapacity(theGraph, 3) != OK)
    {
        gp_ErrorMessage("Unable to initialize graph for add/insert edge tests.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    v = gp_LowerBoundVertices(theGraph);
    initialEdgeCapacity = gp_GetEdgeCapacity(theGraph);

    if (gp_AddEdge(theGraph, v, 0, v + 1, 0) != OK ||
        gp_AddEdge(theGraph, v + 1, 1, v + 2, 0) != OK)
    {
        gp_ErrorMessage("Unable to fill initial edge capacity.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    if (gp_InsertEdge(theGraph, v, NIL, 1, v + 2, NIL, 1) != AT_EDGE_CAPACITY_LIMIT)
    {
        gp_ErrorMessage("gp_InsertEdge() did not report the edge capacity limit.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    if (gp_DynamicInsertEdge(theGraph, v, NIL, 1, v + 2, NIL, 1) != OK ||
        gp_GetEdgeCapacity(theGraph) <= initialEdgeCapacity ||
        gp_GetM(theGraph) != 3)
    {
        gp_ErrorMessage("gp_DynamicInsertEdge() did not grow edge capacity and insert once.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    edgeToDelete = gp_FindEdge(theGraph, v, v + 1);
    if (edgeToDelete == NIL || gp_DeleteEdge(theGraph, edgeToDelete) != OK)
    {
        gp_ErrorMessage("Unable to create an edge hole for add/insert edge tests.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    initialEdgeCapacity = gp_GetEdgeCapacity(theGraph);
    if (gp_DynamicAddEdge(theGraph, v, 0, v + 1, 0) != OK ||
        gp_GetEdgeCapacity(theGraph) != initialEdgeCapacity ||
        gp_GetM(theGraph) != 3 ||
        gp_FindEdge(theGraph, v, v + 1) == NIL)
    {
        gp_ErrorMessage("gp_DynamicAddEdge() did not reuse an available edge hole.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    gp_Message("Finished Add/Insert Edge Tests.\n");

    gp_Free(&theGraph);

    return OK;
}

/********************************************************************
 runReadWithExtensionAtEofTest()

 Reads a graph with an extension attached before gp_Read(), which
 exercises the end-of-input handling in _ReadGraph(): the extra-data
 check must see a true EOF rather than a fabricated byte.

 Before the sf_getc()/sf_ungetc() int conversion (issue #319), on
 platforms where plain char is unsigned, (char)EOF == 255, so this
 read handed a spurious 0xFF extra-data byte to the extension's
 post-processor and failed on a valid input file.
 ********************************************************************/

int runReadWithExtensionAtEofTest(void)
{
    int Result = OK;
    graphP theGraph = NULL;

    gp_Message("Test EOF Handling (for platforms that have char unsigned).");

    if ((theGraph = gp_New()) == NULL)
        Result = NOTOK;

    if (Result == OK && gp_ExtendWith_DrawPlanar(theGraph) != OK)
        Result = NOTOK;

    if (Result == OK && gp_Read(theGraph, "maxPlanar5.txt") != OK)
        Result = NOTOK;

    if (Result == OK)
        gp_Message("Test succeeded.\n");

    gp_Free(&theGraph);

    return Result;
}

/********************************************************************
 runHighByteRoundTripTest()

 Reads the byte 0xFF from a string container directly and through the
 unget buffer, and confirms it is never mistaken for EOF.

 Before the sf_getc()/sf_ungetc() int conversion (issue #319), on
 platforms where plain char is signed, a literal 0xFF byte fetched
 from the string container sign-extended to EOF, and the same
 happened to bytes round-tripped through the unget buffer.
 ********************************************************************/

int runHighByteRoundTripTest(void)
{
    int Result = OK;
    int currChar = EOF;
    unsigned char eofChar = 0xFF;
    char highByteStr[4];
    // char const highByteStr[] = {'a', (char)0xFF, 'b', '\0'};
    strOrFileP inputContainer = NULL;

    gp_Message("Test Support of 0xFF Byte.");

    highByteStr[0] = 'a';
    highByteStr[1] = (char)eofChar;
    highByteStr[2] = 'b';
    highByteStr[3] = '\0';

    if ((inputContainer = sf_NewInputContainer(highByteStr, NULL)) == NULL)
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != 'a')
        Result = NOTOK;

    if (Result == OK)
    {
        currChar = sf_getc(inputContainer);
        if (currChar != 0xFF)
            Result = NOTOK;
    }

    if (Result == OK && sf_ungetc(currChar, inputContainer) != 0xFF)
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != 0xFF)
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != 'b')
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != EOF)
        Result = NOTOK;

    if (Result == OK)
        gp_Message("Test succeeded.\n");

    if (inputContainer != NULL)
        sf_Free(&inputContainer);

    return Result;
}

/********************************************************************
 runCapacityLimitTests()

 Exercises the issue #325 guards: a vertex count or edge capacity
 whose capacity arithmetic cannot be represented in int must be
 rejected by the public wrappers before any allocation is attempted.

 The first rejected vertex count on the default edge capacity is
 (INT_MAX - 2) / 4 / DEFAULT_EDGE_CAPACITY_FACTOR + 1, and the first
 rejected edge capacity is (INT_MAX - 2) / 4 + 1; the largest
 acceptable edge capacity is tested on the accept side too, which is
 safe because storing a capacity before gp_EnsureVertexCapacity()
 allocates nothing.
 ********************************************************************/

int runCapacityLimitTests(void)
{
    int Result = OK;
    graphP theGraph = NULL;
    int firstBadN = (int)((((long long)INT_MAX - 2) / 4) / DEFAULT_EDGE_CAPACITY_FACTOR + 1);
    int maxGoodEdgeCapacity = (int)(((long long)INT_MAX - 2) / 4);
    unsigned quietModeCache = gp_GetQuietMode();

    gp_Message("Testing rejection of excessive vertex and edge capacity requests.");

    if ((theGraph = gp_New()) == NULL)
        Result = NOTOK;

    // The first tests below intentionally cause a NOTOK to come from the APIs,
    // so we set fully quiet mode to prevent error messages on expected behaviors.
    gp_SetQuietMode(QUIETMODE_ALL);

    if (Result == OK && gp_EnsureVertexCapacity(theGraph, INT_MAX) == OK)
    {
        gp_SetQuietMode(quietModeCache);
        Result = NOTOK;
        gp_SetQuietMode(QUIETMODE_ALL);
    }

    if (Result == OK && gp_EnsureVertexCapacity(theGraph, firstBadN) == OK)
    {
        gp_SetQuietMode(quietModeCache);
        Result = NOTOK;
        gp_SetQuietMode(QUIETMODE_ALL);
    }

    if (Result == OK && gp_EnsureEdgeCapacity(theGraph, maxGoodEdgeCapacity + 1) == OK)
    {
        gp_SetQuietMode(quietModeCache);
        Result = NOTOK;
        gp_SetQuietMode(QUIETMODE_ALL);
    }

    // The end of tests intended to produce NOTOK has been reached, so we can go back
    // to normal quiet mode behavior.
    gp_SetQuietMode(quietModeCache);

    if (Result == OK && gp_EnsureEdgeCapacity(theGraph, maxGoodEdgeCapacity) != OK)
        Result = NOTOK;

    gp_Free(&theGraph);

    // The guards must not disturb ordinary use
    if (Result == OK)
    {
        if ((theGraph = gp_New()) == NULL)
            Result = NOTOK;

        if (Result == OK && gp_EnsureVertexCapacity(theGraph, 5) != OK)
            Result = NOTOK;

        gp_Free(&theGraph);
    }

    if (Result == OK)
        gp_Message("Test succeeded.\n");

    return Result;
}

/********************************************************************
 runSparse6ReadTests()

 Exercises the sparse6 and incremental sparse6 read iterator (EPIC #38).

 N5-all.s6 and N5-all.inc.s6 were produced from N5-all.g6 by nauty's
 copyg (-s and -i respectively), and nauty_example.s6 from
 nauty_example.g6 in the same way, so the graph6 reader serves as
 the oracle: every graph read through the sparse6 reader must encode
 to the same graph6 line as the graph read from the .g6 file. The
 string cases pin the corners of the encoding (the padding rules, an
 incomplete final pair, CRLF, the four-byte order, the incremental
 toggle) and the inputs the reader must reject.
 ********************************************************************/

/****************************************************************************
 runGeneralReadIteratorTests()

 The general read iterator decides the format of an input from its first
 line and then reads it with the read iterator of that format, so these
 cases check that the decision is made correctly, that it is made once,
 and that an input in none of the formats is refused rather than handed
 to a reader that cannot make sense of it.
 ****************************************************************************/

/****************************************************************************
 runStringInputLineTests()

 Reading a line from a string, as from a file, stops after its newline,
 so that a read iterator initialized with a string of several graphs
 reads each of them: here graph6 graphs with 4, 1 and 2 edges.
 ****************************************************************************/

int runStringInputLineTests(void)
{
    char lines[] = "a\nbc\n";
    char graphs[] = "D?{\nD?_\nD?o\n";
    int const expectedNumEdges[] = {4, 1, 2};
    char buffer[16];
    strOrFileP inputContainer = NULL;
    graphP theGraph = NULL;
    GPReadIteratorP theReadIterator = NULL;
    int numGraphs = 0;
    int Result = OK;

    gp_Message("Starting String Input Line Tests");

    if ((inputContainer = sf_NewInputContainer(lines, NULL)) == NULL)
        return NOTOK;

    if (sf_fgets(buffer, sizeof(buffer), inputContainer) == NULL || strcmp(buffer, "a\n") != 0 ||
        sf_fgets(buffer, sizeof(buffer), inputContainer) == NULL || strcmp(buffer, "bc\n") != 0 ||
        sf_fgets(buffer, sizeof(buffer), inputContainer) != NULL)
    {
        gp_ErrorMessage("Reading lines from a string did not return one line per read.");
        Result = NOTOK;
    }

    sf_Free(&inputContainer);

    if (Result == OK &&
        ((theGraph = gp_New()) == NULL ||
         gp_NewReader((&theReadIterator), theGraph) != OK ||
         gp_InitReaderWithString(theReadIterator, graphs) != OK))
        Result = NOTOK;

    while (Result == OK)
    {
        if (gp_ReadGraph(theReadIterator) != OK)
            Result = NOTOK;
        else if (gp_EndReached(theReadIterator))
            break;
        else if (numGraphs == 3 || gp_GetM(theGraph) != expectedNumEdges[numGraphs])
            Result = NOTOK;
        else
            numGraphs++;
    }

    if (Result != OK || numGraphs != 3)
    {
        gp_ErrorMessage("Reading the graph6 graphs of a string read %d of the 3.", numGraphs);
        Result = NOTOK;
    }

    gp_FreeReader((&theReadIterator));
    gp_Free(&theGraph);

    if (Result == OK)
        gp_Message("Finished String Input Line Tests.\n");

    return Result;
}

int runGeneralReadIteratorTests(void)
{
    int Result = OK;
    unsigned origQuietMode = gp_GetQuietMode();
    size_t i = 0;

    // {input string, encoding of the first graph it contains}, in graph6
    // except for the digraph6 cases, whose graphs graph6 cannot hold
    char const *acceptCases[][2] = {
        // graph6, with and without its header
        {"D?{\n", "D?{"},
        {">>graph6<<D?{\n", "D?{"},
        // The order-1 graph has an empty adjacency matrix, with or without a
        // final line terminator
        {"@\n", "@"},
        {"@", "@"},
        // sparse6 and incremental sparse6, the latter with the whole graph
        // that its second line modifies
        {":Fa@x^\n", "Fw??G"},
        {":D\n;oN\n", "D??"},
        {">>sparse6<<:Fa@x^\n", "Fw??G"},
        // digraph6, with and without its header: the arc 1 -> 0, then on the
        // second line the arc 0 -> 1
        {"&AG\n&AO\n", "&AG"},
        {">>digraph6<<&AG\n", "&AG"},
    };

    // Inputs in none of the formats the iterator reads
    char const *rejectCases[] = {
        // A '&' without the order that digraph6 puts after it
        "&\n",
        // GraphML, whose first character is below the graph6 range
        "<graphml>\n",
        // An adjacency list, which gp_Read() recognizes by its "N=" but a
        // file of graphs cannot contain: its first byte is in the graph6
        // range and its second is not
        "N=3\n1: 2 3 0\n2: 1 0\n3: 1 0\n",
    };

    // graph6 marks its lines with nothing but the order, so a first line that
    // begins like graph6 is handed to the graph6 reader, which then fails on
    // the content of the line: here the order says 5 vertices but the line
    // ends before their edges do.
    char const *readFailCases[] = {
        "D?\n",
        // The order byte alone is a complete line only when its graph has no
        // adjacency-matrix bytes
        "D",
    };

    char const *g6Header = ">>graph6<<";
    char const *d6Header = ">>digraph6<<";
    graphP theGraph = NULL;
    GPReadIteratorP theReadIterator = NULL;
    char *actualEncoding = NULL;

    // {first line, expected s6_IsSparse6Input(), expected g6_IsGraph6Input(),
    // expected d6_IsDigraph6Input()}
    struct
    {
        char const *firstLine;
        int isSparse6;
        int isGraph6;
        int isDigraph6;
    } predicateCases[] = {
        {">>sparse6<<:Fa@x^", TRUE, FALSE, FALSE},
        {":Fa@x^", TRUE, FALSE, FALSE},
        {";oN", TRUE, FALSE, FALSE},
        {">>graph6<<D?{", FALSE, TRUE, FALSE},
        {"D?{", FALSE, TRUE, FALSE},
        // The order byte spans 63 to 126, and 126 introduces the longer
        // order encodings; a graph of order 0 or 1 is its order byte alone,
        // with or without the line terminator sf_fgets() leaves on it
        {"?", FALSE, TRUE, FALSE},
        {"@\n", FALSE, TRUE, FALSE},
        {"~~~~", FALSE, TRUE, FALSE},
        // digraph6, with and without its header, its order byte at either end
        // of the range, and a '&' followed by no order byte
        {">>digraph6<<&AG", FALSE, FALSE, TRUE},
        {"&AG", FALSE, FALSE, TRUE},
        {"&?", FALSE, FALSE, TRUE},
        {"&~??~", FALSE, FALSE, TRUE},
        {"&", FALSE, FALSE, FALSE},
        {"&\n", FALSE, FALSE, FALSE},
        {"& ", FALSE, FALSE, FALSE},
        // GraphML, and a line whose first byte is below the range
        {"<graphml>", FALSE, FALSE, FALSE},
        // An adjacency list: the first byte is in the range, the second is not
        {"N=3", FALSE, FALSE, FALSE},
        // The byte after the range, and one with the high bit set
        {"\x7f", FALSE, FALSE, FALSE},
        {"\xff", FALSE, FALSE, FALSE},
        {"\n", FALSE, FALSE, FALSE},
        {"", FALSE, FALSE, FALSE},
        {NULL, FALSE, FALSE, FALSE},
    };

    gp_Message("Starting Generalized Read Iterator Tests");

    for (i = 0; i < (sizeof(predicateCases) / sizeof(predicateCases[0])); i++)
    {
        if (s6_IsSparse6Input(predicateCases[i].firstLine) != predicateCases[i].isSparse6 ||
            g6_IsGraph6Input(predicateCases[i].firstLine) != predicateCases[i].isGraph6 ||
            d6_IsDigraph6Input(predicateCases[i].firstLine) != predicateCases[i].isDigraph6)
        {
            gp_ErrorMessage("Format of \"%s\" was decided as sparse6=%d graph6=%d digraph6=%d.",
                            predicateCases[i].firstLine == NULL ? "(null)" : predicateCases[i].firstLine,
                            s6_IsSparse6Input(predicateCases[i].firstLine),
                            g6_IsGraph6Input(predicateCases[i].firstLine),
                            d6_IsDigraph6Input(predicateCases[i].firstLine));
            return NOTOK;
        }
    }

    // A reader without a graph to read into is refused
    gp_SetQuietMode(QUIETMODE_ALL);

    if (gp_NewReader((&theReadIterator), NULL) == OK)
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("Reader was created without a graph.");
        gp_FreeReader((&theReadIterator));
        return NOTOK;
    }

    gp_SetQuietMode(origQuietMode);

    for (i = 0; Result == OK && i < (sizeof(acceptCases) / sizeof(acceptCases[0])); i++)
    {
        char *inputStr = copySparse6TestString(acceptCases[i][0]);

        if (inputStr == NULL)
            return NOTOK;

        if ((theGraph = gp_New()) == NULL ||
            gp_NewReader((&theReadIterator), theGraph) != OK ||
            gp_InitReaderWithString(theReadIterator, inputStr) != OK ||
            gp_ReadGraph(theReadIterator) != OK ||
            gp_EndReached(theReadIterator))
        {
            gp_ErrorMessage("Unable to read the first graph of accept case %d.", (int)i);
            Result = NOTOK;
        }
        else if (gp_WriteToString(theGraph, (&actualEncoding),
                                  acceptCases[i][1][0] == '&' ? WRITE_DIGRAPH6 : WRITE_GRAPH6) != OK ||
                 actualEncoding == NULL ||
                 strncmp(actualEncoding + strlen(acceptCases[i][1][0] == '&' ? d6Header : g6Header),
                         acceptCases[i][1], strlen(acceptCases[i][1])) != 0)
        {
            gp_ErrorMessage("Graph of accept case %d is \"%s\" rather than \"%s\".",
                            (int)i, actualEncoding == NULL ? "" : actualEncoding,
                            acceptCases[i][1]);
            Result = NOTOK;
        }
        // A reader carries one input, so a second initialization is refused,
        // and refusing it leaves the reader reading the input it has
        else
        {
            GPReadIteratorP theSameReadIterator = theReadIterator;

            gp_SetQuietMode(QUIETMODE_ALL);

            if (gp_InitReaderWithString(theReadIterator, inputStr) == OK)
            {
                gp_SetQuietMode(origQuietMode);
                gp_ErrorMessage("Reader of accept case %d was initialized twice.", (int)i);
                Result = NOTOK;
            }
            // Allocating over an existing reader would lose it
            else if (gp_NewReader((&theSameReadIterator), theGraph) == OK)
            {
                gp_SetQuietMode(origQuietMode);
                gp_ErrorMessage("Reader of accept case %d was allocated over.", (int)i);
                Result = NOTOK;
            }
            else if (gp_ReadGraph(theReadIterator) != OK)
            {
                gp_SetQuietMode(origQuietMode);
                gp_ErrorMessage("Reader of accept case %d stopped reading after "
                                "the refusals.",
                                (int)i);
                Result = NOTOK;
            }

            gp_SetQuietMode(origQuietMode);
        }

        if (actualEncoding != NULL)
        {
            free(actualEncoding);
            actualEncoding = NULL;
        }

        gp_FreeReader((&theReadIterator));
        gp_Free(&theGraph);
        free(inputStr);
    }

    for (i = 0; Result == OK && i < (sizeof(rejectCases) / sizeof(rejectCases[0])); i++)
    {
        char *inputStr = copySparse6TestString(rejectCases[i]);

        if (inputStr == NULL)
            return NOTOK;

        if ((theGraph = gp_New()) == NULL ||
            gp_NewReader((&theReadIterator), theGraph) != OK)
            Result = NOTOK;
        else
        {
            gp_SetQuietMode(QUIETMODE_ALL);

            if (gp_InitReaderWithString(theReadIterator, inputStr) == OK)
            {
                gp_SetQuietMode(origQuietMode);
                gp_ErrorMessage("Reject case %d was accepted by the reader.", (int)i);
                Result = NOTOK;
            }

            gp_SetQuietMode(origQuietMode);
        }

        gp_FreeReader((&theReadIterator));
        gp_Free(&theGraph);
        free(inputStr);
    }

    for (i = 0; Result == OK && i < (sizeof(readFailCases) / sizeof(readFailCases[0])); i++)
    {
        char *inputStr = copySparse6TestString(readFailCases[i]);

        if (inputStr == NULL)
            return NOTOK;

        gp_SetQuietMode(QUIETMODE_ALL);

        if ((theGraph = gp_New()) == NULL ||
            gp_NewReader((&theReadIterator), theGraph) != OK ||
            gp_InitReaderWithString(theReadIterator, inputStr) != OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Read-failure case %d was refused before it was read.", (int)i);
            Result = NOTOK;
        }
        else if (gp_ReadGraph(theReadIterator) == OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Read-failure case %d was read as a graph.", (int)i);
            Result = NOTOK;
        }

        gp_SetQuietMode(origQuietMode);

        gp_FreeReader((&theReadIterator));
        gp_Free(&theGraph);
        free(inputStr);
    }

    // The same decision from a file, where the first line is read and pushed
    // back before the reader of that format is given the input
    if (Result == OK)
    {
        // The first graph of each sample is the edgeless graph of order 5,
        // whatever the format of the file
        char const *fileCases[][2] = {
            {"N5-all.g6", "D??"},
            {"N5-all.s6", "D??"},
            {"N5-all.inc.s6", "D??"},
        };

        for (i = 0; Result == OK && i < (sizeof(fileCases) / sizeof(fileCases[0])); i++)
        {
            if ((theGraph = gp_New()) == NULL ||
                gp_NewReader((&theReadIterator), theGraph) != OK ||
                gp_InitReaderWithFileName(theReadIterator, fileCases[i][0]) != OK ||
                gp_ReadGraph(theReadIterator) != OK ||
                gp_EndReached(theReadIterator))
            {
                gp_ErrorMessage("Unable to read the first graph of \"%s\".", fileCases[i][0]);
                Result = NOTOK;
            }
            else if (gp_WriteToString(theGraph, (&actualEncoding), WRITE_GRAPH6) != OK ||
                     actualEncoding == NULL ||
                     strncmp(actualEncoding + strlen(g6Header), fileCases[i][1],
                             strlen(fileCases[i][1])) != 0)
            {
                gp_ErrorMessage("First graph of \"%s\" is \"%s\" rather than \"%s\".",
                                fileCases[i][0], actualEncoding == NULL ? "" : actualEncoding,
                                fileCases[i][1]);
                Result = NOTOK;
            }

            else
            {
                // The samples hold every graph of order 5, and a reader that
                // never advances or never ends would not count them
                int numGraphsRead = 1;

                while (Result == OK)
                {
                    if (gp_ReadGraph(theReadIterator) != OK)
                    {
                        gp_ErrorMessage("Unable to read graph %d of \"%s\".",
                                        numGraphsRead + 1, fileCases[i][0]);
                        Result = NOTOK;
                    }
                    else if (gp_EndReached(theReadIterator))
                        break;
                    else
                        numGraphsRead++;
                }

                if (Result == OK && numGraphsRead != 34)
                {
                    gp_ErrorMessage("Read %d graphs of \"%s\" rather than 34.",
                                    numGraphsRead, fileCases[i][0]);
                    Result = NOTOK;
                }
            }

            if (actualEncoding != NULL)
            {
                free(actualEncoding);
                actualEncoding = NULL;
            }

            gp_FreeReader((&theReadIterator));
            gp_Free(&theGraph);
        }
    }

    if (Result == OK)
        gp_Message("Successfully finished Generalized Read Iterator Tests.\n");
    else
        gp_Message("Error detected in Generalized Read Iterator Tests.\n");

    return Result;
}

/****************************************************************************
 runGeneralWriteIteratorFormatsTest()

 Reads the graphs of N5-all.s6 with a general read iterator and writes
 each of them with a general write iterator for each of graph6,
 sparse6 and digraph6, all three on the reader's graph, then compares
 each output with the sample file of its format.
 ****************************************************************************/

static int runGeneralWriteIteratorFormatsTest(void)
{
    int Result = OK;
    graphP theGraph = NULL;
    GPReadIteratorP theReadIterator = NULL;
    int const writeModes[] = {WRITE_G6, WRITE_SPARSE6, WRITE_DIGRAPH6};
    char const *expectedFileNames[] = {"N5-all.g6", "N5-all.s6", "N5-all.d6"};
    GPWriteIteratorP theWriteIterators[] = {NULL, NULL, NULL};
    char *outputStrings[] = {NULL, NULL, NULL};
    int numGraphs = 0;
    size_t i = 0;

    if ((theGraph = gp_New()) == NULL ||
        gp_NewReader((&theReadIterator), theGraph) != OK ||
        gp_InitReaderWithFileName(theReadIterator, "N5-all.s6") != OK)
    {
        gp_ErrorMessage("Unable to open \"N5-all.s6\" for the general write iterator test.");
        Result = NOTOK;
    }

    // Initializing the reader gives the graph its order, which the writers
    // need, so they are created after it and before any graph is read
    for (i = 0; Result == OK && i < sizeof(writeModes) / sizeof(writeModes[0]); i++)
    {
        if (gp_NewWriter((&theWriteIterators[i]), theGraph, writeModes[i]) != OK ||
            gp_InitWriterWithString(theWriteIterators[i], (&outputStrings[i])) != OK)
        {
            gp_ErrorMessage("Unable to create the writer for \"%s\".", expectedFileNames[i]);
            Result = NOTOK;
        }
    }

    while (Result == OK)
    {
        if (gp_ReadGraph(theReadIterator) != OK)
        {
            gp_ErrorMessage("Unable to read graph %d of \"N5-all.s6\".", numGraphs + 1);
            Result = NOTOK;
            break;
        }

        if (gp_EndReached(theReadIterator))
            break;

        numGraphs++;

        for (i = 0; Result == OK && i < sizeof(writeModes) / sizeof(writeModes[0]); i++)
        {
            if (gp_WriteGraph(theWriteIterators[i]) != OK)
            {
                gp_ErrorMessage("Unable to write graph %d of \"N5-all.s6\" for \"%s\".",
                                numGraphs, expectedFileNames[i]);
                Result = NOTOK;
            }
        }
    }

    // Freeing a writer hands its output string over
    for (i = 0; i < sizeof(writeModes) / sizeof(writeModes[0]); i++)
    {
        gp_FreeWriter((&theWriteIterators[i]));

        if (theWriteIterators[i] != NULL)
        {
            gp_ErrorMessage("The writer for \"%s\" was not cleared when freed.", expectedFileNames[i]);
            Result = NOTOK;
        }
    }

    if (Result == OK && numGraphs != 34)
    {
        gp_ErrorMessage("Read %d graphs of \"N5-all.s6\" rather than 34.", numGraphs);
        Result = NOTOK;
    }

    for (i = 0; Result == OK && i < sizeof(writeModes) / sizeof(writeModes[0]); i++)
    {
        if (outputStrings[i] == NULL ||
            TextFileMatchesString(expectedFileNames[i], outputStrings[i]) != TRUE)
        {
            gp_ErrorMessage("The general write iterator's output differs from \"%s\".",
                            expectedFileNames[i]);
            Result = NOTOK;
        }
    }

    for (i = 0; i < sizeof(writeModes) / sizeof(writeModes[0]); i++)
        if (outputStrings[i] != NULL)
            free(outputStrings[i]);

    gp_FreeReader((&theReadIterator));
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runGeneralWriteIteratorIncrementalTest()

 Regenerates N5-all.inc.s6 with a sparse6 general write iterator: the
 first graph is written whole, and before each later graph is read, the
 changes that the general read iterator retrieves for it are stored in
 the writer, which then writes them as an incremental line.

 The writer applies each batch to its graph once the line is written,
 and the reader must not see its graph changed by anything but itself,
 so the writer works on a copy of the first graph. A deletion names an
 edge record of the reader's graph, so the writer is given its own
 record of the same edge. The regenerated bytes equal the file's when
 no line toggles an edge more than once and the file has one ':' line,
 both true of this sample: a second ':' line would leave the copy
 behind, which the comparison of the graphs after each write reports.
 ****************************************************************************/

static int runGeneralWriteIteratorIncrementalTest(void)
{
    int Result = OK;
    graphP theGraph = NULL;
    graphP writerGraph = NULL;
    GPReadIteratorP theReadIterator = NULL;
    GPWriteIteratorP theWriteIterator = NULL;
    char *outputStr = NULL;
    int numGraphs = 0;
    int e = NIL, u = NIL, v = NIL;

    if ((theGraph = gp_New()) == NULL ||
        gp_NewReader((&theReadIterator), theGraph) != OK ||
        gp_InitReaderWithFileName(theReadIterator, "N5-all.inc.s6") != OK)
    {
        gp_ErrorMessage("Unable to open \"N5-all.inc.s6\" for the general write iterator test.");
        Result = NOTOK;
    }

    while (Result == OK)
    {
        // Once the first graph is written, store the changes to the next one
        while (Result == OK && theWriteIterator != NULL)
        {
            if (gp_RetrieveGraphChange(theReadIterator, &e, &u, &v) != OK)
            {
                gp_ErrorMessage("Unable to retrieve a change to graph %d of \"N5-all.inc.s6\".",
                                numGraphs + 1);
                Result = NOTOK;
            }
            else if (e == NIL && u == NIL && v == NIL)
                break;
            else if (e != NIL)
            {
                int writerEdge = gp_FindEdge(writerGraph,
                                             gp_GetNeighbor(theGraph, gp_GetTwin(theGraph, e)),
                                             gp_GetNeighbor(theGraph, e));

                if (!gp_IsEdge(writerGraph, writerEdge))
                {
                    gp_ErrorMessage("The writer's graph lacks an edge deleted before graph %d "
                                    "of \"N5-all.inc.s6\".",
                                    numGraphs + 1);
                    Result = NOTOK;
                }
                else if (gp_StoreGraphChange(theWriteIterator, writerEdge, NIL, NIL) != OK)
                {
                    gp_ErrorMessage("Unable to store a deletion for graph %d of \"N5-all.inc.s6\".",
                                    numGraphs + 1);
                    Result = NOTOK;
                }
            }
            else if (gp_StoreGraphChange(theWriteIterator, NIL, u, v) != OK)
            {
                gp_ErrorMessage("Unable to store an addition for graph %d of \"N5-all.inc.s6\".",
                                numGraphs + 1);
                Result = NOTOK;
            }
        }

        if (Result != OK)
            break;

        if (gp_ReadGraph(theReadIterator) != OK)
        {
            gp_ErrorMessage("Unable to read graph %d of \"N5-all.inc.s6\".", numGraphs + 1);
            Result = NOTOK;
            break;
        }

        if (gp_EndReached(theReadIterator))
            break;

        numGraphs++;

        if (theWriteIterator == NULL)
        {
            if ((writerGraph = gp_New()) == NULL ||
                gp_EnsureVertexCapacity(writerGraph, gp_GetN(theGraph)) != OK ||
                gp_CopyGraph(writerGraph, theGraph) != OK ||
                gp_NewWriter((&theWriteIterator), writerGraph, WRITE_SPARSE6) != OK ||
                gp_InitWriterWithString(theWriteIterator, (&outputStr)) != OK ||
                gp_WriteGraph(theWriteIterator) != OK)
            {
                gp_ErrorMessage("Unable to write the first graph of \"N5-all.inc.s6\".");
                Result = NOTOK;
            }

            continue;
        }

        if (gp_WriteGraph(theWriteIterator) != OK)
        {
            gp_ErrorMessage("Unable to write the changes for graph %d of \"N5-all.inc.s6\".",
                            numGraphs);
            Result = NOTOK;
        }
        else
        {
            // Applying the batch must leave the writer's graph equal to the
            // graph read
            char *writerG6 = NULL, *readG6 = NULL;

            if (gp_WriteToString(writerGraph, (&writerG6), WRITE_GRAPH6) != OK ||
                gp_WriteToString(theGraph, (&readG6), WRITE_GRAPH6) != OK ||
                strcmp(writerG6, readG6) != 0)
            {
                gp_ErrorMessage("After graph %d of \"N5-all.inc.s6\" the writer's graph "
                                "is \"%s\" rather than \"%s\".",
                                numGraphs, writerG6 == NULL ? "" : writerG6,
                                readG6 == NULL ? "" : readG6);
                Result = NOTOK;
            }

            if (writerG6 != NULL)
                free(writerG6);
            if (readG6 != NULL)
                free(readG6);
        }
    }

    // The string is handed over when the writer is freed
    gp_FreeWriter((&theWriteIterator));

    if (Result == OK && numGraphs != 34)
    {
        gp_ErrorMessage("Read %d graphs of \"N5-all.inc.s6\" rather than 34.", numGraphs);
        Result = NOTOK;
    }

    if (Result == OK &&
        (outputStr == NULL || TextFileMatchesString("N5-all.inc.s6", outputStr) != TRUE))
    {
        gp_ErrorMessage("The general write iterator's output differs from \"N5-all.inc.s6\".");
        Result = NOTOK;
    }

    if (outputStr != NULL)
        free(outputStr);

    gp_FreeReader((&theReadIterator));
    gp_Free(&writerGraph);
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runGeneralWriteIteratorRefusalTests()

 Checks that the general write iterator refuses invalid parameters,
 leaves nothing allocated when it refuses to be created, and refuses
 incremental changes for the formats that have none.
 ****************************************************************************/

static int runGeneralWriteIteratorRefusalTests(void)
{
    int Result = OK;
    unsigned origQuietMode = gp_GetQuietMode();
    graphP theGraph = NULL;
    graphP emptyGraph = NULL;
    GPWriteIteratorP theWriteIterator = NULL;
    GPWriteIteratorP nullWriteIterator = NULL;
    char *outputStr = NULL;
    // Formats that hold one graph, a non-format and the slots around the range
    int const invalidModes[] = {0, WRITE_ADJLIST, WRITE_ADJMATRIX, WRITE_DEBUGINFO, WRITE_GRAPHML, -1, 99};
    int const singleOnlyModes[] = {WRITE_G6, WRITE_DIGRAPH6};
    int const allModes[] = {WRITE_G6, WRITE_SPARSE6, WRITE_DIGRAPH6};
    // A path no test environment has, so that opening it for writing fails
    char badFileName[] = "no-such-directory-for-planarity-tests/graphs.out";
    size_t i = 0;

    if ((theGraph = gp_New()) == NULL || gp_EnsureVertexCapacity(theGraph, 3) != OK ||
        (emptyGraph = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to create the graphs for the general write iterator refusal tests.");
        gp_Free(&theGraph);
        gp_Free(&emptyGraph);
        return NOTOK;
    }

    gp_SetQuietMode(QUIETMODE_ALL);

    if (gp_NewWriter(NULL, theGraph, WRITE_G6) == OK)
        Result = NOTOK;
    else if (gp_NewWriter((&theWriteIterator), NULL, WRITE_G6) == OK || theWriteIterator != NULL)
        Result = NOTOK;
    // The writer of the format refuses a graph without vertices, which must
    // leave nothing allocated
    else if (gp_NewWriter((&theWriteIterator), emptyGraph, WRITE_SPARSE6) == OK || theWriteIterator != NULL)
        Result = NOTOK;

    for (i = 0; Result == OK && i < sizeof(invalidModes) / sizeof(invalidModes[0]); i++)
        if (gp_NewWriter((&theWriteIterator), theGraph, invalidModes[i]) == OK || theWriteIterator != NULL)
            Result = NOTOK;

    // Allocating over an existing writer would lose it
    if (Result == OK)
    {
        GPWriteIteratorP theSameWriteIterator = NULL;

        if (gp_NewWriter((&theWriteIterator), theGraph, WRITE_SPARSE6) != OK)
            Result = NOTOK;
        else
        {
            theSameWriteIterator = theWriteIterator;

            if (gp_NewWriter((&theSameWriteIterator), theGraph, WRITE_G6) == OK ||
                theSameWriteIterator != theWriteIterator)
                Result = NOTOK;
        }

        gp_FreeWriter((&theWriteIterator));
    }

    // Formats without incremental lines refuse a change even after a write
    for (i = 0; Result == OK && i < sizeof(singleOnlyModes) / sizeof(singleOnlyModes[0]); i++)
    {
        if (gp_NewWriter((&theWriteIterator), theGraph, singleOnlyModes[i]) != OK ||
            gp_InitWriterWithString(theWriteIterator, (&outputStr)) != OK ||
            gp_WriteGraph(theWriteIterator) != OK ||
            gp_StoreGraphChange(theWriteIterator, NIL, gp_LowerBoundVertices(theGraph),
                                gp_LowerBoundVertices(theGraph) + 1) == OK)
            Result = NOTOK;

        gp_FreeWriter((&theWriteIterator));

        // The refusal leaves the output intact
        if (outputStr == NULL)
            Result = NOTOK;
        else
        {
            free(outputStr);
            outputStr = NULL;
        }
    }

    // Initializing with a file that cannot be created is refused for each
    // format, and leaves the writer free to be initialized with a string
    for (i = 0; Result == OK && i < sizeof(allModes) / sizeof(allModes[0]); i++)
    {
        if (gp_NewWriter((&theWriteIterator), theGraph, allModes[i]) != OK ||
            gp_InitWriterWithFileName(theWriteIterator, badFileName) == OK ||
            gp_InitWriterWithString(theWriteIterator, (&outputStr)) != OK ||
            gp_WriteGraph(theWriteIterator) != OK)
            Result = NOTOK;

        gp_FreeWriter((&theWriteIterator));

        if (outputStr == NULL)
            Result = NOTOK;
        else
        {
            free(outputStr);
            outputStr = NULL;
        }
    }

    // The error flag makes the writer withhold its output when freed
    if (Result == OK)
    {
        if (gp_NewWriter((&theWriteIterator), theGraph, WRITE_G6) != OK ||
            gp_InitWriterWithString(theWriteIterator, (&outputStr)) != OK ||
            gp_WriteGraph(theWriteIterator) != OK)
            Result = NOTOK;

        gp_SetOutputErrorFlag(theWriteIterator);
        gp_FreeWriter((&theWriteIterator));

        if (outputStr != NULL)
        {
            free(outputStr);
            outputStr = NULL;
            Result = NOTOK;
        }
    }

    // A missing iterator is refused, and the void methods ignore it
    if (Result == OK &&
        (gp_InitWriterWithString(NULL, (&outputStr)) == OK ||
         gp_InitWriterWithFileName(NULL, badFileName) == OK ||
         gp_StoreGraphChange(NULL, NIL, NIL, NIL) == OK ||
         gp_WriteGraph(NULL) == OK))
        Result = NOTOK;

    gp_SetOutputErrorFlag(NULL);
    gp_FreeWriter(NULL);
    gp_FreeWriter((&nullWriteIterator));

    gp_SetQuietMode(origQuietMode);

    if (Result != OK)
        gp_ErrorMessage("The general write iterator accepted an invalid call.");

    gp_Free(&emptyGraph);
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runGeneralWriteIteratorTests()
 ****************************************************************************/

int runGeneralWriteIteratorTests(void)
{
    int Result = OK;

    gp_Message("Starting Generalized Write Iterator Tests");

    if (Result == OK)
        Result = runGeneralWriteIteratorFormatsTest();
    if (Result == OK)
        Result = runGeneralWriteIteratorIncrementalTest();
    if (Result == OK)
        Result = runGeneralWriteIteratorRefusalTests();

    if (Result == OK)
        gp_Message("Successfully finished Generalized Write Iterator Tests.\n");
    else
        gp_Message("Error detected in Generalized Write Iterator Tests.\n");

    return Result;
}

int runFaceListTest(void)
{
    graphP theGraph = NULL, origGraph = NULL;
    char *faceList = NULL, *drawing = NULL;
    char const *infileName = NULL, *expectedOutfileName = NULL, *expectedDrawingFileName = NULL;
    int embedResult, retVal = OK;

#ifdef USE_1BASEDARRAYS
    infileName = "faceListComponents.txt";
    expectedOutfileName = "faceListComponents.out.txt";
    expectedDrawingFileName = "faceListComponents.Drawing.txt";
#else
    infileName = "faceListComponents.0-based.txt";
    expectedOutfileName = "faceListComponents.0-based.out.txt";
    expectedDrawingFileName = "faceListComponents.0-based.Drawing.txt";
#endif

    gp_Message("Starting Face List Test");

    if ((theGraph = gp_New()) == NULL ||
        gp_Read(theGraph, infileName) != OK ||
        (origGraph = gp_DupGraph(theGraph)) == NULL ||
        gp_ExtendWith_DrawPlanar(theGraph) != OK)
    {
        gp_ErrorMessage("Unable to set up the face list sample graph.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    embedResult = gp_Embed(theGraph, EMBEDFLAGS_DRAWPLANAR);
    if (embedResult != OK ||
        gp_TestEmbedResultIntegrity(theGraph, origGraph, embedResult) != OK)
    {
        gp_ErrorMessage("Unable to embed the face list sample graph.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_SortVertices(theGraph) != OK)
    {
        gp_ErrorMessage("Unable to restore original vertex labelling.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_CountEmbeddingFaces(theGraph) != 7)
    {
        gp_ErrorMessage("Unexpected face count for the face list sample graph.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_CreateEmbeddingFaceList(theGraph, &faceList) != OK || faceList == NULL)
    {
        gp_ErrorMessage("Unable to create the face list sample output.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (TextFileMatchesString(expectedOutfileName, faceList) != TRUE)
    {
        gp_ErrorMessage("Face list sample output did not match the expected output.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_DrawPlanar_RenderToString(theGraph, &drawing) != OK || drawing == NULL ||
        TextFileMatchesString(expectedDrawingFileName, drawing) != TRUE)
    {
        gp_ErrorMessage("Face list sample drawing did not match the expected output.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    gp_Message("Finished Face List Test.\n");

runFaceListTest_Cleanup:

    if (drawing != NULL)
    {
        free(drawing);
        drawing = NULL;
    }

    if (faceList != NULL)
    {
        free(faceList);
        faceList = NULL;
    }

    gp_Free(&origGraph);
    gp_Free(&theGraph);

    return retVal;
}

int runHideRestoreTests(void)
{
    graphP theGraph = NULL;
    G6ReadIteratorP theG6ReadIterator = NULL;
    platform_time start, end;
    int Result = OK;
    int lineNum = 0;

    gp_Message("Starting Hide/Restore Tests");
    platform_GetTime(start);

    if ((theGraph = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph for hide/restore tests.");
        return NOTOK;
    }

    if (g6_NewReader((&theG6ReadIterator), theGraph) != OK ||
        g6_InitReaderWithFileName(theG6ReadIterator, "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("Unable to allocate or initialize G6 read iterator for hide/restore tests.");
        Result = NOTOK;
    }

    while (Result == OK)
    {
        if (g6_ReadGraph(theG6ReadIterator) != OK)
        {
            gp_ErrorMessage("Unable to read graph on line %d for hide/restore tests.", lineNum + 1);
            Result = NOTOK;
            break;
        }

        if (g6_EndReached(theG6ReadIterator))
            break;

        lineNum++;

        if (runHideRestoreTest(theGraph) != OK)
        {
            gp_ErrorMessage("Hide/restore test failed for graph on line %d.", lineNum);
            Result = NOTOK;
            break;
        }
    }

    platform_GetTime(end);

    if (Result == OK)
        gp_Message("Done running Hide/Restore Tests (%.3lf seconds).", platform_GetDuration(start, end));

    gp_Message(" ");

    g6_FreeReader((&theG6ReadIterator));
    gp_Free(&theGraph);

    return Result;
}

int runHideRestoreTest(graphP theGraph)
{
    char *beforeStr = NULL, *afterStr = NULL;
    int Result = OK;
    int v;

    if (theGraph == NULL)
    {
        gp_ErrorMessage("runHideRestoreTest() received NULL graph.");
        return NOTOK;
    }

    if (gp_WriteToString(theGraph, &beforeStr, WRITE_ADJLIST) != OK || beforeStr == NULL)
    {
        gp_ErrorMessage("Unable to write graph to string before hide/restore test.");
        Result = NOTOK;
    }

    for (v = gp_LowerBoundVertices(theGraph); Result == OK && v < gp_UpperBoundVertices(theGraph); ++v)
    {
        if (gp_HideVertex(theGraph, v) != OK)
        {
            gp_ErrorMessage("gp_HideVertex() failed during hide/restore test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && gp_RestoreVertices(theGraph) != OK)
    {
        gp_ErrorMessage("gp_RestoreVertices() failed during hide/restore test.");
        Result = NOTOK;
    }

    if (Result == OK)
    {
        if (gp_WriteToString(theGraph, &afterStr, WRITE_ADJLIST) != OK || afterStr == NULL)
        {
            gp_ErrorMessage("Unable to write graph to string after hide/restore test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && strcmp(beforeStr, afterStr) != 0)
    {
        gp_ErrorMessage("Graph changed after hide/restore test.");
        Result = NOTOK;
    }

    if (beforeStr != NULL)
    {
        free(beforeStr);
        beforeStr = NULL;
    }

    if (afterStr != NULL)
    {
        free(afterStr);
        afterStr = NULL;
    }

    return Result;
}

int runIdentifyContractTests(void)
{
    graphP theGraph = NULL;
    G6ReadIteratorP theG6ReadIterator = NULL;
    platform_time start, end;
    int Result = OK;
    int lineNum = 0;

    gp_Message("Starting Identify/Contract Tests");
    platform_GetTime(start);

    if ((theGraph = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph for identify/contract tests.");
        return NOTOK;
    }

    if (g6_NewReader((&theG6ReadIterator), theGraph) != OK ||
        g6_InitReaderWithFileName(theG6ReadIterator, "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("Unable to allocate or initialize G6 read iterator for identify/contract tests.");
        Result = NOTOK;
    }

    while (Result == OK)
    {
        if (g6_ReadGraph(theG6ReadIterator) != OK)
        {
            gp_ErrorMessage("Unable to read graph on line %d for identify/contract tests.", lineNum + 1);
            Result = NOTOK;
            break;
        }

        if (g6_EndReached(theG6ReadIterator))
            break;

        lineNum++;

        if (runIdentifyContractTest(theGraph) != OK)
        {
            gp_ErrorMessage("Identify/contract test failed for graph on line %d.", lineNum);
            Result = NOTOK;
            break;
        }
    }

    platform_GetTime(end);

    if (Result == OK)
        gp_Message("Done running Identify/Contract Tests (%.3lf seconds).", platform_GetDuration(start, end));

    gp_Message(" ");

    g6_FreeReader((&theG6ReadIterator));
    gp_Free(&theGraph);

    return Result;
}

int runIdentifyContractTest(graphP theGraph)
{
    char *beforeStr = NULL, *afterStr = NULL;
    int Result = OK;
    int vertexOffset;
    int pairs[7][2];
    int i;

    if (theGraph == NULL)
    {
        gp_ErrorMessage("runIdentifyContractTest() received NULL graph.");
        return NOTOK;
    }

    vertexOffset = gp_LowerBoundVertices(theGraph);

    pairs[0][0] = vertexOffset;
    pairs[0][1] = vertexOffset + 1;
    pairs[1][0] = vertexOffset + 2;
    pairs[1][1] = vertexOffset + 3;
    pairs[2][0] = vertexOffset + 4;
    pairs[2][1] = vertexOffset + 5;
    pairs[3][0] = vertexOffset + 6;
    pairs[3][1] = vertexOffset + 7;
    pairs[4][0] = vertexOffset;
    pairs[4][1] = vertexOffset + 2;
    pairs[5][0] = vertexOffset + 4;
    pairs[5][1] = vertexOffset + 6;
    pairs[6][0] = vertexOffset;
    pairs[6][1] = vertexOffset + 4;

    if (gp_WriteToString(theGraph, &beforeStr, WRITE_ADJLIST) != OK || beforeStr == NULL)
    {
        gp_ErrorMessage("Unable to write graph to string before identify/contract test.");
        Result = NOTOK;
    }

    for (i = 0; Result == OK && i < 7; i++)
    {
        if (gp_IdentifyVertices(theGraph, pairs[i][0], pairs[i][1], NIL) != OK)
        {
            gp_ErrorMessage("gp_IdentifyVertices() failed during identify/contract test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && gp_RestoreVertices(theGraph) != OK)
    {
        gp_ErrorMessage("gp_RestoreVertices() failed during identify/contract test.");
        Result = NOTOK;
    }

    if (Result == OK)
    {
        if (gp_WriteToString(theGraph, &afterStr, WRITE_ADJLIST) != OK || afterStr == NULL)
        {
            gp_ErrorMessage("Unable to write graph to string after identify/contract test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && strcmp(beforeStr, afterStr) != 0)
    {
        gp_ErrorMessage("Graph changed after identify/contract test.");
        Result = NOTOK;
    }

    if (beforeStr != NULL)
    {
        free(beforeStr);
        beforeStr = NULL;
    }

    if (afterStr != NULL)
    {
        free(afterStr);
        afterStr = NULL;
    }

    return Result;
}

/****************************************************************************
 callRandomGraphs()
 ****************************************************************************/

// 'planarity -r [-q] C K N [O]': Random graphs
int callRandomGraphs(int argc, char *argv[])
{
    int offset = 0, NumGraphs = 0, SizeOfGraphs = 0;
    char *commandString = NULL, *outfileName = NULL;

    if (argc < 5 || argc > 7)
        return NOTOK;

    if (strncmp(argv[2], "-q", 2) == 0)
    {
        if (argc < 6)
            return NOTOK;

        offset = 1;
    }

    if (argc > (6 + offset))
        return NOTOK;

    commandString = argv[2 + offset];
    NumGraphs = atoi(argv[3 + offset]);
    SizeOfGraphs = atoi(argv[4 + offset]);

    if (argc == (6 + offset))
        outfileName = argv[5 + offset];

    return RandomGraphs(commandString, NumGraphs, SizeOfGraphs, outfileName, FALSE, FALSE, NULL);
}

/****************************************************************************
 callSpecificGraph()
 ****************************************************************************/

// 'planarity -s [-q] C I O [O2]': Specific graph
int callSpecificGraph(int argc, char *argv[])
{
    int offset = 0;
    char *commandString = NULL;
    char *infileName = NULL, *outfileName = NULL, *outfile2Name = NULL;

    if (argc < 5)
        return NOTOK;

    if (strncmp(argv[2], "-q", 2) == 0)
    {
        if (argc < 6)
            return NOTOK;

        offset = 1;
    }

    if (argc > (6 + offset))
        return NOTOK;

    commandString = argv[2 + offset];
    infileName = argv[3 + offset];
    outfileName = argv[4 + offset];

    if (argc == 6 + offset)
        outfile2Name = argv[5 + offset];

    return SpecificGraph(commandString, infileName, outfileName, outfile2Name, NULL, NULL, NULL);
}

/****************************************************************************
 callRandomMaxPlanarGraph()
 ****************************************************************************/

// 'planarity -rm [-q] N O [O2]': Maximal planar random graph
int callRandomMaxPlanarGraph(int argc, char *argv[])
{
    int offset = 0, numVertices = 0;
    char *outfileName = NULL, *outfile2Name = NULL;

    if (argc < 4)
        return NOTOK;

    if (strncmp(argv[2], "-q", 2) == 0)
    {
        if (argc < 5)
            return NOTOK;

        offset = 1;
    }

    if (argc > (5 + offset))
        return NOTOK;

    numVertices = atoi(argv[2 + offset]);
    outfileName = argv[3 + offset];

    if (argc == 5 + offset)
        outfile2Name = argv[4 + offset];

    return RandomGraph("-p", 0, numVertices, outfileName, outfile2Name);
}

/****************************************************************************
 callRandomNonplanarGraph()
 ****************************************************************************/

// 'planarity -rn [-q] N O [O2]': Non-planar random graph (maximal planar plus edge)
int callRandomNonplanarGraph(int argc, char *argv[])
{
    int offset = 0, numVertices = 0;
    char *outfileName = NULL, *outfile2Name = NULL;

    if (argc < 4)
        return NOTOK;

    if (strncmp(argv[2], "-q", 2) == 0)
    {
        if (argc < 5)
            return NOTOK;

        offset = 1;
    }

    if (argc > (5 + offset))
        return NOTOK;

    numVertices = atoi(argv[2 + offset]);
    outfileName = argv[3 + offset];

    if (argc == 5 + offset)
        outfile2Name = argv[4 + offset];

    return RandomGraph("-p", 1, numVertices, outfileName, outfile2Name);
}

/****************************************************************************
 callTransformGraph()
 ****************************************************************************/

// 'planarity -x [-q] -(gam) I O': Input file I is transformed from its given
// format to the format given by the g (g6), a (adjacency list) or m (matrix),
// and written to output file O.
int callTransformGraph(int argc, char *argv[])
{
    int offset = 0;
    char *commandString = NULL;
    char *infileName = NULL, *outfileName = NULL;

    if (argc < 5)
        return NOTOK;

    if (argv[2][0] == '-' && argv[2][1] == 'q')
    {
        if (argc < 6)
            return NOTOK;

        offset = 1;
    }

    if (argc > (5 + offset))
        return NOTOK;

    commandString = argv[2 + offset];

    infileName = argv[3 + offset];
    outfileName = argv[4 + offset];

    // We don't want to read from string, so inputStr is NULL
    // We don't want to write to string, so pOutputStr is NULL
    // We don't need to capture whether output is 0- or 1-based, so zeroBasedOutputFlag arg is NULL
    return TransformGraph(commandString, infileName, NULL, NULL, outfileName, NULL);
}

/****************************************************************************
 callTestAllGraphs()
 ****************************************************************************/

// 'planarity -t [-q] C I O': If the command line argument after -t [-q] is a
// recognized algorithm command C, then the input file I must be in ".g6" format
// (report an error otherwise), and the algorithm(s) indicated by C are executed
// on the graph(s) in the input file, with the results of the execution stored
// in output file O.
int callTestAllGraphs(int argc, char *argv[])
{
    int offset = 0;
    char *commandString = NULL;
    char *infileName = NULL, *outfileName = NULL;

    if (argc < 5)
        return NOTOK;

    if (argv[2][0] == '-' && argv[2][1] == 'q')
    {
        if (argc < 6)
            return NOTOK;

        offset = 1;
    }

    if (argc > (5 + offset))
        return NOTOK;

    commandString = argv[2 + offset];

    infileName = argv[3 + offset];
    outfileName = argv[4 + offset];

    // NOTE: We don't want to write to string, so pOutputStr is NULL
    return TestAllGraphs(commandString, infileName, outfileName, NULL, NULL);
}
