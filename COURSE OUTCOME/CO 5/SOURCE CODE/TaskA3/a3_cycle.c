/* A3: cycle test (Launch -> Lease added) with Kahn and with DFS finishing order; also the DFS version on the valid DAG */
#include <stdio.h>
#define V 6
const char *task[V] = {"Lease", "Power", "Racks", "Network", "Staff", "Launch"};
int g[V][V];
static const int base[V][V] = {{0,1,1,0,1,0},{0,0,0,1,0,0},{0,0,0,1,0,0},{0,0,0,0,0,1},{0,0,0,0,0,1},{0,0,0,0,0,0}};
int color[V], order[V], on;        /* 0 white, 1 grey (on stack), 2 black */
int cyc;
void dfs(int u) {
    color[u] = 1;
    for (int v = 0; v < V; v++) if (g[u][v]) { if (color[v] == 1) cyc = 1; else if (color[v] == 0) dfs(v); }
    color[u] = 2; order[on++] = u;
}
void run(const char *title) {
    printf("== %s ==\n", title);
    int indeg[V] = {0}, q[V], h = 0, t = 0, done = 0;
    for (int u = 0; u < V; u++) for (int v = 0; v < V; v++) indeg[v] += g[u][v];
    for (int v = 0; v < V; v++) if (!indeg[v]) q[t++] = v;
    printf("Kahn: ");
    while (h < t) { int u = q[h++]; printf("%s ", task[u]); done++; for (int v = 0; v < V; v++) if (g[u][v] && --indeg[v] == 0) q[t++] = v; }
    printf("\n  -> %s (processed %d of %d)\n", done == V ? "valid schedule" : "cycle detected", done, V);
    for (int i = 0; i < V; i++) color[i] = 0;
    on = 0; cyc = 0;
    for (int u = 0; u < V; u++) if (!color[u]) dfs(u);
    printf("DFS (reverse finishing order): ");
    if (cyc) printf("(back edge found, no order)\n  -> cycle detected\n");
    else { for (int i = V - 1; i >= 0; i--) printf("%s ", task[order[i]]); printf("\n  -> valid schedule\n"); }
}
int main(void) {
    for (int i = 0; i < V; i++) for (int j = 0; j < V; j++) g[i][j] = base[i][j];
    run("original DAG");
    g[5][0] = 1;
    run("with edge Launch -> Lease");
    return 0;
}
