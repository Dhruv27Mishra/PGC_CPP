#include <algorithm>
#include <sys/time.h>
#include "ECLgraph.h"


static const int BPI = 32;  // bits per int
static const int MSB = 1 << (BPI - 1);
static const int Mask = (1 << (BPI / 2)) - 1;


// source of hash function: https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
static unsigned int hash(unsigned int val)
{
  val = ((val >> 16) ^ val) * 0x45d9f3b;
  val = ((val >> 16) ^ val) * 0x45d9f3b;
  return (val >> 16) ^ val;
}


static int init(const int nodes, const int edges, const int* const __restrict__ nidx, const int* const __restrict__ nlist, int* const __restrict__ nlist2, int* const __restrict__ posscol, int* const __restrict__ posscol2, int* const __restrict__ color, int* const __restrict__ wl, const int threads)
{
  int wlsize = 0;
  int maxrange = -1;
  #pragma omp parallel for num_threads(threads) default(none) reduction(max: maxrange) shared(nodes, wlsize, wl, nidx, nlist, nlist2, color, posscol)
  for (int v = 0; v < nodes; v++) {
    int active;
    const int beg = nidx[v];
    const int end = nidx[v + 1];
    const int degv = end - beg;
    const bool cond = (degv >= BPI);
    int pos = beg;
    if (cond) {
      int tmp;
      #pragma omp atomic capture
      tmp = wlsize++;
      wl[tmp] = v;
      for (int i = beg; i < end; i++) {
        const int nei = nlist[i];
        const int degn = nidx[nei + 1] - nidx[nei];
        if ((degv < degn) || ((degv == degn) && (hash(v) < hash(nei))) || ((degv == degn) && (hash(v) == hash(nei)) && (v < nei))) {
          nlist2[pos] = nei;
          pos++;
        }
      }
    } else {
      active = 0;
      for (int i = beg; i < end; i++) {
        const int nei = nlist[i];
        const int degn = nidx[nei + 1] - nidx[nei];
        if ((degv < degn) || ((degv == degn) && (hash(v) < hash(nei))) || ((degv == degn) && (hash(v) == hash(nei)) && (v < nei))) {
          active |= (unsigned int)MSB >> (i - beg);
          pos++;
        }
      }
    }
    const int range = pos - beg;
    maxrange = std::max(maxrange, range);  // reduction
    color[v] = (cond || (range == 0)) ? (range << (BPI / 2)) : active;
    posscol[v] = (range >= BPI) ? -1 : (MSB >> range);
  }
  if (maxrange >= Mask) {printf("too many active neighbors\n"); exit(-1);}
  #pragma omp parallel for num_threads(threads) default(none) shared(edges, posscol2)
  for (int i = 0; i < edges / BPI + 1; i++) posscol2[i] = -1;
  return wlsize;
}


void runLarge(const int* const __restrict__ nidx, const int* const __restrict__ nlist, int* const __restrict__ posscol, volatile int* const __restrict__ posscol2, volatile int* const __restrict__ color, const int* const __restrict__ wl, const int wlsize, const int threads)
{
  if (wlsize != 0) {
    bool again;
    #pragma omp parallel num_threads(threads) default(none) shared(wlsize, wl, nidx, nlist, color, posscol, posscol2) private(again)
    do {
      again = false;
      #pragma omp for nowait
      for (int w = 0; w < wlsize; w++) {
        bool shortcut = true;
        bool done = true;
        const int v = wl[w];
        int data;  // const
        #pragma omp atomic read
        data = color[v];
        const int range = data >> (BPI / 2);
        if (range > 0) {
          const int beg = nidx[v];
          int pcol = posscol[v];
          const int mincol = data & Mask;
          const int maxcol = mincol + range;
          const int end = beg + maxcol;
          const int offs = beg / BPI;
          for (int i = beg; i < end; i++) {
            const int nei = nlist[i];
            int neidata;  // const
            #pragma omp atomic read
            neidata = color[nei];
            const int neirange = neidata >> (BPI / 2);
            if (neirange == 0) {
              const int neicol = neidata;
              if (neicol < BPI) {
                pcol &= ~((unsigned int)MSB >> neicol);
              } else {
                if ((mincol <= neicol) && (neicol < maxcol)) {
                  int pc;  // const
                  #pragma omp atomic read
                  pc = posscol2[offs + neicol / BPI];
                  if ((pc << (neicol % BPI)) < 0) {
                    #pragma omp atomic update
                    posscol2[offs + neicol / BPI] &= ~((unsigned int)MSB >> (neicol % BPI));
                  }
                }
              }
            } else {
              done = false;
              const int neimincol = neidata & Mask;
              const int neimaxcol = neimincol + neirange;
              if ((neimincol <= mincol) && (neimaxcol >= mincol)) shortcut = false;
            }
          }
          int val = pcol;
          int mc = 0;
          if (pcol == 0) {
            const int offs = beg / BPI;
            mc = std::max(1, mincol / BPI) - 1;
            do {
              mc++;
              #pragma omp atomic read
              val = posscol2[offs + mc];
            } while (val == 0);
          }
          int newmincol = mc * BPI + __builtin_clz(val);
          if (mincol != newmincol) shortcut = false;
          if (shortcut || done) {
            pcol = (newmincol < BPI) ? ((unsigned int)MSB >> newmincol) : 0;
          } else {
            const int maxcol = mincol + range;
            const int range = maxcol - newmincol;
            newmincol = (range << (BPI / 2)) | newmincol;
            again = true;
          }
          posscol[v] = pcol;
          #pragma omp atomic write
          color[v] = newmincol;
        }
      }
    } while (again);
  }
}


void runSmall(const int nodes, const int* const __restrict__ nidx, const int* const __restrict__ nlist, volatile int* const __restrict__ posscol, int* const __restrict__ color, const int threads)
{
  bool again;
  #pragma omp parallel num_threads(threads) default(none) shared(nodes, nidx, nlist, color, posscol) private(again)
  do {
    again = false;
    #pragma omp for nowait
    for (int v = 0; v < nodes; v++) {
      int pcol;
      #pragma omp atomic read
      pcol = posscol[v];
      if (__builtin_popcount(pcol) > 1) {
        const int beg = nidx[v];
        int active = color[v];
        int allnei = 0;
        int keep = active;
        do {
          const int old = active;
          active &= active - 1;
          const int curr = old ^ active;
          const int i = beg + __builtin_clz(curr);
          const int nei = nlist[i];
          int neipcol;  // const
          #pragma omp atomic read
          neipcol = posscol[nei];
          allnei |= neipcol;
          if ((pcol & neipcol) == 0) {
            pcol &= pcol - 1;
            keep ^= curr;
          } else if (__builtin_popcount(neipcol) == 1) {
            pcol ^= neipcol;
            keep ^= curr;
          }
        } while (active != 0);
        if (keep != 0) {
          const int best = (unsigned int)MSB >> __builtin_clz(pcol);
          if ((best & ~allnei) != 0) {
            pcol = best;
            keep = 0;
          }
        }
        again |= keep;
        if (keep == 0) keep = __builtin_clz(pcol);
        color[v] = keep;
        #pragma omp atomic write
        posscol[v] = pcol;
      }
    }
  } while (again);
}


struct CPUTimer
{
  timeval beg, end;
  void start() {gettimeofday(&beg, NULL);}
  float stop() {gettimeofday(&end, NULL); return end.tv_sec - beg.tv_sec + (end.tv_usec - beg.tv_usec) * 0.000001f;}
};


int main(int argc, char* argv[])
{
  printf("ECL-GC OpenMP v1.2 (%s)\n", __FILE__);
  printf("Copyright 2020 Texas State University\n\n");

  if (argc != 3) {printf("USAGE: %s input_file_name thread_count\n\n", argv[0]);  exit(-1);}
  if (BPI != sizeof(int) * 8) {printf("ERROR: bits per int size must be %ld\n\n", sizeof(int) * 8);  exit(-1);}
  const int threads = atoi(argv[2]);
  if (threads < 1) {fprintf(stderr, "ERROR: thread_count must be at least 1\n"); exit(-1);}

  ECLgraph g = readECLgraph(argv[1]);
  printf("input: %s\n", argv[1]);
  printf("nodes: %d\n", g.nodes);
  printf("edges: %d\n", g.edges);
  printf("avg degree: %.2f\n", 1.0 * g.edges / g.nodes);

  int* const color = new int [g.nodes];
  int* const nlist2 = new int [g.edges];
  int* const posscol = new int [g.nodes];
  int* const posscol2 = new int [g.edges / BPI + 1];
  int* const wl = new int [g.nodes];

  CPUTimer timer;
  timer.start();
  const int wlsize = init(g.nodes, g.edges, g.nindex, g.nlist, nlist2, posscol, posscol2, color, wl, threads);
  runLarge(g.nindex, nlist2, posscol, posscol2, color, wl, wlsize, threads);
  runSmall(g.nodes, g.nindex, g.nlist, posscol, color, threads);
  const float runtime = timer.stop();

  printf("runtime:    %.6f s\n", runtime);
  printf("throughput: %.6f Mnodes/s\n", g.nodes * 0.000001 / runtime);
  printf("throughput: %.6f Medges/s\n", g.edges * 0.000001 / runtime);

  for (int v = 0; v < g.nodes; v++) {
    if (color[v] < 0) {printf("ERROR: found unprocessed node in graph (node %d with deg %d)\n\n", v, g.nindex[v + 1] - g.nindex[v]);  exit(-1);}
    for (int i = g.nindex[v]; i < g.nindex[v + 1]; i++) {
      if (color[g.nlist[i]] == color[v]) {printf("ERROR: found adjacent nodes with same color %d (%d %d)\n\n", color[v], v, g.nlist[i]);  exit(-1);}
    }
  }
  printf("result verification passed\n");

  const int vals = 16;
  int c[vals];
  for (int i = 0; i < vals; i++) c[i] = 0;
  int cols = -1;
  for (int v = 0; v < g.nodes; v++) {
    cols = std::max(cols, color[v]);
    if (color[v] < vals) c[color[v]]++;
  }
  cols++;
  printf("colors used: %d\n", cols);

  int sum = 0;
  for (int i = 0; i < std::min(vals, cols); i++) {
    sum += c[i];
    printf("col %2d: %10d (%5.1f%%)\n", i, c[i], 100.0 * sum / g.nodes);
  }

  delete [] color;
  delete [] nlist2;
  delete [] posscol;
  delete [] posscol2;
  delete [] wl;
  freeECLgraph(g);
  return 0;
}

