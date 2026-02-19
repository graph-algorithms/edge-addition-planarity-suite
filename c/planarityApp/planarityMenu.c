/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"

/****************************************************************************
 MENU-DRIVEN PROGRAM
 ****************************************************************************/
int TransformGraphMenu(void);
int TestAllGraphsMenu(void);

int menu(void)
{
    int Result = OK;

    char lineBuff[MAXLINE + 1];

    int numCharsToReprCOMMANDSTRINGMAXLENGTH = 0;
    char const *choiceStringFormatFormat = " %%%ds";
    char *choiceStringFormat = NULL;
    char choiceString[COMMANDSTRINGMAXLENGTH + 1];

    char *secondOutfile = NULL;

    char command = '\0';

    memset(lineBuff, '\0', (MAXLINE + 1));
    memset(choiceString, '\0', (COMMANDSTRINGMAXLENGTH + 1));

    if (GetNumCharsToReprInt(COMMANDSTRINGMAXLENGTH, &numCharsToReprCOMMANDSTRINGMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent COMMANDSTRINGMAXLENGTH.\n");

        Result = NOTOK;
    }
    else
    {
        choiceStringFormat = (char *)malloc((strlen(choiceStringFormatFormat) + numCharsToReprCOMMANDSTRINGMAXLENGTH + 1) * sizeof(char));
        if (choiceStringFormat == NULL)
        {
            ErrorMessage("Unable to allocate memory for choice string format string.\n");

            Result = NOTOK;
        }
    }

    if (Result == OK)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        sprintf(choiceStringFormat, choiceStringFormatFormat, COMMANDSTRINGMAXLENGTH);
#pragma GCC diagnostic pop

        while (1)
        {
            Message(GetProjectTitle());

            Message(GetAlgorithmSpecifiers());

            Message(
                "X. Transform single graph in supported file to .g6, adjacency list, or adjacency matrix\n"
                "T. Perform an algorithm test on all graphs in .g6 input file\n"
                "H. Help message for command line version\n"
                "R. Reconfigure options\n"
                "Q. Quit\n"
                "\n");

            Prompt("Enter Choice: ");

            if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
            {
                ErrorMessage("Unable to fetch menu choice from stdin; exiting.\n");

                Result = NOTOK;

                break;
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            if (strlen(lineBuff) == 0 || strlen(lineBuff) > 2 ||
                sscanf(lineBuff, choiceStringFormat, choiceString) != 1)
            {
                ErrorMessage("Invalid input; please retry.\n");

                continue;
            }
#pragma GCC diagnostic pop

            choiceString[0] = (char)tolower(choiceString[0]);
            choiceString[1] = (char)tolower(choiceString[1]);

            if (strcmp(choiceString, "h") == 0)
                helpMessage(NULL);

            else if (strcmp(choiceString, "r") == 0)
            {
                if (Reconfigure() != OK)
                {
                    ErrorMessage("Encountered unrecoverable error when reconfiguring; exiting.\n");

                    Result = NOTOK;

                    break;
                }

                continue;
            }

            else if (strcmp(choiceString, "x") == 0)
            {
                if ((Result = TransformGraphMenu()) != OK)
                    ErrorMessage("Transform Graph Menu emitted an error.\n");
            }
            else if (strcmp(choiceString, "t") == 0)
            {
                if ((Result = TestAllGraphsMenu()) != OK)
                    ErrorMessage("Test All Graphs Menu emitted an error.\n");
            }
            else if (strcmp(choiceString, "q") == 0)
                break;
            else
            {
                char *commandString = choiceString;
                if (GetCommandAndOptionalModifier(commandString, &command, NULL) != OK)
                {
                    ErrorMessage("Unable to extract command from choice, please retry.\n");

                    commandString = NULL;

                    continue;
                }

                if (command == 'p' || command == 'd' || command == 'o')
                    secondOutfile = (char *)"";

                if (!strchr(GetAlgorithmChoices(), command))
                    ErrorMessage("Invalid algorithm command choice, please retry.\n");

                else
                {
                    switch (tolower(Mode))
                    {
                    case 's':
                        Result = SpecificGraph(commandString, NULL, NULL, secondOutfile, NULL, NULL, NULL);
                        break;
                    case 'r':
                        Result = RandomGraphs(commandString, 0, 0, NULL);
                        break;
                    case 'm':
                        Result = RandomGraph(commandString, 0, 0, NULL, NULL);
                        break;
                    case 'n':
                        Result = RandomGraph(commandString, 1, 0, NULL, NULL);
                        break;
                    default:
                        break;
                    }
                }
            }

            Prompt("\nPress return key to continue...");
            if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
            {
                ErrorMessage("Unable to fetch from stdin; exiting.\n");

                Result = NOTOK;

                break;
            }

            Message("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            FlushConsole(stdout);
        }
    }

    // Certain debuggers don't terminate correctly with pending output content
    FlushConsole(stdout);
    FlushConsole(stderr);

    if (choiceStringFormat != NULL)
    {
        free(choiceStringFormat);
        choiceStringFormat = NULL;
    }

    // NOTE: Translates internal planarity codes to appropriate exit codes
    return Result == OK ? 0 : (Result == NONEMBEDDABLE ? 1 : -1);
}

int TransformGraphMenu(void)
{
    int Result = OK;

    int numCharsToReprFILENAMEMAXLENGTH = 0;
    char const *fileNameFormatFormat = " %%%d[^\r\n]";
    char *fileNameFormat = NULL;
    char lineBuff[MAXLINE + 1];
    char infileName[FILENAMEMAXLENGTH + 1];
    char outfileName[FILENAMEMAXLENGTH + 1];
    char commandStr[COMMANDSTRINGMAXLENGTH + 1];
    char outputFormat = '\0';

    memset(lineBuff, '\0', (MAXLINE + 1));
    memset(infileName, '\0', (FILENAMEMAXLENGTH + 1));
    memset(outfileName, '\0', (FILENAMEMAXLENGTH + 1));
    memset(commandStr, '\0', (COMMANDSTRINGMAXLENGTH + 1));

    if (GetNumCharsToReprInt(FILENAMEMAXLENGTH, &numCharsToReprFILENAMEMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent FILENAMEMAXLENGTH.\n");

        return NOTOK;
    }

    fileNameFormat = (char *)malloc((strlen(fileNameFormatFormat) + numCharsToReprFILENAMEMAXLENGTH + 1) * sizeof(char));
    if (fileNameFormat == NULL)
    {
        ErrorMessage("Unable to allocate memory for filename format string.\n");

        return NOTOK;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(fileNameFormat, fileNameFormatFormat, FILENAMEMAXLENGTH);
#pragma GCC diagnostic pop

    while (1)
    {
        Prompt("Enter input filename:\n");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
        {
            ErrorMessage("Unable to read input filename from stdin.\n");

            Result = NOTOK;

            break;
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        if (strlen(lineBuff) == 0 || strlen(lineBuff) > FILENAMEMAXLENGTH ||
            sscanf(lineBuff, fileNameFormat, infileName) != 1 ||
            strlen(infileName) == 0)
            ErrorMessage("Invalid input filename.\n");
        else if (strncmp(infileName, "stdin", strlen("stdin")) == 0)
        {
            ErrorMessage("\n\tPlease choose an input file path: stdin not supported from menu.\n\n");
            memset(infileName, '\0', (FILENAMEMAXLENGTH + 1));
        }
        else
            break;
#pragma GCC diagnostic pop
    }

    if (Result == OK)
    {
        while (1)
        {
            Prompt("Enter output filename, or type \"stdout\" to output to console:\n");
            if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
            {
                ErrorMessage("Unable to read output filename from stdin.\n");

                Result = NOTOK;

                break;
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            if (strlen(lineBuff) == 0 || strlen(lineBuff) > FILENAMEMAXLENGTH ||
                sscanf(lineBuff, fileNameFormat, outfileName) != 1 ||
                strlen(outfileName) == 0)
                ErrorMessage("Invalid output filename.\n");
            else
                break;
#pragma GCC diagnostic pop
        }
    }

    if (Result == OK)
    {
        while (1)
        {
            Message(GetSupportedOutputChoices());
            Prompt("Enter output format: ");
            if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
            {
                ErrorMessage("Unable to read output format from stdin.\n");

                Result = NOTOK;

                break;
            }

            if (strlen(lineBuff) != 1 ||
                sscanf(lineBuff, " %c", &outputFormat) != 1 ||
                !strchr(GetSupportedOutputFormats(), tolower(outputFormat)))
                ErrorMessage("Invalid choice for output format.\n");
            else
            {
                if (sprintf(commandStr, "-%c", (char)tolower(outputFormat)) < 1)
                {
                    ErrorMessage("Unable to construct commandStr.\n");

                    Result = NOTOK;
                }

                break;
            }
        }
    }

    if (Result == OK)
        Result = TransformGraph(commandStr, infileName, NULL, NULL, outfileName, NULL);

    if (fileNameFormat != NULL)
    {
        free(fileNameFormat);
        fileNameFormat = NULL;
    }

    return Result;
}

int TestAllGraphsMenu(void)
{
    int Result = OK;

    char lineBuff[MAXLINE + 1];

    int numCharsToReprFILENAMEMAXLENGTH = 0;
    char const *fileNameFormatFormat = " %%%d[^\r\n]";
    char *fileNameFormat = NULL;
    char infileName[FILENAMEMAXLENGTH + 1];
    char outfileName[FILENAMEMAXLENGTH + 1];

    int numCharsToReprCOMMANDSTRINGMAXLENGTH = 0;
    char const *commandStringFormatFormat = " %%%ds";
    char *commandStringFormat = NULL;
    char commandString[COMMANDSTRINGMAXLENGTH + 1];

    memset(lineBuff, '\0', (MAXLINE + 1));
    memset(infileName, '\0', (FILENAMEMAXLENGTH + 1));
    memset(outfileName, '\0', (FILENAMEMAXLENGTH + 1));
    memset(commandString, '\0', (COMMANDSTRINGMAXLENGTH + 1));

    if (GetNumCharsToReprInt(COMMANDSTRINGMAXLENGTH, &numCharsToReprCOMMANDSTRINGMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent COMMANDSTRINGMAXLENGTH.\n");

        return NOTOK;
    }

    commandStringFormat = (char *)malloc((strlen(commandStringFormatFormat) + numCharsToReprCOMMANDSTRINGMAXLENGTH + 1) * sizeof(char));
    if (commandStringFormat == NULL)
    {
        ErrorMessage("Unable to allocate memory for command string format string.\n");

        return NOTOK;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(commandStringFormat, commandStringFormatFormat, COMMANDSTRINGMAXLENGTH);
#pragma GCC diagnostic pop

    if (GetNumCharsToReprInt(FILENAMEMAXLENGTH, &numCharsToReprFILENAMEMAXLENGTH) != OK)
    {
        ErrorMessage("Unable to determine number of characters required to represent FILENAMEMAXLENGTH.\n");

        if (commandStringFormat != NULL)
        {
            free(commandStringFormat);
            commandStringFormat = NULL;
        }

        return NOTOK;
    }

    fileNameFormat = (char *)malloc((strlen(fileNameFormatFormat) + numCharsToReprFILENAMEMAXLENGTH + 1) * sizeof(char));
    if (fileNameFormat == NULL)
    {
        ErrorMessage("Unable to allocate memory for filename format string.\n");

        if (commandStringFormat != NULL)
        {
            free(commandStringFormat);
            commandStringFormat = NULL;
        }

        return NOTOK;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(fileNameFormat, fileNameFormatFormat, FILENAMEMAXLENGTH);
#pragma GCC diagnostic pop

    while (1)
    {
        Prompt("Enter input filename:\n");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
        {
            ErrorMessage("Unable to read input filename from stdin.\n");

            Result = NOTOK;

            break;
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        if (strlen(lineBuff) == 0 || strlen(lineBuff) > FILENAMEMAXLENGTH ||
            sscanf(lineBuff, fileNameFormat, infileName) != 1 ||
            strlen(infileName) == 0)
            ErrorMessage("Invalid input filename.\n");
        else if (strncmp(infileName, "stdin", strlen("stdin")) == 0)
        {
            ErrorMessage("\n\tPlease choose an input file path: stdin not supported from menu.\n\n");
            memset(infileName, '\0', (FILENAMEMAXLENGTH + 1));
        }
        else
            break;
#pragma GCC diagnostic pop
    }

    if (Result == OK)
    {
        while (1)
        {
            Prompt("Enter output filename, or type \"stdout\" to output to console:\n");
            if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
            {
                ErrorMessage("Unable to read output filename from stdin.\n");

                Result = NOTOK;

                break;
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            if (strlen(lineBuff) == 0 || strlen(lineBuff) > FILENAMEMAXLENGTH ||
                sscanf(lineBuff, fileNameFormat, outfileName) != 1 ||
                strlen(outfileName) == 0)
                ErrorMessage("Invalid output filename.\n");
            else
                break;
#pragma GCC diagnostic pop
        }
    }

    if (Result == OK)
    {
        while (1)
        {
            Message(GetAlgorithmSpecifiers());
            Prompt("Enter algorithm specifier (with optional modifier): ");
            if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
            {
                ErrorMessage("Unable to read command and optional modifier from stdin.\n");

                Result = NOTOK;

                break;
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            if (strlen(lineBuff) == 0 || strlen(lineBuff) > 2 ||
                sscanf(lineBuff, commandStringFormat, commandString) != 1 ||
                strlen(commandString) == 0)
                ErrorMessage("Invalid command and optional modifier.\n");
            else
                break;
#pragma GCC diagnostic pop
        }
    }

    if (Result == OK)
        Result = TestAllGraphs(commandString, infileName, outfileName, NULL);

    if (commandStringFormat != NULL)
    {
        free(commandStringFormat);
        commandStringFormat = NULL;
    }

    if (fileNameFormat != NULL)
    {
        free(fileNameFormat);
        fileNameFormat = NULL;
    }

    return Result;
}
