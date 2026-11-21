#include <stdio.h>
#include <limits.h>

#define V 5

/* Cost (Rs lakh) of laying fibre cable between villages (0 = not possible). */
int c[V][V] = {
    {0, 2, 0, 6, 0},
    {2, 0, 3, 8, 5},
    {0, 3, 0, 0, 7},
    {6, 8, 0, 0, 9},
    {0, 5, 7, 9, 0},
};

int main(void)
{
    int key[V], parent[V], in[V] = {0}, total = 0;
    for (int v = 0; v < V; v++) { key[v] = INT_MAX; parent[v] = -1; }
    key[0] = 0;
    for (int k = 0; k < V; k++) {
        /* TODO 1 done: Prim's algorithm */
        int u = -1;
        for (int v = 0; v < V; v++)
            if (!in[v] && (u == -1 || key[v] < key[u])) u = v;
        in[u] = 1;
        total += key[u];
        for (int v = 0; v < V; v++)
            if (!in[v] && c[u][v] > 0 && c[u][v] < key[v]) { key[v] = c[u][v]; parent[v] = u; }
    }
    for (int v = 1; v < V; v++) printf("%d-%d(%d) ", parent[v], v, c[parent[v]][v]);
    printf("\ntotal cost = %d\n", total);
    return 0;
}
