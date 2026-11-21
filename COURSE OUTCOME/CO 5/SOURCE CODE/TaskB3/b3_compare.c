/* Task B3: Dijkstra (array), Dijkstra (binary heap), Floyd-Warshall - correctness, scaling; A* vs Dijkstra on grids */
#include "graphlib.h"
#include <math.h>

static void b31(void) {
    printf("== B3.1 Correctness on a graph of 8 vertices (all-pairs distances) ==\n");
    rs_ = 12345; CSR g = make_graph(8, 1.75);
    int V = 8, bad = 0; int *fw = malloc(64 * sizeof(int)); long long da[8], dh[8]; Heap h = {0};
    for (int i = 0; i < 64; i++) fw[i] = INT_MAX / 4;
    for (int v = 0; v < V; v++) fw[v * V + v] = 0;
    for (int u = 0; u < V; u++) for (int i = g.start[u]; i < g.start[u+1]; i++) if (g.w[i] < fw[u * V + g.to[i]]) fw[u * V + g.to[i]] = g.w[i];
    floyd(V, fw);
    printf("edges (u-v:w):"); for (int u = 0; u < V; u++) for (int i = g.start[u]; i < g.start[u+1]; i++) if (u < g.to[i]) printf(" %d-%d:%d", u, g.to[i], g.w[i]); printf("\n");
    printf("Floyd-Warshall matrix:\n");
    for (int i = 0; i < V; i++) { for (int j = 0; j < V; j++) printf("%4d", fw[i * V + j]); printf("\n"); }
    for (int s = 0; s < V; s++) { dijkstra_array(&g, s, da); dijkstra_heap(&g, s, dh, NULL, &h);
        for (int v = 0; v < V; v++) if (da[v] != dh[v] || da[v] != fw[s * V + v]) bad++; }
    printf("compared 8 x 8 = 64 distances across the three algorithms: %d disagreement(s) -> %s\n", bad, bad ? "FAIL" : "all three agree");
    free(fw); free(h.a); free_graph(&g);
}

static void b32(void) {
    printf("\n== B3.2 Time versus size (about 3 roads per vertex, weights 1..100) ==\n");
    printf("%-8s %-18s %-18s %-20s %-22s\n", "V", "Dijkstra array ms", "Dijkstra heap ms", "Floyd (all pairs) ms", "heap x V (all pairs) ms");
    int Vs[] = {100, 1000, 10000}; double fl1000 = 0, fl100 = 0;
    for (int z = 0; z < 3; z++) {
        int V = Vs[z]; rs_ = 777 + (unsigned)V; CSR g = make_graph(V, 3.0);
        long long *d = malloc((size_t)V * sizeof(long long)), *d2 = malloc((size_t)V * sizeof(long long)); Heap h = {0};
        int reps = V == 100 ? 2000 : (V == 1000 ? 50 : 3);
        double t0 = now_(); for (int k = 0; k < reps; k++) dijkstra_array(&g, k % V, d); double ta = (now_() - t0) * 1e3 / reps;
        int reps2 = V == 100 ? 20000 : (V == 1000 ? 2000 : 100);
        t0 = now_(); for (int k = 0; k < reps2; k++) dijkstra_heap(&g, k % V, d2, NULL, &h); double th = (now_() - t0) * 1e3 / reps2;
        dijkstra_array(&g, 0, d); dijkstra_heap(&g, 0, d2, NULL, &h); for (int v = 0; v < V; v++) if (d[v] != d2[v]) printf("MISMATCH at V=%d\n", V);
        char fb[40], ab[40];
        if (V <= 1000) {
            int *fw = malloc((size_t)V * V * sizeof(int)); for (size_t i = 0; i < (size_t)V * V; i++) fw[i] = INT_MAX / 4;
            for (int v = 0; v < V; v++) fw[(size_t)v * V + v] = 0;
            for (int u = 0; u < V; u++) for (int i = g.start[u]; i < g.start[u+1]; i++) if (g.w[i] < fw[(size_t)u * V + g.to[i]]) fw[(size_t)u * V + g.to[i]] = g.w[i];
            t0 = now_(); floyd(V, fw); double tf = (now_() - t0) * 1e3;
            /* cross-check one row */ for (int v = 0; v < V; v++) if (fw[v] != d2[v]) printf("FLOYD MISMATCH at V=%d\n", V);
            if (V == 100) fl100 = tf; else fl1000 = tf;
            snprintf(fb, sizeof fb, "%.2f", tf); snprintf(ab, sizeof ab, "%.2f", th * V); free(fw);
        } else {
            double est = fl1000 * 1000.0; snprintf(fb, sizeof fb, "%.0f (est.)", est); snprintf(ab, sizeof ab, "%.0f (est.)", th * V);
        }
        printf("%-8d %-18.4f %-18.4f %-20s %-22s\n", V, ta, th, fb, ab);
        free(d); free(d2); free(h.a); free_graph(&g);
    }
    printf("Floyd 10,000 is NOT run: 10^12 relaxations. Estimate = measured V=1,000 time x 1000 (cubic growth) = %.0f ms (about %.1f hours); matrix alone = 400 MB.\n", fl1000 * 1000, fl1000 * 1000 / 3.6e6);
    printf("growth Floyd 100 -> 1000: x%.0f (cubic predicts x1000)\n", fl1000 / fl100);
    printf("'heap x V' = single-source heap time x V (all pairs by running Dijkstra from every vertex); est. at 10,000.\n");
}

/* ---- A* on a grid ---- */
#define GW 500
#define GH 500
static unsigned char wall[GW * GH];
static int gdist[GW * GH]; static char closed_[GW * GH];
typedef struct { int f, idx; } GN;
static GN *gh; static int gn;
static void gpush(int f, int idx) { int i = gn++; gh[i] = (GN){f, idx}; while (i > 0 && gh[(i-1)/2].f > gh[i].f) { GN t = gh[i]; gh[i] = gh[(i-1)/2]; gh[(i-1)/2] = t; i = (i-1)/2; } }
static GN gpop(void) { GN top = gh[0]; gh[0] = gh[--gn]; int i = 0; for (;;) { int l = 2*i+1, r = l+1, s = i; if (l < gn && gh[l].f < gh[s].f) s = l; if (r < gn && gh[r].f < gh[s].f) s = r; if (s == i) break; GN t = gh[i]; gh[i] = gh[s]; gh[s] = t; i = s; } return top; }
/* returns path length (or -1); *explored = vertices settled; use_h = 0 -> Dijkstra, 1 -> Manhattan heuristic */
static int search(int sx, int sy, int tx, int ty, int use_h, long *explored) {
    for (int i = 0; i < GW * GH; i++) { gdist[i] = INT_MAX; closed_[i] = 0; }
    gn = 0; int s = sy * GW + sx, t = ty * GW + tx; gdist[s] = 0; gpush(use_h ? abs(sx - tx) + abs(sy - ty) : 0, s); *explored = 0;
    static const int dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
    while (gn) {
        GN n = gpop(); int u = n.idx; if (closed_[u]) continue; closed_[u] = 1; (*explored)++;
        if (u == t) return gdist[u];
        int ux = u % GW, uy = u / GW;
        for (int k = 0; k < 4; k++) { int vx = ux + dx[k], vy = uy + dy[k]; if (vx < 0 || vy < 0 || vx >= GW || vy >= GH) continue;
            int v = vy * GW + vx; if (wall[v] || closed_[v]) continue; int nd = gdist[u] + 1;
            if (nd < gdist[v]) { gdist[v] = nd; gpush(nd + (use_h ? abs(vx - tx) + abs(vy - ty) : 0), v); } }
    }
    return -1;
}
static void b33(void) {
    printf("\n== B3.3 A* (Manhattan heuristic) vs Dijkstra on a %d x %d grid, 4-neighbour moves, unit cost ==\n", GW, GH);
    gh = malloc(4 * (size_t)GW * GH * sizeof(GN));
    printf("%-30s %-9s %-18s %-14s %-12s %-14s\n", "map", "path len", "Dijkstra explored", "A* explored", "fewer by", "time ms D / A*");
    const char *names[] = {"open grid, no obstacles", "20% random obstacles", "30% random obstacles", "vertical wall with one gap"};
    for (int m = 0; m < 4; m++) {
        rs_ = 9001 + (unsigned)m; memset(wall, 0, sizeof wall);
        if (m == 1 || m == 2) for (int i = 0; i < GW * GH; i++) wall[i] = (rnd_() % 100) < (m == 1 ? 20u : 30u);
        if (m == 3) { for (int y = 0; y < GH; y++) wall[y * GW + 250] = 1; wall[400 * GW + 250] = 0; }
        for (int dy2 = 0; dy2 < 3; dy2++) for (int dx2 = 0; dx2 < 3; dx2++) { wall[dy2 * GW + dx2] = 0; wall[(GH - 1 - dy2) * GW + (GW - 1 - dx2)] = 0; }   /* keep start and goal free */
        long ed, ea; double t0 = now_(); int ld = search(0, 0, GW - 1, GH - 1, 0, &ed); double td = (now_() - t0) * 1e3;
        t0 = now_(); int la = search(0, 0, GW - 1, GH - 1, 1, &ea); double ta = (now_() - t0) * 1e3;
        printf("%-30s %-9d %-18ld %-14ld x%-11.1f %.1f / %.1f   %s\n", names[m], ld, ed, ea, ea ? (double)ed / (double)ea : 0.0, td, ta, ld == la ? "(same length: correct)" : "LENGTH MISMATCH");
    }
    free(gh);
}
int main(void) { b31(); b32(); b33(); return 0; }
