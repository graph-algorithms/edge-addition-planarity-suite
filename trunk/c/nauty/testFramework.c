/*
Copyright (c) 1997-2015, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdlib.h>

#include "testFramework.h"

char *commands = "pdo234c";

#include "../graphK23Search.h"
#include "../graphK33Search.h"
#include "../graphK4Search.h"
#include "../graphDrawPlanar.h"

/* Forward Declarations of Private functions */
graphP createGraph(char command, int n, int maxe);
int attachAlgorithmExtension(char command, graphP aGraph);
void initBaseTestResult(baseTestResultStruct *pBaseResult);
int initTestResult(testResultP testResult, char command, int n, int maxe);
void releaseTestResult(testResultP testResult);

/***********************************************************************
 tf_AllocateTestFramework()
 ***********************************************************************/

testResultFrameworkP tf_AllocateTestFramework(char command, int n, int maxe)
{
	testResultFrameworkP framework = (testResultFrameworkP) malloc(sizeof(testResultFrameworkStruct));

	if (framework != NULL)
	{
		int i;

		framework->algResultsSize = command == 'a' ? NUMCOMMANDSTOTEST : 1;
		framework->algResults = (testResultP) malloc(framework->algResultsSize * sizeof(testResultStruct));
		for (i=0; i < framework->algResultsSize; i++)
			if (initTestResult(framework->algResults+i,
					command=='a'?commands[i]:command, n, maxe) != OK)
			{
				framework->algResultsSize = i;
				tf_FreeTestFramework(&framework);
				return NULL;
			}
	}

	return framework;
}

/***********************************************************************
 tf_FreeTestFramework()
 ***********************************************************************/

void tf_FreeTestFramework(testResultFrameworkP *pTestFramework)
{
	if (pTestFramework != NULL && *pTestFramework != NULL)
	{
		testResultFrameworkP framework = *pTestFramework;

		if (framework->algResults != NULL)
		{
			int i;
			for (i=0; i < framework->algResultsSize; i++)
				releaseTestResult(framework->algResults+i);

			free((void *) framework->algResults);
			framework->algResults = NULL;
			framework->algResultsSize = 0;
		}

		free(*pTestFramework);
		*pTestFramework = framework = NULL;
	}
}

/***********************************************************************
 tf_GetTestResult()
 ***********************************************************************/

testResultP tf_GetTestResult(testResultFrameworkP framework, char command)
{
	testResultP testResult = NULL;

	if (framework != NULL && framework->algResults != NULL)
	{
		int i;
		for (i=0; i < framework->algResultsSize; i++)
			if (framework->algResults[i].command == command)
				testResult = framework->algResults+i;
	}

	return testResult;
}

/***********************************************************************
 initTestResult()
 ***********************************************************************/

int initTestResult(testResultP testResult, char command, int n, int maxe)
{
	if (testResult != NULL)
	{
		testResult->command = command;
		initBaseTestResult(&testResult->result);
		testResult->edgeResults = NULL;
		testResult->edgeResultsSize = maxe;
		testResult->theGraph = NULL;
		testResult->origGraph = NULL;

		testResult->edgeResults = (baseTestResultStruct *) malloc((maxe+1) * sizeof(baseTestResultStruct));
		if (testResult->edgeResults == NULL)
		{
			releaseTestResult(testResult);
			return NOTOK;
		}
		else
		{
			int j;
			for (j=0; j <= maxe; j++)
				initBaseTestResult(&testResult->edgeResults[j]);
		}

		if ((testResult->theGraph = createGraph(command, n, maxe)) == NULL ||
			(testResult->origGraph = createGraph(command, n, maxe)) == NULL)
		{
			releaseTestResult(testResult);
			return NOTOK;
		}
	}

	return OK;
}

/***********************************************************************
 releaseTestResult()
 Releases all memory resources used by the testResult, but does not
 free the testResult since it points into an array.
 ***********************************************************************/

void releaseTestResult(testResultP testResult)
{
	if (testResult->edgeResults != NULL)
	{
		free(testResult->edgeResults);
		testResult->edgeResults = NULL;
	}
	gp_Free(&(testResult->theGraph));
	gp_Free(&(testResult->origGraph));
}

/***********************************************************************
 initBaseTestResult()
 ***********************************************************************/

void initBaseTestResult(baseTestResultStruct *pBaseResult)
{
	memset(pBaseResult, 0, sizeof(baseTestResultStruct));
}

/***********************************************************************
 createGraph()
 ***********************************************************************/

graphP createGraph(char command, int n, int maxe)
{
	graphP theGraph;
	int numArcs = 2*(maxe > 0 ? maxe : 1);

    if ((theGraph = gp_New()) != NULL)
    {
		if (gp_EnsureArcCapacity(theGraph, numArcs) != OK ||
				gp_InitGraph(theGraph, n) != OK ||
				attachAlgorithmExtension(command, theGraph) != OK)
			gp_Free(&theGraph);
    }

    return theGraph;
}

/***********************************************************************
 attachAlgorithmExtension()
 ***********************************************************************/

int attachAlgorithmExtension(char command, graphP aGraph)
{
    switch (command)
    {
		case 'p' : break;
		case 'd' : gp_AttachDrawPlanar(aGraph);	break;
		case 'o' : break;
		case '2' : gp_AttachK23Search(aGraph); break;
		case '3' : gp_AttachK33Search(aGraph); break;
		case '4' : gp_AttachK4Search(aGraph); break;
		default  : return NOTOK;
    }

    return OK;
}
