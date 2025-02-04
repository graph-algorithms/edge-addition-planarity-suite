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

 Accepts a FILE pointer XOR a string, which are owned by the container.

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
            theStrOrFile->pFile = pFile;
        else if ((theStr != NULL))
        {
            theStrOrFile->theStr = theStr;
            theStrOrFile->theStrPos = 0;
        }

        theStrOrFile->ungetBuf = sp_New(MAXLINE);
        if (theStrOrFile->ungetBuf == NULL)
        {
            sf_Free(&theStrOrFile);
            return NULL;
        }
    }

    return theStrOrFile;
}

/********************************************************************
 sf_ValidateStrOrFile()

 Ensures that theStrOrFile:
 1. Is not NULL
 2. Has ungetBuf allocated
 3. Both pFile and theStr are not NULL
 4. Both pFile and theStr are not both assigned (since this container
    should only contain one source).

 Returns NOTOK if any of these conditions are not met, otherwise OK.
 ********************************************************************/
int sf_ValidateStrOrFile(strOrFileP theStrOrFile)
{
    if (theStrOrFile == NULL ||
        theStrOrFile->ungetBuf == NULL ||
        (theStrOrFile->pFile == NULL && theStrOrFile->theStr == NULL) ||
        (theStrOrFile->pFile != NULL && theStrOrFile->theStr != NULL))
        return NOTOK;

    return OK;
}

/********************************************************************
 sf_getc()

 If strOrFileP has a non-empty ungetBuf, pop and return the character.
 If the ungetBuf is empty, then we'll read from pFile using getc() OR
 from theStr by fetching the character at theStrPos and incrementing
 theStrPos.
 ********************************************************************/

char sf_getc(strOrFileP theStrOrFile)
{
    char theChar = EOF;

    if (theStrOrFile != NULL)
    {
        if ((theStrOrFile->ungetBuf != NULL) && (sp_GetCurrentSize(theStrOrFile->ungetBuf) > 0))
        {
            int currChar = 0;
            sp_Pop(theStrOrFile->ungetBuf, currChar);
            theChar = (char)currChar;
        }
        else if (theStrOrFile->pFile != NULL)
            theChar = getc(theStrOrFile->pFile);
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
 sf_ReadSkipWhitespace()

 Repeatedly calls sf_getc() to find the next non-space character
 before hitting EOF
 ********************************************************************/

void sf_ReadSkipWhitespace(strOrFileP theStrOrFile)
{
    char currChar = '\0';
    if (theStrOrFile != NULL)
    {
        while ((currChar = sf_getc(theStrOrFile)) != EOF && isspace(currChar))
        {
            continue;
        }
        sf_ungetc(currChar, theStrOrFile);
    }
}

/********************************************************************
 sf_ReadSingleDigit()

 Calls sf_getc() and tests whether the character read corresponds to
 a digit.

 Returns NOTOK if the character returned from sf_getc() is not a digit.
 Assigns the digit read to the int * and returns OK upon success.
 ********************************************************************/

int sf_ReadSingleDigit(int *digitToRead, strOrFileP theStrOrFile)
{
    int candidateDigit = EOF;

    candidateDigit = sf_getc(theStrOrFile);
    if (!isdigit(candidateDigit))
        return NOTOK;

    // N.B. Subtract '0' = 48 to convert the digit
    // char to the corresponding int
    (*digitToRead) = candidateDigit - '0';
    return OK;
}

/********************************************************************
 sf_ReadInteger()

 Repeatedly calls sf_getc() to obtain the characters corresponding to
 an int, then parses that char* using sscanf() to extract the integer.

 Returns OK if successfully extracted the digits of and produced the
 int from theStrOrFile, or NOTOK otherwise.
 ********************************************************************/

int sf_ReadInteger(int *intToRead, strOrFileP theStrOrFile)
{
    int exitCode = OK;

    if (theStrOrFile == NULL ||
        (theStrOrFile->pFile == NULL && theStrOrFile->theStr == NULL))
        return NOTOK;

    char intCandidateStr[MAXCHARSFOR32BITINT];
    memset(intCandidateStr, '\0', MAXCHARSFOR32BITINT);

    int intCandidate = 0;
    char currChar, nextChar = '\0';
    bool startedReadingInt = FALSE;
    int intCandidateIndex = 0;
    while ((currChar = sf_getc(theStrOrFile)) != EOF)
    {
        if (intCandidateIndex > (MAXCHARSFOR32BITINT - 2))
        {
            exitCode = NOTOK;
            break;
        }

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
                if (!isdigit(nextChar))
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
                    intCandidateStr[intCandidateIndex++] = currChar;
                }
            }
        }
        else if (isdigit(currChar))
        {
            intCandidateStr[intCandidateIndex++] = currChar;
            startedReadingInt = TRUE;
        }
        else if (currChar != EOF)
        {
            if (sf_ungetc(currChar, theStrOrFile) != currChar)
            {
                exitCode = NOTOK;
            }
            break;
        }
    }

    if (exitCode == OK)
    {
        if (sscanf(intCandidateStr, "%d", &intCandidate) != 1)
            exitCode = NOTOK;

        (*intToRead) = intCandidate;
    }

    return exitCode;
}

/********************************************************************
 sf_ReadSkipInteger()

 Calls sf_ReadInteger() and discards the result.
 ********************************************************************/

int sf_ReadSkipInteger(strOrFileP theStrOrFile)
{
    int temp = 0;
    if (sf_ReadInteger(&temp, theStrOrFile) != OK)
        return NOTOK;
    return OK;
}

/********************************************************************
 sf_ReadSkipLineRemainder()

 Calls sf_fgets() and discards the result.
 ********************************************************************/

int sf_ReadSkipLineRemainder(strOrFileP theStrOrFile)
{
    char lineRemainderToSkip[255];
    memset(lineRemainderToSkip, '\0', 255);

    if (sf_fgets(lineRemainderToSkip, 254, theStrOrFile) == NULL)
        return NOTOK;

    return OK;
}

/********************************************************************
 sf_ungetc()

 Order of parameters matches stdio ungetc().

 For both the case where the strOrFile contains a FILE * and the case
 where it contains a char *, we unget to the ungetBuf; this ungetBuf
 is consumed first when we sf_getc(), sf_fgets(), etc.

 Like ungetc() in stdio, on success theChar is returned. On failure,
 EOF is returned.
 ********************************************************************/

char sf_ungetc(char theChar, strOrFileP theStrOrFile)
{
    if (theChar == EOF ||
        theStrOrFile == NULL ||
        theStrOrFile->ungetBuf == NULL ||
        sp_GetCurrentSize(theStrOrFile->ungetBuf) >= sp_GetCapacity(theStrOrFile->ungetBuf))
        return EOF;

    // FIXME: If you use the speed macro, I don't know what value is returned... If you
    // don't use the speed macros, then OK/NOTOK are returned; does this mean there's no
    // real way to check the success of sp_Push()? No other occurrence seems to check success
    sp_Push(theStrOrFile->ungetBuf, theChar);
    return theChar;
}

/********************************************************************
 sf_ungets()

 Pushes characters of strToUnget in reverse order to the ungetBuf so
 that they can be fetched from the ungetBuf in the order of the
 original string.

 Returns OK on success and NOTOK on failure.
 ********************************************************************/

int sf_ungets(char *strToUnget, strOrFileP theStrOrFile)
{
    if (sf_ValidateStrOrFile(theStrOrFile) != OK ||
        strlen(strToUnget) > (sp_GetCapacity(theStrOrFile->ungetBuf) - sp_GetCurrentSize(theStrOrFile->ungetBuf)))
        return NOTOK;

    // FIXME: Do we need to inspect fileMode to error out if we are
    // trying to unget contents "to" an output stream

    for (int i = (strlen(strToUnget) - 1); i >= 0; i--)
    {
        // FIXME: when speed macros aren't used, sp_Push() returns OK/NOTOK,
        // but I don't know if there's a return value; if it is, it probably returns
        // the value that was assigned to the specific index, so then comparing against
        // OK/NOTOK may erroneously fail
        sp_Push(theStrOrFile->ungetBuf, strToUnget[i]);
    }

    return OK;
}

/********************************************************************
 sf_fgets()

 Order of parameters matches stdio fgets().

 First param is the string to populate (assumes allocated (count + 1)
 bytes), second param is the max number of characters to read, and
 third param is the pointer to the string-or-file container from which
 we wish to read count characters (or up to and including \n).

 Like fgets() in stdio, this function doesn't check that enough memory
 is allocated for str to contain count characters plus \0.

 Like fgets() in stdio, on success the pointer to the buffer is returned.
 On failure, NULL is returned.
 ********************************************************************/

char *sf_fgets(char *str, int count, strOrFileP theStrOrFile)
{
    if (str == NULL || count < 0 || sf_ValidateStrOrFile(theStrOrFile) != OK)
        return NULL;

    int charsToReadFromUngetBuf = 0;
    int charsToReadFromStrOrFile = count;
    if (theStrOrFile->ungetBuf != NULL)
    {
        int numCharsInUngetBuf = sp_GetCurrentSize(theStrOrFile->ungetBuf);
        if (numCharsInUngetBuf > 0)
        {
            charsToReadFromUngetBuf = (count > numCharsInUngetBuf) ? numCharsInUngetBuf : count;
            char currChar = '\0';
            bool encounteredNewline = FALSE;
            for (int i = 0; i < charsToReadFromUngetBuf; i++)
            {
                currChar = sf_getc(theStrOrFile);
                if (currChar == EOF)
                    return NULL;
                str[i] = currChar;
                str[i + 1] = '\0';
                // N.B. fgets() includes the \n in the string returned, and
                // no further characters shall be read
                if (currChar == '\n')
                {
                    encounteredNewline = TRUE;
                    break;
                }
            }
            // N.B. If we broke out of the loop early due to \n, do not read
            // any further characters from stream
            charsToReadFromStrOrFile = (encounteredNewline) ? 0 : ((count > numCharsInUngetBuf) ? (count - charsToReadFromUngetBuf) : 0);
        }
    }

    if (charsToReadFromStrOrFile > 0)
    {
        if (theStrOrFile->pFile != NULL)
        {
            // N.B. if fgets() returns NULL (can't read more characters) AND the ungetBuf was empty,
            // then return NULL (error trying to read from empty stream). Otherwise, return str (that
            // was read from ungetBuf)
            if (fgets(str + charsToReadFromUngetBuf, charsToReadFromStrOrFile, theStrOrFile->pFile) == NULL)
            {
                if (charsToReadFromUngetBuf == 0)
                    return NULL;
            }
        }
        else if (theStrOrFile->theStr != NULL)
        {
            if (strlen(theStrOrFile->theStr + theStrOrFile->theStrPos) == 0 && charsToReadFromUngetBuf == 0)
                return NULL;
            strncpy(str + charsToReadFromUngetBuf, theStrOrFile->theStr + theStrOrFile->theStrPos, charsToReadFromStrOrFile);
            str[count] = '\0';
            // Handles \n and \r\n; sets \0 at index *after* \n
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

            theStrOrFile->theStrPos += strlen(str + charsToReadFromUngetBuf);
        }
    }

    return str;
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

    if (strToWrite == NULL || sf_ValidateStrOrFile(theStrOrFile) != OK)
        return EOF;

    int lenOfStringToPuts = strlen(strToWrite);
    // N.B. fputs() will fail and return EOF if pFile doesn't correspond
    // to an output stream
    if (theStrOrFile->pFile != NULL)
        outputLen = fputs(strToWrite, theStrOrFile->pFile);
    else if (theStrOrFile->theStr != NULL)
    {
        // Want to be able to contain the original theStr contents, the
        // strToWrite, and a null terminator (added by strcat)
        char *newStr = realloc(
            theStrOrFile->theStr,
            (strlen(theStrOrFile->theStr) + lenOfStringToPuts + 1) * sizeof(char));
        // If realloc failed, pointer returned will be NULL; error will
        // be handled by eventually freeing iterator, which will
        // clean up the old memory for theStrOrFile->theStr
        if (newStr == NULL)
            return EOF;
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
        sp_Free(&(theStrOrFile->ungetBuf));
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

        // TODO: (#56) if the strOrFile container's FILE pointer
        // corresponds to an output file, i.e. fileMode is 'w',
        // we should try to remove the file since the error state
        // means the contents are invalid
        if ((*pStrOrFile)->pFile != NULL)
            sf_closeFile((*pStrOrFile));
        (*pStrOrFile)->pFile = NULL;

        if ((*pStrOrFile)->ungetBuf != NULL)
        {
            sp_Free(&((*pStrOrFile)->ungetBuf));
        }
        (*pStrOrFile)->ungetBuf = NULL;

        free(*pStrOrFile);
        (*pStrOrFile) = NULL;
    }
}
