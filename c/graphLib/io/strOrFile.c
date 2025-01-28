/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../lowLevelUtils/appconst.h"
#include "strOrFile.h"

/********************************************************************
 sf_New()
 Accepts a FILE pointer XOR a string, which are not owned by the container.

 Returns the allocated string-or-file container, or NULL on error.
 ********************************************************************/
// TODO: (#56) add char fileMode to differentiate between read and write modes
strOrFileP sf_New(FILE *pFile, char *theStr)
{
    strOrFileP theStrOrFile;
    if (((pFile == NULL) && (theStr == NULL)) || ((pFile != NULL) && (theStr != NULL)))
        return NULL;

    theStrOrFile = (strOrFileP)calloc(sizeof(strOrFile), 1);
    if (theStrOrFile != NULL)
    {
        if (pFile != NULL)
        {
            theStrOrFile->pFile = pFile;
            theStrOrFile->ungetBuf = sb_New(MAXLINE);
            if (theStrOrFile->ungetBuf == NULL)
            {
                sf_Free(&theStrOrFile);
                return NULL;
            }
        }
        else if ((theStr != NULL))
        {
            theStrOrFile->theStr = theStr;
            theStrOrFile->theStrPos = 0;
        }
    }

    return theStrOrFile;
}

/********************************************************************
 sf_getc()
 If strOrFileP has FILE pointer to input file, calls getc().
 If strOrFileP has input string, returns character at theStrPos and
 increments theStrPos.
 ********************************************************************/
char sf_getc(strOrFileP theStrOrFile)
{
    char theChar = '\0';

    if (theStrOrFile != NULL)
    {
        if (theStrOrFile->pFile != NULL)
        {
            int ungetBufSize = sb_GetSize(theStrOrFile->ungetBuf);
            int ungetBufReadPos = sb_GetReadPos(theStrOrFile->ungetBuf);
            int numCharsInUngetBuf = ungetBufSize - ungetBufReadPos;
            if ((theStrOrFile->ungetBuf != NULL) && (numCharsInUngetBuf > 0))
            {
                // TODO: Feels like we should add more accessor functions
                // to strbuf.c or macros to strbuf.h rather than my directly
                // indexing into the underlying strOrfile's strBuf's char*
                theChar = (theStrOrFile->ungetBuf)->buf[(theStrOrFile->ungetBuf)->readPos++];
            }
            else
                theChar = getc(theStrOrFile->pFile);
        }
        else if (theStrOrFile->theStr != NULL)
        {
            if ((theStrOrFile->theStr + theStrOrFile->theStrPos)[0] == '\0')
                return EOF;

            theChar = theStrOrFile->theStr[theStrOrFile->theStrPos++];
        }
    }

    return theChar;
}

/********************************************************************
 sf_ReadSkipChar()
 Calls sf_getc() and does nothing with the returned character
 ********************************************************************/
void sf_ReadSkipChar(strOrFileP theStrOrFile)
{
    sf_getc(theStrOrFile);
}

/********************************************************************
 sf_ReadInteger()
 Calls sf_getc() and does nothing with the returned character
 ********************************************************************/
int sf_ReadInteger(int *intToRead, strOrFileP theStrOrFile)
{
    int exitCode = OK;

    if (theStrOrFile == NULL || (theStrOrFile->pFile == NULL && theStrOrFile->theStr == NULL))
        return NOTOK;

    strBufP intCandidateStr = sb_New(20);
    if (intCandidateStr == NULL)
        return NOTOK;

    int intCandidate = 0;
    char currChar, nextChar = '\0';
    bool startedReadingInt = FALSE;

    while ((currChar = sf_getc(theStrOrFile)) != EOF)
    {
        if (currChar == '-')
        {
            if (startedReadingInt)
            {
                exitCode = NOTOK;
                break;
            }
            else
            {
                nextChar = sf_getc(theStrOrFile);
                if (nextChar == EOF)
                {
                    exitCode = NOTOK;
                    break;
                }
                else if (!isdigit(nextChar))
                {
                    exitCode = NOTOK;
                    break;
                }
                else
                {
                    if (sf_ungetc(nextChar, theStrOrFile) != nextChar)
                    {
                        exitCode = NOTOK;
                        break;
                    }
                    if (sb_ConcatChar(intCandidateStr, currChar) != OK)
                    {
                        exitCode = NOTOK;
                        break;
                    }
                }
            }
        }
        else if (isdigit(currChar))
        {
            if (sb_ConcatChar(intCandidateStr, currChar) != OK)
            {
                exitCode = NOTOK;
                break;
            }
            startedReadingInt = TRUE;
        }
        else if (isalpha(currChar) || ispunct(currChar))
        {
            if (sf_ungetc(currChar, theStrOrFile) != currChar)
            {
                exitCode = NOTOK;
            }
            break;
        }
        else if (strchr("\n\r", currChar) != NULL || currChar == EOF)
            break;
        else if (isspace(currChar))
            // If we encounter a space after we've already started reading
            // the digits that comprise an int, then we must stop trying
            // to query additional characters
            if (startedReadingInt)
                break;
            else
                continue;
        else
            exitCode = NOTOK;
    }

    if (exitCode == OK)
    {
        if (sscanf(sb_GetReadString(intCandidateStr), " %d ", &intCandidate) != 1)
            exitCode = NOTOK;

        (*intToRead) = intCandidate;
    }

    sb_Free(&intCandidateStr);
    intCandidateStr = NULL;

    return exitCode;
}

/********************************************************************
 sf_ungetc()
 Order of parameters matches stdio ungetc().

 If strOrFileP has FILE pointer to input file, calls ungetc().
 If strOrFileP has input string, decrements theStrPos and returns
 character at theStrPos.

 Like ungetc() in stdio, on success theChar is returned. On failure, EOF
 is returned.
 ********************************************************************/
char sf_ungetc(char theChar, strOrFileP theStrOrFile)
{
    char charToReturn = EOF;

    if (theStrOrFile != NULL)
    {
        if (theStrOrFile->pFile != NULL)
        {
            if (theStrOrFile->ungetBuf == NULL)
                return EOF;
            // FIXME: the ungetBuf is a strBufP, so it dynamically resizes.
            // Do we want to impose an artificial limit on the number of
            // characters we want to be able to unget?
            if (sb_ConcatChar(theStrOrFile->ungetBuf, theChar) == OK)
                charToReturn = theChar;
        }
        // Don't want to ungetc to an index before theStrOrFile->theStr start
        else if (theStrOrFile->theStr != NULL)
        {
            if (theStrOrFile->theStrPos <= 0)
                return EOF;

            // Decrement theStrPos, then replace the character in theStr at that position with theChar
            charToReturn = theStrOrFile->theStr[--(theStrOrFile->theStrPos)] = theChar;
        }
    }

    return charToReturn;
}

/********************************************************************
 sf_ungetcontents()

 If strOrFileP has FILE pointer to input file, appends to the ungetBuf.

 If strOrFileP has input string, TODO: EXPLAIN BEHAVIOUR WHEN STRORFILE CONTAINS STRING

 Returns OK on success and NOTOK on failure.
 ********************************************************************/
int sf_ungetContent(char *contentsToUnget, strOrFileP theStrOrFile)
{
    if (theStrOrFile == NULL || theStrOrFile->pFile == NULL || theStrOrFile->ungetBuf == NULL || theStrOrFile->theStr != NULL)
        return NOTOK;

    // FIXME: Do we need to inspect fileMode to error out if we are ungetContents
    // for file opened in write mode, or if pFile corresponds to stdout/stderr?

    return sb_ConcatString(theStrOrFile->ungetBuf, contentsToUnget);
}

/********************************************************************
 sf_fgets()
 Order of parameters matches stdio fgets().

 First param is the string to populate, second param
 is the max number of characters to read, and third param is the pointer
 to the string-or-file container from which we wish to read count
 characters.

 Like fgets() in stdio, this function doesn't check that enough memory
 is allocated for str to contain (count - 1) characters.

 Like fgets() in stdio, on success the pointer to the buffer is returned.
 On failure, NULL is returned.
 ********************************************************************/
char *sf_fgets(char *str, int count, strOrFileP theStrOrFile)
{
    if (str == NULL || count < 0 || theStrOrFile == NULL)
        return NULL;

    if (theStrOrFile->pFile != NULL)
    {
        int charsToReadFromUngetBuf = 0;
        int charsToReadFromStream = count;
        if ((theStrOrFile->ungetBuf != NULL))
        {
            int ungetBufSize = sb_GetSize(theStrOrFile->ungetBuf);
            int ungetBufReadPos = sb_GetReadPos(theStrOrFile->ungetBuf);
            int numCharsInUngetBuf = ungetBufSize - ungetBufReadPos;
            if (numCharsInUngetBuf > 0)
            {
                int newlineIndex = strcspn(sb_GetReadString(theStrOrFile->ungetBuf), "\n");
                if (newlineIndex == numCharsInUngetBuf)
                {
                    // N.B. No newline found
                    charsToReadFromUngetBuf = (count > numCharsInUngetBuf) ? numCharsInUngetBuf : count;
                    charsToReadFromStream = (count > numCharsInUngetBuf) ? (count - charsToReadFromUngetBuf) : 0;
                }
                else
                {
                    // N.B. Newline found
                    charsToReadFromUngetBuf = (count > newlineIndex) ? (newlineIndex + 1) : count;
                    charsToReadFromStream = 0;
                }

                if (charsToReadFromUngetBuf > 0)
                {
                    strncpy(str, sb_GetReadString(theStrOrFile->ungetBuf), charsToReadFromUngetBuf);
                    sb_SetReadPos(theStrOrFile->ungetBuf, (ungetBufReadPos + charsToReadFromUngetBuf));
                }
            }
        }

        if (charsToReadFromStream > 0)
        {
            // N.B. if fgets() returns NULL (can't read more characters) AND the ungetBuf was empty,
            // then return NULL (error trying to read from empty stream). Otherwise, return str (that
            // was read from ungetBuf)
            if (fgets(str + charsToReadFromUngetBuf, charsToReadFromStream, theStrOrFile->pFile) == NULL)
            {
                if (charsToReadFromUngetBuf == 0)
                    return NULL;
            }
        }

        return str;
    }
    else if (theStrOrFile->theStr != NULL && theStrOrFile->theStr[theStrOrFile->theStrPos] != '\0')
    {
        strncpy(str, theStrOrFile->theStr + theStrOrFile->theStrPos, count);
        str[count - 1] = '\0';
        // Handles \n and \r\n
        char *findDelim = strchr(str, '\n');
        if (findDelim != NULL)
            findDelim[1] = '\0';
        // Handles \r
        else
        {
            findDelim = strchr(str, '\r');
            if (findDelim != NULL)
                findDelim[1] = '\0';
        }

        theStrOrFile->theStrPos += strlen(str);

        return str;
    }

    return NULL;
}

/********************************************************************
 sf_fputs()
 Order of parameters matches stdio fputs().

 First param is the string to append, and the second param is the
 string-or-file container to which we wish to append.

 On success, returns the number of characters written.
 On failure, returns EOF.
 ********************************************************************/
int sf_fputs(char *strToWrite, strOrFileP theStrOrFile)
{
    int outputLen = EOF;

    if (strToWrite == NULL || theStrOrFile == NULL)
        return outputLen;

    int lenOfStringToPuts = strlen(strToWrite);
    // N.B. fputs() will fail and return EOF if pFile doesn't correspond to an output stream
    if (theStrOrFile->pFile != NULL)
        outputLen = fputs(strToWrite, theStrOrFile->pFile);
    else if (theStrOrFile->theStr != NULL)
    {
        // Want to be able to contain the original theStr contents, the strToWrite, and a null terminator (added by strcat)
        char *newStr = realloc(theStrOrFile->theStr, (strlen(theStrOrFile->theStr) + lenOfStringToPuts + 1) * sizeof(char));
        // If realloc failed, pointer returned will be NULL; error will be handled by eventually freeing iterator, which will
        // clean up the old memory for theStrOrFile->theStr
        if (newStr == NULL)
            return outputLen;
        else
            theStrOrFile->theStr = newStr;
        strcat(theStrOrFile->theStr, strToWrite);
        theStrOrFile->theStrPos += lenOfStringToPuts;
        outputLen = lenOfStringToPuts;
    }

    return outputLen;
}

/********************************************************************
 sf_takeTheStr()
 Returns the char * stored in the string-or-file container and NULLs
 out the internal reference so ownership of the memory is transferred
 to the caller.

 The pointer returned will be NULL if the strOrFile contains a FILE *.
 ********************************************************************/
char *sf_takeTheStr(strOrFileP theStrOrFile)
{
    char *theStr = theStrOrFile->theStr;
    theStrOrFile->theStr = NULL;
    return theStr;
}

/********************************************************************
 sf_closeFile()
 If the strOrFile container contains a string, degenerately returns OK.

 If the strOrFile container contains a FILE pointer:
   - if the FILE pointer is one of stdin, stdout, or stderr, calls
   fflush() on the stream and captures the errorCode
   - else, closes pFile and sets the internal pointer to NULL, then
   captures the errorCode from fclose()
 If the errorCode is less than 0, returns NOTOK, otherwise returns OK.
 ********************************************************************/
int sf_closeFile(strOrFileP theStrOrFile)
{
    FILE *pFile = theStrOrFile->pFile;
    theStrOrFile->pFile = NULL;
    if (pFile != NULL)
    {
        int errorCode = 0;

        if (pFile == stdin || pFile == stdout || pFile == stderr)
            errorCode = fflush(pFile);
        else
            errorCode = fclose(pFile);

        if (errorCode < 0)
            return NOTOK;
    }

    if (theStrOrFile->ungetBuf != NULL)
    {
        sb_Free(&(theStrOrFile->ungetBuf));
    }
    theStrOrFile->ungetBuf = NULL;

    return OK;
}

/********************************************************************
 sf_Free()
 Receives a pointer-pointer to a string-or-file container.

 If the strOrFile contains a string which has not yet been "taken"
 using sf_takeTheStr() (i.e. we want inputStr to be freed, and in an
 error state we want to free outputStr), the string is freed, the
 internal pointer is set to NULL, and theStrPos is set to 0.

 If the strOrFile contains a FILE pointer, we call sf_closeFile()
 and set the internal pointer to NULL. Note that unless we are in an
 error state when sf_Free() is called, sf_closeFile should have already
 been called.

 Finally, we use the indirection operator to free the strOrFile
 container and set the pointer to NULL.
 ********************************************************************/
void sf_Free(strOrFileP *pStrOrFile)
{
    if (pStrOrFile != NULL && (*pStrOrFile) != NULL)
    {
        if ((*pStrOrFile)->theStr != NULL)
            free((*pStrOrFile)->theStr);
        (*pStrOrFile)->theStr = NULL;
        (*pStrOrFile)->theStrPos = 0;

        if ((*pStrOrFile)->pFile != NULL)
        {
            // If sf_closeFile() has not previously been called, we must be in an error state
            sf_closeFile((*pStrOrFile));
            // TODO: (#56) if the strOrFile container's FILE pointer corresponds to an output file,
            // i.e. fileMode is 'w', we should try to remove the file since the error state means
            // the contents are invalid
        }
        (*pStrOrFile)->pFile = NULL;

        if ((*pStrOrFile)->ungetBuf != NULL)
        {
            sb_Free(&((*pStrOrFile)->ungetBuf));
        }
        (*pStrOrFile)->ungetBuf = NULL;

        free(*pStrOrFile);
        (*pStrOrFile) = NULL;
    }
}
