/* Q1 - BST: all operations, traversals, experiments (CO3, CSA0307) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Node { int k; struct Node *l, *r; } Node;

Node *new_node(int k){ Node *n = malloc(sizeof *n); n->k = k; n->l = n->r = NULL; return n; }
/* iterative insert (safe for 100,000 sorted keys); returns 1 if inserted, 0 if duplicate */
int insert(Node **root, int k)
{
    Node **p = root;
    while (*p) { if (k == (*p)->k) return 0; p = k < (*p)->k ? &(*p)->l : &(*p)->r; }
    *p = new_node(k); return 1;
}
int search(const Node *t, int k, int *cmp)
{
    *cmp = 0;
    while (t) { (*cmp)++; if (k == t->k) return 1; t = k < t->k ? t->l : t->r; }
    return 0;
}
int height(const Node *t){ if (!t) return -1; int a = height(t->l), b = height(t->r); return 1 + (a > b ? a : b); }
int count(const Node *t){ return t ? 1 + count(t->l) + count(t->r) : 0; }
/* delete: three cases; inorder successor for two children. *case_used reports which one */
Node *del(Node *t, int k, int *case_used)
{
    if (!t) { *case_used = 0; return NULL; }
    if (k < t->k) t->l = del(t->l, k, case_used);
    else if (k > t->k) t->r = del(t->r, k, case_used);
    else if (t->l && t->r) {
        Node *s = t->r; while (s->l) s = s->l;
        t->k = s->k; *case_used = 3;
        int dummy; t->r = del(t->r, s->k, &dummy);
    } else {
        Node *c = t->l ? t->l : t->r;
        *case_used = c ? 2 : 1;                       /* 1 = leaf, 2 = one child */
        free(t); return c;
    }
    return t;
}
void pre(const Node *t, FILE *f){ if (t){ fprintf(f, "%d ", t->k); pre(t->l, f); pre(t->r, f);} }
void in(const Node *t, FILE *f){ if (t){ in(t->l, f); fprintf(f, "%d ", t->k); in(t->r, f);} }
void post(const Node *t, FILE *f){ if (t){ post(t->l, f); post(t->r, f); fprintf(f, "%d ", t->k);} }
void level(const Node *t, FILE *f)
{
    if (!t) return;
    int cap = 1024, h = 0, tl = 0; const Node **q = malloc(cap * sizeof *q);
    q[tl++] = t;
    while (h < tl) {
        const Node *n = q[h++]; fprintf(f, "%d ", n->k);
        if (tl + 2 >= cap) { cap *= 2; q = realloc(q, cap * sizeof *q); }
        if (n->l) q[tl++] = n->l;
        if (n->r) q[tl++] = n->r;
    }
    free(q);
}
void destroy(Node *t){ if (t){ destroy(t->l); destroy(t->r); free(t);} }
/* verification: BST property + ordering by inorder array */
static int is_bst(const Node *t, long lo, long hi){ return !t || (t->k > lo && t->k < hi && is_bst(t->l, lo, t->k) && is_bst(t->r, t->k, hi)); }
static void fill(const Node *t, int *a, int *n){ if (t){ fill(t->l, a, n); a[(*n)++] = t->k; fill(t->r, a, n);} }
int verify(const Node *t, int expect_n)
{
    int *a = malloc((expect_n + 10) * sizeof *a), n = 0; fill(t, a, &n);
    int ok = is_bst(t, -1000000000000L, 1000000000000L) && n == expect_n;
    for (int i = 1; i < n && ok; i++) if (a[i - 1] >= a[i]) ok = 0;
    free(a); return ok;
}
/* figure data: node lines "id key left right" */
static int fid;
static int dump(const Node *t, FILE *f)
{
    if (!t) return -1;
    int me = fid++, l = dump(t->l, f), r = dump(t->r, f);
    fprintf(f, "N %d %d x %d %d\n", me, t->k, l, r); return me;
}
void fig(const Node *t, FILE *f, const char *name){ fprintf(f, "@FIG %s\n", name); fid = 0; int root = dump(t, f); fprintf(f, "ROOT %d\n", root); }
static unsigned long long rs = 88172645463325252ULL;
static unsigned rnd(void){ rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (unsigned)(rs >> 11); }
static void shuffle(int *a, int n){ for (int i = n - 1; i > 0; i--){ int j = rnd() % (i + 1), t = a[i]; a[i] = a[j]; a[j] = t; } }

int main(void)
{
    FILE *fg = fopen("fig_q1.txt", "w");
    puts("=== 1. Library demo: keys 500 300 700 200 400 600 800 350 ===");
    int a[] = {500, 300, 700, 200, 400, 600, 800, 350};
    Node *t = NULL; for (int i = 0; i < 8; i++) insert(&t, a[i]);
    fig(t, fg, "demo");
    printf("preorder : "); pre(t, stdout); puts("");
    printf("inorder  : "); in(t, stdout); puts("");
    printf("postorder: "); post(t, stdout); puts("");
    printf("levelord : "); level(t, stdout); puts("");
    printf("height = %d, nodes = %d\n", height(t), count(t));
    int c; for (int i = 0; i < 3; i++){ int k = i == 0 ? 350 : i == 1 ? 700 : 999; int f = search(t, k, &c); printf("search %d -> %s, %d comparisons\n", k, f ? "found" : "missing", c); }

    puts("\n=== 2. Deletion of the three cases (tree verified after each) ===");
    int tests[3] = {200, 700, 500}; const char *nm[3] = {"leaf", "one child", "two children"};
    /* make 700 a one-child node: delete 800 first */
    int cs; t = del(t, 800, &cs); printf("prepare: delete 800 (case %d: leaf) -> ", cs); pre(t, stdout); puts("");
    fig(t, fg, "after800");
    for (int i = 0; i < 3; i++) {
        int n_before = count(t); t = del(t, tests[i], &cs);
        printf("delete %d (%s): case reported = %d, nodes %d -> %d, BST valid = %s, preorder: ", tests[i], nm[i], cs, n_before, count(t), verify(t, n_before - 1) ? "YES" : "NO");
        pre(t, stdout); printf("| inorder: "); in(t, stdout); puts("");
        char nmf[32]; sprintf(nmf, "del%d", tests[i]); fig(t, fg, nmf);
    }
    destroy(t);

    puts("\n=== 3. Empty tree and duplicate keys ===");
    Node *e = NULL; int cmp;
    printf("empty: height = %d, count = %d, search(5) = %d (comparisons %d), ", height(e), count(e), search(e, 5, &cmp), cmp);
    e = del(e, 5, &cs); printf("delete(5) on empty -> %s, traversal prints '", e == NULL ? "NULL, no crash" : "?"); in(e, stdout); puts("'");
    int r1 = insert(&e, 10), r2 = insert(&e, 10), r3 = insert(&e, 10);
    printf("insert 10 three times -> returns %d,%d,%d; count = %d (duplicates ignored)\n", r1, r2, r3, count(e)); destroy(e);

    puts("\n=== 4. 2,000 keys: random order vs sorted order ===");
    enum { N = 2000 };
    static int keys[N]; for (int i = 0; i < N; i++) keys[i] = 100000 + 3 * i;      /* accession numbers */
    double avg_r = 0, avg_s = 0;
    for (int variant = 0; variant < 2; variant++) {
        static int ord[N]; memcpy(ord, keys, sizeof keys); if (variant == 0) shuffle(ord, N);
        Node *T = NULL; for (int i = 0; i < N; i++) insert(&T, ord[i]);
        long tot = 0, mx = 0; for (int i = 0; i < N; i++){ search(T, keys[i], &cmp); tot += cmp; if (cmp > mx) mx = cmp; }
        double avg = (double)tot / N; int h = height(T);
        printf("%-12s height = %4d, average comparisons (successful) = %8.2f, worst = %ld, verify = %s\n", variant == 0 ? "random order" : "sorted order", h, avg, mx, verify(T, N) ? "ok" : "BAD");
        if (variant == 0) avg_r = avg; else avg_s = avg;
        destroy(T);
    }
    printf("ratio sorted/random = %.1f; log2(2000) = 10.97, 2 ln n = %.1f\n", avg_s / avg_r, 2 * 7.6009);
    /* repeated trials for the random case */
    double sum_h = 0, sum_a = 0; int trials = 20;
    for (int tr = 0; tr < trials; tr++){ static int ord[N]; memcpy(ord, keys, sizeof keys); shuffle(ord, N);
        Node *T = NULL; for (int i = 0; i < N; i++) insert(&T, ord[i]);
        long tot = 0; for (int i = 0; i < N; i++){ search(T, keys[i], &cmp); tot += cmp; }
        sum_h += height(T); sum_a += (double)tot / N; printf("  shuffle %2d: height %d\n", tr + 1, height(T)); destroy(T); }
    printf("random order, mean of %d shuffles: height %.1f, average comparisons %.2f\n", trials, sum_h / trials, sum_a / trials);

    puts("\n=== 5. Save with a traversal and rebuild ===");
    Node *T = NULL; static int ord[N]; memcpy(ord, keys, sizeof keys); shuffle(ord, N); for (int i = 0; i < 200; i++) insert(&T, ord[i]);
    FILE *f = fopen("catalogue_pre.txt", "w"); pre(T, f); fclose(f);
    f = fopen("catalogue_in.txt", "w"); in(T, f); fclose(f);
    Node *R1 = NULL, *R2 = NULL; int k;
    f = fopen("catalogue_pre.txt", "r"); while (fscanf(f, "%d", &k) == 1) insert(&R1, k); fclose(f);
    f = fopen("catalogue_in.txt", "r"); while (fscanf(f, "%d", &k) == 1) insert(&R2, k); fclose(f);
    FILE *a1 = fopen("a.tmp", "w"), *a2 = fopen("b.tmp", "w"), *a3 = fopen("c.tmp", "w"); pre(T, a1); pre(R1, a2); pre(R2, a3); fclose(a1); fclose(a2); fclose(a3);
    printf("original (200 keys): height %d\n", height(T));
    printf("rebuilt from PREORDER file : height %d, identical shape = %s\n", height(R1), system("cmp -s a.tmp b.tmp") == 0 ? "YES" : "NO");
    printf("rebuilt from INORDER file  : height %d, identical shape = %s\n", height(R2), system("cmp -s a.tmp c.tmp") == 0 ? "YES" : "NO");
    destroy(T); destroy(R1); destroy(R2); remove("a.tmp"); remove("b.tmp"); remove("c.tmp");
    fclose(fg); return 0;
}
