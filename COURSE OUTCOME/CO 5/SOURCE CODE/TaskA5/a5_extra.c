/* A5: Kruskal with union-find on the same graph, and tree path vs shortest path between villages 4 and 2 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#define V 5
int c[V][V] = {{0,2,0,6,0},{2,0,3,8,5},{0,3,0,0,7},{6,8,0,0,9},{0,5,7,9,0}};
typedef struct { int u, v, w; } E;
int cmp(const void *a, const void *b) { return ((const E *)a)->w - ((const E *)b)->w; }
int par[V]; int find(int x) { return par[x] == x ? x : (par[x] = find(par[x])); }
int main(void) {
    E e[16]; int m = 0;
    for (int i = 0; i < V; i++) for (int j = i + 1; j < V; j++) if (c[i][j]) e[m++] = (E){i, j, c[i][j]};
    qsort(e, (size_t)m, sizeof e[0], cmp);
    for (int i = 0; i < V; i++) par[i] = i;
    int tot = 0; printf("Kruskal: ");
    for (int i = 0; i < m; i++) { int a = find(e[i].u), b = find(e[i].v); if (a != b) { par[a] = b; tot += e[i].w; printf("%d-%d(%d) ", e[i].u, e[i].v, e[i].w); } }
    printf("\ntotal cost = %d\n", tot);
    int d[V]; int done[V] = {0}; for (int i = 0; i < V; i++) d[i] = INT_MAX; d[4] = 0;
    for (int r = 0; r < V; r++) { int u = -1; for (int v = 0; v < V; v++) if (!done[v] && d[v] != INT_MAX && (u < 0 || d[v] < d[u])) u = v;
        done[u] = 1; for (int v = 0; v < V; v++) if (c[u][v] && d[u] + c[u][v] < d[v]) d[v] = d[u] + c[u][v]; }
    printf("route 4 -> 2 through the MST: 4-1 (5) + 1-2 (3) = 8\n");
    printf("cheapest route 4 -> 2 in the full graph (Dijkstra): %d (direct road 4-2)\n", d[2]);
    return 0;
}
