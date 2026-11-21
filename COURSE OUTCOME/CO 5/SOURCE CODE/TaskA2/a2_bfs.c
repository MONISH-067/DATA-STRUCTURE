/* A2.3 trace support: BFS order from 0 on the same graph, queue printed after every removal */
#include <stdio.h>
#define V 6
int g[V][V] = {{0,1,1,0,0,0},{1,0,0,1,1,0},{1,0,0,0,1,0},{0,1,0,0,0,1},{0,1,1,0,0,1},{0,0,0,1,1,0}};
int main(void)
{
    int seen[V] = {0}, q[V], head = 0, tail = 0;
    q[tail++] = 0; seen[0] = 1;
    printf("BFS order: ");
    while (head < tail) {
        int u = q[head++]; printf("%d ", u);
        for (int v = 0; v < V; v++) if (g[u][v] && !seen[v]) { seen[v] = 1; q[tail++] = v; }
    }
    printf("\n\nqueue after each removal:\n");
    int seen2[V] = {0}; head = tail = 0; q[tail++] = 0; seen2[0] = 1;
    while (head < tail) {
        int u = q[head++];
        for (int v = 0; v < V; v++) if (g[u][v] && !seen2[v]) { seen2[v] = 1; q[tail++] = v; }
        printf("remove %d -> [", u);
        for (int i = head; i < tail; i++) printf("%s%d", i > head ? " " : "", q[i]);
        printf("]\n");
    }
    /* DFS with an explicit stack (alternative concept), same graph */
    int st[V * V], sp = 0, seen3[V] = {0};
    st[sp++] = 0; printf("\nexplicit-stack DFS order: ");
    while (sp) {
        int u = st[--sp]; if (seen3[u]) continue; seen3[u] = 1; printf("%d ", u);
        for (int v = V - 1; v >= 0; v--) if (g[u][v] && !seen3[v]) st[sp++] = v;
    }
    printf("\n");
    return 0;
}
