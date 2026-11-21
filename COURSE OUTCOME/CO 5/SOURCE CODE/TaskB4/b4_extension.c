/* Task B4: Prim cable plan for 20 hubs (and when a route is unavailable), scalability at 250,000 junctions, precomputed table */
#include "graphlib.h"
#include <math.h>
#define H 20
static int cost[H][H], hx[H], hy[H];

static int prim(int blocked_u, int blocked_v, int *parent, int *key) {
    int in[H] = {0}, total = 0;
    for (int v = 0; v < H; v++) { key[v] = INT_MAX; parent[v] = -1; }
    key[0] = 0;
    for (int k = 0; k < H; k++) {
        int u = -1; for (int v = 0; v < H; v++) if (!in[v] && key[v] != INT_MAX && (u < 0 || key[v] < key[u])) u = v;
        if (u < 0) return -1;                       /* hubs cannot all be connected */
        in[u] = 1; total += key[u];
        for (int v = 0; v < H; v++) {
            if (in[v] || v == u) continue;
            if ((u == blocked_u && v == blocked_v) || (u == blocked_v && v == blocked_u)) continue;   /* route unavailable */
            if (cost[u][v] < key[v]) { key[v] = cost[u][v]; parent[v] = u; }
        }
    }
    return total;
}
typedef struct { int u, v, w; } E3;
static int cmpe(const void *a, const void *b) { return ((const E3 *)a)->w - ((const E3 *)b)->w; }
static int up[H]; static int fnd(int x) { return up[x] == x ? x : (up[x] = fnd(up[x])); }

int main(void) {
    printf("== B4.1 Prim's algorithm: connect 20 hubs with fibre (cost = straight-line km x 10, rounded) ==\n");
    rs_ = 2026; for (int i = 0; i < H; i++) { hx[i] = (int)(rnd_() % 100); hy[i] = (int)(rnd_() % 100); }
    for (int i = 0; i < H; i++) for (int j = 0; j < H; j++) cost[i][j] = (int)lround(10.0 * sqrt((double)((hx[i]-hx[j])*(hx[i]-hx[j]) + (hy[i]-hy[j])*(hy[i]-hy[j]))));
    int parent[H], key[H]; int total = prim(-1, -1, parent, key);
    printf("cable plan (parent-hub, cost):"); for (int v = 1; v < H; v++) printf(" %d-%d(%d)", parent[v], v, cost[parent[v]][v]);
    printf("\ntotal cost = %d  (%d cables for %d hubs)\n", total, H - 1, H);
    E3 e[H * H]; int m = 0; for (int i = 0; i < H; i++) for (int j = i + 1; j < H; j++) e[m++] = (E3){i, j, cost[i][j]};
    qsort(e, (size_t)m, sizeof e[0], cmpe); for (int i = 0; i < H; i++) up[i] = i; int kt = 0;
    for (int i = 0; i < m; i++) { int a = fnd(e[i].u), b = fnd(e[i].v); if (a != b) { up[a] = b; kt += e[i].w; } }
    printf("cross-check with Kruskal (union-find) total = %d -> %s\n", kt, kt == total ? "same cost" : "MISMATCH");
    int bu = 1, bw = -1; for (int v = 1; v < H; v++) if (cost[parent[v]][v] > bw) { bw = cost[parent[v]][v]; bu = v; }
    int ba = parent[bu], bb = bu; int par2[H], key2[H];
    int t2 = prim(ba, bb, par2, key2);
    printf("\nroute %d-%d (cost %d, the most expensive cable of the plan) becomes unavailable:\n", ba, bb, bw);
    printf("new plan:"); for (int v = 1; v < H; v++) printf(" %d-%d(%d)", par2[v], v, cost[par2[v]][v]);
    printf("\nnew total cost = %d (increase %d); cables that changed:", t2, t2 - total);
    for (int v = 1; v < H; v++) if (par2[v] != parent[v]) printf(" hub %d now via %d (was %d)", v, par2[v], parent[v]);
    printf("\n");

    printf("\n== B4.2 Scalability at 250,000 junctions (about 2.4 roads per junction) ==\n");
    int V = 250000; rs_ = 99; CSR g = make_graph(V, 2.4);
    long long *dist = malloc((size_t)V * sizeof(long long)); int *prev = malloc((size_t)V * sizeof(int)); Heap hp = {0};
    dijkstra_heap(&g, 0, dist, prev, &hp);
    double t0 = now_(); int runs = 20; for (int k = 0; k < runs; k++) dijkstra_heap(&g, (k * 12347) % V, dist, prev, &hp); double one = (now_() - t0) * 1e3 / runs;
    size_t mem = csr_bytes(&g) + (size_t)V * (sizeof(long long) + sizeof(int)) + (size_t)hp.cap * sizeof(HN);
    printf("graph: V = %d, undirected roads = %d, adjacency entries = %d\n", V, g.E2 / 2, g.E2);
    printf("Dijkstra with binary heap, one full single-source search (average of %d sources): %.2f ms\n", runs, one);
    printf("memory: CSR %.1f MB + dist/prev %.1f MB + heap array %.1f MB (peak, %d entries) = %.1f MB\n", csr_bytes(&g) / 1048576.0, (double)V * 12 / 1048576.0, (double)hp.cap * sizeof(HN) / 1048576.0, hp.cap, mem / 1048576.0);
    printf("adjacency matrix for the same map: %.1f GB at 1 byte/cell, %.0f GB at 4 bytes/cell -> not possible\n", (double)V * V / 1e9, (double)V * V * 4 / 1e9);
    printf("Floyd-Warshall all pairs: %.2e relaxations and %.0f GB for the table -> not possible (estimate at 1e9 relaxations per 0.36 s: %.0f days)\n", (double)V * V * V, (double)V * V * 4 / 1e9, (double)V * V * V / 1e9 * 0.36 / 86400);

    printf("\n== Alternative concept: precomputed table between 100 major junctions ==\n");
    int M = 100; int *major = malloc((size_t)M * sizeof(int)); for (int i = 0; i < M; i++) major[i] = (int)((long)i * (V / M));
    int *tab = malloc((size_t)M * M * sizeof(int)); int **pt = malloc((size_t)M * sizeof(int *));
    t0 = now_();
    for (int i = 0; i < M; i++) { dijkstra_heap(&g, major[i], dist, prev, &hp); pt[i] = malloc((size_t)V * sizeof(int)); memcpy(pt[i], prev, (size_t)V * sizeof(int));
        for (int j = 0; j < M; j++) tab[i * M + j] = (int)dist[major[j]]; }
    double pre = now_() - t0;
    printf("precompute: %d Dijkstra runs = %.2f s  (budget between traffic updates: 120 s -> %.1f%% of the interval, one core)\n", M, pre, pre / 120.0 * 100);
    printf("space: distance table %d x %d ints = %.0f KB; predecessor trees (to rebuild routes) %d x %d ints = %.1f MB\n", M, M, M * M * 4 / 1024.0, M, V, (double)M * V * 4 / 1048576.0);
    volatile long long sink = 0; int reps = 5000000; t0 = now_();
    for (int k = 0; k < reps; k++) sink += tab[(k % M) * M + ((k * 7) % M)];
    double lk = (now_() - t0) / reps * 1e9;
    printf("per request: table look-up %.1f ns  versus a fresh heap-Dijkstra search %.2f ms (%.0fx slower)\n", lk, one, one * 1e6 / lk);
    /* correctness spot check: table entry equals a fresh search */
    dijkstra_heap(&g, major[7], dist, prev, &hp); printf("spot check table[7][42] = %d, fresh search = %lld -> %s\n", tab[7 * M + 42], dist[major[42]], tab[7 * M + 42] == dist[major[42]] ? "equal" : "DIFFERENT");
    for (int i = 0; i < M; i++) free(pt[i]);
    free(pt); free(tab); free(major); free(dist); free(prev); free(hp.a); free_graph(&g);
    (void)sink; return 0;
}
