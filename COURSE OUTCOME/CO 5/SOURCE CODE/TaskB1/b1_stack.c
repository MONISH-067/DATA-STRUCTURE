/* A2 alternative concept: recursive DFS vs explicit-stack DFS on a chain of n junctions (worst case for depth) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int *start_, *adj_; static char *seen_; static int depth_, maxd_;
static void rec(int u) { seen_[u] = 1; if (++depth_ > maxd_) maxd_ = depth_; for (int i = start_[u]; i < start_[u+1]; i++) if (!seen_[adj_[i]]) rec(adj_[i]); depth_--; }
int main(int argc, char **argv) {
    int n = argc > 2 ? atoi(argv[2]) : 50000; int mode = argc > 1 && !strcmp(argv[1], "stack");
    start_ = malloc(((size_t)n + 1) * sizeof(int)); adj_ = malloc(2 * (size_t)n * sizeof(int)); seen_ = calloc((size_t)n, 1);
    int p = 0; for (int v = 0; v < n; v++) { start_[v] = p; if (v > 0) adj_[p++] = v - 1; if (v < n - 1) adj_[p++] = v + 1; } start_[n] = p;
    if (!mode) { rec(0); printf("recursive DFS on a chain of %d junctions: finished, max depth %d\n", n, maxd_); }
    else { int *st = malloc((size_t)n * 2 * sizeof(int)), sp = 0, cnt = 0, maxs = 0; st[sp++] = 0;
        while (sp) { int u = st[--sp]; if (seen_[u]) continue; seen_[u] = 1; cnt++;
            for (int i = start_[u]; i < start_[u+1]; i++) { if (!seen_[adj_[i]]) st[sp++] = adj_[i]; }
            if (sp > maxs) maxs = sp; }
        printf("explicit-stack DFS on a chain of %d junctions: finished, visited %d, max stack entries %d\n", n, cnt, maxs); }
    return 0;
}
