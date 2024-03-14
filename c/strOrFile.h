/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef STR_OR_FILE_H
#define STR_OR_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef struct {
    FILE *inputFile;
    char *inputStr;
    int inputStrPos;
} strOrFile;

typedef strOrFile *strOrFileP;

strOrFileP sf_New(FILE * inputFile, char *inputStr);

char sf_getc(strOrFileP theStrOrFile);
char sf_ungetc(char theChar, strOrFileP theStrOrFile);
char * sf_fgets(char *str, int count, strOrFileP theStrOrFile);

void sf_Free(strOrFileP *pStrOrFile);

#ifdef __cplusplus
}
#endif

#endif /* STR_OR_FILE_H */
