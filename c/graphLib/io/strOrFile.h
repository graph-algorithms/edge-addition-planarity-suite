/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef STR_OR_FILE_H
#define STR_OR_FILE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#include "../lowLevelUtils/stack.h"
#include "strbuf.h"

    typedef struct
    {
        char fileMode;
        FILE *pFile;
        stackP ungetBuf;
        strBufP theStr;
    } strOrFile;

    typedef strOrFile *strOrFileP;

    strOrFileP sf_New(FILE *pFile, char *theStr);
    int sf_ValidateStrOrFile(strOrFileP);

    char sf_getc(strOrFileP theStrOrFile);
    void sf_ReadSkipChar(strOrFileP theStrOrFile);
    void sf_ReadSkipWhitespace(strOrFileP theStrOrFile);
    int sf_ReadSingleDigit(int *digitToRead, strOrFileP theStrOrFile);
    int sf_ReadInteger(int *intToRead, strOrFileP theStrOrFile);
    int sf_ReadSkipInteger(strOrFileP theStrOrFile);
    int sf_ReadSkipLineRemainder(strOrFileP theStrOrFile);

    char sf_ungetc(char theChar, strOrFileP theStrOrFile);
    int sf_ungets(char *contentsToUnget, strOrFileP theStrOrFile);

    char *sf_fgets(char *str, int count, strOrFileP theStrOrFile);

    int sf_fputs(char *strToWrite, strOrFileP theStrOrFile);

    char *sf_takeTheStr(strOrFileP theStrOrFile);

    int sf_closeFile(strOrFileP theStrOrFile);

    void sf_Free(strOrFileP *pStrOrFile);

#ifdef __cplusplus
}
#endif

#endif /* STR_OR_FILE_H */
