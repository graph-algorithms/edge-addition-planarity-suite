/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"

char * GetProjectTitle()
{
	// This message is the main location of the version number.
	// The format is major.minor.maintenance.tweak
	// Major is for an overhaul (e.g. many features, data structure change, change of backward compatibility)
	// Minor is for feature addition (e.g. a new algorithm implementation added, new interface)
	// Maintenance is for functional revision (e.g. bug fix to existing algorithm implementation)
	// Tweak is for a non-functional revision (e.g. change of build scripts or testing code, user-facing string changes)

	// If the version here is increased, also increase it in configure.ac
	// Furthermore, a change of Major, Minor or Maintenance here should cause a change
	// of Current, Revision and/or Age as documented in configure.ac

	return  "\n=================================================="
			"\nThe Edge Addition Planarity Suite version 3.0.2.0"
			"\nCopyright (c) 1997-2024 by John M. Boyer"
			"\nAll rights reserved."
			"\nSee the LICENSE.TXT file for licensing information."
			"\nContact info: jboyer at acm.org"
			"\n==================================================\n";
}

/****************************************************************************
 ALGORITHM FLAGS/SPECIFIERS
****************************************************************************/

char * GetAlgorithmFlags(void)
{
	return  "C = command (algorithm implementation to run)\n"
			"    -p = Planar embedding and Kuratowski subgraph isolation\n"
			"    -d = Planar graph drawing by visibility representation\n"
			"    -o = Outerplanar embedding and obstruction isolation\n"
			"    -2 = Search for subgraph homeomorphic to K_{2,3}\n"
			"    -3 = Search for subgraph homeomorphic to K_{3,3}\n"
			"    -4 = Search for subgraph homeomorphic to K_4\n"
			"\n";
}

char * GetAlgorithmSpecifiers(void)
{
	return  "P. Planar embedding and Kuratowski subgraph isolation\n"
			"D. Planar graph drawing by visibility representation\n"
			"O. Outerplanar embedding and obstruction isolation\n"
			"2. Search for subgraph homeomorphic to K_{2,3}\n"
			"3. Search for subgraph homeomorphic to K_{3,3}\n"
			"4. Search for subgraph homeomorphic to K_4\n";
}

char * GetAlgorithmChoices(void)
{
	return "pdo234";
}

char * GetSupportedOutputChoices(void)
{
	return  "G. G6 format\n"
			"A. Adjacency List format\n"
			"M. Adjacency Matrix format\n";
}

char * GetSupportedOutputFormats(void)
{
	return "gam";
}

/****************************************************************************
 helpMessage()
 ****************************************************************************/

int helpMessage(char *param)
{
	Message(GetProjectTitle());

	if (param == NULL)
	{
		Message(
			"'planarity': if no command-line, then menu-driven\n"
			"'planarity (-h|-help)': this message\n"
			"'planarity (-h|-help) -menu': more help with menu-based command line\n"
			"'planarity (-i|-info): copyright and license information\n"
			"'planarity -test [-q] [samples dir]': runs tests (optional quiet mode)\n"
			"\n"
		);

		Message(
			"Common usages\n"
			"-------------\n"
			"planarity -s -q -p infile.txt embedding.out [obstruction.out]\n"
			"Process infile.txt in quiet mode (-q), putting planar embedding in \n"
			"embedding.out or (optionally) a Kuratowski subgraph in Obstruction.out\n"
			"Process returns 0=planar, 1=nonplanar, -1=error\n"
			"\n"
			"planarity -s -q -d infile.txt embedding.out [drawing.out]\n"
			"If graph in infile.txt is planar, then put embedding in embedding.out \n"
			"and (optionally) an ASCII art drawing in drawing.out\n"
			"Process returns 0=planar, 1=nonplanar, -1=error\n"
		);
	}

	else if (strcmp(param, "-i") == 0 || strcmp(param, "-info") == 0)
	{
		Message(
			"Includes a reference implementation of the following:\n"
			"\n"
			"* John M. Boyer. \"Subgraph Homeomorphism via the Edge Addition Planarity \n"
			"  Algorithm\".  Journal of Graph Algorithms and Applications, Vol. 16, \n"
			"  no. 2, pp. 381-410, 2012. http://dx.doi.org/10.7155/jgaa.00268\n"
			"\n"
			"* John M. Boyer. \"A New Method for Efficiently Generating Planar Graph\n"
			"  Visibility Representations\". In P. Eades and P. Healy, editors,\n"
			"  Proceedings of the 13th International Conference on Graph Drawing 2005,\n"
			"  Lecture Notes Comput. Sci., Volume 3843, pp. 508-511, Springer-Verlag, 2006.\n"
			"  http://dx.doi.org/10.1007/11618058_47\n"
			"\n"
			"* John M. Boyer and Wendy J. Myrvold. \"On the Cutting Edge: Simplified O(n)\n"
			"  Planarity by Edge Addition\". Journal of Graph Algorithms and Applications,\n"
			"  Vol. 8, No. 3, pp. 241-273, 2004. http://dx.doi.org/10.7155/jgaa.00091\n"
			"\n"
			"* John M. Boyer. \"Simplified O(n) Algorithms for Planar Graph Embedding,\n"
			"  Kuratowski Subgraph Isolation, and Related Problems\". Ph.D. Dissertation,\n"
			"  University of Victoria, 2001. https://dspace.library.uvic.ca/handle/1828/9918\n"
			"\n"
		);
	}

	else if (strcmp(param, "-menu") == 0)
	{
		Message(
			"'planarity -r [-q] C K N': Random graphs\n"
			"'planarity -s [-q] C I O [O2]': Specific graph\n"
			"'planarity -rm [-q] N O [O2]': Random maximal planar graph\n"
			"'planarity -rn [-q] N O [O2]': Random nonplanar graph (maximal planar + edge)\n"
			"'planarity -t [-q] C|-t(gam) I O': Test algorithm on graphs or transform graph\n"
			"'planarity I O [-n O2]': Legacy command-line (default -s -p)\n"
			"\n"
		);

		Message("-q is for quiet mode (no messages to stdout and stderr)\n\n");

		Message(GetAlgorithmFlags());

		Message(
			"K = # of graphs to randomly generate\n"
			"N = # of vertices in each randomly generated graph\n"
			"I = Input file (for work on a specific graph)\n"
			"O = Primary output file\n"
			"    For example, if C=-p then O receives the planar embedding\n"
			"    If C=-3, then O receives a subgraph containing a K_{3,3}\n"
			"O2= Secondary output file\n"
			"    For -s, if C=-p or -o, then O2 receives the embedding obstruction\n"
		   	"    For -s, if C=-d, then O2 receives a drawing of the planar graph\n"
			"    For -rm and -rn, O2 contains the original randomly generated graph\n"
			"\n"
		);

		Message(
			"planarity process results: 0=OK, -1=NOTOK, 1=NONEMBEDDABLE\n"
			"    1 result only produced by specific graph mode (-s)\n"
			"      with command -2,-3,-4: found K_{2,3}, K_{3,3} or K_4\n"
			"      with command -p,-d: found planarity obstruction\n"
			"      with command -o: found outerplanarity obstruction\n"
		);
	}

	FlushConsole(stdout);
	return OK;
}

/****************************************************************************
 MAIN
 ****************************************************************************/

int main(int argc, char *argv[])
{
	int retVal=0;

	if (argc <= 1)
		retVal = menu();

	else if (argv[1][0] == '-')
		retVal = commandLine(argc, argv);

	else
		retVal = legacyCommandLine(argc, argv);

	// Close the log file if logging
	gp_Log(NULL);

	return retVal;
}

/****************************************************************************
 MENU-DRIVEN PROGRAM
 ****************************************************************************/
void TransformOrTestMenu(void);
void TransformMenu(void);
void TestMenu(void);

int menu()
{
char Choice;

	 do {
		Message(GetProjectTitle());

		Message(GetAlgorithmSpecifiers());

		Message(
				"T. Test graph functionality\n"
				"H. Help message for command line version\n"
				"R. Reconfigure options\n"
				"X. Exit\n"
				"\n"
		);

		Prompt("Enter Choice: ");
		fflush(stdin);
		scanf(" %c", &Choice);
		Choice = tolower(Choice);

		if (Choice == 'h')
			helpMessage(NULL);

		else if (Choice == 'r')
			Reconfigure();

		else if (Choice == 't')
			TransformOrTestMenu();

		else if (Choice != 'x')
		{
			char *secondOutfile = NULL;
			if (Choice == 'p'  || Choice == 'd' || Choice == 'o')
				secondOutfile ="";

			if (!strchr(GetAlgorithmChoices(), Choice)) {
				Message("Invalid menu choice, please try again.");
			} else {
				switch (tolower(Mode))
				{
					case 's' : SpecificGraph(Choice, NULL, NULL, secondOutfile, NULL, NULL, NULL); break;
					case 'r' : RandomGraphs(Choice, 0, 0); break;
					case 'm' : RandomGraph(Choice, 0, 0, NULL, NULL); break;
					case 'n' : RandomGraph(Choice, 1, 0, NULL, NULL); break;
				}
			}
		}

		if (Choice != 'r' && Choice != 'x')
		{
			Prompt("\nPress a key then hit ENTER to continue...");
			fflush(stdin);
			scanf(" %*c");
			fflush(stdin);
			Message("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
			FlushConsole(stdout);
		}

	 }  while (Choice != 'x');

	 // Certain debuggers don't terminate correctly with pending output content
	 FlushConsole(stdout);
	 FlushConsole(stderr);

	 return 0;
}

void TransformOrTestMenu()
{
	char Choice;
	char *messageFormat;
	char messageContents[MAXLINE + 1];

	Message("\n");
	Message("T. Transform single graph in supported file to .g6, adjacency list, or adjacency matrix\n");
	Message("A. Perform an algorithm test on all graphs in .g6 input file\n");
	Message("\nPress any other key to return to the main menu.\n");
	Message("\n");

	Prompt("Enter Choice: ");
	fflush(stdin);
	scanf(" %c", &Choice);
	Choice = tolower(Choice);

	switch(Choice)
	{
		case 't':TransformMenu(); return;
		case 'a': TestMenu(); return;
	}
}

void TransformMenu()
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
	}
	while(strlen(infileName) == 0);

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
			sprintf(commandStr, "-t%c", outputFormat);
	}
	while(strlen(commandStr) == 0);

	if (strlen(outfileName) == 0)
	{
		Result = TestGraphFunctionality(commandStr, infileName, NULL, NULL, NULL, &outputStr);
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
		Result = TestGraphFunctionality(commandStr, infileName, NULL, NULL, outfileName, NULL);
		if (Result != OK)
			ErrorMessage("Failed to perform transformation.\n");
	}
}

void TestMenu()
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
	}
	while(strlen(infileName) == 0);
	
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
	}
	while(strlen(commandStr) == 0);

	if (strlen(outfileName) == 0)
	{
		Result = TestGraphFunctionality(commandStr, infileName, NULL, NULL, NULL, &outputStr);
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
		Result = TestGraphFunctionality(commandStr, infileName, NULL, NULL, outfileName, NULL);
		if (Result != OK)
			ErrorMessage("Algorithm test on all graphs in .g6 input file failed.\n");
	}
}
