#ifndef ECL_GRAPH
#define ECL_GRAPH

#include <cstdlib>
#include <cstdio>

struct ECLgraph {
  int nodes;
  int edges;
  int* nindex;
  int* nlist;
  int* eweight;
};

ECLgraph readECLgraph(const char* const fname)
{
  ECLgraph g;
  int cnt;

  FILE* f = fopen(fname, "rb");  if (f == NULL) {fprintf(stderr, "ERROR: could not open file %s\n\n", fname);  exit(-1);}
  cnt = fread(&g.nodes, sizeof(g.nodes), 1, f);  if (cnt != 1) {fprintf(stderr, "ERROR: failed to read nodes\n\n");  exit(-1);}
  cnt = fread(&g.edges, sizeof(g.edges), 1, f);  if (cnt != 1) {fprintf(stderr, "ERROR: failed to read edges\n\n");  exit(-1);}
  if ((g.nodes < 1) || (g.edges < 0)) {fprintf(stderr, "ERROR: node or edge count too low\n\n");  exit(-1);}

  g.nindex = (int*)malloc((g.nodes + 1) * sizeof(g.nindex[0]));
  g.nlist = (int*)malloc(g.edges * sizeof(g.nlist[0]));
  g.eweight = (int*)malloc(g.edges * sizeof(g.eweight[0]));
  if ((g.nindex == NULL) || (g.nlist == NULL) || (g.eweight == NULL)) {fprintf(stderr, "ERROR: memory allocation failed\n\n");  exit(-1);}

  cnt = fread(g.nindex, sizeof(g.nindex[0]), g.nodes + 1, f);  if (cnt != g.nodes + 1) {fprintf(stderr, "ERROR: failed to read neighbor index list\n\n");  exit(-1);}
  cnt = fread(g.nlist, sizeof(g.nlist[0]), g.edges, f);  if (cnt != g.edges) {fprintf(stderr, "ERROR: failed to read neighbor list\n\n");  exit(-1);}
  cnt = fread(g.eweight, sizeof(g.eweight[0]), g.edges, f);
  if (cnt == 0) {
    free(g.eweight);
    g.eweight = NULL;
  } else {
    if (cnt != g.edges) {fprintf(stderr, "ERROR: failed to read edge weights\n\n");  exit(-1);}
  }
  fclose(f);

  return g;
}

void writeECLgraph(const ECLgraph g, const char* const fname)
{
  if ((g.nodes < 1) || (g.edges < 0)) {fprintf(stderr, "ERROR: node or edge count too low\n\n");  exit(-1);}
  int cnt;
  FILE* f = fopen(fname, "wb");  if (f == NULL) {fprintf(stderr, "ERROR: could not open file %s\n\n", fname);  exit(-1);}
  cnt = fwrite(&g.nodes, sizeof(g.nodes), 1, f);  if (cnt != 1) {fprintf(stderr, "ERROR: failed to write nodes\n\n");  exit(-1);}
  cnt = fwrite(&g.edges, sizeof(g.edges), 1, f);  if (cnt != 1) {fprintf(stderr, "ERROR: failed to write edges\n\n");  exit(-1);}

  cnt = fwrite(g.nindex, sizeof(g.nindex[0]), g.nodes + 1, f);  if (cnt != g.nodes + 1) {fprintf(stderr, "ERROR: failed to write neighbor index list\n\n");  exit(-1);}
  cnt = fwrite(g.nlist, sizeof(g.nlist[0]), g.edges, f);  if (cnt != g.edges) {fprintf(stderr, "ERROR: failed to write neighbor list\n\n");  exit(-1);}
  if (g.eweight != NULL) {
    cnt = fwrite(g.eweight, sizeof(g.eweight[0]), g.edges, f);  if (cnt != g.edges) {fprintf(stderr, "ERROR: failed to write edge weights\n\n");  exit(-1);}
  }
  fclose(f);
}

void freeECLgraph(ECLgraph &g)
{
  if (g.nindex != NULL) free(g.nindex);
  if (g.nlist != NULL) free(g.nlist);
  if (g.eweight != NULL) free(g.eweight);
  g.nindex = NULL;
  g.nlist = NULL;
  g.eweight = NULL;
}

#endif
