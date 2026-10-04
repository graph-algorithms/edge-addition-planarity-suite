/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../planarity.h"

#define GRAPHML_XMLNS "http://graphml.graphdrawing.org/xmlns"
#define GRAPHML_ROOT_OPEN "<graphml xmlns='" GRAPHML_XMLNS "'>"
#define GRAPHML_GRAPH_OPEN \
    "<graph edgedefault='undirected' parse.nodeids='canonical' " \
    "parse.edgeids='canonical' parse.order='nodesfirst'>"
#define GRAPHML_VALID_BODY \
    "<node id='n0'/><node id='n1'/><edge id='e0' source='n0' target='n1'/>"
#define GRAPHML_VALID_DOCUMENT \
    GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph></graphml>"

/* Called from planarityCommandLine.c */
int runGraphMLTests(void);

typedef struct
{
    int source;
    int target;
    int direction;
} GraphMLEdgeExpectation;

typedef struct
{
    char const *name;
    char const *input;
    int order;
    int size;
    int zeroBased;
    GraphMLEdgeExpectation edges[4];
} GraphMLAcceptCase;

typedef struct
{
    char const *name;
    char const *input;
} GraphMLRejectCase;

static char *copyGraphMLTestString(char const *graphMLStr)
{
    char *copy = NULL;
    size_t length = 0;

    if (graphMLStr == NULL)
        return NULL;

    length = strlen(graphMLStr) + 1;
    copy = (char *)malloc(length);
    if (copy != NULL)
        memcpy(copy, graphMLStr, length);

    return copy;
}

static int runGraphMLWriteTest(char const *inputFileName,
                               char const *expectedOutputFileName)
{
    graphP G = gp_New();
    char *actualOutput = NULL;
    int Result = OK;

    if (G == NULL)
        return NOTOK;

    if (gp_Read(G, inputFileName) != OK ||
        gp_WriteToString(G, &actualOutput, WRITE_GRAPHML) != OK ||
        actualOutput == NULL ||
        TextFileMatchesString(expectedOutputFileName, actualOutput) != TRUE)
        Result = NOTOK;

    free(actualOutput);
    gp_Free(&G);

    return Result;
}

static int runBasicGraphMLWriteTest(void)
{
    if (runGraphMLWriteTest("Digraph.transposeTest.txt",
                            "Digraph.transposeTest.graphml") != OK ||
        runGraphMLWriteTest("Digraph.transposeTest.0-based.txt",
                            "Digraph.transposeTest.0-based.graphml") != OK)
        return NOTOK;

    return OK;
}

static int runGraphMLReadWriteTest(char const *inputFileName)
{
    graphP G = gp_New();
    char *actualOutput = NULL;
    int Result = OK;

    if (G == NULL)
        return NOTOK;

    if (gp_Read(G, inputFileName) != OK ||
        gp_WriteToString(G, &actualOutput, WRITE_GRAPHML) != OK ||
        actualOutput == NULL ||
        TextFileMatchesString(inputFileName, actualOutput) != TRUE)
        Result = NOTOK;

    free(actualOutput);
    gp_Free(&G);

    return Result;
}

static int runBasicGraphMLReadTest(void)
{
    if (runGraphMLReadWriteTest("Digraph.transposeTest.graphml") != OK ||
        runGraphMLReadWriteTest("Digraph.transposeTest.0-based.graphml") != OK)
        return NOTOK;

    return OK;
}

static int runGraphMLAcceptTest(GraphMLAcceptCase const *testCase)
{
    graphP G = gp_New();
    char *inputCopy = copyGraphMLTestString(testCase->input);
    char *actualOutput = NULL;
    int Result = OK;
    int edgeIndex = 0;
    int vertexOffset = 0;

    if (G == NULL || inputCopy == NULL || gp_ReadFromString(G, inputCopy) != OK)
        Result = NOTOK;

    if (Result == OK &&
        (gp_GetN(G) != testCase->order || gp_GetM(G) != testCase->size))
        Result = NOTOK;

    if (Result == OK)
    {
        vertexOffset = gp_LowerBoundVertexStorage(G);
        for (edgeIndex = 0; edgeIndex < testCase->size; edgeIndex++)
        {
            int edge = gp_FindEdge(G,
                                   vertexOffset + testCase->edges[edgeIndex].source,
                                   vertexOffset + testCase->edges[edgeIndex].target);
            if (edge == NIL ||
                gp_GetDirection(G, edge) != testCase->edges[edgeIndex].direction)
            {
                Result = NOTOK;
                break;
            }
        }
    }

    if (Result == OK &&
        (gp_WriteToString(G, &actualOutput, WRITE_GRAPHML) != OK ||
         actualOutput == NULL))
        Result = NOTOK;

    if (Result == OK &&
        ((testCase->zeroBased &&
          strstr(actualOutput,
                 "<data key=\"graphflags_zerobasedio\">true</data>") == NULL) ||
         (!testCase->zeroBased &&
          strstr(actualOutput, "graphflags_zerobasedio") != NULL)))
        Result = NOTOK;

    if (Result != OK)
        gp_ErrorMessage("Valid GraphML case '%s' was not read as expected.",
                        testCase->name);

    free(inputCopy);
    free(actualOutput);
    gp_Free(&G);

    return Result;
}

static int runGraphMLRejectTest(GraphMLRejectCase const *testCase)
{
    graphP G = gp_New();
    char *inputCopy = copyGraphMLTestString(testCase->input);
    unsigned quietModeCache = gp_GetQuietMode();
    int Result = OK;

    if (G == NULL || inputCopy == NULL)
        Result = NOTOK;

    gp_SetQuietMode(QUIETMODE_ALL);
    if (Result == OK && gp_ReadFromString(G, inputCopy) == OK)
        Result = NOTOK;
    gp_SetQuietMode(quietModeCache);

    if (Result != OK)
        gp_ErrorMessage("Invalid GraphML case '%s' was accepted.", testCase->name);

    free(inputCopy);
    gp_Free(&G);

    return Result;
}

static int runGraphMLAcceptTests(void)
{
    static GraphMLAcceptCase const acceptCases[] = {
        {
            "whitespace, comments, and paired empty elements",
            " \t\r\n<!--top-->" GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
            "<node id='n0'/><node id='n1'></node>"
            "<edge source='n0' target='n1'></edge></graph>",
            2, 1, FALSE, {{0, 1, 0}}
        },
        {
            "UTF-8 BOM, key default, and character decoding",
            "\xEF\xBB\xBF<?xml version='1.0' encoding='UTF-8' standalone='yes'?>"
            "<!--prolog--><graphml xmlns='" GRAPHML_XMLNS "' "
            "xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
            "xsi:schemaLocation='caf\xC3\xA9 &amp; test' "
            "testencoding='\xC2\xA0\xC3\xBF &lt;&gt;&amp;&apos;&quot;'>"
            "<key id=' graphflags_zerobasedio ' for='graph' "
            "attr.name=' graphflags_zerobasedio ' attr.type='boolean'>"
            "<!--default--><default> 1 </default></key>"
            "<graph edgedefault='directed' parse.nodes='2' parse.edges='1' "
            "parse.nodeids='canonical' parse.edgeids='canonical' "
            "parse.order='nodesfirst'>"
            "<node id='n0'/><!--nodes--><node id='n1'/>"
            "<edge id='e0' source='n0' target='n1'/></graph>",
            2, 1, TRUE, {{0, 1, EDGEFLAG_DIRECTION_OUTONLY}}
        },
        {
            "ISO-8859-1 declaration and false graph flag",
            "<?xml version='1.0' encoding='ISO-8859-1'?>"
            GRAPHML_ROOT_OPEN
            "<key id='graphflags_zerobasedio' for='graph' "
            "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
            "<graph edgedefault='directed' parse.nodeids='canonical' "
            "parse.edgeids='canonical' parse.order='nodesfirst'>"
            "<data key='graphflags_zerobasedio'> false </data>"
            "<node id='n0'/><node id='n1'/>"
            "<edge source='n0' target='n1' directed='false'/></graph>",
            2, 1, FALSE, {{0, 1, 0}}
        },
        {
            "undirected default with directed override",
            GRAPHML_ROOT_OPEN
            "<graph edgedefault='undirected' parse.nodes='3' parse.edges='2' "
            "parse.nodeids='canonical' parse.edgeids='canonical' "
            "parse.order='nodesfirst'>"
            "<node id='n0'/><node id='n1'/><node id='n2'/>"
            "<edge id='e0' source='n0' target='n1'/>"
            "<edge id='e1' source='n1' target='n2' directed='true'/></graph>",
            3, 2, FALSE,
            {{0, 1, 0}, {1, 2, EDGEFLAG_DIRECTION_OUTONLY}}
        },
        {
            "directed default with undirected override",
            GRAPHML_ROOT_OPEN
            "<graph edgedefault='directed' parse.nodes='3' parse.edges='2' "
            "parse.nodeids='canonical' parse.edgeids='canonical' "
            "parse.order='nodesfirst'>"
            "<node id='n0'/><node id='n1'/><node id='n2'/>"
            "<edge id='e0' source='n0' target='n1'/>"
            "<edge id='e1' source='n1' target='n2' directed='false'/></graph>",
            3, 2, FALSE,
            {{0, 1, EDGEFLAG_DIRECTION_OUTONLY}, {1, 2, 0}}
        }
    };
    size_t index = 0;

    for (index = 0; index < sizeof(acceptCases) / sizeof(acceptCases[0]); index++)
    {
        if (runGraphMLAcceptTest(&acceptCases[index]) != OK)
            return NOTOK;
    }

    return OK;
}

static int runGraphMLRejectTests(void)
{
    static GraphMLRejectCase const rejectCases[] = {
        {"XML version", "<?xml version='1.1'?>" GRAPHML_VALID_DOCUMENT},
        {"XML encoding", "<?xml version='1.0' encoding='UTF-16'?>" GRAPHML_VALID_DOCUMENT},
        {"XML standalone", "<?xml version='1.0' standalone='no'?>" GRAPHML_VALID_DOCUMENT},
        {"processing instruction", "<?bad?>" GRAPHML_VALID_DOCUMENT},
        {"document type", "<!DOCTYPE graphml>" GRAPHML_VALID_DOCUMENT},
        {"missing namespace", "<graphml>" GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"wrong namespace", "<graphml xmlns='wrong'>" GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"schema location without xsi namespace",
         "<graphml xmlns='" GRAPHML_XMLNS "' xsi:schemaLocation='value'>"
         GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"graphml attribute", "<graphml xmlns='" GRAPHML_XMLNS "' extra='no'>"
                              GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"graph attribute", GRAPHML_ROOT_OPEN
                            "<graph extra='no' edgedefault='undirected' "
                            "parse.nodeids='canonical' parse.edgeids='canonical' "
                            "parse.order='nodesfirst'>" GRAPHML_VALID_BODY "</graph>"},
        {"node attribute", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                           "<node id='n0' extra='no'/><node id='n1'/>"
                           "<edge source='n0' target='n1'/></graph>"},
        {"edge attribute", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                           "<node id='n0'/><node id='n1'/>"
                           "<edge source='n0' target='n1' extra='no'/></graph>"},
        {"key attribute", GRAPHML_ROOT_OPEN
                          "<key id='graphflags_zerobasedio' for='graph' "
                          "attr.name='graphflags_zerobasedio' attr.type='boolean' extra='no'/>"
                          GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"data attribute", GRAPHML_ROOT_OPEN
                           "<key id='graphflags_zerobasedio' for='graph' "
                           "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
                           GRAPHML_GRAPH_OPEN "<data key='graphflags_zerobasedio' extra='no'>true</data>"
                           GRAPHML_VALID_BODY "</graph>"},
        {"unsupported graphml child", GRAPHML_ROOT_OPEN "<bogus/>" GRAPHML_GRAPH_OPEN
                                       GRAPHML_VALID_BODY "</graph>"},
        {"unsupported graph child", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN "<bogus/>"
                                     GRAPHML_VALID_BODY "</graph>"},
        {"unsupported node child", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                    "<node id='n0'><bogus/></node><node id='n1'/>"
                                    "<edge source='n0' target='n1'/></graph>"},
        {"unsupported edge child", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                    "<node id='n0'/><node id='n1'/>"
                                    "<edge source='n0' target='n1'><bogus/></edge></graph>"},
        {"unsupported key child", GRAPHML_ROOT_OPEN
                                   "<key id='graphflags_zerobasedio' for='graph' "
                                   "attr.name='graphflags_zerobasedio' attr.type='boolean'>"
                                   "<bogus/></key>" GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"unsupported data child", GRAPHML_ROOT_OPEN
                                    "<key id='graphflags_zerobasedio' for='graph' "
                                    "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
                                    GRAPHML_GRAPH_OPEN
                                    "<data key='graphflags_zerobasedio'><bogus/></data>"
                                    GRAPHML_VALID_BODY "</graph>"},
        {"unsupported key id", GRAPHML_ROOT_OPEN
                               "<key id='other' for='graph' attr.name='graphflags_zerobasedio' "
                               "attr.type='boolean'/>" GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"unsupported key for", GRAPHML_ROOT_OPEN
                                "<key id='graphflags_zerobasedio' for='node' "
                                "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
                                GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"unsupported key name", GRAPHML_ROOT_OPEN
                                 "<key id='graphflags_zerobasedio' for='graph' "
                                 "attr.name='other' attr.type='boolean'/>"
                                 GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"unsupported key type", GRAPHML_ROOT_OPEN
                                 "<key id='graphflags_zerobasedio' for='graph' "
                                 "attr.name='graphflags_zerobasedio' attr.type='string'/>"
                                 GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"second key", GRAPHML_ROOT_OPEN
                       "<key id='graphflags_zerobasedio' for='graph' "
                       "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
                       "<key id='graphflags_zerobasedio' for='graph' "
                       "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
                       GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"unsupported data key", GRAPHML_ROOT_OPEN
                                 "<key id='graphflags_zerobasedio' for='graph' "
                                 "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
                                 GRAPHML_GRAPH_OPEN "<data key='other'>true</data>"
                                 GRAPHML_VALID_BODY "</graph>"},
        {"non-boolean graph data", GRAPHML_ROOT_OPEN
                                    "<key id='graphflags_zerobasedio' for='graph' "
                                    "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
                                    GRAPHML_GRAPH_OPEN
                                    "<data key='graphflags_zerobasedio'>perhaps</data>"
                                    GRAPHML_VALID_BODY "</graph>"},
        {"edgedefault", GRAPHML_ROOT_OPEN
                        "<graph edgedefault='sometimes' parse.nodeids='canonical' "
                        "parse.edgeids='canonical' parse.order='nodesfirst'>"
                        GRAPHML_VALID_BODY "</graph>"},
        {"parse.nodeids", GRAPHML_ROOT_OPEN
                          "<graph edgedefault='undirected' parse.nodeids='free' "
                          "parse.edgeids='canonical' parse.order='nodesfirst'>"
                          GRAPHML_VALID_BODY "</graph>"},
        {"parse.edgeids", GRAPHML_ROOT_OPEN
                          "<graph edgedefault='undirected' parse.nodeids='canonical' "
                          "parse.edgeids='free' parse.order='nodesfirst'>"
                          GRAPHML_VALID_BODY "</graph>"},
        {"parse.order", GRAPHML_ROOT_OPEN
                        "<graph edgedefault='undirected' parse.nodeids='canonical' "
                        "parse.edgeids='canonical' parse.order='free'>"
                        GRAPHML_VALID_BODY "</graph>"},
        {"parse.nodes malformed", GRAPHML_ROOT_OPEN
                                  "<graph edgedefault='undirected' parse.nodes='two' "
                                  "parse.nodeids='canonical' parse.edgeids='canonical' "
                                  "parse.order='nodesfirst'>" GRAPHML_VALID_BODY "</graph>"},
        {"parse.edges malformed", GRAPHML_ROOT_OPEN
                                  "<graph edgedefault='undirected' parse.edges='one' "
                                  "parse.nodeids='canonical' parse.edgeids='canonical' "
                                  "parse.order='nodesfirst'>" GRAPHML_VALID_BODY "</graph>"},
        {"parse.nodes mismatch", GRAPHML_ROOT_OPEN
                                 "<graph edgedefault='undirected' parse.nodes='3' "
                                 "parse.nodeids='canonical' parse.edgeids='canonical' "
                                 "parse.order='nodesfirst'>" GRAPHML_VALID_BODY "</graph>"},
        {"parse.edges mismatch", GRAPHML_ROOT_OPEN
                                 "<graph edgedefault='undirected' parse.edges='2' "
                                 "parse.nodeids='canonical' parse.edgeids='canonical' "
                                 "parse.order='nodesfirst'>" GRAPHML_VALID_BODY "</graph>"},
        {"noncanonical node id", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                 "<node id='node0'/><node id='n1'/>"
                                 "<edge source='n0' target='n1'/></graph>"},
        {"nonsequential node id", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                  "<node id='n0'/><node id='n2'/>"
                                  "<edge source='n0' target='n1'/></graph>"},
        {"noncanonical edge id", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                 "<node id='n0'/><node id='n1'/>"
                                 "<edge id='edge0' source='n0' target='n1'/></graph>"},
        {"nonsequential edge id", GRAPHML_ROOT_OPEN
                                  "<graph edgedefault='undirected' parse.nodeids='canonical' "
                                  "parse.edgeids='canonical' parse.order='nodesfirst'>"
                                  "<node id='n0'/><node id='n1'/><node id='n2'/>"
                                  "<edge id='e0' source='n0' target='n1'/>"
                                  "<edge id='e2' source='n1' target='n2'/></graph>"},
        {"node after edge", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                            "<node id='n0'/><node id='n1'/>"
                            "<edge source='n0' target='n1'/><node id='n2'/></graph>"},
        {"data after node", GRAPHML_ROOT_OPEN
                            "<key id='graphflags_zerobasedio' for='graph' "
                            "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
                            GRAPHML_GRAPH_OPEN "<node id='n0'/><node id='n1'/>"
                            "<data key='graphflags_zerobasedio'>true</data>"
                            "<edge source='n0' target='n1'/></graph>"},
        {"source is not canonical", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                    "<node id='n0'/><node id='n1'/>"
                                    "<edge source='xyz' target='n1'/></graph>"},
        {"source is outside graph", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                    "<node id='n0'/><node id='n1'/>"
                                    "<edge source='n1000' target='n1'/></graph>"},
        {"target is not canonical", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                    "<node id='n0'/><node id='n1'/>"
                                    "<edge source='n0' target='xyz'/></graph>"},
        {"target is outside graph", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                    "<node id='n0'/><node id='n1'/>"
                                    "<edge source='n0' target='n1000'/></graph>"},
        {"directed value", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                           "<node id='n0'/><node id='n1'/>"
                           "<edge source='n0' target='n1' directed='1'/></graph>"},
        {"raw less-than in attribute", "<graphml xmlns='" GRAPHML_XMLNS
                                         "' testencoding='bad < value'>"
                                         GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"raw ampersand in attribute", "<graphml xmlns='" GRAPHML_XMLNS
                                         "' testencoding='bad & value'>"
                                         GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"unknown entity", "<graphml xmlns='" GRAPHML_XMLNS
                            "' testencoding='bad &unknown; value'>"
                            GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"single quote in single-quoted value", "<graphml xmlns='" GRAPHML_XMLNS
                                                  "' testencoding='bad ' quote'>"
                                                  GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"double quote in double-quoted value", "<graphml xmlns='" GRAPHML_XMLNS
                                                  "' testencoding=\"bad \" quote\">"
                                                  GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"bad UTF-8 C2 continuation", "<graphml xmlns='" GRAPHML_XMLNS
                                       "' testencoding='\xC2\x7F'>"
                                       GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"bad UTF-8 C3 continuation", "<graphml xmlns='" GRAPHML_XMLNS
                                       "' testencoding='\xC3\x7F'>"
                                       GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"unsupported UTF-8 lead byte", "<graphml xmlns='" GRAPHML_XMLNS
                                          "' testencoding='\xC4\x80'>"
                                          GRAPHML_GRAPH_OPEN GRAPHML_VALID_BODY "</graph>"},
        {"UTF-8 BOM conflicts with declaration",
         "\xEF\xBB\xBF<?xml version='1.0' encoding='ISO-8859-1'?>"
         GRAPHML_VALID_DOCUMENT},
        {"UTF-16 big-endian BOM", "\xFE\xFF\x00\x3C"},
        {"UTF-16 little-endian BOM", "\xFF\xFE\x3C\x00"},
        {"EBCDIC signature", "\x4C\x6F\xA7\x94"},
        {"truncated graphml start tag", "<graphm"},
        {"truncated key start tag", GRAPHML_ROOT_OPEN "<ke"},
        {"truncated default start tag", GRAPHML_ROOT_OPEN
                                        "<key id='graphflags_zerobasedio' for='graph' "
                                        "attr.name='graphflags_zerobasedio' attr.type='boolean'><defaul"},
        {"truncated graph start tag", GRAPHML_ROOT_OPEN "<grap"},
        {"truncated data start tag", GRAPHML_ROOT_OPEN
                                     "<key id='graphflags_zerobasedio' for='graph' "
                                     "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
                                     GRAPHML_GRAPH_OPEN "<dat"},
        {"truncated node start tag", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN "<nod"},
        {"truncated edge start tag", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                     "<node id='n0'/><node id='n1'/><edg"},
        {"truncated key end tag", GRAPHML_ROOT_OPEN
                                  "<key id='graphflags_zerobasedio' for='graph' "
                                  "attr.name='graphflags_zerobasedio' attr.type='boolean'>"
                                  "<default>true</default></key"},
        {"truncated default end tag", GRAPHML_ROOT_OPEN
                                      "<key id='graphflags_zerobasedio' for='graph' "
                                      "attr.name='graphflags_zerobasedio' attr.type='boolean'>"
                                      "<default>true</defaul"},
        {"truncated graph end tag", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                    GRAPHML_VALID_BODY "</grap"},
        {"truncated data end tag", GRAPHML_ROOT_OPEN
                                   "<key id='graphflags_zerobasedio' for='graph' "
                                   "attr.name='graphflags_zerobasedio' attr.type='boolean'/>"
                                   GRAPHML_GRAPH_OPEN
                                   "<data key='graphflags_zerobasedio'>true</dat"},
        {"truncated node end tag", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                   "<node id='n0'></nod"},
        {"truncated edge end tag", GRAPHML_ROOT_OPEN GRAPHML_GRAPH_OPEN
                                   "<node id='n0'/><node id='n1'/>"
                                   "<edge source='n0' target='n1'></edg"}
    };
    size_t index = 0;

    for (index = 0; index < sizeof(rejectCases) / sizeof(rejectCases[0]); index++)
    {
        if (runGraphMLRejectTest(&rejectCases[index]) != OK)
            return NOTOK;
    }

    return OK;
}

static int runGraphMLControlCharacterTests(void)
{
    unsigned int character = 0;

    for (character = 0x01; character <= 0x1F; character++)
    {
        char input[2048];
        char name[80];
        GraphMLRejectCase testCase;
        int charsWritten = 0;

        if (character == '\t' || character == '\n' || character == '\r')
            continue;

        charsWritten = snprintf(input, sizeof(input),
                                "<graphml xmlns='%s' testencoding='before%cafter'>%s%s</graph>",
                                GRAPHML_XMLNS, (int)character,
                                GRAPHML_GRAPH_OPEN, GRAPHML_VALID_BODY);
        if (charsWritten < 0 || (size_t)charsWritten >= sizeof(input))
            return NOTOK;

        charsWritten = snprintf(name, sizeof(name),
                                "control character 0x%02X", character);
        if (charsWritten < 0 || (size_t)charsWritten >= sizeof(name))
            return NOTOK;

        testCase.name = name;
        testCase.input = input;
        if (runGraphMLRejectTest(&testCase) != OK)
            return NOTOK;
    }

    return OK;
}

static int runGraphMLReaderStringTests(void)
{
    if (runGraphMLAcceptTests() != OK ||
        runGraphMLRejectTests() != OK ||
        runGraphMLControlCharacterTests() != OK)
        return NOTOK;

    return OK;
}

int runGraphMLTests(void)
{
    int Result = OK;

    gp_Message("Starting GraphML Tests");

    if (runBasicGraphMLWriteTest() != OK)
    {
        gp_ErrorMessage("Basic GraphML write test failed.");
        Result = NOTOK;
    }
    else if (runBasicGraphMLReadTest() != OK)
    {
        gp_ErrorMessage("Basic GraphML read test failed.");
        Result = NOTOK;
    }
    else if (runGraphMLReaderStringTests() != OK)
    {
        gp_ErrorMessage("GraphML reader string test failed.");
        Result = NOTOK;
    }
    else
        gp_Message("Finished GraphML Tests.\n");

    return Result;
}
