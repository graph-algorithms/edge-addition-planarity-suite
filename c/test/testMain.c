/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#if defined(_MSC_VER) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
// MSVC under Windows doesn't have unistd.h, but does define functions like getcwd and chdir
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include "testMain.h"
#include "testMain.private.h"

// extern int runRandomGraphsTests(void);

// extern int runFaceListTest(void);
// extern int runAddInsertEdgeTests(void);

// extern int runHideRestoreTests(void);
// extern int runIdentifyContractTests(void);

// extern int runReadWithExtensionAtEofTest(void);
// extern int runHighByteRoundTripTest(void);
// extern int runCapacityLimitTests(void);

// extern int runIOErrorTests(void);

// extern int runStringInputLineTests(void);
// extern int runGeneralReadIteratorTests(void);

/****************************************************************************
 Quick regression test
 ****************************************************************************/

int runTestMain(int argc, char *argv[])
{
    char const *samplesDir = "samples";
    int samplesDirArgLocation = 2;
    int retVal = OK;
    char origDir[2 * MAXLINE + 1];

    // Skip optional -q quiet mode command-line parameter, if present
    if (argc > samplesDirArgLocation && strcmp(argv[samplesDirArgLocation], "-q") == 0)
        samplesDirArgLocation++;

    // Accept overriding sample directory command-line parameter, if present
    if (argc > samplesDirArgLocation)
        samplesDir = argv[samplesDirArgLocation];

    memset(origDir, '\0', (2 * MAXLINE + 1));

    if (!getcwd(origDir, 2 * MAXLINE))
        return NOTOK;

    // Preserve original behavior before the samplesDir command-line parameter was available
    if (strcmp(samplesDir, "samples") == 0)
    {
        if (chdir(samplesDir) != 0)
        {
            if (chdir("..") != 0 || chdir(samplesDir) != 0)
            {
                // Give success result, but Warn if no samples (except no warning if in quiet mode)
                gp_Message("WARNING: Unable to change to samples directory to "
                           "run tests on samples.");
                if (chdir(origDir) != 0)
                    gp_Message("WARNING: Unable to restore the original "
                               "working directory.");

                return OK;
            }
        }
    }
    else
    {
        // New behavior if samplesDir command-line parameter was specified
        if (chdir(samplesDir) != 0)
        {
            gp_Message("WARNING: Unable to change to samples directory to run "
                       "tests on samples.");

            return OK;
        }
    }

    if (runSpecificGraphTests() != OK)
        retVal = NOTOK;
    else if (runRandomGraphsTests() != OK)
        retVal = NOTOK;
    else if (runGraphTransformationTests() != OK)
        retVal = NOTOK;
    else if (runTestAllGraphsTests() != OK)
        retVal = NOTOK;
    // else if (runFaceListTest() != OK)
    //     retVal = NOTOK;
    // else if (runAddInsertEdgeTests() != OK)
    //     retVal = NOTOK;
    // else if (runHideRestoreTests() != OK)
    //     retVal = NOTOK;
    // else if (runIdentifyContractTests() != OK)
    //     retVal = NOTOK;
    // else if (runDigraphTests() != OK)
    //     retVal = NOTOK;
    else if (runParallelEdgeTests() != OK)
        retVal = NOTOK;
    else if (runLoopEdgeTests() != OK)
        retVal = NOTOK;
    // else if (runHighByteRoundTripTest() != OK)
    //     retVal = NOTOK;
    // else if (runCapacityLimitTests() != OK)
    //     retVal = NOTOK;
    else if (runExtensionCopyTests() != OK)
        retVal = NOTOK;
    // else if (runIOErrorTests() != OK)
    //     retVal = NOTOK;
    // else if (runReadWithExtensionAtEofTest() != OK)
    //     retVal = NOTOK;
    // else if (runStringInputLineTests() != OK)
    //     retVal = NOTOK;
    // else if (runGeneralReadIteratorTests() != OK)
    //     retVal = NOTOK;
    // else if (runGeneralWriteIteratorTests() != OK)
    //     retVal = NOTOK;
    // else if (runSparse6ReadTests() != OK)
    //     retVal = NOTOK;
    // else if (runSparse6WriteTests() != OK)
    //     retVal = NOTOK;
    // else if (runSparse6LookaheadTests() != OK)
    //     retVal = NOTOK;
    // else if (runDigraph6Tests() != OK)
    //     retVal = NOTOK;
    // else if (runGraphMLTests() != OK)
    //     retVal = NOTOK;

    // All done.
    if (retVal == OK)
        gp_Message("============\n\nAll tests have succeeded.");
    else
        gp_Message("============\n\nOne or more tests FAILED.");

    if (chdir(origDir) != 0)
        gp_Message("WARNING: Unable to restore the original working directory.");

    FlushConsole(stdout);

    return retVal;
}
