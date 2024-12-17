/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"

/****************************************************************************
 MENU-DRIVEN PROGRAM
 ****************************************************************************/
void TransformGraphMenu(void);
void TestAllGraphsMenu(void);

int menu(void)
{
    char Choice;

    do
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
        fflush(stdin);
        scanf(" %c", &Choice);
        Choice = tolower(Choice);

        if (Choice == 'h')
            helpMessage(NULL);

        else if (Choice == 'r')
            Reconfigure();

        else if (Choice == 'x')
            TransformGraphMenu();

        else if (Choice == 't')
            TestAllGraphsMenu();

        else if (Choice != 'q')
        {
            char *secondOutfile = NULL;
            if (Choice == 'p' || Choice == 'd' || Choice == 'o')
                secondOutfile = "";

            if (!strchr(GetAlgorithmChoices(), Choice))
            {
                Message("Invalid menu choice, please try again.");
            }
            else
            {
                switch (tolower(Mode))
                {
                case 's':
                    SpecificGraph(Choice, NULL, NULL, secondOutfile, NULL, NULL, NULL);
                    break;
                case 'r':
                    RandomGraphs(Choice, 0, 0, NULL, NULL);
                    break;
                case 'm':
                    RandomGraph(Choice, 0, 0, NULL, NULL);
                    break;
                case 'n':
                    RandomGraph(Choice, 1, 0, NULL, NULL);
                    break;
                }
            }
        }

        if (Choice != 'r' && Choice != 'q')
        {
            Prompt("\nPress a key then hit ENTER to continue...");
            fflush(stdin);
            scanf(" %*c");
            fflush(stdin);
            Message("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            FlushConsole(stdout);
        }

    } while (Choice != 'q');

    // Certain debuggers don't terminate correctly with pending output content
    FlushConsole(stdout);
    FlushConsole(stderr);

    return 0;
}

void TransformGraphMenu(void)
{
    int Result = OK;

    char infileName[MAXLINE + 1];
    infileName[0] = '\0';
    char outfileName[MAXLINE + 1];
    outfileName[0] = '\0';
    char *outputStr = NULL;
    char outputFormat = '\0';
    char commandStr[4];
    commandStr[0] = '\0';

    do
    {
        Prompt("Enter input filename:\n");
        fflush(stdin);
        fgets(infileName, MAXLINE, stdin);

        if (strncmp(infileName, "stdin", 5) == 0)
        {
            ErrorMessage("stdin not supported from menu.\n");
            infileName[0] = '\0';
        }
    } while (strlen(infileName) == 0);

    infileName[strcspn(infileName, "\n\r")] = '\0';

    Prompt("Enter output filename, or press return to output to console:\n");
    fflush(stdin);
    fgets(outfileName, MAXLINE, stdin);
    outfileName[strcspn(outfileName, "\n\r")] = '\0';

    do
    {
        Message(GetSupportedOutputChoices());
        Prompt("Enter output format: ");
        fflush(stdin);
        scanf(" %c", &outputFormat);
        outputFormat = tolower(outputFormat);
        if (strchr(GetSupportedOutputFormats(), outputFormat))
            sprintf(commandStr, "-%c", outputFormat);
    } while (strlen(commandStr) == 0);

    if (strlen(outfileName) == 0)
    {
        Result = TransformGraph(commandStr, infileName, NULL, NULL, NULL, &outputStr);
        if (Result != OK || outputStr == NULL)
            ErrorMessage("Failed to perform transformation.\n");
        else
        {
            Message("Output:\n");
            Message(outputStr);
            Message("\n");
        }
    }
    else
    {
        Result = TransformGraph(commandStr, infileName, NULL, NULL, outfileName, NULL);
        if (Result != OK)
            ErrorMessage("Failed to perform transformation.\n");
    }

    if (outputStr != NULL)
    {
        free(outputStr);
        outputStr = NULL;
    }
}

void TestAllGraphsMenu(void)
{
    int Result = OK;

    char infileName[MAXLINE + 1];
    infileName[0] = '\0';
    char outfileName[MAXLINE + 1];
    outfileName[0] = '\0';
    char *outputStr = NULL;
    char algorithmSpecifier = '\0';
    char commandStr[3];
    commandStr[0] = '\0';

    do
    {
        Prompt("Enter input filename:\n");
        fflush(stdin);
        fgets(infileName, MAXLINE, stdin);

        if (strncmp(infileName, "stdin", 5) == 0)
        {
            ErrorMessage("stdin not supported from menu.\n");
            infileName[0] = '\0';
        }
    } while (strlen(infileName) == 0);

    infileName[strcspn(infileName, "\n\r")] = '\0';

    Prompt("Enter output filename, or press return to output to console:\n");
    fflush(stdin);
    fgets(outfileName, MAXLINE, stdin);
    outfileName[strcspn(outfileName, "\n\r")] = '\0';

    do
    {
        Message(GetAlgorithmSpecifiers());

        Prompt("Enter algorithm specifier: ");
        fflush(stdin);
        scanf(" %c", &algorithmSpecifier);
        algorithmSpecifier = tolower(algorithmSpecifier);
        if (strchr(GetAlgorithmChoices(), algorithmSpecifier))
            sprintf(commandStr, "-%c", algorithmSpecifier);
    } while (strlen(commandStr) == 0);

    if (strlen(outfileName) == 0)
    {
        Result = TestAllGraphs(commandStr, infileName, NULL, &outputStr);
        if (Result != OK || outputStr == NULL)
            ErrorMessage("Algorithm test on all graphs in .g6 input file failed.\n");
        else
        {
            Message("Output:\n");
            Message(outputStr);
            Message("\n");
        }
    }
    else
    {
        Result = TestAllGraphs(commandStr, infileName, outfileName, NULL);
        if (Result != OK)
            ErrorMessage("Algorithm test on all graphs in .g6 input file failed.\n");
    }
}
