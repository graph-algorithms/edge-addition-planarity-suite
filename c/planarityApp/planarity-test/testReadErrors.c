/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#if defined(HAVE_FOPENCOOKIE) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "../planarity.h"
#include "../../graphLib/extensionSystem/graphFunctionTable.h"
#include "../../graphLib/io/strOrFile.h"

extern int _ReadGraph(graphP theGraph, strOrFileP *pInputContainer);
extern int _g6_InitReaderWithStrOrFile(G6ReadIteratorP reader, strOrFileP *input);
extern int _g6_ReadGraphFromStrOrFile(graphP theGraph, strOrFileP *input);
int runReadErrorTests(void);

#if defined(HAVE_FOPENCOOKIE) || defined(HAVE_FUNOPEN)

typedef struct
{
    const char *data;
    size_t position;
    int failAtEnd;
} TestInput;

static int failures = 0;
static int checks = 0;
static int postprocessCalls = 0;
static const char *expectedExtraData = NULL;

#define CHECK(condition)                                                        \
    do                                                                         \
    {                                                                          \
        checks++;                                                              \
        if (!(condition))                                                      \
        {                                                                      \
            gp_ErrorMessage("Line %d: %s", __LINE__, #condition);               \
            failures++;                                                        \
        }                                                                      \
    } while (0)

#ifdef HAVE_FOPENCOOKIE
static ssize_t readInput(void *cookie, char *buffer, size_t size)
#else
static int readInput(void *cookie, char *buffer, int size)
#endif
{
    TestInput *input = (TestInput *)cookie;

    if (size == 0)
        return 0;
    if (input->data[input->position] != '\0')
    {
        // One byte per read makes the failure boundary independent of buffering.
        buffer[0] = input->data[input->position++];
        return 1;
    }
    if (input->failAtEnd)
    {
        errno = EIO;
        return -1;
    }
    return 0;
}

static int newInput(TestInput *input, strOrFileP *pContainer)
{
    strOrFileP container = sf_NewInputContainer("placeholder", NULL);
    FILE *stream = NULL;

#ifdef HAVE_FOPENCOOKIE
    cookie_io_functions_t functions = {readInput, NULL, NULL, NULL};
    stream = fopencookie(input, "r", functions);
#else
    stream = funopen(input, readInput, NULL, NULL, NULL);
#endif
    if (container == NULL || stream == NULL)
    {
        gp_ErrorMessage("Unable to create test input stream.");
        sf_Free(&container);
        if (stream != NULL)
            fclose(stream);
        return NOTOK;
    }
    sb_Free(&container->theStrBuf);
    container->pFile = stream;
    *pContainer = container;
    return OK;
}

static int testLineReads(void)
{
    char buffer[32] = {0};
    char pushed[] = "pushed";
    char retry[] = "retry";
    TestInput input = {"line\n", 0, FALSE};
    strOrFileP container = NULL;

    if (newInput(&input, &container) != OK)
        return NOTOK;

    CHECK(container->inputErrorFlag == FALSE);
    errno = EIO;
    CHECK(sf_fgets(buffer, sizeof(buffer), container) == buffer);
    CHECK(strcmp(buffer, "line\n") == 0);
    CHECK(sf_fgets(buffer, sizeof(buffer), container) == NULL);
    CHECK(container->inputErrorFlag == FALSE);
    CHECK(sf_ungets(pushed, container) == OK);
    CHECK(sf_fgets(buffer, sizeof(buffer), container) == buffer);
    CHECK(strcmp(buffer, "pushed") == 0);
    CHECK(container->inputErrorFlag == FALSE);
    sf_Free(&container);

    input.position = 0;
    input.failAtEnd = TRUE;
    if (newInput(&input, &container) != OK)
        return NOTOK;
    CHECK(sf_fgets(buffer, sizeof(buffer), container) == buffer);
    CHECK(sf_ungets(pushed, container) == OK);
    CHECK(sf_fgets(buffer, sizeof(buffer), container) == NULL);
    CHECK(container->inputErrorFlag == TRUE);
    CHECK(ferror(container->pFile) != 0);
    // Clearing stdio's indicator must not erase the container's error history.
    clearerr(container->pFile);
    input.failAtEnd = FALSE;
    CHECK(sf_ungets(retry, container) == OK);
    CHECK(sf_fgets(buffer, sizeof(buffer), container) == NULL);
    CHECK(sf_getc(container) == EOF);
    CHECK(container->inputErrorFlag == TRUE);
    sf_Free(&container);

    input.data = "partial";
    input.position = 0;
    input.failAtEnd = TRUE;
    if (newInput(&input, &container) != OK)
        return NOTOK;
    CHECK(sf_fgets(buffer, sizeof(buffer), container) == NULL);
    CHECK(container->inputErrorFlag == TRUE);
    sf_Free(&container);

    container = sf_NewInputContainer("string", NULL);
    if (container == NULL)
    {
        gp_ErrorMessage("Unable to create string-backed test input.");
        return NOTOK;
    }
    CHECK(sf_fgets(buffer, sizeof(buffer), container) == buffer);
    CHECK(strcmp(buffer, "string") == 0);
    CHECK(sf_fgets(buffer, sizeof(buffer), container) == NULL);
    CHECK(container->inputErrorFlag == FALSE);
    sf_Free(&container);
    return OK;
}

static int testCharacterReads(void)
{
    TestInput input = {"x", 0, TRUE};
    strOrFileP container = NULL;

    if (newInput(&input, &container) != OK)
        return NOTOK;
    CHECK(sf_getc(container) == 'x');
    CHECK(sf_getc(container) == EOF);
    CHECK(container->inputErrorFlag == TRUE);
    sf_Free(&container);

    for (int failAtEnd = FALSE; failAtEnd <= TRUE; failAtEnd++)
    {
        int value = 77;
        input.data = "123";
        input.position = 0;
        input.failAtEnd = failAtEnd;
        if (newInput(&input, &container) != OK)
            return NOTOK;
        CHECK(sf_ReadInteger(&value, container) == (failAtEnd ? NOTOK : OK));
        CHECK(value == (failAtEnd ? 77 : 123));
        CHECK(container->inputErrorFlag == failAtEnd);
        sf_Free(&container);

        input.data = " \t";
        input.position = 0;
        if (newInput(&input, &container) != OK)
            return NOTOK;
        CHECK(sf_ReadSkipWhitespace(container) == (failAtEnd ? NOTOK : OK));
        CHECK(container->inputErrorFlag == failAtEnd);
        sf_Free(&container);
    }
    return OK;
}

static int readPostprocess(graphP graph, char *extraData)
{
    (void)graph;
    CHECK(expectedExtraData != NULL && strcmp(extraData, expectedExtraData) == 0);
    postprocessCalls++;
    return OK;
}

static int testGraphReads(void)
{
    const struct
    {
        const char *data;
        const char *extraData;
    } inputs[] = {
        {"N=1\n0: -1\n", NULL},
        {"N=1\n0: -1\nmetadata\n", "metadata\n"},
        {"N=1\n0: -1\nmetadata", "metadata"},
        {"2\n0", NULL},
        {"2\n0\nmetadata\n", "\nmetadata\n"}};

    for (int failAtEnd = FALSE; failAtEnd <= TRUE; failAtEnd++)
    {
        for (size_t index = 0; index < sizeof(inputs) / sizeof(inputs[0]); index++)
        {
            TestInput input = {inputs[index].data, 0, failAtEnd};
            strOrFileP container = NULL;
            graphP graph = NULL;

            if (newInput(&input, &container) != OK)
                return NOTOK;
            graph = gp_New();
            if (graph == NULL)
            {
                gp_ErrorMessage("Unable to create graph for read-error test.");
                sf_Free(&container);
                return NOTOK;
            }
            postprocessCalls = 0;
            expectedExtraData = inputs[index].extraData;
            graph->functions->fpReadPostprocess = readPostprocess;

            CHECK(_ReadGraph(graph, &container) == (failAtEnd ? NOTOK : OK));
            CHECK(container == NULL);
            CHECK(postprocessCalls == (!failAtEnd && expectedExtraData != NULL ? 1 : 0));
            gp_Free(&graph);
        }
    }
    return OK;
}

static int testGraph6Reads(void)
{
    for (int failAtEnd = FALSE; failAtEnd <= TRUE; failAtEnd++)
    {
        TestInput input = {"A?\n", 0, failAtEnd};
        strOrFileP container = NULL;
        G6ReadIteratorP reader = NULL;
        graphP graph = NULL;

        if (newInput(&input, &container) != OK)
            return NOTOK;
        graph = gp_New();
        if (graph == NULL)
        {
            gp_ErrorMessage("Unable to create graph for graph6 read-error test.");
            sf_Free(&container);
            return NOTOK;
        }
        CHECK(g6_NewReader(&reader, graph) == OK);
        CHECK(_g6_InitReaderWithStrOrFile(reader, &container) == OK);
        CHECK(container == NULL);
        CHECK(g6_ReadGraph(reader) == OK);
        CHECK(g6_EndReached(reader) == FALSE);
        CHECK(g6_ReadGraph(reader) == (failAtEnd ? NOTOK : OK));
        CHECK(g6_EndReached(reader) == !failAtEnd);
        g6_FreeReader(&reader);
        gp_Free(&graph);

        input.data = "A?";
        input.position = 0;
        if (newInput(&input, &container) != OK)
            return NOTOK;
        graph = gp_New();
        if (graph == NULL)
        {
            gp_ErrorMessage("Unable to create graph for graph6 read-error test.");
            sf_Free(&container);
            return NOTOK;
        }
        CHECK(_g6_ReadGraphFromStrOrFile(graph, &container) == (failAtEnd ? NOTOK : OK));
        CHECK(container == NULL);
        gp_Free(&graph);
    }

    // A read failure while obtaining a multibyte order must not reach a shift.
    for (size_t length = 1; length <= 3; length++)
    {
        char order[] = "~???";
        TestInput input = {order, 0, TRUE};
        strOrFileP container = NULL;
        G6ReadIteratorP reader = NULL;
        graphP graph = NULL;
        order[length] = '\0';
        if (newInput(&input, &container) != OK)
            return NOTOK;
        graph = gp_New();
        if (graph == NULL)
        {
            gp_ErrorMessage("Unable to create graph for graph6 read-error test.");
            sf_Free(&container);
            return NOTOK;
        }
        CHECK(g6_NewReader(&reader, graph) == OK);
        CHECK(_g6_InitReaderWithStrOrFile(reader, &container) == NOTOK);
        g6_FreeReader(&reader);
        sf_Free(&container);
        gp_Free(&graph);
    }
    return OK;
}

int runReadErrorTests(void)
{
    failures = 0;
    checks = 0;
    gp_Message("Starting Read Error Tests");
    if (testLineReads() != OK)
        return NOTOK;
    if (testCharacterReads() != OK)
        return NOTOK;
    if (testGraphReads() != OK)
        return NOTOK;
    if (testGraph6Reads() != OK)
        return NOTOK;
    gp_Message("Read-error regression checks: %d passed, %d failed.", checks - failures, failures);
    return failures == 0 ? OK : NOTOK;
}

#else

int runReadErrorTests(void)
{
    gp_Message("Custom FILE read callbacks are unavailable; skipping read-error fault-injection tests.");
    return OK;
}

#endif
