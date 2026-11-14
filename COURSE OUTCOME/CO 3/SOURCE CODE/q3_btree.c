/* Q3 - B-tree (CLRS, minimum degree t): insert with split trace, search with node-visit (disk read) counting,
   height for order 100 and 5 million keys, comparison with BST and AVL. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef struct BN { int n, leaf; int *k; struct BN **c; } BN;
static int T;                       /* minimum degree */
static long splits, node_count;
static int tracing;
static BN *bnew(int leaf){ BN *x = calloc(1, sizeof *x); x->k = calloc(2 * T - 1, sizeof(int)); x->c = leaf ? NULL : calloc(2 * T, sizeof(BN *)); x->leaf = leaf; node_count++; return x; }
static void pr(BN *x){ printf("["); for (int i = 0; i < x->n; i++) printf("%s%d", i ? " " : "", x->k[i]); printf("]"); }
static void split_child(BN *x, int i)
{
    BN *y = x->c[i], *z = bnew(y->leaf); z->n = T - 1; splits++;
    if (tracing){ printf("    split: full node "); pr(y); printf(" -> median %d moves up, left [", y->k[T - 1]); for (int j = 0; j < T - 1; j++) printf("%s%d", j ? " " : "", y->k[j]); printf("], right ["); for (int j = 0; j < T - 1; j++) printf("%s%d", j ? " " : "", y->k[j + T]); printf("]\n"); }
    for (int j = 0; j < T - 1; j++) z->k[j] = y->k[j + T];
    if (!y->leaf) for (int j = 0; j < T; j++) z->c[j] = y->c[j + T];
    y->n = T - 1;
    for (int j = x->n; j >= i + 1; j--) x->c[j + 1] = x->c[j];
    x->c[i + 1] = z;
    for (int j = x->n - 1; j >= i; j--) x->k[j + 1] = x->k[j];
    x->k[i] = y->k[T - 1]; x->n++;
}
static void insert_nonfull(BN *x, int key)
{
    int i = x->n - 1;
    if (x->leaf){ while (i >= 0 && key < x->k[i]){ x->k[i + 1] = x->k[i]; i--; } x->k[i + 1] = key; x->n++; return; }
    while (i >= 0 && key < x->k[i]) i--;
    i++;
    if (x->c[i]->n == 2 * T - 1){ split_child(x, i); if (key > x->k[i]) i++; }
    insert_nonfull(x->c[i], key);
}
static BN *insert(BN *root, int key)
{
    if (!root){ root = bnew(1); root->k[0] = key; root->n = 1; return root; }
    if (root->n == 2 * T - 1){
        BN *s = bnew(0); s->c[0] = root; s->n = 0;
        if (tracing){ printf("    root "); pr(root); printf(" is full: new root created, "); printf("\n"); }
        split_child(s, 0); root = s;
    }
    insert_nonfull(root, key); return root;
}
/* search: returns 1 if found; *visits = nodes read (one disk block each) */
static int search(BN *x, int key, int *visits)
{
    *visits = 0;
    while (x){
        (*visits)++; int i = 0; while (i < x->n && key > x->k[i]) i++;
        if (i < x->n && key == x->k[i]) return 1;
        if (x->leaf) return 0;
        x = x->c[i];
    }
    return 0;
}
static int bheight(BN *x){ int h = 0; while (!x->leaf){ x = x->c[0]; h++; } return h; }
static int fid;
static int dumpn(BN *x, FILE *f)
{
    int ch[200]; int nc = 0;
    if (!x->leaf) for (int i = 0; i <= x->n; i++) ch[nc++] = dumpn(x->c[i], f);
    int me = fid++; fprintf(f, "B %d", me); for (int i = 0; i < x->n; i++) fprintf(f, " %d", x->k[i]); fprintf(f, " |"); for (int i = 0; i < nc; i++) fprintf(f, " %d", ch[i]); fprintf(f, "\n"); return me;
}
static void fig(BN *r, FILE *f, const char *name){ fprintf(f, "@FIG %s\n", name); fid = 0; int root = dumpn(r, f); fprintf(f, "ROOT %d\n", root); }
static void show(BN *x, int depth)
{
    for (int i = 0; i < depth; i++) printf("    "); pr(x); printf("\n");
    if (!x->leaf) for (int i = 0; i <= x->n; i++) show(x->c[i], depth + 1);
}
static int check_levels(BN *x, int d, int *leafd){ if (x->leaf){ if (*leafd < 0) *leafd = d; return *leafd == d; } for (int i = 0; i <= x->n; i++) if (!check_levels(x->c[i], d + 1, leafd)) return 0; return 1; }
static int min_keys_ok(BN *x, int root){ if (!root && x->n < T - 1) return 0; if (!x->leaf) for (int i = 0; i <= x->n; i++) if (!min_keys_ok(x->c[i], 0)) return 0; return 1; }
static void freeb(BN *x){ if (!x->leaf) for (int i = 0; i <= x->n; i++) freeb(x->c[i]); free(x->k); free(x->c); free(x); }

/* ---- BST and AVL for node-visit comparison (5 million keys) ---- */
typedef struct N { int k, h; struct N *l, *r; } N;
static int hh(N *n){ return n ? n->h : 0; }
static N *rr(N *y){ N *x = y->l; y->l = x->r; x->r = y; y->h = 1 + (hh(y->l) > hh(y->r) ? hh(y->l) : hh(y->r)); x->h = 1 + (hh(x->l) > hh(x->r) ? hh(x->l) : hh(x->r)); return x; }
static N *rl(N *x){ N *y = x->r; x->r = y->l; y->l = x; x->h = 1 + (hh(x->l) > hh(x->r) ? hh(x->l) : hh(x->r)); y->h = 1 + (hh(y->l) > hh(y->r) ? hh(y->l) : hh(y->r)); return y; }
static N *avl_insert(N *n, int k)
{
    if (!n){ n = malloc(sizeof *n); n->k = k; n->h = 1; n->l = n->r = NULL; return n; }
    if (k < n->k) n->l = avl_insert(n->l, k); else if (k > n->k) n->r = avl_insert(n->r, k); else return n;
    n->h = 1 + (hh(n->l) > hh(n->r) ? hh(n->l) : hh(n->r)); int b = hh(n->l) - hh(n->r);
    if (b > 1 && k < n->l->k) return rr(n); if (b < -1 && k > n->r->k) return rl(n);
    if (b > 1){ n->l = rl(n->l); return rr(n); } if (b < -1){ n->r = rr(n->r); return rl(n); } return n;
}
static void bst_insert(N **root, int k){ N **p = root; while (*p){ p = k < (*p)->k ? &(*p)->l : &(*p)->r; } *p = malloc(sizeof **p); (*p)->k = k; (*p)->h = 1; (*p)->l = (*p)->r = NULL; }
static int tsearch(N *t, int k){ int c = 0; while (t){ c++; if (k == t->k) return c; t = k < t->k ? t->l : t->r; } return c; }
static int theight(N *t){ int best = 0; /* iterative depth via Morris-free stack */ int cap = 1 << 16, sp = 0; N **sn = malloc(cap * sizeof *sn); int *sd = malloc(cap * sizeof *sd);
    if (!t) return -1; sn[sp] = t; sd[sp++] = 0; while (sp){ N *n = sn[--sp]; int d = sd[sp]; if (d > best) best = d; if (sp + 2 >= cap){ cap *= 2; sn = realloc(sn, cap * sizeof *sn); sd = realloc(sd, cap * sizeof *sd); } if (n->l){ sn[sp] = n->l; sd[sp++] = d + 1; } if (n->r){ sn[sp] = n->r; sd[sp++] = d + 1; } } free(sn); free(sd); return best; }
static void tfree(N *t){ while (t){ if (t->l){ N *l = t->l; t->l = l->r; l->r = t; t = l; } else { N *r = t->r; free(t); t = r; } } }
static unsigned long long rs = 88172645463325252ULL;
static unsigned rnd(void){ rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (unsigned)(rs >> 11); }

int main(void)
{
    FILE *fg = fopen("fig_q3.txt", "w");
    puts("=== 1. By hand, minimum degree t = 2 (order 4: at most 3 keys, 2..4 children): keys 10 20 5 6 12 30 7 17 ===");
    T = 2; tracing = 1; BN *root = NULL; int ks[] = {10, 20, 5, 6, 12, 30, 7, 17};
    for (int i = 0; i < 8; i++){
        printf("insert %d:\n", ks[i]); long s0 = splits; root = insert(root, ks[i]);
        printf("  splits this step: %ld\n  tree:\n", splits - s0); show(root, 2);
        char nm[16]; sprintf(nm, "bt%d", ks[i]); fig(root, fg, nm);
    }
    int ld = -1; printf("total splits %ld, height %d (edges), all leaves at same depth = %s, min-keys rule = %s\n", splits, bheight(root), check_levels(root, 0, &ld) ? "YES" : "NO", min_keys_ok(root, 1) ? "OK" : "VIOLATED");
    puts("\n--- search tests on this tree (node reads) ---");
    int v; int tst[] = {5, 30, 17, 8};
    for (int i = 0; i < 4; i++){ int f = search(root, tst[i], &v); printf("search %2d -> %-7s, %d node reads\n", tst[i], f ? "found" : "missing", v); }
    freeb(root); tracing = 0;

    puts("\n=== 2. Height of a B-tree of order 100 holding 5,000,000 keys (formulae) ===");
    double n = 5e6; int m = 100, tt = 50;
    printf("order m=100: at most 99 keys and 100 children per node; at least ceil(m/2)=50 children (min degree t=50) except the root\n");
    printf("maximum height h (edges) for n keys: h <= log_t((n+1)/2) = log_50(%.1f) = %.3f -> h <= 3 (at most 4 levels = 4 disk reads)\n", (n + 1) / 2, log((n + 1) / 2) / log(tt));
    printf("minimum height: n <= m^(h+1) - 1  ->  h+1 >= log_100(%.0f+1) = %.3f -> at least 4 levels\n", n, log(n + 1) / log(m));
    printf("so every search reads exactly 4 nodes at worst, i.e. height 3 edges / 4 levels; binary tree: log2(5e6) = %.2f -> about 23 reads\n", log(n) / log(2));
    (void)m;

    puts("\n=== 3. Build real structures with 5,000,000 keys and simulate disk reads ===");
    enum { NK = 5000000 };
    int *perm = malloc(NK * sizeof *perm); for (int i = 0; i < NK; i++) perm[i] = i + 1;
    for (int i = NK - 1; i > 0; i--){ int j = rnd() % (i + 1), t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
    T = 50; node_count = 0; splits = 0; BN *bt = NULL; for (int i = 0; i < NK; i++) bt = insert(bt, perm[i]);
    ld = -1; printf("B-tree (t=50, random insertion order): height %d edges = %d levels, nodes %ld, leaves same depth = %s, min-keys = %s\n", bheight(bt), bheight(bt) + 1, node_count, check_levels(bt, 0, &ld) ? "YES" : "NO", min_keys_ok(bt, 1) ? "OK" : "VIOLATED");
    N *bst = NULL; for (int i = 0; i < NK; i++) bst_insert(&bst, perm[i]);
    N *avl = NULL; for (int i = 1; i <= NK; i++) avl = avl_insert(avl, i);                 /* sorted insertion: worst case for a BST */
    printf("BST  (random order) height %d edges; AVL (sorted insertion) height %d edges\n", theight(bst), hh(avl) - 1);
    int Q = 200000; double sb = 0, sa = 0, sbt = 0; int mb = 0, ma = 0, mbt = 0;
    for (int i = 0; i < Q; i++){ int key = perm[rnd() % NK]; int c = tsearch(bst, key); sb += c; if (c > mb) mb = c;
        N *x = avl; c = 0; while (x){ c++; if (key == x->k) break; x = key < x->k ? x->l : x->r; } sa += c; if (c > ma) ma = c;
        search(bt, key, &v); sbt += v; if (v > mbt) mbt = v; }
    printf("%d random successful searches (average / worst node visits):\n", Q);
    printf("  BST   %6.2f / %d   -> %.1f ms average, %d ms worst at 10 ms per read\n", sb / Q, mb, sb / Q * 10, mb * 10);
    printf("  AVL   %6.2f / %d   -> %.1f ms average, %d ms worst\n", sa / Q, ma, sa / Q * 10, ma * 10);
    printf("  B-tree%6.2f / %d   -> %.1f ms average, %d ms worst\n", sbt / Q, mbt, sbt / Q * 10, mbt * 10);
    puts("--- smallest, largest and missing key (node visits) ---");
    int probe[3] = {1, NK, NK + 7}; const char *pn[3] = {"smallest (1)", "largest (5000000)", "missing (5000007)"};
    for (int i = 0; i < 3; i++){ int f1; int cb = tsearch(bst, probe[i]); N *x = avl; int ca = 0; while (x){ ca++; if (probe[i] == x->k) break; x = probe[i] < x->k ? x->l : x->r; } f1 = search(bt, probe[i], &v);
        printf("  %-18s BST %2d visits, AVL %2d visits, B-tree %d visits (%s)  -> %d ms / %d ms / %d ms\n", pn[i], cb, ca, v, f1 ? "found" : "not found", cb * 10, ca * 10, v * 10); }
    printf("node size: B-tree node = 99 keys*4 B + 100 child refs*8 B + header = about %d bytes (fits a 4 KB block when 4-byte block numbers are used: 99*4+100*4+8 = %d bytes)\n", 99 * 4 + 100 * 8 + 8, 99 * 4 + 100 * 4 + 8);
    free(perm); freeb(bt); tfree(bst); tfree(avl); fclose(fg); return 0;
}
