#ifndef PLANARITY_H
#define PLANARITY_H

/*
Copyright (c) 1997-2022, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "../graphLib/graph.h"
#include "../graphLib/lowLevelUtils/platformTime.h"

#include "../graphLib/homeomorphSearch/graphK23Search.h"
#include "../graphLib/homeomorphSearch/graphK33Search.h"
#include "../graphLib/homeomorphSearch/graphK4Search.h"
#include "../graphLib/planarityRelated/graphDrawPlanar.h"

     char *GetProjectTitle(void);
     char *GetAlgorithmFlags(void);
     char *GetAlgorithmSpecifiers(void);
     char *GetAlgorithmChoices(void);
     char *GetSupportedOutputChoices(void);
     char *GetSupportedOutputFormats(void);

     int helpMessage(char *param);

     /* Functions that call the Graph Library */
     int SpecificGraph(
         char command,
         char *infileName, char *outfileName, char *outfile2Name,
         char *inputStr, char **pOutputStr, char **pOutput2Str);
     int RandomGraph(char command, int extraEdges, int numVertices, char *outfileName, char *outfile2Name);
     int RandomGraphs(char command, int NumGraphs, int SizeOfGraphs);
     int TestGraphFunctionality(char *commandString, char *infileName, char *inputStr, int *outputBase, char *outfileName, char **outputStr);

     /* Command line, Menu, and Configuration */
     int menu();
     int commandLine(int argc, char *argv[]);
     int legacyCommandLine(int argc, char *argv[]);

     extern char Mode,
         OrigOut,
         EmbeddableOut,
         ObstructedOut,
         AdjListsForEmbeddingsOut;

     void Reconfigure();

     /* Low-level Utilities */
     void FlushConsole(FILE *f);
     void Prompt(char *message);

     void SaveAsciiGraph(graphP theGraph, char *filename);

     char *ReadTextFileIntoString(char *infileName);
     int TextFileMatchesString(char *theFilename, char *theString);
     int TextFilesEqual(char *file1Name, char *file2Name);
     int BinaryFilesEqual(char *file1Name, char *file2Name);

     int GetEmbedFlags(char command);
     char *GetAlgorithmName(char command);
     char *GetTransformationName(char command);
     char *GetBaseName(int baseFlag);
     void AttachAlgorithm(graphP theGraph, char command);

     char *ConstructInputFilename(char *infileName);
     char *ConstructPrimaryOutputFilename(char *infileName, char *outfileName, char command);
     int ConstructTransformationExpectedResultFilename(char *infileName, char **outfileName, char command, int actualOrExpectedFlag);
     void WriteAlgorithmResults(graphP theGraph, int Result, char command, platform_time start, platform_time end, char *infileName);

#ifdef __cplusplus
}
#endif

#endif
