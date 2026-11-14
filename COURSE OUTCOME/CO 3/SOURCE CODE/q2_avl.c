/* Q2 - AVL: traced insertion, rotations, BST vs AVL experiments (100,000 keys) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct Node { int key, h; struct Node *l, *r; } Node;
static long rot_LL, rot_RR, rot_LR, rot_RL;
static const char *last_rot;
static int ht(Node *n){ return n ? n->h : 0; }
static int mx(int a, int b){ return a > b ? a : b; }
static void fix(Node *n){ n->h = 1 + mx(ht(n->l), ht(n->r)); }
static Node *rr(Node *y){ Node *x = y->l; y->l = x->r; x->r = y; fix(y); fix(x); return x; }
static Node *rl(Node *x){ Node *y = x->r; x->r = y->l; y->l = x; fix(x); fix(y); return y; }
static Node *insert(Node *n, int key)
{
    if (!n){ n = malloc(sizeof *n); n->key = key; n->h = 1; n->l = n->r = NULL; return n; }
    if (key < n->key) n->l = insert(n->l, key); else if (key > n->key) n->r = insert(n->r, key); else return n;
    fix(n); int bal = ht(n->l) - ht(n->r);
    if (bal > 1 && key < n->l->key){ rot_LL++; last_rot = "LL (single right rotation)"; return rr(n); }
    if (bal < -1 && key > n->r->key){ rot_RR++; last_rot = "RR (single left rotation)"; return rl(n); }
    if (bal > 1){ rot_LR++; last_rot = "LR (left then right rotation)"; n->l = rl(n->l); return rr(n); }
    if (bal < -1){ rot_RL++; last_rot = "RL (right then left rotation)"; n->r = rr(n->r); return rl(n); }
    return n;
}
static void pre(Node *n){ if (n){ printf("%d ", n->key); pre(n->l); pre(n->r);} }
static void show(Node *n){ if (!n){ printf("."); return; } if (!n->l && !n->r){ printf("%d", n->key); return; } printf("%d(", n->key); show(n->l); printf(","); show(n->r); printf(")"); }
static void destroy(Node *n){ if (n){ destroy(n->l); destroy(n->r); free(n);} }
static int fid;
static int dump(Node *t, FILE *f){ if (!t) return -1; int me = fid++, l = dump(t->l, f), r = dump(t->r, f); fprintf(f, "N %d %d h=%d %d %d\n", me, t->key, t->h, l, r); return me; }
static void fig(Node *t, FILE *f, const char *name){ fprintf(f, "@FIG %s\n", name); fid = 0; int root = dump(t, f); fprintf(f, "ROOT %d\n", root); }
/* balance-factor verification: |bf| <= 1 and stored height correct, BST order */
static int check(Node *n, int lo, int hi, int *hout, long *cnt)
{
    if (!n){ *hout = 0; return 1; }
    if (n->key <= lo || n->key >= hi) return 0;
    int hl, hr; (*cnt)++;
    if (!check(n->l, lo, n->key, &hl, cnt) || !check(n->r, n->key, hi, &hr, cnt)) return 0;
    if (abs(hl - hr) > 1 || n->h != 1 + mx(hl, hr)) return 0;
    *hout = n->h; return 1;
}
/* plain BST (iterative) with comparison counter */
typedef struct B { int k; struct B *l, *r; } B;
static void bst_insert(B **root, int k){ B **p = root; while (*p){ if (k == (*p)->k) return; p = k < (*p)->k ? &(*p)->l : &(*p)->r; } *p = malloc(sizeof **p); (*p)->k = k; (*p)->l = (*p)->r = NULL; }
static int bst_search(B *t, int k){ int c = 0; while (t){ c++; if (k == t->k) return c; t = k < t->k ? t->l : t->r; } return c; }
static int bst_height(B *t){ /* iterative via explicit stack of (node,depth) */
    if (!t) return -1; int cap = 1 << 20, sp = 0, best = 0; B **sn = malloc(cap * sizeof *sn); int *sd = malloc(cap * sizeof *sd);
    sn[sp] = t; sd[sp++] = 0; while (sp){ B *n = sn[--sp]; int d = sd[sp]; if (d > best) best = d; if (sp + 2 >= cap){ cap *= 2; sn = realloc(sn, cap * sizeof *sn); sd = realloc(sd, cap * sizeof *sd);} if (n->l){ sn[sp] = n->l; sd[sp++] = d + 1; } if (n->r){ sn[sp] = n->r; sd[sp++] = d + 1; } }
    free(sn); free(sd); return best; }
static void bst_free(B *t){ while (t){ if (t->l){ B *l = t->l; t->l = l->r; l->r = t; t = l; } else { B *r = t->r; free(t); t = r; } } }
static double now(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec * 1e-9; }
static unsigned long long rs = 88172645463325252ULL;
static unsigned rnd(void){ rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (unsigned)(rs >> 11); }

int main(void)
{
    FILE *fg = fopen("fig_q2.txt", "w");
    puts("=== 1. Traced insertion of 10 20 30 40 50 25 ===");
    int p[] = {10, 20, 30, 40, 50, 25}; Node *root = NULL;
    for (int i = 0; i < 6; i++){
        Node *before = root; (void)before; last_rot = "none";
        /* snapshot BEFORE rebalancing: insert into a copy is awkward, so print the plain-BST shape too */
        root = insert(root, p[i]);
        printf("insert %2d: rotation = %-30s tree: ", p[i], last_rot); show(root); printf("   preorder: "); pre(root); printf("  height %d\n", ht(root));
        char nm[16]; sprintf(nm, "ins%d", p[i]); fig(root, fg, nm);
    }
    printf("balance factors of the final tree: "); 
    long c0 = 0; int h0; printf("valid AVL = %s\n", check(root, -1, 1 << 30, &h0, &c0) ? "YES" : "NO");
    destroy(root);

    puts("\n=== 2. 100,000 keys inserted in SORTED order: BST vs AVL ===");
    enum { N = 100000 };
    static int sortedk[N], randk[N]; for (int i = 0; i < N; i++){ sortedk[i] = i + 1; randk[i] = i + 1; }
    for (int i = N - 1; i > 0; i--){ int j = rnd() % (i + 1), t = randk[i]; randk[i] = randk[j]; randk[j] = t; }
    double t0, t1;
    B *bs = NULL; t0 = now(); for (int i = 0; i < N; i++) bst_insert(&bs, sortedk[i]); t1 = now(); double bst_ins_sorted = t1 - t0;
    Node *as = NULL; rot_LL = rot_RR = rot_LR = rot_RL = 0; t0 = now(); for (int i = 0; i < N; i++) as = insert(as, sortedk[i]); t1 = now(); double avl_ins_sorted = t1 - t0;
    long rs_LL = rot_LL, rs_RR = rot_RR, rs_LR = rot_LR, rs_RL = rot_RL;
    printf("BST  (sorted): height = %d, insertion time = %.3f s\n", bst_height(bs), bst_ins_sorted);
    printf("AVL  (sorted): height = %d, insertion time = %.3f s, 1.44*log2(n) bound = %.1f, log2(n) = %.1f\n", ht(as) - 1, avl_ins_sorted, 1.44 * 16.61, 16.61);
    printf("AVL rotations (sorted): LL %ld RR %ld LR %ld RL %ld, total %ld\n", rs_LL, rs_RR, rs_LR, rs_RL, rs_LL + rs_RR + rs_LR + rs_RL);

    puts("\n=== 3. Rotations for RANDOM insertion (100,000 keys) ===");
    Node *ar = NULL; rot_LL = rot_RR = rot_LR = rot_RL = 0; t0 = now(); for (int i = 0; i < N; i++) ar = insert(ar, randk[i]); t1 = now(); double avl_ins_rand = t1 - t0;
    printf("AVL  (random): height = %d, insertion time = %.3f s, rotations: LL %ld RR %ld LR %ld RL %ld, total %ld (%.2f per insertion)\n", ht(ar) - 1, avl_ins_rand, rot_LL, rot_RR, rot_LR, rot_RL, rot_LL + rot_RR + rot_LR + rot_RL, (double)(rot_LL + rot_RR + rot_LR + rot_RL) / N);
    printf("sorted: %.2f rotations per insertion (single rotations only: LL=%ld, RR=%ld)\n", (double)(rs_LL + rs_RR + rs_LR + rs_RL) / N, rs_LL, rs_RR);
    B *br = NULL; t0 = now(); for (int i = 0; i < N; i++) bst_insert(&br, randk[i]); t1 = now(); double bst_ins_rand = t1 - t0;
    printf("BST  (random): height = %d, insertion time = %.3f s\n", bst_height(br), bst_ins_rand);

    puts("\n=== 4. Search time: 100,000 successful searches of random keys (every key once) ===");
    long tot; t0 = now(); tot = 0; for (int i = 0; i < N; i += 10) tot += bst_search(bs, randk[i]); t1 = now();
    printf("BST sorted-built : %d searches, avg comparisons %.1f, time %.4f s (%.1f us/search)\n", N / 10, (double)tot / (N / 10), t1 - t0, (t1 - t0) / (N / 10) * 1e6);
    t0 = now(); tot = 0; for (int i = 0; i < N; i++) tot += bst_search(br, randk[i]); t1 = now();
    printf("BST random-built : %d searches, avg comparisons %.1f, time %.4f s (%.2f us/search)\n", N, (double)tot / N, t1 - t0, (t1 - t0) / N * 1e6);
    t0 = now(); tot = 0; for (int i = 0; i < N; i++){ Node *n = as; int c = 0; while (n){ c++; if (randk[i] == n->key) break; n = randk[i] < n->key ? n->l : n->r; } tot += c; } t1 = now();
    printf("AVL sorted-built : %d searches, avg comparisons %.1f, time %.4f s (%.2f us/search)\n", N, (double)tot / N, t1 - t0, (t1 - t0) / N * 1e6);
    t0 = now(); tot = 0; for (int i = 0; i < N; i++){ Node *n = ar; int c = 0; while (n){ c++; if (randk[i] == n->key) break; n = randk[i] < n->key ? n->l : n->r; } tot += c; } t1 = now();
    printf("AVL random-built : %d searches, avg comparisons %.1f, time %.4f s (%.2f us/search)\n", N, (double)tot / N, t1 - t0, (t1 - t0) / N * 1e6);

    puts("\n=== 5. Verify balance factor of every node ===");
    long cnt = 0; int hh; int ok1 = check(as, 0, 1 << 30, &hh, &cnt); long cnt1 = cnt; cnt = 0; int ok2 = check(ar, 0, 1 << 30, &hh, &cnt);
    printf("AVL built from sorted keys: %ld nodes checked, all |balance| <= 1, heights consistent, BST order = %s\n", cnt1, ok1 ? "YES" : "NO");
    printf("AVL built from random keys: %ld nodes checked, all |balance| <= 1, heights consistent, BST order = %s\n", cnt, ok2 ? "YES" : "NO");
    /* minimum nodes for an AVL tree of height h: N(h) = N(h-1)+N(h-2)+1 */
    printf("\nminimum AVL nodes N(h): "); long a = 0, b = 1; for (int h = 1; h <= 12; h++){ printf("N(%d)=%ld ", h, b); long c = a + b + 1; a = b; b = c; } puts("");
    bst_free(bs); bst_free(br); destroy(as); destroy(ar); fclose(fg); return 0;
}
