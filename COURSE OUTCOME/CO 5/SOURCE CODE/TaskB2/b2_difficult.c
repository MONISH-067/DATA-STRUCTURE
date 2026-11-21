/* Task B2: engineered difficult inputs for Dijkstra, Floyd-Warshall, Bellman-Ford and topological sort */
#include <stdio.h>
#include <limits.h>
#include <stdint.h>

#define N 5
#define NOEDGE 0x7fffffff

typedef struct { int u, v, w; } Edge;

/* Dijkstra on an edge list (directed); returns dist with INF marking unreachable; long long arithmetic and an INF guard */
static void dijkstra(int n, const Edge *e, int m, int src, long long *dist, int safe)
{
    const long long INF = (long long)4e18;
    int done[N] = {0};
    for (int v = 0; v < n; v++) dist[v] = INF;
    dist[src] = 0;
    for (int r = 0; r < n; r++) {
        int u = -1;
        for (int v = 0; v < n; v++) if (!done[v] && (!safe || dist[v] != INF) && (u < 0 || dist[v] < dist[u])) u = v;
        if (u < 0 || dist[u] == INF) break;
        done[u] = 1;
        for (int i = 0; i < m; i++)
            if (e[i].u == u && !done[e[i].v] && dist[u] + e[i].w < dist[e[i].v]) dist[e[i].v] = dist[u] + e[i].w;
    }
}
static void show(const char *label, int n, const long long *d)
{
    printf("%s", label);
    for (int v = 0; v < n; v++) { if (d[v] >= (long long)4e18) printf(" %d:unreachable", v); else printf(" %d:%lld", v, d[v]); }
    printf("\n");
}

int main(void)
{
    long long d[N];

    printf("== Test 1: unreachable destination ==\n");
    Edge t1[] = {{0,1,4},{1,2,3},{3,4,2}};            /* junctions 3,4 form an island */
    dijkstra(5, t1, 3, 0, d, 1);
    show("source 0 (island 3-4 not connected):", 5, d);
    printf("destination 4 is %s\n", d[4] >= (long long)4e18 ? "UNREACHABLE (reported, not a distance)" : "reachable");

    printf("\n== Test 2: one negative edge (directed): 0->1 (4), 0->2 (5), 2->1 (-3) ==\n");
    Edge t2[] = {{0,1,4},{0,2,5},{2,1,-3}};
    dijkstra(3, t2, 3, 0, d, 1);
    show("Dijkstra  :", 3, d);
    int f[3][3] = {{0,4,5},{NOEDGE,0,NOEDGE},{NOEDGE,-3,0}};
    for (int k = 0; k < 3; k++) for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++)
        if (f[i][k] != NOEDGE && f[k][j] != NOEDGE && f[i][k] + f[k][j] < f[i][j]) f[i][j] = f[i][k] + f[k][j];
    printf("Floyd     : 0:%d 1:%d 2:%d   (true shortest 0->1 = 5 + (-3) = 2)\n", f[0][0], f[0][1], f[0][2]);
    long long bf[3] = {0, (long long)4e18, (long long)4e18};
    for (int pass = 0; pass < 2; pass++) for (int i = 0; i < 3; i++) if (bf[t2[i].u] < (long long)4e18 && bf[t2[i].u] + t2[i].w < bf[t2[i].v]) bf[t2[i].v] = bf[t2[i].u] + t2[i].w;
    printf("Bellman-Ford: 0:%lld 1:%lld 2:%lld\n", bf[0], bf[1], bf[2]);
    printf("Dijkstra gives 4 for junction 1 (WRONG); Floyd and Bellman-Ford give 2.\n");
    printf("negative-cycle check (Bellman-Ford, extra pass): ");
    Edge t2c[] = {{0,1,1},{1,2,-3},{2,1,1}};         /* cycle 1->2->1 costs -2 */
    long long b2[3] = {0, (long long)4e18, (long long)4e18};
    for (int pass = 0; pass < 2; pass++) for (int i = 0; i < 3; i++) if (b2[t2c[i].u] < (long long)4e18 && b2[t2c[i].u] + t2c[i].w < b2[t2c[i].v]) b2[t2c[i].v] = b2[t2c[i].u] + t2c[i].w;
    int neg = 0; for (int i = 0; i < 3; i++) if (b2[t2c[i].u] + t2c[i].w < b2[t2c[i].v]) neg = 1;
    printf("graph with cycle 1->2->1 of cost -2: %s\n", neg ? "negative cycle DETECTED (a V-th pass still relaxes an edge)" : "none");

    printf("\n== Test 3: travel times so large that dist[u] + w overflows int ==\n");
    Edge t3[] = {{0,1,2000000000},{1,2,2000000000}};
    int a = 2000000000, b = 2000000000;
    int wrapped = (int)((unsigned)a + (unsigned)b);   /* what 32-bit int addition produces on this machine (wrap-around; signed overflow is undefined in C) */
    printf("int  : 2000000000 + 2000000000 = %d   (wrapped to a NEGATIVE time, so 'dist[u]+w < dist[v]' becomes true for the wrong reason)\n", wrapped);
    printf("INF  : INT_MAX + 5 as int = %d (wraps negative)\n", (int)((unsigned)INT_MAX + 5u));
    dijkstra(3, t3, 2, 0, d, 1);
    show("long long + INF guard:", 3, d);
    printf("fix: (1) 64-bit distances, (2) never relax from a vertex whose dist is INF (select only dist != INF), (3) bound weights.\n");

    printf("\n== Test 4: topological sort on a task graph with a cycle ==\n");
    int g[4][4] = {{0,1,0,0},{0,0,1,0},{0,0,0,1},{0,1,0,0}};   /* 0->1->2->3->1 : cycle 1,2,3 */
    int indeg[4] = {0}, q[4], h = 0, t = 0, done = 0;
    for (int u = 0; u < 4; u++) for (int v = 0; v < 4; v++) indeg[v] += g[u][v];
    for (int v = 0; v < 4; v++) if (!indeg[v]) q[t++] = v;
    printf("Kahn order: ");
    while (h < t) { int u = q[h++]; printf("%d ", u); done++; for (int v = 0; v < 4; v++) if (g[u][v] && --indeg[v] == 0) q[t++] = v; }
    printf("\nprocessed %d of 4 -> %s\n", done, done == 4 ? "valid schedule" : "cycle detected (tasks 1,2,3 never reach in-degree 0)");
    return 0;
}
