/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef TESTMAIN_PRIVATE_H
#define TESTMAIN_PRIVATE_H

#ifdef __cplusplus
extern "C"
{
#endif

    extern int runDigraphTests(void);             /* test/graphLib/testDigraphs.c */
    extern int runLoopEdgeTests(void);            /* test/graphlib/testLoopEdges.c */
    extern int runParallelEdgeTests(void);        /* test/graphLib/testParallelEdges.c */
    extern int runExtensionCopyTests(void);       /* test/graphLib/extensionSystem/testExtensionCopy.c */
    extern int runDigraph6Tests(void);            /* test/graphLib/io/testDigraph6.c */
    extern int runGraphMLTests(void);             /* test/graphLib/io/testGraphML.c */
    extern int runGraphTransformationTests(void); /* test/graphLib/io/testGraphTransformations.c */
    extern int runIOErrorTests(void);             /* test/graphLib/io/testIOErrors.c */
    extern int runSparse6Tests(void);             /* test/graphLib/io/testSparse6.c */
    extern int runTestAllGraphsTests(void);       /* test/systemVerification/testAllGraphs.c */
    extern int runRandomGraphsTests(void);        /* test/systemVerification/testRandomGraphs.c */
    extern int runSpecificGraphTests(void);       /* test/systemVerification/testSpecificGraphs.c */

#ifdef __cplusplus
}
#endif

#endif /* TESTMAIN_PRIVATE_H */
