/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <string.h>
#include <stdlib.h>

#include "strOrFile.h"

/********************************************************************
 sf_New()
 Accepts a FILE pointer XOR a string, which are not owned by the container.

 Returns the allocated string-or-file container, or NULL on error.
 ********************************************************************/
strOrFileP sf_New(FILE * pFile, char *theStr)
{
strOrFileP theStrOrFile;
	if (((pFile == NULL) && (theStr == NULL))
		|| ((pFile != NULL) && ((theStr != NULL) && (strlen(theStr) >= 0))))
		return NULL;

	theStrOrFile =  (strOrFileP) calloc(sizeof(strOrFile), 1);
	if (theStrOrFile != NULL)
	{
		if (pFile != NULL)
			theStrOrFile->pFile = pFile;
		else if ((theStr != NULL) && (strlen(theStr) >= 0))
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
			charToReturn = ungetc(theChar, theStrOrFile->pFile);
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
 sf_fgets()
 Order of parameters matches stdio fgets().

 First param is the string to populate, second param
 is the max number of characters to read, and third param is the pointer to the
 string-or-file container from which we wish to read count characters.

 Like fgets() in stdio, this function doesn't check that enough memory
 is allocated for str to contain (count - 1) characters.

 Like fgets() in stdio, on success the pointer to the buffer is returned.
 On failure, NULL is returned.
 ********************************************************************/
char * sf_fgets(char *str, int count, strOrFileP theStrOrFile)
{
int numCharsRead = -1;

	if (str == NULL || count < 0 || theStrOrFile == NULL)
		return NULL;
	
	if (theStrOrFile->pFile != NULL)
	{
		return fgets(str, count, theStrOrFile->pFile);
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
	if (theStrOrFile->pFile != NULL)
		outputLen = fputs(strToWrite, theStrOrFile->pFile);
	else if (theStrOrFile->theStr != NULL)
	{
		// Want to be able to contain the original theStr contents, the strToWrite, and a null terminator (added by strcat)
		theStrOrFile->theStr = realloc(theStrOrFile->theStr, (strlen(theStrOrFile->theStr) + lenOfStringToPuts + 1) * sizeof(char));
		if (theStrOrFile->theStr == NULL)
			return outputLen;
		strcat(theStrOrFile->theStr, strToWrite);
		theStrOrFile->theStrPos += lenOfStringToPuts;
		outputLen = lenOfStringToPuts;
	}

	return outputLen;
}

/********************************************************************
 sf_getTheStr()
 Returns the char * stored int he string-or-file container, if any
 (i.e. will be NULL if the string-or-file container is meant to contain
 a FILE *).
 ********************************************************************/
char * sf_getTheStr(strOrFileP theStrOrFile)
{
	return theStrOrFile->theStr;
}

/********************************************************************
 sf_Free()
 Receives a pointer-pointer to a string-or-file container.
 Sets the pointers to theStr and pFile to NULL, then uses the 
 indirection operator to free the string-or-file container and set the
 pointer to NULL.
 ********************************************************************/
void sf_Free(strOrFileP *pStrOrFile)
{
	if (pStrOrFile != NULL && (*pStrOrFile) != NULL)
	{
		(*pStrOrFile)->theStr = NULL;
		(*pStrOrFile)->pFile = NULL;
		(*pStrOrFile)->theStrPos = 0;

		free(*pStrOrFile);
		(*pStrOrFile) = NULL;
	}
}
