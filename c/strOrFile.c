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
strOrFileP sf_New(FILE * inputFile, char *inputStr)
{
strOrFileP theStrOrFile;
	if (((inputFile == NULL) && ((inputStr == NULL) || (strlen(inputStr) == 0)))
		|| ((inputFile != NULL) && ((inputStr != NULL) && (strlen(inputStr) > 0))))
		return NULL;

	theStrOrFile =  (strOrFileP) calloc(sizeof(strOrFile), 1);
	if (theStrOrFile != NULL)
	{
		if (inputFile != NULL)
			theStrOrFile->inputFile = inputFile;
		else if ((inputStr != NULL) && (strlen(inputStr) > 0))
		{
			theStrOrFile->inputStr = inputStr;
			theStrOrFile->inputStrPos = 0;
		}
	}
	
	return theStrOrFile;
}

/********************************************************************
 sf_getc()
 If strOrFileP has FILE pointer to input file, calls getc().
 If strOrFileP has input string, returns character at inputStrPos and
 increments inputStrPos.
 ********************************************************************/
char sf_getc(strOrFileP theStrOrFile)
{
	char theChar = '\0';

	if (theStrOrFile != NULL)
	{
		if (theStrOrFile->inputFile != NULL)
			theChar = getc(theStrOrFile->inputFile);
		else if (theStrOrFile->inputStr != NULL)
		{
			if ((theStrOrFile->inputStr + theStrOrFile->inputStrPos)[0] == '\0')
				return EOF;

			theChar = theStrOrFile->inputStr[theStrOrFile->inputStrPos++];
		} 
	}
	
	return theChar;
}

/********************************************************************
 sf_ungetc()
 Order of parameters matches stdio ungetc().

 If strOrFileP has FILE pointer to input file, calls ungetc().
 If strOrFileP has input string, decrements inputStrPos and returns
 character at inputStrPos.

 Like ungetc() in stdio, on success theChar is returned. On failure, EOF
 is returned. 
 ********************************************************************/
char sf_ungetc(char theChar, strOrFileP theStrOrFile)
{
	char charToReturn = EOF;

	if (theStrOrFile != NULL)
	{
		if (theStrOrFile->inputFile != NULL)
			charToReturn = ungetc(theChar, theStrOrFile->inputFile);
		// Don't want to ungetc to an index before theStrOrFile->inputStr start
		else if (theStrOrFile->inputStr != NULL)
		{
			if (theStrOrFile->inputStrPos <= 0)
				return EOF;

			// Decrement inputStrPos, then replace the character in inputStr at that position with theChar
			charToReturn = theStrOrFile->inputStr[--(theStrOrFile->inputStrPos)] = theChar;
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
	
	if (theStrOrFile->inputFile != NULL)
	{
		return fgets(str, count, theStrOrFile->inputFile);
	}
	else if (theStrOrFile->inputStr != NULL && theStrOrFile->inputStr[theStrOrFile->inputStrPos] != '\0')
	{
		strncpy(str, theStrOrFile->inputStr + theStrOrFile->inputStrPos, count);
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

		theStrOrFile->inputStrPos += strlen(str);

		return str;
	}

	return NULL;
} 

/********************************************************************
 sf_Free()
 Receives a pointer-pointer to a string-or-file container.
 Sets the pointers to inputStr and inputFile to NULL, then uses the 
 indirection operator to free the string-or-file container and set the
 pointer to NULL.
 ********************************************************************/
void sf_Free(strOrFileP *pStrOrFile)
{
	if (pStrOrFile != NULL && (*pStrOrFile) != NULL)
	{
		(*pStrOrFile)->inputStr = NULL;
		(*pStrOrFile)->inputFile = NULL;
		(*pStrOrFile)->inputStrPos = 0;

		free(*pStrOrFile);
		(*pStrOrFile) = NULL;
	}
}

