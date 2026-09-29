/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graphIO.h"
#include "strOrFile.h"

#include "../graph.private.h"

#define GRAPHML_MAX_ATTRIBUTES 8

typedef enum
{
    GRAPHML_ENCODING_UTF8,
    GRAPHML_ENCODING_ISO_8859_1
} GraphMLEncoding;

typedef struct
{
    char const *name;
    int required;
    char const *expectedValue;
} GraphMLAttributeSpec;

typedef struct
{
    unsigned int seen;
    char values[GRAPHML_MAX_ATTRIBUTES][MAXLINE + 1];
} GraphMLAttributeValues;

typedef struct
{
    int keyPresent;
    int keyDefault;
} GraphMLKeyState;

typedef struct
{
    int directedDefault;
    int expectedNodes;
    int expectedEdges;
} GraphMLGraphState;

int _ReadGraphMLGraph(graphP theGraph, strOrFileP inputContainer, int readGraphElemOnly);

static int _ReadGraphMLStartsWith(graphP theGraph, strOrFileP inputContainer,
                                  int *pLineNum, char const *expected, int *pMatches);
static int _ReadGraphMLConsume(graphP theGraph, strOrFileP inputContainer,
                               int *pLineNum, char const *expected);
static int _ReadGraphMLSkipWhitespace(graphP theGraph, strOrFileP inputContainer,
                                      int *pLineNum, int requireWhitespace);
static int _ReadGraphMLSkipComment(graphP theGraph, strOrFileP inputContainer,
                                   int *pLineNum);
static int _ReadGraphMLSkipWhitespaceAndComments(graphP theGraph,
                                                  strOrFileP inputContainer,
                                                  int *pLineNum);
static int _ReadGraphMLReadText(graphP theGraph, strOrFileP inputContainer,
                                int *pLineNum, GraphMLEncoding encoding,
                                int delimiter, char *buffer, size_t bufferSize);
static int _ReadGraphMLReadName(graphP theGraph, strOrFileP inputContainer,
                                int *pLineNum, char *buffer, size_t bufferSize);
static int _ReadGraphMLAttribute(graphP theGraph, strOrFileP inputContainer,
                                 int *pLineNum, GraphMLEncoding encoding,
                                 GraphMLAttributeSpec const *specs, size_t specCount,
                                 GraphMLAttributeValues *values);
static int _ReadGraphMLAttributeList(graphP theGraph, strOrFileP inputContainer,
                                     int *pLineNum, GraphMLEncoding encoding,
                                     GraphMLAttributeSpec const *specs, size_t specCount,
                                     GraphMLAttributeValues *values, int xmlDeclaration,
                                     int allowEmptyTag, int *pEmptyTag);
static int _ReadGraphMLTrim(graphP theGraph, strOrFileP inputContainer,
                            int *pLineNum, char *value);
static int _ReadGraphMLValidateID(graphP theGraph, strOrFileP inputContainer,
                                  int *pLineNum, char *value);
static int _ReadGraphMLParsePositiveInteger(graphP theGraph,
                                             strOrFileP inputContainer,
                                             int *pLineNum, char const *value,
                                             int *pParsedValue);
static int _ReadGraphMLParseCanonicalID(graphP theGraph,
                                         strOrFileP inputContainer,
                                         int *pLineNum, char const *value,
                                         int prefix, int *pParsedValue);
static int _ReadGraphMLIsCanonicalID(graphP theGraph,
                                      strOrFileP inputContainer,
                                      int *pLineNum, char const *value);
static int _ReadGraphMLParseBoolean(graphP theGraph, strOrFileP inputContainer,
                                    int *pLineNum, char *value, int *pBoolean);
static int _ReadGraphMLEmptyElementEnd(graphP theGraph,
                                        strOrFileP inputContainer,
                                        int *pLineNum, char const *elementName,
                                        int emptyTag);
static int _ReadGraphMLProlog(graphP theGraph, strOrFileP inputContainer,
                               int *pLineNum, GraphMLEncoding *pEncoding);
static int _ReadGraphMLXMLDeclaration(graphP theGraph,
                                       strOrFileP inputContainer,
                                       int *pLineNum, int hadUTF8BOM,
                                       GraphMLEncoding *pEncoding);
static int _ReadGraphMLStartTag(graphP theGraph, strOrFileP inputContainer,
                                 int *pLineNum, GraphMLEncoding encoding);
static int _ReadGraphMLKeys(graphP theGraph, strOrFileP inputContainer,
                             int *pLineNum, GraphMLEncoding encoding,
                             GraphMLKeyState *keyState);
static int _ReadGraphMLKey(graphP theGraph, strOrFileP inputContainer,
                            int *pLineNum, GraphMLEncoding encoding,
                            GraphMLKeyState *keyState);
static int _ReadGraphMLGraphElement(graphP theGraph,
                                     strOrFileP inputContainer,
                                     int *pLineNum, GraphMLEncoding encoding,
                                     GraphMLKeyState const *keyState);
static int _ReadGraphMLGraphElementStartTag(graphP theGraph,
                                             strOrFileP inputContainer,
                                             int *pLineNum,
                                             GraphMLEncoding encoding,
                                             GraphMLGraphState *graphState);
static int _ReadGraphMLGraphData(graphP theGraph, strOrFileP inputContainer,
                                  int *pLineNum, GraphMLEncoding encoding,
                                  GraphMLKeyState const *keyState);
static int _ReadGraphMLGraphNode(graphP theGraph, strOrFileP inputContainer,
                                  int *pLineNum, GraphMLEncoding encoding,
                                  int expectedIndex);
static int _ReadGraphMLGraphEdge(graphP theGraph, strOrFileP inputContainer,
                                  int *pLineNum, GraphMLEncoding encoding,
                                  GraphMLGraphState const *graphState,
                                  int edgeIndex, int nodeCount);

static int _ReadGraphMLStartsWith(graphP theGraph, strOrFileP inputContainer,
                                  int *pLineNum, char const *expected, int *pMatches)
{
    int chars[MAXLINE + 1];
    size_t charsRead = 0;
    size_t index = 0;
    size_t length = 0;

    (void)theGraph;
    (void)pLineNum;

    if (expected == NULL || pMatches == NULL)
        return NOTOK;

    length = strlen(expected);
    if (length > MAXLINE)
        return NOTOK;

    *pMatches = FALSE;
    for (index = 0; index < length; index++)
    {
        chars[index] = sf_getc(inputContainer);
        if (chars[index] == EOF)
        {
            if (inputContainer->inputErrorFlag)
                return NOTOK;
            break;
        }
        charsRead++;
    }

    index = charsRead;
    while (index > 0)
    {
        index--;
        if (sf_ungetc(chars[index], inputContainer) != chars[index])
            return NOTOK;
    }

    if (length == 0)
        *pMatches = TRUE;
    else
    {
        size_t compareIndex = 0;
        *pMatches = charsRead == length;
        for (compareIndex = 0; compareIndex < length; compareIndex++)
        {
            if (!*pMatches ||
                chars[compareIndex] != (unsigned char)expected[compareIndex])
            {
                *pMatches = FALSE;
                break;
            }
        }
    }

    return OK;
}

static int _ReadGraphMLConsume(graphP theGraph, strOrFileP inputContainer,
                               int *pLineNum, char const *expected)
{
    size_t index = 0;

    (void)theGraph;

    if (expected == NULL)
        return NOTOK;

    for (index = 0; expected[index] != '\0'; index++)
    {
        int currChar = sf_getc(inputContainer);
        if (currChar == EOF || currChar != (unsigned char)expected[index])
        {
            gp_ErrorMessage("Expected '%s' on line %d.", expected, *pLineNum);
            return NOTOK;
        }
        if (currChar == '\n')
            (*pLineNum)++;
    }

    return OK;
}

static int _ReadGraphMLSkipWhitespace(graphP theGraph, strOrFileP inputContainer,
                                      int *pLineNum, int requireWhitespace)
{
    int currChar = EOF;
    int foundWhitespace = FALSE;

    (void)theGraph;

    while ((currChar = sf_getc(inputContainer)) != EOF)
    {
        if (currChar == ' ' || currChar == '\t')
        {
            foundWhitespace = TRUE;
            continue;
        }
        if (currChar == '\n')
        {
            foundWhitespace = TRUE;
            (*pLineNum)++;
            continue;
        }
        if (currChar == '\r')
        {
            int nextChar = EOF;
            foundWhitespace = TRUE;
            nextChar = sf_getc(inputContainer);
            if (nextChar == EOF)
            {
                if (inputContainer->inputErrorFlag)
                    return NOTOK;
                break;
            }
            if (nextChar == '\n')
                (*pLineNum)++;
            else if (sf_ungetc(nextChar, inputContainer) != nextChar)
                    return NOTOK;
            continue;
        }

        if (sf_ungetc(currChar, inputContainer) != currChar)
            return NOTOK;
        break;
    }

    if (inputContainer->inputErrorFlag)
        return NOTOK;

    if (requireWhitespace && !foundWhitespace)
    {
        gp_ErrorMessage("Expected whitespace on line %d.", *pLineNum);
        return NOTOK;
    }

    return OK;
}

static int _ReadGraphMLSkipComment(graphP theGraph, strOrFileP inputContainer,
                                   int *pLineNum)
{
    int previous = 0;
    int previousPrevious = 0;

    if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "<!--") != OK)
        return NOTOK;

    while (1)
    {
        int currChar = sf_getc(inputContainer);
        if (currChar == EOF)
        {
            gp_ErrorMessage("Unterminated GraphML comment on line %d.", *pLineNum);
            return NOTOK;
        }

        if (currChar == '\n')
            (*pLineNum)++;
        else if (currChar == '\r')
        {
            int nextChar = sf_getc(inputContainer);
            if (nextChar == EOF)
            {
                gp_ErrorMessage("Unterminated GraphML comment on line %d.", *pLineNum);
                return NOTOK;
            }
            if (nextChar == '\n')
                (*pLineNum)++;
            else if (sf_ungetc(nextChar, inputContainer) != nextChar)
                    return NOTOK;
        }

        if (previousPrevious == '-' && previous == '-' && currChar == '>')
            return OK;

        if (previous == '-' && currChar == '-')
        {
            int nextChar = sf_getc(inputContainer);
            if (nextChar == EOF)
            {
                gp_ErrorMessage("Unterminated GraphML comment on line %d.", *pLineNum);
                return NOTOK;
            }
            if (nextChar != '>')
            {
                gp_ErrorMessage("Invalid '--' inside GraphML comment on line %d.",
                                *pLineNum);
                return NOTOK;
            }
            return OK;
        }

        previousPrevious = previous;
        previous = currChar;
    }
}

static int _ReadGraphMLSkipWhitespaceAndComments(graphP theGraph,
                                                  strOrFileP inputContainer,
                                                  int *pLineNum)
{
    while (1)
    {
        int matches = FALSE;

        if (_ReadGraphMLSkipWhitespace(theGraph, inputContainer, pLineNum, FALSE) != OK ||
            _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum, "<!--", &matches) != OK)
            return NOTOK;

        if (!matches)
            return OK;

        if (_ReadGraphMLSkipComment(theGraph, inputContainer, pLineNum) != OK)
            return NOTOK;
    }
}

static int _ReadGraphMLAppendCharacter(graphP theGraph, strOrFileP inputContainer,
                                        int *pLineNum, char *buffer,
                                        size_t bufferSize, size_t *pLength,
                                        int character)
{
    (void)theGraph;
    (void)inputContainer;

    if (character == 0 || (character < 0x20 && character != '\t' &&
                           character != '\n' && character != '\r'))
    {
        gp_ErrorMessage("Unsupported XML character on line %d.", *pLineNum);
        return NOTOK;
    }

    if (*pLength + 1 >= bufferSize)
    {
        gp_ErrorMessage("GraphML character data exceeds the supported length on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    buffer[*pLength] = (char)(unsigned char)character;
    (*pLength)++;
    buffer[*pLength] = '\0';
    return OK;
}

static int _ReadGraphMLEntity(graphP theGraph, strOrFileP inputContainer,
                               int *pLineNum, char *buffer, size_t bufferSize,
                               size_t *pLength)
{
    char entity[7];
    size_t index = 0;
    int decoded = 0;

    memset(entity, '\0', sizeof(entity));
    for (index = 0; index < sizeof(entity) - 1; index++)
    {
        int currChar = sf_getc(inputContainer);
        if (currChar == EOF)
        {
            gp_ErrorMessage("Unterminated character entity reference on line %d.",
                            *pLineNum);
            return NOTOK;
        }
        entity[index] = (char)currChar;
        if (currChar == '\n')
            (*pLineNum)++;
        if (currChar == ';')
            break;
    }

    if (index == sizeof(entity) - 1 || entity[index] != ';')
    {
        gp_ErrorMessage("Unsupported character entity reference on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    if (strcmp(entity, "lt;") == 0)
        decoded = '<';
    else if (strcmp(entity, "amp;") == 0)
        decoded = '&';
    else if (strcmp(entity, "gt;") == 0)
        decoded = '>';
    else if (strcmp(entity, "apos;") == 0)
        decoded = '\'';
    else if (strcmp(entity, "quot;") == 0)
        decoded = '"';
    else
    {
        gp_ErrorMessage("Unsupported character entity reference on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    return _ReadGraphMLAppendCharacter(theGraph, inputContainer, pLineNum,
                                        buffer, bufferSize, pLength, decoded);
}

static int _ReadGraphMLReadText(graphP theGraph, strOrFileP inputContainer,
                                int *pLineNum, GraphMLEncoding encoding,
                                int delimiter, char *buffer, size_t bufferSize)
{
    size_t length = 0;

    if (buffer == NULL || bufferSize == 0)
        return NOTOK;
    buffer[0] = '\0';

    while (1)
    {
        int currChar = sf_getc(inputContainer);

        if (currChar == EOF)
        {
            gp_ErrorMessage("Unexpected end of GraphML character data on line %d.",
                            *pLineNum);
            return NOTOK;
        }

        if (currChar == delimiter)
        {
            if (delimiter == '<' && sf_ungetc(currChar, inputContainer) != currChar)
                return NOTOK;
            return OK;
        }

        if (currChar == '<')
        {
            gp_ErrorMessage("Raw '<' found in GraphML attribute value on line %d.",
                            *pLineNum);
            return NOTOK;
        }

        if (currChar == '&')
        {
            if (_ReadGraphMLEntity(theGraph, inputContainer, pLineNum,
                                   buffer, bufferSize, &length) != OK)
                return NOTOK;
            continue;
        }

        if (encoding == GRAPHML_ENCODING_UTF8 && currChar >= 0x80)
        {
            int nextChar = sf_getc(inputContainer);
            int converted = 0;
            if (nextChar == EOF ||
                !((currChar == 0xC2 && nextChar >= 0xA0 && nextChar <= 0xBF) ||
                  (currChar == 0xC3 && nextChar >= 0x80 && nextChar <= 0xBF)))
            {
                gp_ErrorMessage("Unsupported UTF-8 character on line %d.", *pLineNum);
                return NOTOK;
            }
            converted = currChar == 0xC2 ? nextChar : nextChar + 0x40;
            if (_ReadGraphMLAppendCharacter(theGraph, inputContainer, pLineNum,
                                            buffer, bufferSize, &length, converted) != OK)
                return NOTOK;
            continue;
        }

        if (currChar == '\n')
            (*pLineNum)++;

        if (_ReadGraphMLAppendCharacter(theGraph, inputContainer, pLineNum,
                                        buffer, bufferSize, &length, currChar) != OK)
            return NOTOK;
    }
}

static int _ReadGraphMLReadName(graphP theGraph, strOrFileP inputContainer,
                                int *pLineNum, char *buffer, size_t bufferSize)
{
    size_t length = 0;
    int currChar = EOF;

    (void)theGraph;

    if (buffer == NULL || bufferSize == 0)
        return NOTOK;

    while ((currChar = sf_getc(inputContainer)) != EOF)
    {
        int allowed = isalnum(currChar) || currChar == '_' || currChar == '-' ||
                      currChar == '.' || currChar == ':';
        if (!allowed)
        {
            if (sf_ungetc(currChar, inputContainer) != currChar)
                return NOTOK;
            break;
        }
        if (length + 1 >= bufferSize)
        {
            gp_ErrorMessage("GraphML name exceeds the supported length on line %d.",
                            *pLineNum);
            return NOTOK;
        }
        buffer[length++] = (char)currChar;
    }

    if (inputContainer->inputErrorFlag || length == 0)
    {
        gp_ErrorMessage("Expected an XML name on line %d.", *pLineNum);
        return NOTOK;
    }

    buffer[length] = '\0';
    return OK;
}

static int _ReadGraphMLAttribute(graphP theGraph, strOrFileP inputContainer,
                                 int *pLineNum, GraphMLEncoding encoding,
                                 GraphMLAttributeSpec const *specs, size_t specCount,
                                 GraphMLAttributeValues *values)
{
    char name[MAXLINE + 1];
    char value[MAXLINE + 1];
    size_t index = 0;
    int quote = 0;
    int foundIndex = -1;

    if (specs == NULL || values == NULL || specCount > GRAPHML_MAX_ATTRIBUTES)
        return NOTOK;

    memset(name, '\0', sizeof(name));
    memset(value, '\0', sizeof(value));
    if (_ReadGraphMLReadName(theGraph, inputContainer, pLineNum,
                            name, sizeof(name)) != OK)
        return NOTOK;

    for (index = 0; index < specCount; index++)
    {
        if (strcmp(name, specs[index].name) == 0)
        {
            foundIndex = (int)index;
            break;
        }
    }

    if (foundIndex < 0)
    {
        gp_ErrorMessage("Unsupported GraphML attribute '%s' on line %d.",
                        name, *pLineNum);
        return NOTOK;
    }
    if ((values->seen & (1U << (unsigned int)foundIndex)) != 0)
    {
        gp_ErrorMessage("Duplicate GraphML attribute '%s' on line %d.",
                        name, *pLineNum);
        return NOTOK;
    }

    if (_ReadGraphMLSkipWhitespace(theGraph, inputContainer, pLineNum, FALSE) != OK ||
        _ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "=") != OK ||
        _ReadGraphMLSkipWhitespace(theGraph, inputContainer, pLineNum, FALSE) != OK)
        return NOTOK;

    quote = sf_getc(inputContainer);
    if (quote != '\'' && quote != '"')
    {
        gp_ErrorMessage("Expected a quoted GraphML attribute value on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    if (_ReadGraphMLReadText(theGraph, inputContainer, pLineNum, encoding,
                             quote, value, sizeof(value)) != OK)
        return NOTOK;

    if (specs[foundIndex].expectedValue != NULL &&
        strcmp(value, specs[foundIndex].expectedValue) != 0)
    {
        gp_ErrorMessage("Unexpected value for GraphML attribute '%s' on line %d.",
                        name, *pLineNum);
        return NOTOK;
    }

    values->seen |= 1U << (unsigned int)foundIndex;
    memcpy(values->values[foundIndex], value, strlen(value) + 1);
    return OK;
}

static int _ReadGraphMLAttributeList(graphP theGraph, strOrFileP inputContainer,
                                     int *pLineNum, GraphMLEncoding encoding,
                                     GraphMLAttributeSpec const *specs, size_t specCount,
                                     GraphMLAttributeValues *values, int xmlDeclaration,
                                     int allowEmptyTag, int *pEmptyTag)
{
    size_t index = 0;

    if (specs == NULL || values == NULL || pEmptyTag == NULL ||
        specCount > GRAPHML_MAX_ATTRIBUTES)
        return NOTOK;

    memset(values, 0, sizeof(*values));
    *pEmptyTag = FALSE;

    while (1)
    {
        int currChar = sf_getc(inputContainer);
        int matches = FALSE;

        if (currChar == EOF)
        {
            gp_ErrorMessage("Unexpected end of GraphML start tag on line %d.",
                            *pLineNum);
            return NOTOK;
        }
        if (sf_ungetc(currChar, inputContainer) != currChar)
            return NOTOK;

        if (currChar == ' ' || currChar == '\t' || currChar == '\r' || currChar == '\n')
        {
            if (_ReadGraphMLSkipWhitespace(theGraph, inputContainer, pLineNum, TRUE) != OK)
                return NOTOK;
        }
        else
        {
            if (xmlDeclaration)
            {
                if (_ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                                           "?>", &matches) != OK)
                    return NOTOK;
            }
            else if (allowEmptyTag)
            {
                if (_ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                                           "/>", &matches) != OK)
                    return NOTOK;
            }
            if (!matches && !xmlDeclaration)
            {
                if (_ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                                           ">", &matches) != OK)
                    return NOTOK;
            }
            if (!matches)
            {
                gp_ErrorMessage("Expected whitespace before a GraphML attribute on line %d.",
                                *pLineNum);
                return NOTOK;
            }
        }

        if (xmlDeclaration &&
            _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                                   "?>", &matches) == OK && matches)
        {
            if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "?>") != OK)
                return NOTOK;
            break;
        }
        if (!xmlDeclaration && allowEmptyTag &&
            _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                                   "/>", &matches) == OK && matches)
        {
            if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "/>") != OK)
                return NOTOK;
            *pEmptyTag = TRUE;
            break;
        }
        if (!xmlDeclaration &&
            _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                                   ">", &matches) == OK && matches)
        {
            if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, ">") != OK)
                return NOTOK;
            break;
        }

        if (_ReadGraphMLAttribute(theGraph, inputContainer, pLineNum, encoding,
                                  specs, specCount, values) != OK)
            return NOTOK;
    }

    for (index = 0; index < specCount; index++)
    {
        if (specs[index].required &&
            (values->seen & (1U << (unsigned int)index)) == 0)
        {
            gp_ErrorMessage("Required GraphML attribute '%s' is missing on line %d.",
                            specs[index].name, *pLineNum);
            return NOTOK;
        }
    }

    return OK;
}

static int _ReadGraphMLTrim(graphP theGraph, strOrFileP inputContainer,
                            int *pLineNum, char *value)
{
    char *start = value;
    char *end = NULL;
    size_t length = 0;

    (void)theGraph;
    (void)inputContainer;
    (void)pLineNum;

    if (value == NULL)
        return NOTOK;

    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')
        start++;
    end = start + strlen(start);
    while (end > start && (end[-1] == ' ' || end[-1] == '\t' ||
                           end[-1] == '\r' || end[-1] == '\n'))
        end--;
    length = (size_t)(end - start);
    memmove(value, start, length);
    value[length] = '\0';
    return OK;
}

static int _ReadGraphMLValidateID(graphP theGraph, strOrFileP inputContainer,
                                  int *pLineNum, char *value)
{
    size_t index = 0;

    if (_ReadGraphMLTrim(theGraph, inputContainer, pLineNum, value) != OK ||
        value[0] == '\0')
    {
        gp_ErrorMessage("Empty GraphML id on line %d.", *pLineNum);
        return NOTOK;
    }

    for (index = 0; value[index] != '\0'; index++)
    {
        int currChar = (unsigned char)value[index];
        if (!isalnum(currChar) && currChar != '_' && currChar != '-' &&
            currChar != '.' && currChar != ':')
        {
            gp_ErrorMessage("Unsupported character in GraphML id on line %d.",
                            *pLineNum);
            return NOTOK;
        }
    }

    return OK;
}

static int _ReadGraphMLParsePositiveInteger(graphP theGraph,
                                             strOrFileP inputContainer,
                                             int *pLineNum, char const *value,
                                             int *pParsedValue)
{
    long result = 0;
    size_t index = 0;

    (void)theGraph;
    (void)inputContainer;

    if (value == NULL || pParsedValue == NULL || value[0] == '\0')
        return NOTOK;

    for (index = 0; value[index] != '\0'; index++)
    {
        int digit = 0;

        if (!isdigit((unsigned char)value[index]))
        {
            gp_ErrorMessage("Expected a positive integer on line %d.", *pLineNum);
            return NOTOK;
        }
        digit = value[index] - '0';
        if (result > (INT_MAX - digit) / 10)
        {
            gp_ErrorMessage("GraphML integer exceeds the supported range on line %d.",
                            *pLineNum);
            return NOTOK;
        }
        result = result * 10 + digit;
    }

    if (result <= 0)
    {
        gp_ErrorMessage("Expected a positive integer on line %d.", *pLineNum);
        return NOTOK;
    }
    *pParsedValue = (int)result;
    return OK;
}

static int _ReadGraphMLParseCanonicalID(graphP theGraph,
                                         strOrFileP inputContainer,
                                         int *pLineNum, char const *value,
                                         int prefix, int *pParsedValue)
{
    long result = 0;
    size_t index = 1;

    (void)theGraph;
    (void)inputContainer;

    if (value == NULL || pParsedValue == NULL || value[0] != prefix ||
        !isdigit((unsigned char)value[1]) ||
        (value[1] == '0' && value[2] != '\0'))
    {
        gp_ErrorMessage("Expected a canonical GraphML id on line %d.", *pLineNum);
        return NOTOK;
    }

    for (index = 1; value[index] != '\0'; index++)
    {
        int digit = 0;

        if (!isdigit((unsigned char)value[index]))
        {
            gp_ErrorMessage("Expected a canonical GraphML id on line %d.", *pLineNum);
            return NOTOK;
        }
        digit = value[index] - '0';
        if (result > (INT_MAX - digit) / 10)
        {
            gp_ErrorMessage("Canonical GraphML id exceeds the supported range on line %d.",
                            *pLineNum);
            return NOTOK;
        }
        result = result * 10 + digit;
    }

    *pParsedValue = (int)result;
    return OK;
}

static int _ReadGraphMLIsCanonicalID(graphP theGraph,
                                      strOrFileP inputContainer,
                                      int *pLineNum, char const *value)
{
    size_t index = 1;

    (void)theGraph;
    (void)inputContainer;
    (void)pLineNum;

    if (value == NULL || (value[0] != 'n' && value[0] != 'e') ||
        !isdigit((unsigned char)value[1]) ||
        (value[1] == '0' && value[2] != '\0'))
        return FALSE;

    for (index = 1; value[index] != '\0'; index++)
    {
        if (!isdigit((unsigned char)value[index]))
            return FALSE;
    }

    return TRUE;
}

static int _ReadGraphMLParseBoolean(graphP theGraph, strOrFileP inputContainer,
                                    int *pLineNum, char *value, int *pBoolean)
{
    if (value == NULL || pBoolean == NULL ||
        _ReadGraphMLTrim(theGraph, inputContainer, pLineNum, value) != OK)
        return NOTOK;

    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0)
        *pBoolean = TRUE;
    else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0)
        *pBoolean = FALSE;
    else
    {
        gp_ErrorMessage("Expected a GraphML boolean value on line %d.", *pLineNum);
        return NOTOK;
    }
    return OK;
}

static int _ReadGraphMLEmptyElementEnd(graphP theGraph,
                                        strOrFileP inputContainer,
                                        int *pLineNum, char const *elementName,
                                        int emptyTag)
{
    char endTag[MAXLINE + 1];
    int charsWritten = 0;

    if (emptyTag)
        return OK;

    if (_ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, pLineNum) != OK)
        return NOTOK;

    charsWritten = snprintf(endTag, sizeof(endTag), "</%s>", elementName);
    if (charsWritten < 0 || (size_t)charsWritten >= sizeof(endTag))
        return NOTOK;

    return _ReadGraphMLConsume(theGraph, inputContainer, pLineNum, endTag);
}

static int _ReadGraphMLXMLDeclaration(graphP theGraph,
                                       strOrFileP inputContainer,
                                       int *pLineNum, int hadUTF8BOM,
                                       GraphMLEncoding *pEncoding)
{
    static GraphMLAttributeSpec const specs[] = {
        {"version", TRUE, "1.0"},
        {"encoding", FALSE, NULL},
        {"standalone", FALSE, NULL}};
    GraphMLAttributeValues values;
    int emptyTag = FALSE;

    if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "<?xml") != OK ||
        _ReadGraphMLAttributeList(theGraph, inputContainer, pLineNum,
                                  GRAPHML_ENCODING_UTF8, specs,
                                  sizeof(specs) / sizeof(specs[0]),
                                  &values, TRUE, FALSE, &emptyTag) != OK)
        return NOTOK;

    if ((values.seen & (1U << 1)) != 0)
    {
        if (strcmp(values.values[1], "UTF-8") == 0)
            *pEncoding = GRAPHML_ENCODING_UTF8;
        else if (strcmp(values.values[1], "ISO-8859-1") == 0 && !hadUTF8BOM)
            *pEncoding = GRAPHML_ENCODING_ISO_8859_1;
        else
        {
            gp_ErrorMessage("Unsupported or inconsistent XML encoding on line %d.",
                            *pLineNum);
            return NOTOK;
        }
    }
    else
        *pEncoding = GRAPHML_ENCODING_UTF8;

    if ((values.seen & (1U << 2)) != 0 &&
        strcmp(values.values[2], "yes") != 0)
    {
        gp_ErrorMessage("GraphML XML declaration must be standalone on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    return OK;
}

static int _ReadGraphMLProlog(graphP theGraph, strOrFileP inputContainer,
                               int *pLineNum, GraphMLEncoding *pEncoding)
{
    int firstChar = EOF;
    int hadUTF8BOM = FALSE;
    int matches = FALSE;

    firstChar = sf_getc(inputContainer);
    if (firstChar == EOF)
    {
        gp_ErrorMessage("Empty GraphML input.");
        return NOTOK;
    }

    if (firstChar == 0xEF)
    {
        int secondChar = sf_getc(inputContainer);
        int thirdChar = sf_getc(inputContainer);
        if (secondChar != 0xBB || thirdChar != 0xBF)
        {
            gp_ErrorMessage("Invalid UTF-8 byte-order mark on line %d.", *pLineNum);
            return NOTOK;
        }
        hadUTF8BOM = TRUE;
        *pEncoding = GRAPHML_ENCODING_UTF8;
    }
    else
    {
        int secondChar = EOF;
        if (firstChar == 0xFE || firstChar == 0xFF || firstChar == 0)
        {
            gp_ErrorMessage("UTF-16 GraphML input is not supported.");
            return NOTOK;
        }
        if (firstChar == 0x4C)
        {
            gp_ErrorMessage("EBCDIC GraphML input is not supported.");
            return NOTOK;
        }
        if (firstChar == '<')
        {
            secondChar = sf_getc(inputContainer);
            if (secondChar == 0)
            {
                gp_ErrorMessage("UTF-16 GraphML input is not supported.");
                return NOTOK;
            }
            if (secondChar != EOF && sf_ungetc(secondChar, inputContainer) != secondChar)
                return NOTOK;
        }
        if (sf_ungetc(firstChar, inputContainer) != firstChar)
            return NOTOK;
        *pEncoding = GRAPHML_ENCODING_UTF8;
    }

    if (_ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                               "<?xml", &matches) != OK)
        return NOTOK;
    if (matches && _ReadGraphMLXMLDeclaration(theGraph, inputContainer, pLineNum,
                                               hadUTF8BOM, pEncoding) != OK)
        return NOTOK;

    if (_ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, pLineNum) != OK ||
        _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                               "<graphml", &matches) != OK)
        return NOTOK;

    if (!matches)
    {
        gp_ErrorMessage("Unsupported GraphML prolog content on line %d.", *pLineNum);
        return NOTOK;
    }

    return OK;
}

static int _ReadGraphMLStartTag(graphP theGraph, strOrFileP inputContainer,
                                 int *pLineNum, GraphMLEncoding encoding)
{
    static GraphMLAttributeSpec const specs[] = {
        {"xmlns", TRUE, "http://graphml.graphdrawing.org/xmlns"},
        {"xmlns:xsi", FALSE, "http://www.w3.org/2001/XMLSchema-instance"},
        {"xsi:schemaLocation", FALSE, NULL}};
    GraphMLAttributeValues values;
    int emptyTag = FALSE;

    if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "<graphml") != OK ||
        _ReadGraphMLAttributeList(theGraph, inputContainer, pLineNum, encoding,
                                  specs, sizeof(specs) / sizeof(specs[0]),
                                  &values, FALSE, FALSE, &emptyTag) != OK)
        return NOTOK;

    if ((values.seen & (1U << 2)) != 0 && (values.seen & (1U << 1)) == 0)
    {
        gp_ErrorMessage("xsi:schemaLocation requires xmlns:xsi on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    return OK;
}

static int _ReadGraphMLKey(graphP theGraph, strOrFileP inputContainer,
                            int *pLineNum, GraphMLEncoding encoding,
                            GraphMLKeyState *keyState)
{
    static GraphMLAttributeSpec const specs[] = {
        {"id", TRUE, NULL},
        {"for", TRUE, "graph"},
        {"attr.name", TRUE, NULL},
        {"attr.type", TRUE, "boolean"}};
    GraphMLAttributeValues values;
    char defaultValue[MAXLINE + 1];
    int emptyTag = FALSE;
    int matches = FALSE;

    if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "<key") != OK ||
        _ReadGraphMLAttributeList(theGraph, inputContainer, pLineNum, encoding,
                                  specs, sizeof(specs) / sizeof(specs[0]),
                                  &values, FALSE, TRUE, &emptyTag) != OK)
        return NOTOK;

    if (_ReadGraphMLValidateID(theGraph, inputContainer, pLineNum,
                               values.values[0]) != OK ||
        _ReadGraphMLValidateID(theGraph, inputContainer, pLineNum,
                               values.values[2]) != OK ||
        strcmp(values.values[0], "graphflags_zerobasedio") != 0 ||
        strcmp(values.values[2], "graphflags_zerobasedio") != 0)
    {
        gp_ErrorMessage("Unsupported GraphML key on line %d.", *pLineNum);
        return NOTOK;
    }

    keyState->keyPresent = TRUE;
    keyState->keyDefault = FALSE;
    if (emptyTag)
        return OK;

    if (_ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, pLineNum) != OK ||
        _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                               "<default>", &matches) != OK)
        return NOTOK;
    if (!matches)
    {
        gp_ErrorMessage("Expected a default element inside key on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "<default>") != OK ||
        _ReadGraphMLReadText(theGraph, inputContainer, pLineNum, encoding,
                             '<', defaultValue, sizeof(defaultValue)) != OK ||
        _ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "</default>") != OK ||
        _ReadGraphMLParseBoolean(theGraph, inputContainer, pLineNum,
                                 defaultValue, &keyState->keyDefault) != OK ||
        _ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, pLineNum) != OK ||
        _ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "</key>") != OK)
        return NOTOK;

    return OK;
}

static int _ReadGraphMLKeys(graphP theGraph, strOrFileP inputContainer,
                             int *pLineNum, GraphMLEncoding encoding,
                             GraphMLKeyState *keyState)
{
    int matches = FALSE;

    keyState->keyPresent = FALSE;
    keyState->keyDefault = FALSE;
    if (_ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, pLineNum) != OK ||
        _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                               "<key", &matches) != OK)
        return NOTOK;

    if (matches)
    {
        if (_ReadGraphMLKey(theGraph, inputContainer, pLineNum,
                            encoding, keyState) != OK ||
            _ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, pLineNum) != OK ||
            _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                                   "<key", &matches) != OK)
            return NOTOK;
        if (matches)
        {
            gp_ErrorMessage("Only one GraphML key is supported on line %d.", *pLineNum);
            return NOTOK;
        }
    }

    return OK;
}

static int _ReadGraphMLGraphElementStartTag(graphP theGraph,
                                             strOrFileP inputContainer,
                                             int *pLineNum,
                                             GraphMLEncoding encoding,
                                             GraphMLGraphState *graphState)
{
    static GraphMLAttributeSpec const specs[] = {
        {"id", FALSE, NULL},
        {"edgedefault", FALSE, NULL},
        {"parse.nodes", FALSE, NULL},
        {"parse.edges", FALSE, NULL},
        {"parse.nodeids", TRUE, "canonical"},
        {"parse.edgeids", TRUE, "canonical"},
        {"parse.order", TRUE, "nodesfirst"}};
    GraphMLAttributeValues values;
    int emptyTag = FALSE;

    graphState->directedDefault = FALSE;
    graphState->expectedNodes = -1;
    graphState->expectedEdges = -1;

    if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "<graph") != OK ||
        _ReadGraphMLAttributeList(theGraph, inputContainer, pLineNum, encoding,
                                  specs, sizeof(specs) / sizeof(specs[0]),
                                  &values, FALSE, FALSE, &emptyTag) != OK)
        return NOTOK;

    if ((values.seen & 1U) != 0)
    {
        char *id = values.values[0];
        if (_ReadGraphMLValidateID(theGraph, inputContainer, pLineNum, id) != OK ||
            strcmp(id, "graphflags_zerobasedio") == 0)
            return NOTOK;
        if (_ReadGraphMLIsCanonicalID(theGraph, inputContainer, pLineNum, id))
        {
            gp_ErrorMessage("Graph id conflicts with canonical node or edge ids on line %d.",
                            *pLineNum);
            return NOTOK;
        }
    }

    if ((values.seen & (1U << 1)) != 0)
    {
        if (strcmp(values.values[1], "directed") == 0)
            graphState->directedDefault = TRUE;
        else if (strcmp(values.values[1], "undirected") != 0)
        {
            gp_ErrorMessage("Unsupported edgedefault value on line %d.", *pLineNum);
            return NOTOK;
        }
    }

    if ((values.seen & (1U << 2)) != 0 &&
        (_ReadGraphMLParsePositiveInteger(theGraph, inputContainer, pLineNum,
                                           values.values[2],
                                           &graphState->expectedNodes) != OK ||
         graphState->expectedNodes < 2))
    {
        gp_ErrorMessage("GraphML parse.nodes must be at least 2 on line %d.",
                        *pLineNum);
        return NOTOK;
    }
    if ((values.seen & (1U << 3)) != 0 &&
        (_ReadGraphMLParsePositiveInteger(theGraph, inputContainer, pLineNum,
                                           values.values[3],
                                           &graphState->expectedEdges) != OK ||
         graphState->expectedEdges < 1))
    {
        gp_ErrorMessage("GraphML parse.edges must be at least 1 on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    return OK;
}

static int _ReadGraphMLGraphData(graphP theGraph, strOrFileP inputContainer,
                                  int *pLineNum, GraphMLEncoding encoding,
                                  GraphMLKeyState const *keyState)
{
    static GraphMLAttributeSpec const specs[] = {
        {"key", TRUE, "graphflags_zerobasedio"}};
    GraphMLAttributeValues values;
    char dataValue[MAXLINE + 1];
    int emptyTag = FALSE;
    int zeroBased = FALSE;

    if (!keyState->keyPresent)
    {
        gp_ErrorMessage("GraphML data element has no matching key on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "<data") != OK ||
        _ReadGraphMLAttributeList(theGraph, inputContainer, pLineNum, encoding,
                                  specs, sizeof(specs) / sizeof(specs[0]),
                                  &values, FALSE, FALSE, &emptyTag) != OK ||
        _ReadGraphMLReadText(theGraph, inputContainer, pLineNum, encoding,
                             '<', dataValue, sizeof(dataValue)) != OK ||
        _ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "</data>") != OK ||
        _ReadGraphMLParseBoolean(theGraph, inputContainer, pLineNum,
                                 dataValue, &zeroBased) != OK)
        return NOTOK;

    if (zeroBased)
        theGraph->graphFlags |= GRAPHFLAGS_ZEROBASEDIO;
    else
        theGraph->graphFlags &= ~GRAPHFLAGS_ZEROBASEDIO;

    return OK;
}

static int _ReadGraphMLGraphNode(graphP theGraph, strOrFileP inputContainer,
                                  int *pLineNum, GraphMLEncoding encoding,
                                  int expectedIndex)
{
    static GraphMLAttributeSpec const specs[] = {
        {"id", TRUE, NULL}};
    GraphMLAttributeValues values;
    int emptyTag = FALSE;
    int actualIndex = -1;

    if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "<node") != OK ||
        _ReadGraphMLAttributeList(theGraph, inputContainer, pLineNum, encoding,
                                  specs, sizeof(specs) / sizeof(specs[0]),
                                  &values, FALSE, TRUE, &emptyTag) != OK ||
        _ReadGraphMLValidateID(theGraph, inputContainer, pLineNum,
                               values.values[0]) != OK ||
        _ReadGraphMLParseCanonicalID(theGraph, inputContainer, pLineNum,
                                     values.values[0], 'n', &actualIndex) != OK ||
        actualIndex != expectedIndex)
    {
        gp_ErrorMessage("Expected canonical node id n%d on line %d.",
                        expectedIndex, *pLineNum);
        return NOTOK;
    }

    return _ReadGraphMLEmptyElementEnd(theGraph, inputContainer, pLineNum,
                                        "node", emptyTag);
}

static int _ReadGraphMLGraphEdge(graphP theGraph, strOrFileP inputContainer,
                                  int *pLineNum, GraphMLEncoding encoding,
                                  GraphMLGraphState const *graphState,
                                  int edgeIndex, int nodeCount)
{
    static GraphMLAttributeSpec const specs[] = {
        {"id", FALSE, NULL},
        {"source", TRUE, NULL},
        {"target", TRUE, NULL},
        {"directed", FALSE, NULL}};
    GraphMLAttributeValues values;
    int emptyTag = FALSE;
    int actualEdgeIndex = -1;
    int source = -1;
    int target = -1;
    int directed = graphState->directedDefault;
    int vertexOffset = gp_LowerBoundVertexStorage(theGraph);

    if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "<edge") != OK ||
        _ReadGraphMLAttributeList(theGraph, inputContainer, pLineNum, encoding,
                                  specs, sizeof(specs) / sizeof(specs[0]),
                                  &values, FALSE, TRUE, &emptyTag) != OK)
        return NOTOK;

    if ((values.seen & 1U) != 0 &&
        (_ReadGraphMLValidateID(theGraph, inputContainer, pLineNum,
                                values.values[0]) != OK ||
         _ReadGraphMLParseCanonicalID(theGraph, inputContainer, pLineNum,
                                      values.values[0], 'e', &actualEdgeIndex) != OK ||
         actualEdgeIndex != edgeIndex))
    {
        gp_ErrorMessage("Expected canonical edge id e%d on line %d.",
                        edgeIndex, *pLineNum);
        return NOTOK;
    }

    if (_ReadGraphMLParseCanonicalID(theGraph, inputContainer, pLineNum,
                                     values.values[1], 'n', &source) != OK ||
        _ReadGraphMLParseCanonicalID(theGraph, inputContainer, pLineNum,
                                     values.values[2], 'n', &target) != OK ||
        source < 0 || source >= nodeCount || target < 0 || target >= nodeCount)
    {
        gp_ErrorMessage("GraphML edge endpoint is outside the node range on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    if ((values.seen & (1U << 3)) != 0)
    {
        if (strcmp(values.values[3], "true") == 0)
            directed = TRUE;
        else if (strcmp(values.values[3], "false") == 0)
            directed = FALSE;
        else
        {
            gp_ErrorMessage("Expected true or false for directed on line %d.",
                            *pLineNum);
            return NOTOK;
        }
    }

    if (_ReadGraphMLEmptyElementEnd(theGraph, inputContainer, pLineNum,
                                    "edge", emptyTag) != OK ||
        gp_DynamicAddEdge(theGraph, source + vertexOffset, 0,
                          target + vertexOffset, 0) != OK)
    {
        gp_ErrorMessage("Unable to add GraphML edge e%d on line %d.",
                        edgeIndex, *pLineNum);
        return NOTOK;
    }

    if (directed)
    {
        gp_SetDirection(theGraph, gp_GetFirstEdge(theGraph, source + vertexOffset),
                        EDGEFLAG_DIRECTION_OUTONLY);
        // This macro expands to a constant conditional expression by design.
    }

    return OK;
}

static int _ReadGraphMLGraphElement(graphP theGraph,
                                     strOrFileP inputContainer,
                                     int *pLineNum, GraphMLEncoding encoding,
                                     GraphMLKeyState const *keyState)
{
    GraphMLGraphState graphState;
    int matches = FALSE;
    int nodeCount = 0;
    int edgeCount = 0;
    int vertex = NIL;

    if (_ReadGraphMLGraphElementStartTag(theGraph, inputContainer, pLineNum,
                                         encoding, &graphState) != OK)
        return NOTOK;

    if (keyState->keyPresent && keyState->keyDefault)
        theGraph->graphFlags |= GRAPHFLAGS_ZEROBASEDIO;
    else
        theGraph->graphFlags &= ~GRAPHFLAGS_ZEROBASEDIO;

    if (_ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, pLineNum) != OK ||
        _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                               "<data", &matches) != OK)
        return NOTOK;
    if (matches)
    {
        if (_ReadGraphMLGraphData(theGraph, inputContainer, pLineNum,
                                  encoding, keyState) != OK)
            return NOTOK;
        if (_ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, pLineNum) != OK ||
            _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                                   "<data", &matches) != OK)
            return NOTOK;
        if (matches)
        {
            gp_ErrorMessage("Only one graph-level data element is supported on line %d.",
                            *pLineNum);
            return NOTOK;
        }
    }

    if (_ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                               "<node", &matches) != OK)
        return NOTOK;
    while (matches)
    {
        if (_ReadGraphMLGraphNode(theGraph, inputContainer, pLineNum,
                                  encoding, nodeCount) != OK)
            return NOTOK;
        nodeCount++;
        if (_ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, pLineNum) != OK ||
            _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                                   "<node", &matches) != OK)
            return NOTOK;
    }

    if (nodeCount < 2 ||
        (graphState.expectedNodes >= 0 && nodeCount != graphState.expectedNodes))
    {
        gp_ErrorMessage("GraphML node count does not match the declared count on line %d.",
                        *pLineNum);
        return NOTOK;
    }
    if (gp_EnsureVertexCapacity(theGraph, nodeCount) != OK)
    {
        gp_ErrorMessage("Unable to allocate GraphML graph vertices.");
        return NOTOK;
    }
    for (vertex = gp_LowerBoundVertices(theGraph);
         vertex < gp_UpperBoundVertices(theGraph); vertex++)
        gp_SetIndex(theGraph, vertex, vertex);

    if (_ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                               "<edge", &matches) != OK)
        return NOTOK;
    while (matches)
    {
        if (_ReadGraphMLGraphEdge(theGraph, inputContainer, pLineNum,
                                  encoding, &graphState, edgeCount, nodeCount) != OK)
            return NOTOK;
        edgeCount++;
        if (_ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, pLineNum) != OK ||
            _ReadGraphMLStartsWith(theGraph, inputContainer, pLineNum,
                                   "<edge", &matches) != OK)
            return NOTOK;
    }

    if (edgeCount < 1 ||
        (graphState.expectedEdges >= 0 && edgeCount != graphState.expectedEdges))
    {
        gp_ErrorMessage("GraphML edge count does not match the declared count on line %d.",
                        *pLineNum);
        return NOTOK;
    }

    if (_ReadGraphMLConsume(theGraph, inputContainer, pLineNum, "</graph>") != OK)
        return NOTOK;

    return OK;
}

int _ReadGraphMLGraph(graphP theGraph, strOrFileP inputContainer, int readGraphElemOnly)
{
    GraphMLEncoding encoding = GRAPHML_ENCODING_UTF8;
    GraphMLKeyState keyState;
    int lineNum = 1;

    keyState.keyPresent = FALSE;
    keyState.keyDefault = FALSE;

    if (theGraph == NULL || !sf_IsValidStrOrFile(inputContainer))
    {
        gp_ErrorMessage("Invalid parameter supplied to GraphML reader.");
        return NOTOK;
    }

    if (!readGraphElemOnly)
    {
        if (_ReadGraphMLProlog(theGraph, inputContainer, &lineNum, &encoding) != OK ||
            _ReadGraphMLStartTag(theGraph, inputContainer, &lineNum, encoding) != OK ||
            _ReadGraphMLKeys(theGraph, inputContainer, &lineNum,
                             encoding, &keyState) != OK)
            return NOTOK;
    }

    if (_ReadGraphMLSkipWhitespaceAndComments(theGraph, inputContainer, &lineNum) != OK ||
        _ReadGraphMLGraphElement(theGraph, inputContainer, &lineNum,
                                 encoding, &keyState) != OK)
        return NOTOK;

    return OK;
}
