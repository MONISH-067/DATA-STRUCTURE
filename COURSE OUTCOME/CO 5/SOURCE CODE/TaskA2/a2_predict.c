#include <stdio.h>

#define V 6

int g[V][V] = {
    {0, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 0},
    {1, 0, 0, 0, 1, 0},
    {0, 1, 0, 0, 0, 1},
    {0, 1, 1, 0, 0, 1},
    {0, 0, 0, 1, 1, 0},
};
int seen[V];

void dfs(int u)
{
    seen[u] = 1;
    printf("%d ", u);
    for (int v = 0; v < V; v++)
        if (g[u][v] && !seen[v]) dfs(v);
}

int main(void)
{
    dfs(0);
    printf("\n");
    return 0;
}
