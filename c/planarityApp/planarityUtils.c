/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"

/****************************************************************************
 Configuration
 ****************************************************************************/

char Mode = 'r',
     OrigOut = 'n',
     OrigOutFormat = 'a',
     EmbeddableOut = 'n',
     ObstructedOut = 'n',
     AdjListsForEmbeddingsOut = 'n';

int Reconfigure(void)
{
    int Result = OK;

    char lineBuff[MAXLINE + 1];

    memset(lineBuff, '\0', (MAXLINE + 1));

    while (1)
    {
        Prompt("\nDo you want to \n"
               "  Randomly generate graphs (r),\n"
               "  Specify a graph (s),\n"
               "  Randomly generate a maximal planar graph (m), or\n"
               "  Randomly generate a non-planar graph (n)?\n\t");

        if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
        {
            ErrorMessage("Unable to fetch reconfigure choice from stdin.\n");
            Result = NOTOK;
            break;
        }

        if (strlen(lineBuff) != 1 ||
            sscanf(lineBuff, " %c", &Mode) != 1 ||
            !strchr("rsmn", tolower(Mode)))
            ErrorMessage("Invalid choice for Mode.\n");
        else
        {
            Mode = (char)tolower(Mode);
            break;
        }
    }

    if (Result == OK && Mode == 'r')
    {
        Message("\nNOTE: The directories for the graphs you want must exist.\n\n");

        while (1)
        {
            Prompt("Do you want original graphs in directory 'random'? (y/n) ");
            if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
            {
                ErrorMessage("Unable to fetch choice from stdin.\n");
                Result = NOTOK;
                break;
            }

            if (strlen(lineBuff) != 1 ||
                sscanf(lineBuff, " %c", &OrigOut) != 1 ||
                !strchr(YESNOCHOICECHARS, OrigOut))
                ErrorMessage("Invalid choice.\n");
            else
            {
                OrigOut = (char)tolower(OrigOut);
                break;
            }
        }

        if (Result == OK && OrigOut == 'y')
        {
            while (1)
            {
                Prompt("Do you want to output generated graphs to Adjacency List (last 10 only) or to G6 (all)? (a/g) ");
                if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
                {
                    ErrorMessage("Unable to fetch choice from stdin.\n");
                    Result = NOTOK;
                    break;
                }

                if (strlen(lineBuff) != 1 ||
                    sscanf(lineBuff, " %c", &OrigOutFormat) != 1 ||
                    !strchr("aAgG", OrigOutFormat))
                    ErrorMessage("Invalid choice.\n");
                else
                {
                    OrigOutFormat = (char)tolower(OrigOutFormat);
                    break;
                }
            }
        }

        if (Result == OK)
        {
            while (1)
            {
                Prompt("Do you want adj. matrix of embeddable graphs in directory 'embedded' (last 10 max))? (y/n) ");
                if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
                {
                    ErrorMessage("Unable to fetch choice from stdin.\n");
                    Result = NOTOK;
                    break;
                }
                if (strlen(lineBuff) != 1 ||
                    sscanf(lineBuff, " %c", &EmbeddableOut) != 1 ||
                    !strchr(YESNOCHOICECHARS, EmbeddableOut))
                    ErrorMessage("Invalid choice.\n");
                else
                {
                    EmbeddableOut = (char)tolower(EmbeddableOut);
                    break;
                }
            }
        }

        if (Result == OK)
        {
            while (1)
            {
                Prompt("Do you want adj. matrix of obstructed graphs in directory 'obstructed' (last 10 max)? (y/n) ");
                if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
                {
                    ErrorMessage("Unable to fetch choice from stdin.\n");
                    Result = NOTOK;
                    break;
                }

                if (strlen(lineBuff) != 1 ||
                    sscanf(lineBuff, " %c", &ObstructedOut) != 1 ||
                    !strchr(YESNOCHOICECHARS, ObstructedOut))
                    ErrorMessage("Invalid choice.\n");
                else
                {
                    ObstructedOut = (char)tolower(ObstructedOut);
                    break;
                }
            }
        }

        if (Result == OK)
        {
            while (1)
            {
                Prompt("Do you want adjacency list format of embeddings in directory 'adjlist' (last 10 max)? (y/n) ");
                if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
                {
                    ErrorMessage("Unable to fetch choice from stdin.\n");
                    Result = NOTOK;
                    break;
                }

                if (strlen(lineBuff) != 1 ||
                    sscanf(lineBuff, " %c", &AdjListsForEmbeddingsOut) != 1 ||
                    !strchr(YESNOCHOICECHARS, AdjListsForEmbeddingsOut))
                    ErrorMessage("Invalid choice.\n");
                else
                {
                    AdjListsForEmbeddingsOut = (char)tolower(AdjListsForEmbeddingsOut);
                    break;
                }
            }
        }
    }

    FlushConsole(stdout);

    return Result;
}

int GetLineFromStdin(char *lineBuff, int lineBuffSize)
{
    if (lineBuff == NULL)
    {
        ErrorMessage("Line buffer to populate is NULL.\n");
        return NOTOK;
    }

    memset(lineBuff, '\0', lineBuffSize);

    if (fgets(lineBuff, lineBuffSize, stdin) == NULL && ferror(stdin))
    {
        ErrorMessage("Call to fgets() from stdin failed.\n");
        return NOTOK;
    }

    // From https://stackoverflow.com/a/28462221, strcspn finds the index of the
    // first char in charset; this way, I replace the char at that index with
    // the null-terminator
    // N.B. See https://cplusplus.com/reference/cstring/strcspn/ : if no chars
    // in str2 appear in str1, will return strlen(str1), so this line is a no-op
    lineBuff[strcspn(lineBuff, "\n\r")] = '\0'; // works for LF, CR, CRLF, LFCR, ...

    return OK;
}

void FlushConsole(FILE *f)
{
    // N.B. fflush(stdin) constitutes undefined behaviour; see:
    // https://c-faq.com/stdio/stdinflush.html
    if (f == stdin)
        return;
    fflush(f);
}

void Prompt(char const *message)
{
    Message(message);
    FlushConsole(stdout);
}

/****************************************************************************
 ****************************************************************************/

void SaveAsciiGraph(graphP theGraph, char *filename)
{
    int e, EsizeOccupied, vertexLabelFix;
    FILE *outfile = fopen(filename, WRITETEXT);

    // The filename may specify a directory that doesn't exist
    if (outfile == NULL)
    {
        char messageContents[MAXLINE + 1];
        char const *messageFormat = "Failed to write to \"%.*s\"\nMake the directory if not present\n";
        int charsAvailForStrToInclude = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        sprintf(messageContents, messageFormat, charsAvailForStrToInclude, filename);
#pragma GCC diagnostic pop
        ErrorMessage(messageContents);
        return;
    }

    // If filename includes path elements, remove them before writing the file's name to the file
    if (strrchr(filename, FILE_DELIMITER))
        filename = strrchr(filename, FILE_DELIMITER) + 1;

    fprintf(outfile, "%s\n", filename);

    // This edge list file format uses 1-based vertex numbering, and the current code
    // internally uses 1-based indexing by default, so this vertex label 'fix' adds zero
    // But earlier code used 0-based indexing and added one on output, so we replicate
    // that behavior in case the current code has been compiled with zero-based indexing.
    vertexLabelFix = 1 - gp_GetFirstVertex(theGraph);

    // Iterate over the edges of the graph
    EsizeOccupied = gp_EdgeInUseIndexBound(theGraph);
    for (e = gp_GetFirstEdge(theGraph); e < EsizeOccupied; e += 2)
    {
        // Only output edges that haven't been deleted (i.e. skip the edge holes)
        if (gp_EdgeInUse(theGraph, e))
        {
            fprintf(outfile, "%d %d\n",
                    gp_GetNeighbor(theGraph, e) + vertexLabelFix,
                    gp_GetNeighbor(theGraph, e + 1) + vertexLabelFix);
        }
    }

    // Since vertex numbers are at least 1, this indicates the end of the edge list
    fprintf(outfile, "0 0\n");

    fclose(outfile);
}

/****************************************************************************
 ReadTextFileIntoString()
 Reads the file content indicated by infileName using a single fread(), and
 returns the result in an allocated string. The caller needs to free() the
 returned string when done with it.
 Returns NULL on error, or an allocated string containing the file content.
 ****************************************************************************/

char *ReadTextFileIntoString(char const *infileName)
{
    FILE *infile = NULL;
    char *inputString = NULL;

    if ((infile = fopen(infileName, "r")) == NULL)
        ErrorMessage("fopen() failed.\n");
    else
    {
        long filePos = ftell(infile);
        long fileSize;

        fseek(infile, 0, SEEK_END);
        fileSize = ftell(infile);
        fseek(infile, filePos, SEEK_SET);

        if ((inputString = (char *)malloc((fileSize + 1) * sizeof(char))) != NULL)
        {
            long bytesRead = fread((void *)inputString, 1, fileSize, infile);
            inputString[bytesRead] = '\0';
        }

        fclose(infile);
    }

    return inputString;
}

/****************************************************************************
 * TextFileMatchesString()
 *
 * Compares the text file content from the file named 'theFilename' with
 * the content of 'theString'.
 *
 * Textual equality is measured as content equality except for suppressing
 * differences between CRLF and LF-only line delimiters.
 *
 * Returns TRUE if the contents are textually equal, FALSE otherwise
 ****************************************************************************/

int TextFileMatchesString(char const *theFilename, char const *theString)
{
    int Result = TRUE;

    FILE *infile = NULL;

    if (theFilename != NULL)
        infile = fopen(theFilename, "r");

    if (infile == NULL || theString == NULL)
        Result = FALSE;
    else
    {
        int c1 = 0, c2 = 0, stringIndex = 0;

        // Read the input file to the end
        while ((c1 = fgetc(infile)) != EOF)
        {
            // Want to suppress distinction between lines ending with CRLF versus LF
            // by looking at only LF characters in the file
            if (c1 == '\r')
                continue;

            // Since c1 now has a non-CR, non-EOF from the input file,  we now also
            // get a character from the string, except ignoring CRs again
            while ((c2 = (int)theString[stringIndex++]) == '\r')
                ;

            // If c1 doesn't equal c2 (whether c2 is a null terminator or a different character)
            // then the file content doesn't match the string
            if (c1 != c2)
            {
                Result = FALSE;
                break;
            }
        }

        // If the outer while loop got to the end of the file
        if (c1 == EOF)
        {
            // Then get another character from the string, once again suppressing CRs, and then...
            while ((c2 = (int)theString[stringIndex++]) == '\r')
                ;
            // Test whether or not the second file also ends, same as the first.
            if (c2 != '\0')
                Result = FALSE;
        }
    }

    if (infile != NULL)
        fclose(infile);

    return Result;
}

/****************************************************************************
 ****************************************************************************/

int TextFilesEqual(char *file1Name, char *file2Name)
{
    int Result = TRUE;

    FILE *infile1 = NULL, *infile2 = NULL;

    infile1 = fopen(file1Name, "r");
    infile2 = fopen(file2Name, "r");

    if (infile1 == NULL || infile2 == NULL)
        Result = FALSE;
    else
    {
        int c1 = 0, c2 = 0;

        // Read the first file to the end
        while ((c1 = fgetc(infile1)) != EOF)
        {
            // Want to suppress distinction between lines ending with CRLF versus LF
            if (c1 == '\r')
                continue;

            // Get a char from the second file, except suppress CR again
            while ((c2 = fgetc(infile2)) == '\r')
                ;

            // If we got a char from the first file, but not from the second
            // then the second file is shorter, so files are not equal
            if (c2 == EOF)
            {
                Result = FALSE;
                break;
            }

            // If we got a char from second file, but not equal to char from
            // first file, then files are not equal
            if (c1 != c2)
            {
                Result = FALSE;
                break;
            }
        }

        // If we got to the end of the first file without breaking the loop...
        if (c1 == EOF)
        {
            // Then, once again, suppress CRs first, and then...
            while ((c2 = fgetc(infile2)) == '\r')
                ;
            // Test whether or not the second file also ends, same as the first.
            if (fgetc(infile2) != EOF)
                Result = FALSE;
        }
    }

    if (infile1 != NULL)
        fclose(infile1);

    if (infile2 != NULL)
        fclose(infile2);

    return Result;
}

/****************************************************************************
 ****************************************************************************/

int BinaryFilesEqual(char *file1Name, char *file2Name)
{
    int Result = TRUE;

    FILE *infile1 = NULL, *infile2 = NULL;

    infile1 = fopen(file1Name, "r");
    infile2 = fopen(file2Name, "r");

    if (infile1 == NULL || infile2 == NULL)
        Result = FALSE;
    else
    {
        int c1 = 0, c2 = 0;

        // Read the first file to the end
        while ((c1 = fgetc(infile1)) != EOF)
        {
            // If we got a char from the first file, but not from the second
            // then the second file is shorter, so files are not equal
            if ((c2 = fgetc(infile2)) == EOF)
            {
                Result = FALSE;
                break;
            }

            // If we got a char from second file, but not equal to char from
            // first file, then files are not equal
            if (c1 != c2)
            {
                Result = FALSE;
                break;
            }
        }

        // If we got to the end of the first file without breaking the loop...
        if (c1 == EOF)
        {
            // Then attempt to read from the second file to ensure it also ends.
            if (fgetc(infile2) != EOF)
                Result = FALSE;
        }
    }

    if (infile1 != NULL)
        fclose(infile1);

    if (infile2 != NULL)
        fclose(infile2);

    return Result;
}

/****************************************************************************
 ALGORITHM FLAGS/SPECIFIERS
****************************************************************************/

char const *GetAlgorithmFlags(void)
{
    return "C = command (algorithm implementation to run)\n"
           "    -p  = Planar embedding and Kuratowski subgraph isolation\n"
           "    -d  = Planar graph drawing by visibility representation\n"
           "    -o  = Outerplanar embedding and obstruction isolation\n"
           "    -2  = Search for subgraph homeomorphic to K_{2,3}\n"
           "    -3  = Search for subgraph homeomorphic to K_{3,3}\n"
           "    -3e = Search for subgraph homeomorphic to K_{3,3} with embedder\n"
           "    -4  = Search for subgraph homeomorphic to K_4\n"
           "\n";
}

char const *GetAlgorithmSpecifiers(void)
{
    return "P.  Planar embedding and Kuratowski subgraph isolation\n"
           "D.  Planar graph drawing by visibility representation\n"
           "O.  Outerplanar embedding and obstruction isolation\n"
           "2.  Search for subgraph homeomorphic to K_{2,3}\n"
           "3.  Search for subgraph homeomorphic to K_{3,3}\n"
           "3e. Search for subgraph homeomorphic to K_{3,3} with embedder\n"
           "4.  Search for subgraph homeomorphic to K_4\n";
}

char const *GetAlgorithmChoices(void)
{
    return "pdo234";
}

/****************************************************************************
 * GetCommandAndOptionalModifier()
 *
 * When provided with a valid commandString, determine the command character
 * (and optional modifier) and return to the caller via the pointers to chars.
 *
 * Note that characters are processed with tolower(), since comparisons are
 * typically performed only against lower-case letters; when applied to a digit
 * (or to any other character where such conversion is not possible), tolower()
 * returns the same character.
 *
 * Returns OK when command (and optional modifier) are successfully extracted
 * from the commandString; no validation of the characters supplied is performed
 * at this point.
 *
 * Returns NOTOK if the commandString is invalid, or if
 * the pointer to the character to which you wish to assign the extracted
 * character is NULL.
 ****************************************************************************/

int GetCommandAndOptionalModifier(const char *commandString, char *command, char *modifier)
{
    if (commandString != NULL && commandString[0] == '-')
        commandString++;

    if (commandString == NULL || strlen(commandString) == 0)
    {
        ErrorMessage("Cannot get embed flags for empty command string.\n");
        return NOTOK;
    }

    if (command == NULL)
    {
        ErrorMessage("Pointer to character to which to write command is NULL.\n");
        return NOTOK;
    }

    (*command) = '\0';

    if (modifier != NULL)
        (*modifier) = '\0';

    if (strlen(commandString) == 1)
    {
        (*command) = (char)tolower(commandString[0]);
    }
    else if (strlen(commandString) == 2)
    {
        (*command) = (char)tolower(commandString[0]);
        if (modifier != NULL)
            (*modifier) = (char)tolower(commandString[1]);
    }

    return OK;
}

/****************************************************************************
 * GetEmbedFlags()
 *
 * Derives the embedFlags when provided with a valid command character (and
 * optionally a modifier)
 *
 * Returns OK if embedFlags successfully derived, NOTOK if command and/or
 * modifier character is invalid, or if the pointer to the integer to which you
 * wish to assign the derived embedFlags is NULL.
 ****************************************************************************/

int GetEmbedFlags(char command, char modifier, int *pEmbedFlags)
{
    if (pEmbedFlags == NULL)
    {
        ErrorMessage("Pointer to embedFlags int is NULL.\n");
        return NOTOK;
    }

    switch (command)
    {
    case 'p':
        (*pEmbedFlags) = EMBEDFLAGS_PLANAR;
        break;
    case 'd':
        (*pEmbedFlags) = EMBEDFLAGS_DRAWPLANAR;
        break;
    case 'o':
        (*pEmbedFlags) = EMBEDFLAGS_OUTERPLANAR;
        break;
    case '2':
        (*pEmbedFlags) = EMBEDFLAGS_SEARCHFORK23;
        break;
    case '3':
        (*pEmbedFlags) = EMBEDFLAGS_SEARCHFORK33;
        break;
    case '4':
        (*pEmbedFlags) = EMBEDFLAGS_SEARCHFORK4;
        break;
    default:
        ErrorMessage("Unrecognized algorithm command specifier.\n");
        return NOTOK;
    }

    if (modifier != '\0')
    {
        if (command == '3')
        {
            switch (modifier)
            {
            case 'e':
                (*pEmbedFlags) |= EMBEDFLAGS_SEARCHWITHEMBEDDER;
                break;
            default:
                ErrorMessage("Only allowed modifier for K_{3,3} search is 'e' (embedder).\n");
                return NOTOK;
            }
        }
        else
        {
            ErrorMessage("Modifiers currently not supported for selected algorithm.\n");
            return NOTOK;
        }
    }

    return OK;
}

/****************************************************************************
 * GetAlgorithmName()
 *
 * When provided with a valid command char, derives the corresponding algorithm
 * name returned via char const * to the caller. This memory should not be
 * freed by the caller.
 * ****************************************************************************/

char const *GetAlgorithmName(char command)
{
    char const *algorithmName = "UnsupportedAlgorithm";

    switch (command)
    {
    case 'p':
        algorithmName = "PlanarEmbed";
        break;
    case 'd':
        algorithmName = DRAWPLANAR_NAME;
        break;
    case 'o':
        algorithmName = "OuterplanarEmbed";
        break;
    case '2':
        algorithmName = K23SEARCH_NAME;
        break;
    case '3':
        algorithmName = K33SEARCH_NAME;
        break;
    case '4':
        algorithmName = K4SEARCH_NAME;
        break;
    default:
        break;
    }

    return algorithmName;
}

/****************************************************************************
 ****************************************************************************/

char const *GetTransformationName(char command)
{
    char const *transformationName = "UnsupportedTransformation";

    switch (command)
    {
    case 'g':
        transformationName = "G6";
        break;
    case 'a':
        transformationName = "AdjList";
        break;
    case 'm':
        transformationName = "AdjMat";
        break;
    default:
        break;
    }

    return transformationName;
}

char const *GetSupportedOutputChoices(void)
{
    return "G. G6 format\n"
           "A. Adjacency List format\n"
           "M. Adjacency Matrix format\n";
}

char const *GetSupportedOutputFormats(void)
{
    return "gam";
}

/****************************************************************************
 ****************************************************************************/

char const *GetBaseName(int baseFlag)
{
    char const *transformationName = baseFlag ? "1-based" : "0-based";

    return transformationName;
}

/****************************************************************************
 * AttachAlgorithm()
 *
 * Determines the main graph algorithm command indicated by the command char
 * and attaches the corresponding graph algorithm extension. The modifier is not
 * required to determine which graph algorithm extension to attach, and is only
 * used to signal that the behaviour of the main extension should be modified.
 *
 * Returns OK if graph algorithm extension corresponding to the command char
 * was successfully attached. Returns NOTOK if theGraph is not properly
 * initialized, if attaching the graph algorithm extension failed, or if an
 * invalid command char was supplied.
 ****************************************************************************/

int AttachAlgorithm(graphP theGraph, char command)
{
    if (theGraph == NULL || theGraph->N <= 0)
    {
        ErrorMessage("Unable to attach graph algorithm extension to NULL or uninitialized graphP.\n");
        return NOTOK;
    }

    switch (command)
    {
    case 'p':
        // Planarity is attached by default
        return OK;
    case 'd':
        return gp_AttachDrawPlanar(theGraph);
    case 'o':
        // Outerplanarity is attached by default
        return OK;
    case '2':
        return gp_AttachK23Search(theGraph);
    case '3':
        return gp_AttachK33Search(theGraph);
    case '4':
        return gp_AttachK4Search(theGraph);
    default:
        break;
    }

    return NOTOK;
}

/****************************************************************************
 A string used to construct input and output filenames.

 The SUFFIXMAXLENGTH is 32 to accommodate ".out.txt" + ".render.txt" + ".test.txt"
 ****************************************************************************/

char theFileName[FILENAMEMAXLENGTH + 1 + ALGORITHMNAMEMAXLENGTH + 1 + SUFFIXMAXLENGTH + 1];

/****************************************************************************
 ConstructInputFilename()
 Returns a string not owned by the caller (do not free string).
 String contains infileName content if infileName is non-NULL.
 If infileName is NULL, then the user is asked to supply a name.
 Returns NULL on error, or a non-NULL string on success.
 ****************************************************************************/

char *ConstructInputFilename(char const *infileName)
{
    int Result = OK;

    int numCharsToReprFILENAMEMAXLENGTH = 0;
    char const *fileNameFormatFormat = " %%%d[^\r\n]";
    char *fileNameFormat = NULL;
    char lineBuff[MAXLINE + 1];

    memset(lineBuff, '\0', (MAXLINE + 1));

    if (GetNumCharsToReprInt(FILENAMEMAXLENGTH, &numCharsToReprFILENAMEMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent FILENAMEMAXLENGTH.\n");
        return NULL;
    }

    fileNameFormat = (char *)malloc((strlen(fileNameFormatFormat) + numCharsToReprFILENAMEMAXLENGTH + 1) * sizeof(char));
    if (fileNameFormat == NULL)
    {
        ErrorMessage("Unable to allocate memory for filename format string.\n");
        return NULL;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(fileNameFormat, fileNameFormatFormat, FILENAMEMAXLENGTH);
#pragma GCC diagnostic pop
    if (infileName == NULL)
    {
        while (1)
        {
            Prompt("Enter graph file name: ");
            if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
            {
                ErrorMessage("Unable to read graph file name from stdin.\n");
                Result = NOTOK;
                break;
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            if (strlen(lineBuff) == 0 || strlen(lineBuff) > FILENAMEMAXLENGTH ||
                sscanf(lineBuff, fileNameFormat, theFileName) != 1)
                ErrorMessage("Invalid input filename.\n");
            else
            {
                if (strncmp(theFileName, "stdin", strlen("stdin")) != 0 && !strchr(theFileName, '.'))
                {
                    Message("Graph file name does not have extension; automatically appending \".txt\".\n");
                    if (strcat(theFileName, ".txt") == NULL)
                    {
                        ErrorMessage("Appending \".txt\" extension to theFileName using strcat() failed.\n");
                        Result = NOTOK;
                    }
                }
                break;
            }
#pragma GCC diagnostic pop
        }
    }
    else
    {
        if (strlen(infileName) > FILENAMEMAXLENGTH)
        {
            ErrorMessage("Filename is too long.\n");
            Result = NOTOK;
        }
        else if (strlen(infileName) == 0)
        {
            ErrorMessage("Filename is empty.\n");
            Result = NOTOK;
        }

        if (Result == OK)
        {
            if (strcpy(theFileName, infileName) == NULL)
            {
                ErrorMessage("Copying infileName into theFileName using strcpy() failed.\n");
                Result = NOTOK;
            }
        }
    }

    if (fileNameFormat != NULL)
    {
        free(fileNameFormat);
        fileNameFormat = NULL;
    }

    return Result != OK ? NULL : theFileName;
}

/****************************************************************************
 * ConstructPrimaryOutputFilename()
 *
 * Returns a string not owned by the caller (do not free string).
 * Reuses the same memory space as ConstructInputFilename().
 * If outfileName is non-NULL, then the result string contains its content.
 * If outfileName is NULL, then the infileName and the command's algorithm name
 * are used to construct a string.
 *
 * Returns non-NULL string
 ****************************************************************************/

char *ConstructPrimaryOutputFilename(char const *infileName, char const *outfileName, char command)
{
    char const *algorithmName = GetAlgorithmName(command);

    if (outfileName == NULL)
    {
        // The output filename is based on the input filename
        if (theFileName != infileName)
            strcpy(theFileName, infileName);

        // If the primary output filename has not been given, then we use
        // the input filename + the algorithm name + a simple suffix
        if (strlen(algorithmName) <= ALGORITHMNAMEMAXLENGTH)
        {
            strcat(theFileName, ".");
            strcat(theFileName, algorithmName);
        }
        else
            ErrorMessage("Algorithm Name is too long, so it will not be used in output filename.\n");

        strcat(theFileName, ".out.txt");
    }
    else
    {
        if (strlen(outfileName) > FILENAMEMAXLENGTH)
        {
            char const *messageFormat = "Outfile filename is too long. Result placed in \"%.*s\"";
            int charsAvailForStrToInclude = (int)(MAXLINE - strlen(messageFormat));
            char messageContents[MAXLINE + 1];
            messageContents[0] = '\0';

            // The output filename is based on the input filename
            if (theFileName != infileName)
                strcpy(theFileName, infileName);

            if (strlen(algorithmName) <= ALGORITHMNAMEMAXLENGTH)
            {
                strcat(theFileName, ".");
                strcat(theFileName, algorithmName);
            }
            strcat(theFileName, ".out.txt");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, charsAvailForStrToInclude, theFileName);
#pragma GCC diagnostic pop
            ErrorMessage(messageContents);
        }
        else
        {
            if (theFileName != outfileName)
                strcpy(theFileName, outfileName);
        }
    }

    return theFileName;
}

/****************************************************************************
 * ConstructTransformationExpectedResultFilename()
 *
 * Returns a string whose ownership will be transferred to the caller (must free
 * string).
 * If outfileName is non-NULL, then the result string contains its content
 * If outfileName is NULL, then the infileName, the command's algorithm name,
 * and whether or not this output file correspond to the actual (0) or expected
 * (1) output file from testing are used to construct a string.
 *
 * Returns non-NULL string
 ****************************************************************************/

int ConstructTransformationExpectedResultFilename(char const *infileName, char **outfileName, char command, int baseFlag)
{
    int Result = OK;

    char const *baseName = GetBaseName(baseFlag);
    char const *transformationName = GetTransformationName(command);
    int infileNameLen = -1;

    if (infileName == NULL || (infileNameLen = strlen(infileName)) < 1)
    {
        ErrorMessage("Cannot construct transformation output filename for empty infileName.\n");
        return NOTOK;
    }

    if ((*outfileName) == NULL)
    {
        (*outfileName) = (char *)calloc(
            infileNameLen + 1 + strlen(baseName) + 1 + strlen(transformationName) +
                ((command == 'g') ? strlen(".out.g6") : strlen(".out.txt")) + 1,
            sizeof(char));

        if ((*outfileName) == NULL)
        {
            ErrorMessage("Unable to allocate memory for output filename.\n");
            return NOTOK;
        }

        strcpy((*outfileName), infileName);
        strcat((*outfileName), ".");
        strcat((*outfileName), baseName);
        strcat((*outfileName), ".");
        strcat((*outfileName), transformationName);
        strcat((*outfileName), command == 'g' ? ".out.g6" : ".out.txt");
    }
    else
    {
        ErrorMessage("outfileName already allocated.\n");
        Result = NOTOK;
    }

    return Result;
}

/****************************************************************************
 * WriteAlgorithmResults()
 ****************************************************************************/

void WriteAlgorithmResults(graphP theGraph, int Result, char command, platform_time start, platform_time end, char const *infileName)
{
    char const *messageFormat = NULL;
    char messageContents[MAXLINE + 1];
    int charsAvailForStr = 0;

    memset(messageContents, '\0', (MAXLINE + 1));

    if (infileName)
    {
        messageFormat = "The graph \"%.*s\" ";
        charsAvailForStr = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        sprintf(messageContents, messageFormat, charsAvailForStr, infileName);
#pragma GCC diagnostic pop
    }
    else
        sprintf(messageContents, "The graph ");
    Message(messageContents);

    switch (command)
    {
    case 'p':
        sprintf(messageContents, "is%s planar.\n", Result == OK ? "" : " not");
        break;
    case 'd':
        sprintf(messageContents, "is%s planar.\n", Result == OK ? "" : " not");
        break;
    case 'o':
        sprintf(messageContents, "is%s outerplanar.\n", Result == OK ? "" : " not");
        break;
    case '2':
        sprintf(messageContents, "has %s subgraph homeomorphic to K_{2,3}.\n", Result == OK ? "no" : "a");
        break;
    case '3':
        sprintf(messageContents, "has %s subgraph homeomorphic to K_{3,3}.\n", Result == OK ? "no" : "a");
        break;
    case '4':
        sprintf(messageContents, "has %s subgraph homeomorphic to K_4.\n", Result == OK ? "no" : "a");
        break;
    default:
        sprintf(messageContents, "has not been processed due to unrecognized command.\n");
        break;
    }
    Message(messageContents);

    sprintf(messageContents, "Algorithm '%s' executed in %.3lf seconds.\n",
            GetAlgorithmName(command), platform_GetDuration(start, end));
    Message(messageContents);
}
