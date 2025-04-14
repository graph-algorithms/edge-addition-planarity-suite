#ifndef PLANARITY_H
#define PLANARITY_H

/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifdef __cplusplus
extern "C"
{
#endif

#include "../graphLib/graphLib.h"

    char const*GetProjectTitle(void);
    char const*GetAlgorithmFlags(void);
    char const*GetAlgorithmSpecifiers(void);
    char const*GetAlgorithmChoices(void);
    char const*GetSupportedOutputChoices(void);
    char const*GetSupportedOutputFormats(void);

    int helpMessage(char *param);

    /* Functions that call the Graph Library */
    int SpecificGraph(
        char command,
        char const*infileName, char*outfileName, char*outfile2Name,
        char *inputStr, char **pOutputStr, char **pOutput2Str);
    int RandomGraph(char command, int extraEdges, int numVertices, char *outfileName, char *outfile2Name);
    int RandomGraphs(char command, int NumGraphs, int SizeOfGraphs, char *outfileName);
    int TransformGraph(char const*commandString, char const*infileName, char *inputStr, int *outputBase, char const*outfileName, char **outputStr);
    int TestAllGraphs(char *commandString, char *infileName, char *outfileName, char **outputStr);

    /* Command line, Menu, and Configuration */
    int menu(void);
    int commandLine(int argc, char *argv[]);
    int legacyCommandLine(int argc, char *argv[]);

    extern char Mode,
        OrigOut,
        OrigOutFormat,
        EmbeddableOut,
        ObstructedOut,
        AdjListsForEmbeddingsOut;

    void Reconfigure(void);

    /* Low-level Utilities */
    void FlushConsole(FILE *f);
    void Prompt(char const*message);

    void SaveAsciiGraph(graphP theGraph, char *filename);

    char *ReadTextFileIntoString(char const*infileName);
    int TextFileMatchesString(char const*theFilename, char const*theString);
    int TextFilesEqual(char *file1Name, char *file2Name);
    int BinaryFilesEqual(char *file1Name, char *file2Name);

    int GetEmbedFlags(char command);
    char const*GetAlgorithmName(char command);
    char const*GetTransformationName(char command);
    char const*GetBaseName(int baseFlag);
    void AttachAlgorithm(graphP theGraph, char command);

    char *ConstructInputFilename(char const*infileName);
    char *ConstructPrimaryOutputFilename(char const*infileName, char const*outfileName, char command);
    int ConstructTransformationExpectedResultFilename(char const*infileName, char **outfileName, char command, int actualOrExpectedFlag);
    void WriteAlgorithmResults(graphP theGraph, int Result, char command, platform_time start, platform_time end, char const*infileName);

#ifdef __cplusplus
}
#endif

#endif
