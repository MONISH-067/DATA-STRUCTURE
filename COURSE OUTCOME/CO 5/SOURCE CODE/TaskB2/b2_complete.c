#include <stdio.h>
#include <limits.h>

#define V 6
#define INF INT_MAX

/* Travel time in minutes between junctions of a city map (0 = no road). */
int w[V][V] = {
    {0, 7, 9, 0, 0, 14},
    {7, 0, 10, 15, 0, 0},
    {9, 10, 0, 11, 0, 2},
    {0, 15, 11, 0, 6, 0},
    {0, 0, 0, 6, 0, 9},
    {14, 0, 2, 0, 9, 0},
};

int main(void)
{
    int dist[V], prev[V], done[V] = {0};
    for (int v = 0; v < V; v++) { dist[v] = INF; prev[v] = -1; }
    dist[0] = 0;
    for (int round = 0; round < V; round++) {
        int u = -1;
        for (int v = 0; v < V; v++)
            if (!done[v] && dist[v] != INF && (u == -1 || dist[v] < dist[u])) u = v;
        if (u == -1) break;
        done[u] = 1;
        /* TODO 1 done: relax every road u -> v */
        for (int v = 0; v < V; v++)
            if (w[u][v] > 0 && !done[v] && dist[u] + w[u][v] < dist[v]) {
                dist[v] = dist[u] + w[u][v];
                prev[v] = u;
            }
    }
    for (int v = 0; v < V; v++) printf("%d ", dist[v]);
    printf("\nroute to 4:");
    for (int v = 4; v != -1; v = prev[v]) printf(" %d", v);
    printf("\n");
    return 0;
}
