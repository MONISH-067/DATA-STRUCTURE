/* Task B1: BFS/DFS on an adjacency list, metro check, flood components, cycle detection, matrix vs list BFS cost */
#include "graphlib.h"
#define MV 7
static const char *nm[MV] = {"Airport","Guindy","Saidapet","Central","Egmore","Vadapalani","Koyambedu"};
static int st[MV + 1] = {0,1,4,6,8,10,12,14};
static int ad[14] = {1,0,2,5,1,3,2,4,3,6,1,6,4,5};

static void dfs_rec(const int *s, const int *a, int u, char *seen) { seen[u] = 1; printf("%s ", nm[u]); for (int i = s[u]; i < s[u+1]; i++) if (!seen[a[i]]) dfs_rec(s, a, a[i], seen); }

/* undirected graph as edge list with a "closed" flag; components by BFS over open roads */
typedef struct { int u, v, closed; } Road;
static int components(int V, Road *r, int m) {
    int *deg = calloc((size_t)V + 1, sizeof(int)), *s = calloc((size_t)V + 2, sizeof(int));
    for (int i = 0; i < m; i++) if (!r[i].closed) { deg[r[i].u]++; deg[r[i].v]++; }
    for (int v = 0; v < V; v++) s[v + 1] = s[v] + deg[v];
    int *a = malloc((size_t)(s[V] + 1) * sizeof(int)), *p = calloc((size_t)V, sizeof(int));
    for (int i = 0; i < m; i++) if (!r[i].closed) { a[s[r[i].u] + p[r[i].u]++] = r[i].v; a[s[r[i].v] + p[r[i].v]++] = r[i].u; }
    char *seen = calloc((size_t)V, 1); int *q = malloc((size_t)V * sizeof(int)), comps = 0;
    for (int x = 0; x < V; x++) if (!seen[x]) { comps++; int h = 0, t = 0; q[t++] = x; seen[x] = 1;
        while (h < t) { int u = q[h++]; for (int i = s[u]; i < s[u+1]; i++) if (!seen[a[i]]) { seen[a[i]] = 1; q[t++] = a[i]; } } }
    free(deg); free(s); free(a); free(p); free(seen); free(q); return comps;
}
static int has_cycle_dfs(int V, Road *r, int m);
static int cyc_rec(int u, int par, int *s, int *a, char *seen) {
    seen[u] = 1;
    for (int i = s[u]; i < s[u+1]; i++) { int v = a[i]; if (v == par) continue; if (seen[v]) return 1; if (cyc_rec(v, u, s, a, seen)) return 1; }
    return 0;
}
static int has_cycle_dfs(int V, Road *r, int m) {
    int *deg = calloc((size_t)V, sizeof(int)), *s = calloc((size_t)V + 1, sizeof(int)), *p = calloc((size_t)V, sizeof(int));
    for (int i = 0; i < m; i++) if (!r[i].closed) { deg[r[i].u]++; deg[r[i].v]++; }
    for (int v = 0; v < V; v++) s[v + 1] = s[v] + deg[v];
    int *a = malloc((size_t)(s[V] + 1) * sizeof(int)); char *seen = calloc((size_t)V, 1);
    for (int i = 0; i < m; i++) if (!r[i].closed) { a[s[r[i].u] + p[r[i].u]++] = r[i].v; a[s[r[i].v] + p[r[i].v]++] = r[i].u; }
    int c = 0; for (int x = 0; x < V && !c; x++) if (!seen[x]) c = cyc_rec(x, -1, s, a, seen);
    free(deg); free(s); free(p); free(a); free(seen); return c;
}

/* BFS on a byte adjacency matrix and on the CSR list; returns reached count */
static int bfs_matrix(const unsigned char *m, int V, int src, int *q, int *dist) {
    for (int v = 0; v < V; v++) dist[v] = -1;
    int h = 0, t = 0, c = 0; q[t++] = src; dist[src] = 0;
    while (h < t) { int u = q[h++]; c++; const unsigned char *row = m + (size_t)u * V; for (int v = 0; v < V; v++) if (row[v] && dist[v] < 0) { dist[v] = dist[u] + 1; q[t++] = v; } }
    return c;
}
static int bfs_list(const CSR *g, int src, int *q, int *dist) {
    for (int v = 0; v < g->V; v++) dist[v] = -1;
    int h = 0, t = 0, c = 0; q[t++] = src; dist[src] = 0;
    while (h < t) { int u = q[h++]; c++; for (int i = g->start[u]; i < g->start[u+1]; i++) { int v = g->to[i]; if (dist[v] < 0) { dist[v] = dist[u] + 1; q[t++] = v; } } }
    return c;
}

int main(void) {
    printf("== B1.1 Metro network: BFS and DFS on the adjacency list (CSR) ==\n");
    int dist[MV], par[MV], q[MV], h = 0, t = 0; for (int v = 0; v < MV; v++) dist[v] = -1; dist[0] = 0; par[0] = -1; q[t++] = 0;
    printf("BFS: "); while (h < t) { int u = q[h++]; printf("%s ", nm[u]); for (int i = st[u]; i < st[u+1]; i++) if (dist[ad[i]] < 0) { dist[ad[i]] = dist[u] + 1; par[ad[i]] = u; q[t++] = ad[i]; } }
    printf("\nstops from Airport: "); for (int v = 0; v < MV; v++) printf("%s=%d ", nm[v], dist[v]);
    printf("\nroute to Koyambedu (hand check: Airport-Guindy-Vadapalani-Koyambedu = 3 stops, the only 3-stop route; the other way, Airport-Guindy-Saidapet-Central-Egmore-Koyambedu, has 5):");
    for (int v = 6; v != -1; v = par[v]) printf(" %s", nm[v]);
    char seen[MV] = {0}; printf("\nDFS: "); dfs_rec(st, ad, 0, seen); printf("\n");

    printf("\n== B1.3 Connected regions after flooding, and cycle detection ==\n");
    /* 12 junctions: two blocks joined by roads 3-4 and 7-8 */
    Road r[] = {{0,1,0},{1,2,0},{2,3,0},{3,0,0},{3,4,0},{4,5,0},{5,6,0},{6,7,0},{7,4,0},{7,8,0},{8,9,0},{9,10,0},{10,11,0}};
    int m = (int)(sizeof r / sizeof r[0]), V = 12;
    printf("all roads open: %d region(s), cycle: %s\n", components(V, r, m), has_cycle_dfs(V, r, m) ? "yes" : "no");
    r[4].closed = 1; /* 3-4 */
    printf("flood closes 3-4:      %d region(s) (junctions 0-3 are cut off from the rest)\n", components(V, r, m));
    r[9].closed = 1; /* 7-8 */
    printf("flood also closes 7-8: %d region(s) (blocks 0-3, 4-7, 8-11)\n", components(V, r, m));
    r[11].closed = 1; /* 9-10 */
    printf("flood also closes 9-10: %d region(s)  (vertices 10,11 cut off from 8,9)\n", components(V, r, m));
    Road tree[] = {{0,1,0},{0,2,0},{1,3,0},{1,4,0}};
    printf("tree with 5 junctions (4 roads): cycle = %s;  after adding road 3-4: cycle = ", has_cycle_dfs(5, tree, 4) ? "yes" : "no");
    Road tc[] = {{0,1,0},{0,2,0},{1,3,0},{1,4,0},{3,4,0}}; printf("%s\n", has_cycle_dfs(5, tc, 5) ? "yes" : "no");

    printf("\n== B1.4 BFS time and memory: adjacency matrix vs adjacency list (2.4 roads per junction) ==\n");
    printf("%-8s %-16s %-14s %-16s %-14s\n", "V", "matrix bytes", "matrix BFS ms", "list bytes", "list BFS ms");
    int Vs[] = {1000, 10000, 50000};
    for (int z = 0; z < 3; z++) {
        int n = Vs[z]; CSR g = make_graph(n, 2.4);
        unsigned char *mx = calloc((size_t)n * n, 1); if (!mx) { printf("%-8d matrix allocation failed\n", n); continue; }
        for (int u = 0; u < n; u++) for (int i = g.start[u]; i < g.start[u+1]; i++) mx[(size_t)u * n + g.to[i]] = 1;
        int *qq = malloc((size_t)n * sizeof(int)), *dd = malloc((size_t)n * sizeof(int));
        int reps = n <= 1000 ? 200 : (n <= 10000 ? 5 : 1); volatile int sink = 0;
        double t0 = now_(); for (int k = 0; k < reps; k++) sink += bfs_matrix(mx, n, 0, qq, dd); double tm = (now_() - t0) * 1e3 / reps;
        int reps2 = n <= 1000 ? 20000 : (n <= 10000 ? 2000 : 200);
        t0 = now_(); for (int k = 0; k < reps2; k++) sink += bfs_list(&g, 0, qq, dd); double tl = (now_() - t0) * 1e3 / reps2;
        printf("%-8d %-16zu %-14.3f %-16zu %-14.4f  (reached %d, matrix/list time = %.0fx)\n", n, (size_t)n * n, tm, csr_bytes(&g), tl, bfs_list(&g, 0, qq, dd), tm / tl);
        free(mx); free(qq); free(dd); free_graph(&g); (void)sink;
    }
    printf("(matrix bytes = V*V at 1 byte per cell; with int cells multiply by 4. List = CSR arrays with 4-byte ints.)\n");
    return 0;
}
