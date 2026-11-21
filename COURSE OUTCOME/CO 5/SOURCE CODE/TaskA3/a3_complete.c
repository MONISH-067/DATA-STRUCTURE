#include <stdio.h>

#define V 6

/* Delivery-hub setup tasks; edge u -> v means u must finish before v. */
const char *task[V] = {"Lease", "Power", "Racks", "Network", "Staff", "Launch"};
int g[V][V] = {
    /* Lease */   {0, 1, 1, 0, 1, 0},
    /* Power */   {0, 0, 0, 1, 0, 0},
    /* Racks */   {0, 0, 0, 1, 0, 0},
    /* Network */ {0, 0, 0, 0, 0, 1},
    /* Staff */   {0, 0, 0, 0, 0, 1},
    /* Launch */  {0, 0, 0, 0, 0, 0},
};

int main(void)
{
    int indeg[V] = {0}, queue[V], head = 0, tail = 0, done = 0;
    for (int u = 0; u < V; u++)
        for (int v = 0; v < V; v++) indeg[v] += g[u][v];
    for (int v = 0; v < V; v++) if (indeg[v] == 0) queue[tail++] = v;
    /* TODO 1 done: Kahn's algorithm */
    while (head < tail) {
        int u = queue[head++];
        printf("%s ", task[u]);
        done++;
        for (int v = 0; v < V; v++)
            if (g[u][v] && --indeg[v] == 0) queue[tail++] = v;
    }

    printf("\n%s\n", done == V ? "valid schedule" : "cycle detected");
    return 0;
}
