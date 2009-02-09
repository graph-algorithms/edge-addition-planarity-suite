#ifndef OUTPROC_H
#define OUTPROC_H

void outprocTestPlanarity(FILE *f, graph *g, int n);
void outprocTestDrawPlanar(FILE *f, graph *g, int n);
void outprocTestOuterplanarity(FILE *f, graph *g, int n);
void outprocTestK23Search(FILE *f, graph *g, int n);
void outprocTestK33Search(FILE *f, graph *g, int n);

void Test_PrintStats(FILE *, char);

#endif
