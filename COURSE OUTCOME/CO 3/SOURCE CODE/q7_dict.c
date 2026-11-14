/* Q7 - Dictionary for a regional-language app: compact (first-child / next-sibling) trie as the chosen design,
   compared with a sorted array + binary search, an AVL tree of strings and a hash table.
   Operations: exact look-up, autocomplete, alphabetical range listing, spell suggestions (edit distance <= 1), weekly update.
   Words are generated (seeded) from syllables because no real dictionary file is available offline.
   usage: q7_dict [n_words] [verify_only] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
static volatile long sink;
static double now(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec * 1e-9; }
static unsigned long long rs = 88172645463325252ULL;
static unsigned rnd(void){ rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (unsigned)(rs >> 11); }
static int cmpstr(const void *a, const void *b){ return strcmp(*(char *const *)a, *(char *const *)b); }

/* ---------- word generator ---------- */
static char *arena; static size_t arena_used;
static char **gen_words(int n)
{
    static const char *cons[] = {"k","m","n","r","s","t","v","l","p","d","b","g","j","h","y"}, *vow[] = {"a","i","u","e","o","aa","ai"};
    size_t cap = (size_t)n * 2 * 16; char *pool = malloc(cap); size_t used = 0; char **c = malloc((size_t)n * 2 * sizeof *c);
    for (int i = 0; i < 2 * n; i++){ char buf[32] = ""; int syl = 2 + rnd() % 4; for (int s = 0; s < syl; s++){ strcat(buf, cons[rnd() % 15]); strcat(buf, vow[rnd() % 7]); } if (rnd() % 3 == 0) strcat(buf, "n");
        c[i] = pool + used; strcpy(c[i], buf); used += strlen(buf) + 1; }
    qsort(c, 2 * n, sizeof *c, cmpstr);
    char **a = malloc((size_t)n * sizeof *a); int m = 0; for (int i = 0; i < 2 * n && m < n; i++) if (m == 0 || strcmp(c[i], a[m - 1])) a[m++] = c[i];
    free(c); if (m < n){ fprintf(stderr, "only %d distinct words\n", m); exit(1); }
    /* compact arena: copy kept words contiguously (sorted order) so the "array" memory figure is honest */
    size_t tot = 0; for (int i = 0; i < n; i++) tot += strlen(a[i]) + 1; arena = malloc(tot); arena_used = 0;
    for (int i = 0; i < n; i++){ size_t L = strlen(a[i]) + 1; memcpy(arena + arena_used, a[i], L); a[i] = arena + arena_used; arena_used += L; }
    free(pool); return a;
}

/* ---------- compact trie ---------- */
typedef struct { uint32_t child, sib; char ch; uint8_t end; } TN;               /* 12 bytes */
static TN *tn; static uint32_t tcount, tcap;
static uint32_t tnew(char ch){ if (tcount == tcap){ tcap = tcap ? tcap * 2 : 1024; tn = realloc(tn, (size_t)tcap * sizeof *tn); } tn[tcount].child = tn[tcount].sib = 0; tn[tcount].ch = ch; tn[tcount].end = 0; return tcount++; }
static void trie_insert(const char *w)
{
    uint32_t cur = 0;                                          /* node 0 = root */
    for (; *w; w++){
        uint32_t *link = &tn[cur].child; uint32_t x = *link; uint32_t prev = 0;
        while (x && tn[x].ch < *w){ prev = x; x = tn[x].sib; }
        if (!x || tn[x].ch != *w){ uint32_t nx = tnew(*w); tn[nx].sib = x; if (prev) tn[prev].sib = nx; else tn[cur].child = nx; x = nx; }
        cur = x;
    }
    tn[cur].end = 1;
}
static uint32_t trie_walk(const char *p){ uint32_t cur = 0; for (; *p; p++){ uint32_t x = tn[cur].child; while (x && tn[x].ch < *p) x = tn[x].sib; if (!x || tn[x].ch != *p) return UINT32_MAX; cur = x; } return cur; }
static int trie_exact(const char *w){ uint32_t x = trie_walk(w); return x != UINT32_MAX && tn[x].end; }
static int trie_delete(const char *w){ uint32_t x = trie_walk(w); if (x == UINT32_MAX || !tn[x].end) return 0; tn[x].end = 0; return 1; }   /* lazy: unmark; space reclaimed at the weekly rebuild */
/* alphabetical collection below node x (prefix already in buf) */
static void trie_collect(uint32_t x, char *buf, int len, char (*out)[40], int *n, int limit)
{
    if (*n >= limit) return;
    if (tn[x].end){ buf[len] = 0; strcpy(out[(*n)++], buf); }
    for (uint32_t c = tn[x].child; c && *n < limit; c = tn[c].sib){ buf[len] = tn[c].ch; trie_collect(c, buf, len + 1, out, n, limit); }
}
static int trie_autocomplete(const char *p, char (*out)[40], int limit){ uint32_t x = trie_walk(p); if (x == UINT32_MAX) return 0; char buf[48]; strcpy(buf, p); int n = 0; trie_collect(x, buf, (int)strlen(p), out, &n, limit); return n; }
/* range listing lo <= w <= hi in alphabetical order (up to limit): DFS with pruning on both bounds */
static void trie_range_rec(uint32_t x, char *buf, int len, const char *lo, const char *hi, int lo_tight, int hi_tight, char (*out)[40], int *n, int limit)
{
    if (*n >= limit) return;
    if (tn[x].end && !(lo_tight && lo[len] != 0)){ buf[len] = 0; /* word equals the current prefix; it is >= lo unless lo is longer */ if (!(hi_tight && 0)) strcpy(out[(*n)++], buf); }
    for (uint32_t c = tn[x].child; c && *n < limit; c = tn[c].sib){
        char ch = tn[c].ch; int lt = lo_tight, ht = hi_tight;
        if (lt){ if (lo[len] == 0) lt = 0; else if (ch < lo[len]) continue; else if (ch > lo[len]) lt = 0; }
        if (ht){ if (hi[len] == 0) continue; if (ch > hi[len]) break; if (ch < hi[len]) ht = 0; }
        buf[len] = ch; trie_range_rec(c, buf, len + 1, lo, hi, lt, ht, out, n, limit);
    }
}
static int trie_range(const char *lo, const char *hi, char (*out)[40], int limit){ char buf[48]; int n = 0; trie_range_rec(0, buf, 0, lo, hi, 1, 1, out, &n, limit); return n; }
/* spell suggestions: words within Levenshtein distance 1 of q, alphabetical (DP row per trie depth, prune when min > 1) */
static void trie_sugg_rec(uint32_t x, int depth, const char *q, int m, int *prow, char *buf, char (*out)[40], int *n, int limit)
{
    for (uint32_t c = tn[x].child; c && *n < limit; c = tn[c].sib){
        int row[48]; row[0] = prow[0] + 1; int mn = row[0];
        for (int j = 1; j <= m; j++){ int v = prow[j - 1] + (q[j - 1] != tn[c].ch); if (prow[j] + 1 < v) v = prow[j] + 1; if (row[j - 1] + 1 < v) v = row[j - 1] + 1; row[j] = v; if (v < mn) mn = v; }
        buf[depth] = tn[c].ch;
        if (tn[c].end && row[m] <= 1){ buf[depth + 1] = 0; strcpy(out[(*n)++], buf); if (*n >= limit) return; }
        if (mn <= 1) trie_sugg_rec(c, depth + 1, q, m, row, buf, out, n, limit);
    }
}
static int trie_suggest(const char *q, char (*out)[40], int limit)
{
    int m = (int)strlen(q), row[48]; for (int j = 0; j <= m; j++) row[j] = j; char buf[48]; int n = 0;
    if (tn[0].end && m <= 1){ strcpy(out[n++], ""); }
    trie_sugg_rec(0, 0, q, m, row, buf, out, &n, limit); return n;
}

/* ---------- sorted array ---------- */
static long arr_lower(char **a, long n, const char *p){ long lo = 0, hi = n; while (lo < hi){ long mid = (lo + hi) / 2; if (strcmp(a[mid], p) < 0) lo = mid + 1; else hi = mid; } return lo; }
static long arr_lower_prefix(char **a, long n, const char *p){ size_t m = strlen(p); long lo = 0, hi = n; while (lo < hi){ long mid = (lo + hi) / 2; if (strncmp(a[mid], p, m) < 0) lo = mid + 1; else hi = mid; } return lo; }
static int arr_exact(char **a, long n, const char *w){ long i = arr_lower(a, n, w); return i < n && !strcmp(a[i], w); }
static int arr_autocomplete(char **a, long n, const char *p, char (*out)[40], int limit){ long i = arr_lower_prefix(a, n, p); size_t m = strlen(p); int k = 0; while (i < n && k < limit && !strncmp(a[i], p, m)) strcpy(out[k++], a[i++]); return k; }
static int arr_range(char **a, long n, const char *lo, const char *hi, char (*out)[40], int limit){ long i = arr_lower(a, n, lo); int k = 0; while (i < n && k < limit && strcmp(a[i], hi) <= 0) strcpy(out[k++], a[i++]); return k; }
/* suggestions by generating every edit-1 variant and probing the array */
static int arr_suggest(char **a, long n, const char *q, char (*out)[40], int limit)
{
    int m = (int)strlen(q), k = 0; char v[48];
    static char found[512][40]; int nf = 0;
    #define TRY(s) do { if (arr_exact(a, n, s)){ int dup = 0; for (int z = 0; z < nf; z++) if (!strcmp(found[z], s)) dup = 1; if (!dup && nf < 512) strcpy(found[nf++], s); } } while (0)
    for (int i = 0; i <= m; i++) for (int c = 'a'; c <= 'z'; c++){ memcpy(v, q, i); v[i] = c; strcpy(v + i + 1, q + i); TRY(v); }       /* insertions */
    for (int i = 0; i < m; i++){ memcpy(v, q, i); strcpy(v + i, q + i + 1); TRY(v); }                                                /* deletions */
    for (int i = 0; i < m; i++) for (int c = 'a'; c <= 'z'; c++){ strcpy(v, q); v[i] = c; TRY(v); }                                    /* substitutions */
    /* alphabetical */
    for (int i = 0; i < nf; i++) for (int j = i + 1; j < nf; j++) if (strcmp(found[j], found[i]) < 0){ char t[40]; strcpy(t, found[i]); strcpy(found[i], found[j]); strcpy(found[j], t); }
    for (int i = 0; i < nf && k < limit; i++) strcpy(out[k++], found[i]); return k;
}
/* ---------- AVL of strings ---------- */
typedef struct AN { const char *s; struct AN *l, *r; int h; } AN;
static int ah(AN *n){ return n ? n->h : 0; }
static void afix(AN *n){ n->h = 1 + (ah(n->l) > ah(n->r) ? ah(n->l) : ah(n->r)); }
static AN *arr_(AN *y){ AN *x = y->l; y->l = x->r; x->r = y; afix(y); afix(x); return x; }
static AN *arl_(AN *x){ AN *y = x->r; x->r = y->l; y->l = x; afix(x); afix(y); return y; }
static AN *pool_nodes; static long pool_used;
static AN *avl_insert(AN *n, const char *s){ if (!n){ AN *x = &pool_nodes[pool_used++]; x->s = s; x->l = x->r = NULL; x->h = 1; return x; } int c = strcmp(s, n->s); if (c < 0) n->l = avl_insert(n->l, s); else if (c > 0) n->r = avl_insert(n->r, s); else return n; afix(n); int b = ah(n->l) - ah(n->r);
    if (b > 1 && strcmp(s, n->l->s) < 0) return arr_(n); if (b < -1 && strcmp(s, n->r->s) > 0) return arl_(n); if (b > 1){ n->l = arl_(n->l); return arr_(n); } if (b < -1){ n->r = arr_(n->r); return arl_(n); } return n; }
static int avl_exact(AN *t, const char *w){ while (t){ int c = strcmp(w, t->s); if (!c) return 1; t = c < 0 ? t->l : t->r; } return 0; }
static void avl_prefix(AN *t, const char *p, size_t m, char (*out)[40], int *k, int limit)
{ if (!t || *k >= limit) return; int c = strncmp(p, t->s, m); if (c <= 0) avl_prefix(t->l, p, m, out, k, limit); if (c == 0 && *k < limit) strcpy(out[(*k)++], t->s); if (c >= 0) avl_prefix(t->r, p, m, out, k, limit); }

/* ---------- hash table ---------- */
static const char **ht; static size_t hcap;
static size_t hh(const char *s){ size_t h = 1469598103934665603ULL; while (*s) h = (h ^ (unsigned char)*s++) * 1099511628211ULL; return h; }
static void h_insert(const char *s){ size_t i = hh(s) & (hcap - 1); while (ht[i]) i = (i + 1) & (hcap - 1); ht[i] = s; }
static int h_exact(const char *s){ size_t i = hh(s) & (hcap - 1); while (ht[i]){ if (!strcmp(ht[i], s)) return 1; i = (i + 1) & (hcap - 1); } return 0; }

/* ---------- verification helpers ---------- */
static int lev1(const char *a, const char *b){ int la = (int)strlen(a), lb = (int)strlen(b); if (abs(la - lb) > 1) return 0; static int d[48][48]; for (int i = 0; i <= la; i++) d[i][0] = i; for (int j = 0; j <= lb; j++) d[0][j] = j;
    for (int i = 1; i <= la; i++) for (int j = 1; j <= lb; j++){ int v = d[i - 1][j - 1] + (a[i - 1] != b[j - 1]); if (d[i - 1][j] + 1 < v) v = d[i - 1][j] + 1; if (d[i][j - 1] + 1 < v) v = d[i][j - 1] + 1; d[i][j] = v; } return d[la][lb] <= 1; }
static void mutate(char *w){ int L = (int)strlen(w); int k = rnd() % 3; int p = rnd() % L; if (k == 0){ w[p] = 'a' + rnd() % 26; } else if (k == 1 && L > 2){ memmove(w + p, w + p + 1, L - p); } else { memmove(w + p + 1, w + p, L - p + 1); w[p] = 'a' + rnd() % 26; } }

int main(int argc, char **argv)
{
    int N = argc > 1 ? atoi(argv[1]) : 200000;
    char **w = gen_words(N); int V = 1000;
    /* ===== Stage 3a: verification on a 1,000-word sample ===== */
    { tcount = 0; tcap = 0; free(tn); tn = NULL; tnew(0);
      /* 1,000 words, shuffled, inserted in random order */
      char **s = malloc(V * sizeof *s); for (int i = 0; i < V; i++) s[i] = w[(long)i * (N / V)];
      char **sh = malloc(V * sizeof *sh); memcpy(sh, s, V * sizeof *s); for (int i = V - 1; i > 0; i--){ int j = rnd() % (i + 1); char *t = sh[i]; sh[i] = sh[j]; sh[j] = t; }
      for (int i = 0; i < V; i++) trie_insert(sh[i]);
      int bad = 0, tests = 0; char (*o1)[40] = malloc(64 * 40), (*o2)[40] = malloc(64 * 40);
      for (int i = 0; i < V; i++){ tests++; if (!trie_exact(s[i])) bad++; char nw[48]; strcpy(nw, s[i]); strcat(nw, "z"); tests++; if (trie_exact(nw) != arr_exact(s, V, nw)) bad++; }
      for (int i = 0; i < 300; i++){ char p[8]; int L = 1 + rnd() % 3; strncpy(p, s[rnd() % V], L); p[L] = 0; tests++; int a = trie_autocomplete(p, o1, 10), b = arr_autocomplete(s, V, p, o2, 10); if (a != b) bad++; else for (int j = 0; j < a; j++) if (strcmp(o1[j], o2[j])) bad++; }
      for (int i = 0; i < 200; i++){ const char *x = s[rnd() % V], *y = s[rnd() % V]; const char *lo = strcmp(x, y) < 0 ? x : y, *hi = strcmp(x, y) < 0 ? y : x; tests++; int a = trie_range(lo, hi, o1, 60), b = arr_range(s, V, lo, hi, o2, 60); if (a != b) bad++; else for (int j = 0; j < a; j++) if (strcmp(o1[j], o2[j])) bad++; }
      int sug_ok = 0, sug_ok2 = 0, sug_tests = 0;
      for (int i = 0; i < 300; i++){ char q[48]; strcpy(q, s[rnd() % V]); mutate(q); sug_tests++;
          int a = trie_suggest(q, o1, 64); char ref[64][40]; int b = 0; for (int j = 0; j < V && b < 64; j++) if (lev1(q, s[j])) strcpy(ref[b++], s[j]);
          int same = (a == b); if (same) for (int j = 0; j < a; j++) if (strcmp(o1[j], ref[j])) same = 0; if (same) sug_ok++;
          int c2 = arr_suggest(s, V, q, o2, 64); int same2 = (c2 == b); if (same2) for (int j = 0; j < c2; j++) if (strcmp(o2[j], ref[j])) same2 = 0; if (same2) sug_ok2++; }
      printf("=== verification on %d words (trie vs sorted array / brute force) ===\n", V);
      printf("exact look-up, autocomplete (300 prefixes), alphabetical range (200 ranges): %d tests, %d mismatches\n", tests, bad);
      printf("spell suggestions (300 mistyped words, edit distance <= 1): trie %d / %d and sorted array %d / %d identical to brute force over the 1,000 words\n", sug_ok, sug_tests, sug_ok2, sug_tests);
      /* readable example */
      const char *demo = s[500]; char q[48]; strcpy(q, demo); q[1] = (q[1] == 'x') ? 'y' : 'x';
      printf("example: look-up \"%s\" -> %d; typo \"%s\" suggestions:", demo, trie_exact(demo), q); int n = trie_suggest(q, o1, 5); for (int j = 0; j < n; j++) printf(" %s", o1[j]); puts("");
      char p2[4]; strncpy(p2, demo, 2); p2[2] = 0; n = trie_autocomplete(p2, o1, 5); printf("example: autocomplete \"%s\" (first 5):", p2); for (int j = 0; j < n; j++) printf(" %s", o1[j]); puts("");
      n = trie_range(s[100], s[110], o1, 20); printf("example: range \"%s\"..\"%s\": %d words (sample sorted list has 10 positions between them)\n", s[100], s[110], n);
      free(o1); free(o2); free(s); free(sh); }
    if (argc > 2) return 0;

    /* ===== build all structures for N words ===== */
    printf("\n=== N = %d words (generated, sorted, distinct), average length %.2f letters ===\n", N, (double)(arena_used - N) / N);
    double t0, t1;
    free(tn); tn = NULL; tcount = 0; tcap = 0; tnew(0);
    char **shuf = malloc((size_t)N * sizeof *shuf); memcpy(shuf, w, (size_t)N * sizeof *w); for (long i = N - 1; i > 0; i--){ long j = rnd() % (i + 1); char *t = shuf[i]; shuf[i] = shuf[j]; shuf[j] = t; }
    t0 = now(); for (int i = 0; i < N; i++) trie_insert(shuf[i]); t1 = now(); double trie_build = t1 - t0;
    size_t trie_bytes = (size_t)tcount * sizeof(TN);
    pool_nodes = malloc((size_t)N * sizeof(AN)); pool_used = 0; AN *avl = NULL; t0 = now(); for (int i = 0; i < N; i++) avl = avl_insert(avl, shuf[i]); t1 = now(); double avl_build = t1 - t0;
    size_t avl_bytes = (size_t)N * sizeof(AN) + arena_used;
    size_t arr_bytes = (size_t)N * sizeof(char *) + arena_used;
    hcap = 1; while (hcap < (size_t)N * 2) hcap <<= 1; ht = calloc(hcap, sizeof *ht); t0 = now(); for (int i = 0; i < N; i++) h_insert(shuf[i]); t1 = now(); double h_build = t1 - t0;
    size_t hash_bytes = hcap * sizeof *ht;
    printf("nodes in compact trie: %u (%.2f nodes per word), node = %zu bytes\n", tcount, (double)tcount / N, sizeof(TN));
    printf("%-40s %14s %14s\n", "structure", "bytes", "bytes per word");
    printf("%-40s %14zu %14.1f\n", "compact trie (child/sibling, 12 B nodes)", trie_bytes, (double)trie_bytes / N);
    printf("%-40s %14zu %14.1f\n", "26-pointer trie (216 B nodes, computed)", (size_t)tcount * 216, (double)tcount * 216 / N);
    printf("%-40s %14zu %14.1f\n", "sorted array of pointers + strings", arr_bytes, (double)arr_bytes / N);
    printf("%-40s %14zu %14.1f\n", "AVL tree of strings (32 B nodes + strings)", avl_bytes, (double)avl_bytes / N);
    printf("%-40s %14zu %14.1f   (plus the sorted array if range/autocomplete are needed: %zu)\n", "hash table (open addressing, load 0.25-0.5)", hash_bytes + arena_used, (double)(hash_bytes + arena_used) / N, hash_bytes + arr_bytes);
    printf("build time (random insertion order): trie %.3f s, AVL %.3f s, hash %.3f s\n", trie_build, avl_build, h_build);

    /* ===== response times ===== */
    puts("\n--- median response time per operation (ns), 5 timed rounds ---");
    enum { Q = 20000 }; char (*qw)[48] = malloc(Q * sizeof *qw), (*qp)[8] = malloc(Q * sizeof *qp), (*qt)[48] = malloc(Q * sizeof *qt);
    for (int i = 0; i < Q; i++){ strcpy(qw[i], w[rnd() % N]); int L = 2 + rnd() % 3; strncpy(qp[i], qw[i], L); qp[i][L] = 0; strcpy(qt[i], qw[i]); mutate(qt[i]); }
    char (*out)[40] = malloc(64 * 40);
    #define BENCH(label, reps, EXPR) do { double v[5]; for (int r = 0; r < 5; r++){ double a = now(); for (int i = 0; i < (reps); i++){ sink += (EXPR); } double b = now(); v[r] = (b - a) / (reps) * 1e9; } \
        for (int i = 0; i < 5; i++) for (int j = i + 1; j < 5; j++) if (v[j] < v[i]){ double t = v[i]; v[i] = v[j]; v[j] = t; } printf("%-62s %12.0f\n", label, v[2]); } while (0)
    BENCH("exact look-up, compact trie", Q, trie_exact(qw[i]));
    BENCH("exact look-up, sorted array + binary search", Q, arr_exact(w, N, qw[i]));
    BENCH("exact look-up, AVL tree of strings", Q, avl_exact(avl, qw[i]));
    BENCH("exact look-up, hash table", Q, h_exact(qw[i]));
    BENCH("autocomplete, first 10 words, compact trie", Q, trie_autocomplete(qp[i], out, 10));
    BENCH("autocomplete, first 10 words, sorted array (2 binary searches)", Q, arr_autocomplete(w, N, qp[i], out, 10));
    BENCH("autocomplete, first 10 words, AVL tree", Q / 4, ({ int k = 0; avl_prefix(avl, qp[i], strlen(qp[i]), out, &k, 10); k; }));
    BENCH("range listing of 50 words from a random start, compact trie", Q / 4, trie_range(qw[i], "zzzzzzzzzzzzzzzzzzzz", out, 50));
    BENCH("range listing of 50 words from a random start, sorted array", Q / 4, arr_range(w, N, qw[i], "zzzzzzzzzzzzzzzzzzzz", out, 50));
    BENCH("spell suggestions (edit distance <= 1), compact trie", 2000, trie_suggest(qt[i], out, 10));
    BENCH("spell suggestions (edit distance <= 1), sorted array + variants", 500, arr_suggest(w, N, qt[i], out, 10));
    BENCH("spell suggestions (edit distance <= 1), hash table + variants", 500, ({ int hit = 0; char v[48]; const char *q = qt[i]; int m = (int)strlen(q); for (int p = 0; p <= m; p++) for (int c = 'a'; c <= 'z'; c++){ memcpy(v, q, p); v[p] = c; strcpy(v + p + 1, q + p); hit += h_exact(v); } for (int p = 0; p < m; p++){ memcpy(v, q, p); strcpy(v + p, q + p + 1); hit += h_exact(v); for (int c = 'a'; c <= 'z'; c++){ strcpy(v, q); v[p] = c; hit += h_exact(v); } } hit; }));
    /* suggestion hit rate: original word among the suggestions */
    int hit = 0, tot = 400; for (int i = 0; i < tot; i++){ char (*o)[40] = out; int n = trie_suggest(qt[i], o, 64); for (int j = 0; j < n; j++) if (!strcmp(o[j], qw[i])) { hit++; break; } }
    printf("suggestion hit rate (original word returned among <= 64 candidates) for 400 one-edit typos: %d / %d\n", hit, tot);

    /* ===== weekly update ===== */
    puts("\n--- weekly update: add 2,000 new words and remove 500 old words ---");
    int U = 2000, D = 500; char **neu = gen_words(U + 1000);
    /* use words not already present */
    char **add = malloc(U * sizeof *add); int na = 0; for (int i = 0; i < U + 1000 && na < U; i++) if (!trie_exact(neu[i])) add[na++] = neu[i];
    t0 = now(); for (int i = 0; i < na; i++) trie_insert(add[i]); for (int i = 0; i < D; i++) trie_delete(w[i * (N / D)]); t1 = now();
    printf("trie: in-place insert %d + lazy delete %d: %.4f s (nodes %u)\n", na, D, t1 - t0, tcount);
    /* sorted array: one-by-one insertion with memmove vs merge rebuild */
    char **A = malloc(((size_t)N + na) * sizeof *A); memcpy(A, w, (size_t)N * sizeof *w); long cnt = N;
    t0 = now(); for (int i = 0; i < na; i++){ long p = arr_lower(A, cnt, add[i]); memmove(A + p + 1, A + p, (size_t)(cnt - p) * sizeof *A); A[p] = add[i]; cnt++; } t1 = now();
    printf("sorted array: %d one-by-one insertions with memmove (O(n) each): %.4f s\n", na, t1 - t0);
    char **B = malloc(((size_t)N + na) * sizeof *B); qsort(add, na, sizeof *add, cmpstr); t0 = now(); { long i = 0, j = 0, k = 0; while (i < N || j < na){ if (j >= na || (i < N && strcmp(w[i], add[j]) < 0)) B[k++] = w[i++]; else B[k++] = add[j++]; } } t1 = now();
    printf("sorted array: one merge of the sorted weekly batch (O(n + k)): %.4f s\n", t1 - t0);
    printf("hash + sorted array must do BOTH: hash insert plus the array update above (and a second copy of every pointer)\n");
    free(A); free(B); free(add);
    return 0;
}
