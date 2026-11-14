/* Q4 - Red-black tree (CLRS, with parent pointers and a sentinel) vs AVL tree.
   Hand trace of 10 20 30 15 25 5; 10,000 random insertions and deletions; five-property checker;
   sorted insertion; cached leftmost node (scheduler "pick next"). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { RED = 0, BLACK = 1 };
typedef struct R { int k, c; struct R *l, *r, *p; } R;
static R NIL_NODE = {0, BLACK, &NIL_NODE, &NIL_NODE, &NIL_NODE}, *NIL = &NIL_NODE;
static R *rb_root, *leftmost;                         /* leftmost = cached minimum */
static long rb_rot_ins, rb_rot_del, recolour_ins, recolour_del, *rot_ctr, *rec_ctr;
static int trace;
static void left_rotate(R *x){ R *y = x->r; x->r = y->l; if (y->l != NIL) y->l->p = x; y->p = x->p; if (x->p == NIL) rb_root = y; else if (x == x->p->l) x->p->l = y; else x->p->r = y; y->l = x; x->p = y; (*rot_ctr)++; if (trace) printf("      left rotation at %d\n", x->k); }
static void right_rotate(R *x){ R *y = x->l; x->l = y->r; if (y->r != NIL) y->r->p = x; y->p = x->p; if (x->p == NIL) rb_root = y; else if (x == x->p->r) x->p->r = y; else x->p->l = y; y->r = x; x->p = y; (*rot_ctr)++; if (trace) printf("      right rotation at %d\n", x->k); }
static void paint(R *n, int c){ if (n != NIL && n->c != c){ n->c = c; (*rec_ctr)++; if (trace) printf("      recolour %d -> %s\n", n->k, c == RED ? "RED" : "BLACK"); } }
static void insert_fixup(R *z)
{
    while (z->p->c == RED){
        if (z->p == z->p->p->l){
            R *y = z->p->p->r;
            if (y->c == RED){ if (trace) printf("      case 1 (uncle %d red)\n", y->k); paint(z->p, BLACK); paint(y, BLACK); paint(z->p->p, RED); z = z->p->p; }
            else { if (z == z->p->r){ if (trace) printf("      case 2 (triangle) -> rotate to line\n"); z = z->p; left_rotate(z); }
                   if (trace) printf("      case 3 (line, uncle black)\n"); paint(z->p, BLACK); paint(z->p->p, RED); right_rotate(z->p->p); }
        } else {
            R *y = z->p->p->l;
            if (y->c == RED){ if (trace) printf("      case 1 (uncle %d red)\n", y->k); paint(z->p, BLACK); paint(y, BLACK); paint(z->p->p, RED); z = z->p->p; }
            else { if (z == z->p->l){ if (trace) printf("      case 2 (triangle) -> rotate to line\n"); z = z->p; right_rotate(z); }
                   if (trace) printf("      case 3 (line, uncle black)\n"); paint(z->p, BLACK); paint(z->p->p, RED); left_rotate(z->p->p); }
        }
    }
    paint(rb_root, BLACK);
}
static void rb_insert(int k)
{
    R *z = malloc(sizeof *z); z->k = k; z->l = z->r = z->p = NIL; z->c = RED;
    R *y = NIL, *x = rb_root; while (x != NIL){ y = x; if (k == x->k){ free(z); return; } x = k < x->k ? x->l : x->r; }
    z->p = y; if (y == NIL) rb_root = z; else if (k < y->k) y->l = z; else y->r = z;
    if (leftmost == NIL || k < leftmost->k) leftmost = z;           /* maintain cached minimum */
    insert_fixup(z);
}
static void transplant(R *u, R *v){ if (u->p == NIL) rb_root = v; else if (u == u->p->l) u->p->l = v; else u->p->r = v; v->p = u->p; }
static R *tree_min(R *x){ while (x->l != NIL) x = x->l; return x; }
static void delete_fixup(R *x)
{
    while (x != rb_root && x->c == BLACK){
        if (x == x->p->l){
            R *w = x->p->r;
            if (w->c == RED){ paint(w, BLACK); paint(x->p, RED); left_rotate(x->p); w = x->p->r; }
            if (w->l->c == BLACK && w->r->c == BLACK){ paint(w, RED); x = x->p; }
            else { if (w->r->c == BLACK){ paint(w->l, BLACK); paint(w, RED); right_rotate(w); w = x->p->r; }
                   if (w->c != x->p->c){ w->c = x->p->c; (*rec_ctr)++; } paint(x->p, BLACK); paint(w->r, BLACK); left_rotate(x->p); x = rb_root; }
        } else {
            R *w = x->p->l;
            if (w->c == RED){ paint(w, BLACK); paint(x->p, RED); right_rotate(x->p); w = x->p->l; }
            if (w->r->c == BLACK && w->l->c == BLACK){ paint(w, RED); x = x->p; }
            else { if (w->l->c == BLACK){ paint(w->r, BLACK); paint(w, RED); left_rotate(w); w = x->p->l; }
                   if (w->c != x->p->c){ w->c = x->p->c; (*rec_ctr)++; } paint(x->p, BLACK); paint(w->l, BLACK); right_rotate(x->p); x = rb_root; }
        }
    }
    if (x != NIL && x->c != BLACK){ x->c = BLACK; (*rec_ctr)++; }
}
static int rb_delete(int k)
{
    R *z = rb_root; while (z != NIL && z->k != k) z = k < z->k ? z->l : z->r;
    if (z == NIL) return 0;
    if (z == leftmost){ R *s; if (z->r != NIL) s = tree_min(z->r); else { s = z->p; } leftmost = s; }   /* successor becomes cached minimum */
    R *y = z, *x; int yc = y->c;
    if (z->l == NIL){ x = z->r; transplant(z, z->r); }
    else if (z->r == NIL){ x = z->l; transplant(z, z->l); }
    else { y = tree_min(z->r); yc = y->c; x = y->r;
           if (y->p == z) x->p = y; else { transplant(y, y->r); y->r = z->r; y->r->p = y; }
           transplant(z, y); y->l = z->l; y->l->p = y; y->c = z->c; }
    free(z); if (yc == BLACK) delete_fixup(x);
    NIL->p = NIL; NIL->l = NIL->r = NIL; NIL->c = BLACK;
    if (rb_root == NIL) leftmost = NIL;
    return 1;
}
/* checker: returns black-height or -1 on violation; props: 1 root black, 2 nodes red/black (by type), 3 NIL leaves black, 4 no red-red, 5 equal black height */
static int rb_check_node(R *n, int lo, int hi, int *bad)
{
    if (n == NIL) return 1;
    if (n->k <= lo || n->k >= hi){ bad[0]++; }
    if (n->c == RED && (n->l->c == RED || n->r->c == RED)) bad[1]++;               /* property 4 */
    int a = rb_check_node(n->l, lo, n->k, bad), b = rb_check_node(n->r, n->k, hi, bad);
    if (a != b) bad[2]++;                                                          /* property 5 */
    return a + (n->c == BLACK);
}
static int rb_valid(int *which)
{
    int bad[3] = {0, 0, 0}; if (rb_root != NIL && rb_root->c != BLACK) bad[1] += 1000;            /* property 2/root */
    if (NIL->c != BLACK) bad[1] += 1000;                                                          /* property 3 */
    rb_check_node(rb_root, -1, 1 << 30, bad); if (which){ which[0] = bad[0]; which[1] = bad[1]; which[2] = bad[2]; }
    return !bad[0] && !bad[1] && !bad[2];
}
static int rb_height(R *n){ return n == NIL ? 0 : 1 + (rb_height(n->l) > rb_height(n->r) ? rb_height(n->l) : rb_height(n->r)); }
static int rb_count(R *n){ return n == NIL ? 0 : 1 + rb_count(n->l) + rb_count(n->r); }
static void rb_free(R *n){ if (n != NIL){ rb_free(n->l); rb_free(n->r); free(n);} }
static void rb_show(R *n){ if (n == NIL){ printf("."); return; } printf("%d%c", n->k, n->c == RED ? 'r' : 'b'); if (n->l == NIL && n->r == NIL) return; printf("("); rb_show(n->l); printf(","); rb_show(n->r); printf(")"); }
static int fid;
static int rb_dump(R *t, FILE *f){ if (t == NIL) return -1; int me = fid++, l = rb_dump(t->l, f), r = rb_dump(t->r, f); fprintf(f, "N %d %d %c %d %d\n", me, t->k, t->c == RED ? 'R' : 'B', l, r); return me; }
static void rb_fig(FILE *f, const char *name){ fprintf(f, "@FIG %s\n", name); fid = 0; int root = rb_dump(rb_root, f); fprintf(f, "ROOT %d\n", root); }

/* ---------------- AVL with deletion ---------------- */
typedef struct A { int k, h; struct A *l, *r; } A;
static long avl_rot_ins, avl_rot_del, *arot;
static int ah(A *n){ return n ? n->h : 0; }
static void afix(A *n){ n->h = 1 + (ah(n->l) > ah(n->r) ? ah(n->l) : ah(n->r)); }
static A *arr(A *y){ A *x = y->l; y->l = x->r; x->r = y; afix(y); afix(x); (*arot)++; return x; }
static A *arl(A *x){ A *y = x->r; x->r = y->l; y->l = x; afix(x); afix(y); (*arot)++; return y; }
static A *abal(A *n)
{
    afix(n); int b = ah(n->l) - ah(n->r);
    if (b > 1){ if (ah(n->l->l) < ah(n->l->r)) n->l = arl(n->l); return arr(n); }
    if (b < -1){ if (ah(n->r->r) < ah(n->r->l)) n->r = arr(n->r); return arl(n); }
    return n;
}
static A *ains(A *n, int k){ if (!n){ n = malloc(sizeof *n); n->k = k; n->h = 1; n->l = n->r = NULL; return n; } if (k < n->k) n->l = ains(n->l, k); else if (k > n->k) n->r = ains(n->r, k); else return n; return abal(n); }
static A *adel(A *n, int k)
{
    if (!n) return NULL;
    if (k < n->k) n->l = adel(n->l, k); else if (k > n->k) n->r = adel(n->r, k);
    else { if (!n->l || !n->r){ A *c = n->l ? n->l : n->r; free(n); return c; } A *s = n->r; while (s->l) s = s->l; n->k = s->k; n->r = adel(n->r, s->k); }
    return abal(n);
}
static int acheck(A *n, int lo, int hi, int *h){ if (!n){ *h = 0; return 1; } if (n->k <= lo || n->k >= hi) return 0; int a, b; if (!acheck(n->l, lo, n->k, &a) || !acheck(n->r, n->k, hi, &b)) return 0; if (abs(a - b) > 1 || n->h != 1 + (a > b ? a : b)) return 0; *h = n->h; return 1; }
static void afree(A *n){ if (n){ afree(n->l); afree(n->r); free(n);} }
static unsigned long long rs = 88172645463325252ULL;
static unsigned rnd(void){ rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (unsigned)(rs >> 11); }

int main(void)
{
    FILE *fg = fopen("fig_q4.txt", "w"); rb_root = NIL; leftmost = NIL;
    puts("=== 1. Hand trace: insert 10 20 30 15 25 5 (r = red, b = black) ===");
    rot_ctr = &rb_rot_ins; rec_ctr = &recolour_ins; trace = 1; int ks[] = {10, 20, 30, 15, 25, 5};
    for (int i = 0; i < 6; i++){
        printf("insert %d:\n", ks[i]); rb_insert(ks[i]); printf("    result: "); rb_show(rb_root); int w[3]; printf("   valid = %s, black-height ok\n", rb_valid(w) ? "YES" : "NO");
        char nm[16]; sprintf(nm, "rb%d", ks[i]); rb_fig(fg, nm);
    }
    trace = 0; printf("rotations %ld, recolourings %ld; smallest (leftmost) = %d\n", rb_rot_ins, recolour_ins, tree_min(rb_root)->k);
    rb_free(rb_root); rb_root = NIL; leftmost = NIL; rb_rot_ins = recolour_ins = 0;

    puts("\n=== 2. 10,000 random insertions then 10,000 random deletions: AVL vs red-black ===");
    enum { N = 10000 }; static int keys[N], order[N];
    for (int i = 0; i < N; i++) keys[i] = i + 1; for (int i = N - 1; i > 0; i--){ int j = rnd() % (i + 1), t = keys[i]; keys[i] = keys[j]; keys[j] = t; }
    memcpy(order, keys, sizeof keys); for (int i = N - 1; i > 0; i--){ int j = rnd() % (i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    rb_root = NIL; leftmost = NIL; A *avl = NULL; int viol = 0, minbad = 0, hmax_rb = 0, hmax_avl = 0, hbad = 0;
    rot_ctr = &rb_rot_ins; rec_ctr = &recolour_ins; arot = &avl_rot_ins;
    for (int i = 0; i < N; i++){
        rb_insert(keys[i]); avl = ains(avl, keys[i]);
        if (!rb_valid(NULL)) viol++; if (leftmost != tree_min(rb_root)) minbad++;
        int h; if (!acheck(avl, 0, 1 << 30, &h)) hbad++;
        if (rb_height(rb_root) > hmax_rb) hmax_rb = rb_height(rb_root); if (ah(avl) > hmax_avl) hmax_avl = ah(avl);
    }
    int hr = rb_height(rb_root), ha = ah(avl);
    printf("after %d insertions: RB height %d (bound 2*log2(n+1) = %.1f), AVL height %d (bound 1.44*log2(n+2) = %.1f)\n", N, hr, 2 * 13.288, ha, 1.44 * 13.288);
    printf("insertions: AVL rotations %ld (%.3f per insertion); RB rotations %ld (%.3f per insertion), RB recolourings %ld\n", avl_rot_ins, (double)avl_rot_ins / N, rb_rot_ins, (double)rb_rot_ins / N, recolour_ins);
    rot_ctr = &rb_rot_del; rec_ctr = &recolour_del; arot = &avl_rot_del;
    for (int i = 0; i < N; i++){
        if (leftmost != tree_min(rb_root)) minbad++;
        rb_delete(order[i]); avl = adel(avl, order[i]);
        if (!rb_valid(NULL)) viol++; if (rb_root != NIL && leftmost != tree_min(rb_root)) minbad++;
        int h; if (!acheck(avl, 0, 1 << 30, &h)) hbad++;
    }
    printf("deletions : AVL rotations %ld (%.3f per deletion); RB rotations %ld (%.3f per deletion), RB recolourings %ld\n", avl_rot_del, (double)avl_rot_del / N, rb_rot_del, (double)rb_rot_del / N, recolour_del);
    printf("tree empty at the end: RB nodes %d, AVL nodes %s\n", rb_count(rb_root), avl ? "not empty" : "0");
    printf("verification after every one of the %d operations: RB property violations = %d, AVL balance violations = %d, cached-leftmost mismatches = %d\n", 2 * N, viol, hbad, minbad);
    printf("maximum heights seen during the run: RB %d, AVL %d\n", hmax_rb, hmax_avl);
    printf("TOTAL rotations insertion+deletion: AVL %ld, RB %ld\n", avl_rot_ins + avl_rot_del, rb_rot_ins + rb_rot_del);
    afree(avl);

    puts("\n=== 3. Mixed workload: 10,000 random insertions, then 10,000 operations (random insert or delete) ===");
    rb_root = NIL; leftmost = NIL; avl = NULL; long ri = 0, ai = 0, rd = 0, ad = 0; rb_rot_ins = rb_rot_del = avl_rot_ins = avl_rot_del = 0; recolour_ins = recolour_del = 0;
    rot_ctr = &rb_rot_ins; rec_ctr = &recolour_ins; arot = &avl_rot_ins; for (int i = 0; i < N; i++){ rb_insert(keys[i]); avl = ains(avl, keys[i]); }
    long rot_r0 = rb_rot_ins, rot_a0 = avl_rot_ins; int next = N + 1; int live[N * 2]; int nl = N; memcpy(live, keys, sizeof keys); int ins = 0, del = 0;
    for (int i = 0; i < N; i++){
        if (rnd() % 2){ rot_ctr = &rb_rot_ins; arot = &avl_rot_ins; rb_insert(next); avl = ains(avl, next); live[nl++] = next++; ins++; }
        else { int j = rnd() % nl; int k = live[j]; live[j] = live[--nl]; rot_ctr = &rb_rot_del; arot = &avl_rot_del; rb_delete(k); avl = adel(avl, k); del++; }
    }
    (void)ri; (void)ai; (void)rd; (void)ad;
    printf("%d random inserts and %d random deletes on the 10,000-node trees: AVL rotations insert %ld / delete %ld, RB rotations insert %ld / delete %ld\n", ins, del, avl_rot_ins - rot_a0, avl_rot_del, rb_rot_ins - rot_r0, rb_rot_del);
    printf("   per operation: AVL %.3f, RB %.3f; heights now RB %d, AVL %d, nodes %d\n", (double)(avl_rot_ins - rot_a0 + avl_rot_del) / N, (double)(rb_rot_ins - rot_r0 + rb_rot_del) / N, rb_height(rb_root), ah(avl), rb_count(rb_root));
    rb_free(rb_root); afree(avl); rb_root = NIL; leftmost = NIL;

    puts("\n=== 4. Keys inserted in INCREASING order (1..10,000) ===");
    avl = NULL; rb_rot_ins = avl_rot_ins = recolour_ins = 0; rot_ctr = &rb_rot_ins; rec_ctr = &recolour_ins; arot = &avl_rot_ins; viol = 0;
    for (int i = 1; i <= N; i++){ rb_insert(i); avl = ains(avl, i); if (!rb_valid(NULL)) viol++; }
    printf("RB height %d (bound %.1f), AVL height %d; rotations RB %ld, AVL %ld; RB violations %d; cached minimum = %d, leftmost walk = %d\n", rb_height(rb_root), 2 * 13.288, ah(avl), rb_rot_ins, avl_rot_ins, viol, leftmost->k, tree_min(rb_root)->k);
    rb_free(rb_root); afree(avl);
    fclose(fg); return 0;
}
