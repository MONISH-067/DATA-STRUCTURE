/* Q5 - Splay tree with parent pointers (bottom-up splaying: zig, zig-zig, zig-zag).
   Trace on a given tree, 80/20 workload, uniform workload, sorted access, worst access, root after ten accesses. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct S { int k; struct S *l, *r, *p; } S;
static long n_zig, n_zigzig, n_zigzag; static int trace;
static S *root;
static S *nn(int k){ S *n = calloc(1, sizeof *n); n->k = k; return n; }
static void rotate_up(S *x)                                         /* single rotation of x over its parent */
{
    S *p = x->p, *g = p->p;
    if (p->l == x){ p->l = x->r; if (x->r) x->r->p = p; x->r = p; } else { p->r = x->l; if (x->l) x->l->p = p; x->l = p; }
    p->p = x; x->p = g; if (g){ if (g->l == p) g->l = x; else g->r = x; } else root = x;
}
static void splay(S *x)
{
    while (x->p){
        S *p = x->p, *g = p->p;
        if (!g){ n_zig++; if (trace) printf("      zig: %d is a child of the root %d -> rotate %s\n", x->k, p->k, p->l == x ? "right" : "left"); rotate_up(x); }
        else if ((g->l == p) == (p->l == x)){ n_zigzig++; if (trace) printf("      zig-zig: %d, parent %d, grandparent %d on the same side -> rotate parent first, then %d\n", x->k, p->k, g->k, x->k); rotate_up(p); rotate_up(x); }
        else { n_zigzag++; if (trace) printf("      zig-zag: %d, parent %d, grandparent %d on opposite sides -> rotate %d twice\n", x->k, p->k, g->k, x->k); rotate_up(x); rotate_up(x); }
    }
}
/* find (no restructuring) -> returns node or last node on the path; *vis = nodes visited */
static S *find(int k, long *vis)
{
    S *t = root, *last = NULL; *vis = 0;
    while (t){ (*vis)++; last = t; if (k == t->k) return t; t = k < t->k ? t->l : t->r; }
    return last;
}
static long access_key(int k){ long v; S *x = find(k, &v); if (x) splay(x); return v; }
static void plain_insert(int k){ S **pp = &root, *par = NULL; while (*pp){ par = *pp; pp = k < par->k ? &par->l : &par->r; } *pp = nn(k); (*pp)->p = par; }
static void splay_insert(int k){ plain_insert(k); long v; S *x = find(k, &v); splay(x); }
static void show(S *n){ if (!n){ printf("."); return; } if (!n->l && !n->r){ printf("%d", n->k); return; } printf("%d(", n->k); show(n->l); printf(","); show(n->r); printf(")"); }
static int fid; static int dump(S *t, FILE *f){ if (!t) return -1; int me = fid++, l = dump(t->l, f), r = dump(t->r, f); fprintf(f, "N %d %d x %d %d\n", me, t->k, l, r); return me; }
static void fig(FILE *f, const char *name){ fprintf(f, "@FIG %s\n", name); fid = 0; int r = dump(root, f); fprintf(f, "ROOT %d\n", r); }
static void freeall(S *t){ while (t){ if (t->l){ S *l = t->l; t->l = l->r; l->r = t; t = l; } else { S *r = t->r; free(t); t = r; } } root = NULL; }
static int height(S *t){ if (!t) return -1; int a = height(t->l), b = height(t->r); return 1 + (a > b ? a : b); }
static int bst_ok(S *t, long lo, long hi){ return !t || (t->k > lo && t->k < hi && (!t->l || t->l->p == t) && (!t->r || t->r->p == t) && bst_ok(t->l, lo, t->k) && bst_ok(t->r, t->k, hi)); }
/* AVL for comparison (insert only, counts nodes visited) */
typedef struct A { int k, h; struct A *l, *r; } A;
static int ah(A *n){ return n ? n->h : 0; }
static void afix(A *n){ n->h = 1 + (ah(n->l) > ah(n->r) ? ah(n->l) : ah(n->r)); }
static A *arr(A *y){ A *x = y->l; y->l = x->r; x->r = y; afix(y); afix(x); return x; }
static A *arl(A *x){ A *y = x->r; x->r = y->l; y->l = x; afix(x); afix(y); return y; }
static A *ains(A *n, int k){ if (!n){ n = malloc(sizeof *n); n->k = k; n->h = 1; n->l = n->r = NULL; return n; } if (k < n->k) n->l = ains(n->l, k); else n->r = ains(n->r, k); afix(n); int b = ah(n->l) - ah(n->r);
    if (b > 1 && k < n->l->k) return arr(n); if (b < -1 && k > n->r->k) return arl(n); if (b > 1){ n->l = arl(n->l); return arr(n); } if (b < -1){ n->r = arr(n->r); return arl(n); } return n; }
static int adepth(A *t, int k){ int c = 0; while (t){ c++; if (k == t->k) return c; t = k < t->k ? t->l : t->r; } return c; }
static void afree(A *n){ if (n){ afree(n->l); afree(n->r); free(n);} }
static unsigned long long rs = 88172645463325252ULL;
static unsigned rnd(void){ rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (unsigned)(rs >> 11); }

int main(void)
{
    FILE *fg = fopen("fig_q5.txt", "w");
    puts("=== 1. Given tree (assumed: the brief gives none): keys inserted 8 4 12 2 6 10 14 1 3 9 11 into a plain BST ===");
    int g[] = {8, 4, 12, 2, 6, 10, 14, 1, 3, 9, 11}; for (int i = 0; i < 11; i++) plain_insert(g[i]);
    printf("initial: "); show(root); puts(""); fig(fg, "start");
    trace = 1; int acc[] = {3, 9};
    for (int i = 0; i < 2; i++){ long v; printf("access %d (path length %ld nodes before splaying):\n", acc[i], (find(acc[i], &v), v)); access_key(acc[i]); printf("    result: "); show(root); printf("   root = %d, valid BST = %s\n", root->k, bst_ok(root, -1, 1 << 30) ? "YES" : "NO"); char nm[8]; sprintf(nm, "acc%d", acc[i]); fig(fg, nm); }
    trace = 0; freeall(root);

    puts("\n=== 2. 10,000 accesses per workload: 80% to 5% of the keys, uniform, and 90% to just 10 keys; n = 1,000 and 10,000 ===");
    int sizes[2] = {1000, 10000};
    for (int si = 0; si < 2; si++){
        int n = sizes[si]; int *perm = malloc(n * sizeof *perm); for (int i = 0; i < n; i++) perm[i] = i + 1;
        for (int i = n - 1; i > 0; i--){ int j = rnd() % (i + 1), t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
        int hot = n / 20;                                                /* hot keys are perm[0..hot-1] */
        for (int wl = 0; wl < 3; wl++){
            root = NULL; A *avl = NULL; for (int i = 0; i < n; i++){ plain_insert(perm[i]); avl = ains(avl, perm[i]); }
            int h0 = height(root);
            long sv = 0, av = 0, worst = 0, firstw = 0; int hotc = 0;
            for (int q = 0; q < 10000; q++){
                int key; if (wl == 0){ if (rnd() % 100 < 80){ key = perm[rnd() % hot]; hotc++; } else key = perm[hot + rnd() % (n - hot)]; } else if (wl == 1) key = perm[rnd() % n]; else { if (rnd() % 100 < 90) key = perm[rnd() % 10]; else key = perm[10 + rnd() % (n - 10)]; }
                av += adepth(avl, key); long v = access_key(key); sv += v; if (v > worst) worst = v; if (q == 0) firstw = v;
                if (wl == 0 && si == 0 && q < 10){ static int ten = 0; if (ten == 0) printf("--- first ten accesses of the skewed run (n=1000): access -> root after splay ---\n"); ten++; printf("   %2d. access %4d (%s) -> root = %d, path length was %ld\n", q + 1, key, hotc && 0 ? "" : "", root->k, v); }
            }
            printf("n=%5d %-9s: splay avg nodes/access %6.2f  | AVL avg %6.2f | splay worst single access %ld (first access %ld) | initial BST height %d | final splay height %d | zig %ld zigzig %ld zigzag %ld\n", n, wl == 0 ? "80/20" : wl == 1 ? "uniform" : "90% to 10", (double)sv / 10000, (double)av / 10000, worst, firstw, h0, height(root), n_zig, n_zigzig, n_zigzag);
            n_zig = n_zigzig = n_zigzag = 0; freeall(root); afree(avl);
        }
        free(perm);
    }

    puts("\n=== 3. Sorted access ===");
    int n = 10000; root = NULL;
    for (int i = 1; i <= n; i++) splay_insert(i);
    printf("after %d sorted splay-insertions: height %d (a chain), root = %d\n", n, height(root), root->k);
    long tot = 0, worst = 0, first = 0; for (int i = 1; i <= n; i++){ long v = access_key(i); tot += v; if (v > worst) worst = v; if (i == 1) first = v; }
    printf("accessing 1..%d in sorted order: first access %ld nodes, worst %ld, total %ld nodes = %.2f per access (amortised O(1) here), final height %d\n", n, first, worst, tot, (double)tot / n, height(root));
    tot = 0; worst = 0; for (int i = n; i >= 1; i--){ long v = access_key(i); tot += v; if (v > worst) worst = v; }
    printf("then accessing %d..1 in reverse order: worst %ld, %.2f per access\n", n, worst, (double)tot / n);
    printf("single worst access in the whole sorted experiment: %ld nodes = O(n), yet the average stays small: this is amortised O(log n), not worst-case\n", first);
    freeall(root);

    puts("\n=== 4. Sequential scans and uniform random access, n = 1,000 ===");
    root = NULL; A *avl = NULL; for (int i = 1; i <= 1000; i++){ splay_insert(i); avl = ains(avl, i); }
    long sv = 0, av = 0; for (int r = 0; r < 20; r++) for (int i = 1; i <= 1000; i++){ sv += access_key(i); av += adepth(avl, i); }
    printf("20 sequential scans of 1,000 keys: splay %.2f nodes/access, AVL %.2f nodes/access\n", (double)sv / 20000, (double)av / 20000);
    sv = 0; av = 0; for (int q = 0; q < 20000; q++){ int key = 1 + rnd() % 1000; sv += access_key(key); av += adepth(avl, key); }
    printf("20,000 uniform random accesses : splay %.2f nodes/access, AVL %.2f nodes/access (every splay access also restructures the tree: extra writes)\n", (double)sv / 20000, (double)av / 20000);
    freeall(root); afree(avl); fclose(fg); return 0;
}
