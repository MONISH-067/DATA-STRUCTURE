#define _POSIX_C_SOURCE 199309L
/* Shared helpers for Part B: random connected graphs in CSR form, Dijkstra (array and binary heap), Floyd-Warshall */
#ifndef GRAPHLIB_H
#define GRAPHLIB_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#define UNUSED __attribute__((unused))
#define INFL 0x3f3f3f3f3f3f3f3fLL

static unsigned long long rs_ = 88172645463325252ULL;
static unsigned long long rnd_(void) { rs_ ^= rs_ << 13; rs_ ^= rs_ >> 7; rs_ ^= rs_ << 17; return rs_; }
UNUSED static double now_(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (double)t.tv_sec + (double)t.tv_nsec * 1e-9; }

typedef struct { int V, E2; int *start, *to, *w; } CSR;   /* E2 = adjacency entries = 2 x undirected edges */

/* connected undirected graph: random tree + extra random edges; weights 1..100 */
UNUSED static CSR make_graph(int V, double edges_per_vertex) {
    long E = (long)(edges_per_vertex * V); if (E < V - 1) E = V - 1;
    int *eu = malloc((size_t)E * sizeof(int)), *ev = malloc((size_t)E * sizeof(int)), *ew = malloc((size_t)E * sizeof(int));
    for (int i = 1; i < V; i++) { eu[i - 1] = i; ev[i - 1] = (int)(rnd_() % (unsigned)i); ew[i - 1] = 1 + (int)(rnd_() % 100); }
    for (long e = V - 1; e < E; e++) { eu[e] = (int)(rnd_() % (unsigned)V); ev[e] = (int)(rnd_() % (unsigned)V); if (eu[e] == ev[e]) ev[e] = (ev[e] + 1) % V; ew[e] = 1 + (int)(rnd_() % 100); }
    CSR g; g.V = V; g.E2 = (int)(2 * E);
    g.start = calloc((size_t)V + 2, sizeof(int)); g.to = malloc((size_t)g.E2 * sizeof(int)); g.w = malloc((size_t)g.E2 * sizeof(int));
    for (long e = 0; e < E; e++) { g.start[eu[e] + 1]++; g.start[ev[e] + 1]++; }
    for (int v = 0; v < V; v++) g.start[v + 1] += g.start[v];
    int *pos = malloc((size_t)V * sizeof(int)); memcpy(pos, g.start, (size_t)V * sizeof(int));
    for (long e = 0; e < E; e++) { g.to[pos[eu[e]]] = ev[e]; g.w[pos[eu[e]]++] = ew[e]; g.to[pos[ev[e]]] = eu[e]; g.w[pos[ev[e]]++] = ew[e]; }
    free(pos); free(eu); free(ev); free(ew);
    return g;
}
UNUSED static void free_graph(CSR *g) { free(g->start); free(g->to); free(g->w); }
UNUSED static size_t csr_bytes(const CSR *g) { return (size_t)(g->V + 1) * sizeof(int) + 2 * (size_t)g->E2 * sizeof(int); }

/* Dijkstra with an array (O(V^2)) */
UNUSED static void dijkstra_array(const CSR *g, int src, long long *dist) {
    int V = g->V; char *done = calloc((size_t)V, 1);
    for (int v = 0; v < V; v++) dist[v] = INFL;
    dist[src] = 0;
    for (int r = 0; r < V; r++) {
        int u = -1; for (int v = 0; v < V; v++) if (!done[v] && dist[v] < INFL && (u < 0 || dist[v] < dist[u])) u = v;
        if (u < 0) break;
        done[u] = 1;
        for (int i = g->start[u]; i < g->start[u + 1]; i++) { int v = g->to[i]; if (!done[v] && dist[u] + g->w[i] < dist[v]) dist[v] = dist[u] + g->w[i]; }
    }
    free(done);
}
/* binary min-heap of (key, vertex), lazy deletion */
typedef struct { long long k; int v; } HN;
typedef struct { HN *a; int n, cap; } Heap;
static void hpush(Heap *h, long long k, int v) {
    if (h->n == h->cap) { h->cap = h->cap ? 2 * h->cap : 1024; h->a = realloc(h->a, (size_t)h->cap * sizeof(HN)); }
    int i = h->n++; h->a[i] = (HN){k, v};
    while (i > 0 && h->a[(i - 1) / 2].k > h->a[i].k) { HN t = h->a[i]; h->a[i] = h->a[(i - 1) / 2]; h->a[(i - 1) / 2] = t; i = (i - 1) / 2; }
}
static HN hpop(Heap *h) {
    HN top = h->a[0]; h->a[0] = h->a[--h->n]; int i = 0;
    for (;;) { int l = 2 * i + 1, r = l + 1, s = i;
        if (l < h->n && h->a[l].k < h->a[s].k) s = l;
        if (r < h->n && h->a[r].k < h->a[s].k) s = r;
        if (s == i) break;
        HN t = h->a[i]; h->a[i] = h->a[s]; h->a[s] = t; i = s; }
    return top;
}
/* returns number of vertices settled; prev may be NULL */
UNUSED static int dijkstra_heap(const CSR *g, int src, long long *dist, int *prev, Heap *h) {
    int V = g->V, settled = 0;
    for (int v = 0; v < V; v++) { dist[v] = INFL; if (prev) prev[v] = -1; }
    dist[src] = 0; h->n = 0; hpush(h, 0, src);
    while (h->n) { HN t = hpop(h); if (t.k > dist[t.v]) continue; settled++; int u = t.v;
        for (int i = g->start[u]; i < g->start[u + 1]; i++) { int v = g->to[i]; long long nd = dist[u] + g->w[i];
            if (nd < dist[v]) { dist[v] = nd; if (prev) prev[v] = u; hpush(h, nd, v); } } }
    return settled;
}
UNUSED static void floyd(int V, int *d) {   /* d: V*V ints, INT_MAX/4 = no road */
    for (int k = 0; k < V; k++) for (int i = 0; i < V; i++) { int dik = d[(size_t)i * V + k]; if (dik >= INT_MAX / 4) continue;
        for (int j = 0; j < V; j++) { int s = dik + d[(size_t)k * V + j]; if (s < d[(size_t)i * V + j]) d[(size_t)i * V + j] = s; } }
}
#endif
