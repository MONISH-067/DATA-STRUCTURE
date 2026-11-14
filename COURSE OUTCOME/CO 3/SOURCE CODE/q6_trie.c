/* Q6 - Trie: insert, search, count_prefix, autocomplete (alphabetical), delete, scaling test with 500/5,000/50,000 names,
   memory of trie vs sorted array, edge cases. Names are generated (seeded) from syllables: no real contacts are used. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct Trie { struct Trie *child[26]; int words, end; } Trie;
static long nodes_alive; static volatile long sink;
static Trie *node(void){ nodes_alive++; return calloc(1, sizeof(Trie)); }
static void insert(Trie *t, const char *w){ t->words++; for (; *w; w++){ int c = *w - 'a'; if (!t->child[c]) t->child[c] = node(); t = t->child[c]; t->words++; } t->end = 1; }
static const Trie *walk(const Trie *t, const char *p){ for (; *p; p++){ t = t->child[*p - 'a']; if (!t) return NULL; } return t; }
static int search(const Trie *t, const char *w){ const Trie *x = walk(t, w); return x && x->end; }
static int count_prefix(const Trie *t, const char *p){ const Trie *x = walk(t, p); return x ? x->words : 0; }
/* collect up to 'limit' words with the prefix in alphabetical order (children visited a..z) */
static void collect(const Trie *t, char *buf, int len, char out[][32], int *n, int limit)
{
    if (*n >= limit) return;
    if (t->end){ buf[len] = 0; strcpy(out[(*n)++], buf); }
    for (int i = 0; i < 26 && *n < limit; i++) if (t->child[i]){ buf[len] = 'a' + i; collect(t->child[i], buf, len + 1, out, n, limit); }
}
static int autocomplete(const Trie *t, const char *p, char out[][32], int limit)
{
    const Trie *x = walk(t, p); int n = 0; char buf[64]; strcpy(buf, p); if (x) collect(x, buf, (int)strlen(p), out, &n, limit); return n;
}
/* delete a word: unmark, decrement counts along the path, free nodes whose count falls to 0; returns 1 if the word existed */
static int delete_word(Trie *root, const char *w)
{
    if (!search(root, w)) return 0;
    root->words--;
    Trie *t = root; for (const char *s = w; *s; s++){ int c = *s - 'a'; Trie *ch = t->child[c]; ch->words--;
        if (ch->words == 0){ /* free the whole remaining subtree (it only contains this word) */
            Trie *stack[64]; int sp = 0; stack[sp++] = ch; t->child[c] = NULL;
            while (sp){ Trie *x = stack[--sp]; for (int i = 0; i < 26; i++) if (x->child[i]) stack[sp++] = x->child[i]; free(x); nodes_alive--; }
            return 1; }
        t = ch; }
    t->end = 0; return 1;
}
static void destroy(Trie *t){ if (!t) return; for (int i = 0; i < 26; i++) destroy(t->child[i]); free(t); nodes_alive--; }
static double now(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec * 1e-9; }
static unsigned long long rs = 88172645463325252ULL;
static unsigned rnd(void){ rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (unsigned)(rs >> 11); }
static int cmpstr(const void *a, const void *b){ return strcmp(*(char *const *)a, *(char *const *)b); }
/* generate n distinct names from syllables */
static char **gen_names(int n)
{
    static const char *cons[] = {"k","m","n","r","s","t","v","l","p","d","b","g","j","h","y"}, *vow[] = {"a","i","u","e","o","aa","ai"};
    char **a = malloc(n * sizeof *a); int made = 0;
    char **cand = malloc((size_t)n * 2 * sizeof *cand); int nc = 0;
    while (nc < 2 * n){ char buf[32] = ""; int syl = 2 + rnd() % 3; for (int s = 0; s < syl; s++){ strcat(buf, cons[rnd() % 15]); strcat(buf, vow[rnd() % 7]); } if (rnd() % 3 == 0) strcat(buf, "n"); cand[nc++] = strdup(buf); }
    qsort(cand, nc, sizeof *cand, cmpstr);
    for (int i = 0; i < nc; i++){ if (made < n && (made == 0 || strcmp(cand[i], a[made - 1]))) a[made++] = cand[i]; else free(cand[i]); }
    free(cand); if (made < n){ fprintf(stderr, "not enough distinct\n"); exit(1); } return a;
}
/* two binary searches for a prefix range in a sorted array */
static long lower_bound(char **a, long n, const char *p){ long lo = 0, hi = n; size_t m = strlen(p); while (lo < hi){ long mid = (lo + hi) / 2; if (strncmp(a[mid], p, m) < 0) lo = mid + 1; else hi = mid; } return lo; }
static long upper_bound(char **a, long n, const char *p){ long lo = 0, hi = n; size_t m = strlen(p); while (lo < hi){ long mid = (lo + hi) / 2; if (strncmp(a[mid], p, m) <= 0) lo = mid + 1; else hi = mid; } return lo; }

int main(void)
{
    puts("=== 1. Six contacts: counts, nodes, memory ===");
    const char *contacts[] = {"anand", "anitha", "anu", "arun", "bala", "anusuya"};
    Trie *root = node(); for (int i = 0; i < 6; i++) insert(root, contacts[i]);
    const char *typed[] = {"a", "an", "anu", "b", "z"}; for (int i = 0; i < 5; i++) printf("%s:%d ", typed[i], count_prefix(root, typed[i])); puts("");
    printf("nodes (including root) = %ld, memory = %ld x %zu B = %zu B; sum of letters = %d, so sharing saved %d nodes\n", nodes_alive, nodes_alive, sizeof(Trie), nodes_alive * sizeof(Trie), 5 + 6 + 3 + 4 + 4 + 7, 5 + 6 + 3 + 4 + 4 + 7 - (int)(nodes_alive - 1));
    puts("--- autocomplete verified by hand (alphabetical) ---");
    char out[64][32]; const char *pf[] = {"an", "anu", "", "anuz", "anusuya", "b"};
    for (int i = 0; i < 6; i++){ int n = autocomplete(root, pf[i], out, 64); printf("prefix \"%s\" -> %d: ", pf[i], n); for (int j = 0; j < n; j++) printf("%s%s", j ? ", " : "", out[j]); puts(""); }
    puts("--- search, delete ---");
    printf("search(anu)=%d search(an)=%d search(anusuya)=%d\n", search(root, "anu"), search(root, "an"), search(root, "anusuya"));
    long before = nodes_alive; int r1 = delete_word(root, "anusuya"); printf("delete(anusuya) -> %d, nodes %ld -> %ld, count(anu)=%d, search(anu)=%d (still present: prefix word)\n", r1, before, nodes_alive, count_prefix(root, "anu"), search(root, "anu"));
    before = nodes_alive; r1 = delete_word(root, "anu"); printf("delete(anu)     -> %d, nodes %ld -> %ld, count(an)=%d\n", r1, before, nodes_alive, count_prefix(root, "an"));
    { int a1 = delete_word(root, "zzz"), a2 = delete_word(root, "an"); printf("delete(missing) -> %d; delete(an) (a prefix, not a word) -> %d\n", a1, a2); }
    destroy(root);

    puts("\n=== 2. Scaling: prefix search time and memory for 500, 5,000 and 50,000 names ===");
    printf("%-8s %-10s %-14s %-14s %-12s %-14s %-14s %-12s\n", "names", "trie nodes", "trie bytes", "array bytes", "ratio", "trie ns/query", "array ns/query", "strncmp scan");
    int ns[3] = {500, 5000, 50000};
    for (int si = 0; si < 3; si++){
        int n = ns[si]; char **names = gen_names(n);
        nodes_alive = 0; Trie *t = node(); for (int i = 0; i < n; i++) insert(t, names[i]);
        long tb = nodes_alive * (long)sizeof(Trie); long ab = 0; for (int i = 0; i < n; i++) ab += strlen(names[i]) + 1 + sizeof(char *);
        /* 1,000 prefixes of length 3 taken from names */
        int Q = 20000; char (*pref)[8] = malloc(Q * sizeof *pref); for (int i = 0; i < Q; i++){ const char *s = names[rnd() % n]; strncpy(pref[i], s, 3); pref[i][3] = 0; }
        double t0 = now(); for (int r = 0; r < 5; r++) for (int i = 0; i < Q; i++) sink += count_prefix(t, pref[i]); double t1 = now();
        double tt = (t1 - t0) / (5.0 * Q) * 1e9;
        t0 = now(); for (int r = 0; r < 5; r++) for (int i = 0; i < Q; i++) sink += upper_bound(names, n, pref[i]) - lower_bound(names, n, pref[i]); t1 = now();
        double ta = (t1 - t0) / (5.0 * Q) * 1e9;
        /* naive strncmp over every name (what the phone does now) */
        int Qs = Q / 20; t0 = now(); for (int i = 0; i < Qs; i++){ long c = 0; for (int j = 0; j < n; j++) if (!strncmp(names[j], pref[i], 3)) c++; sink += c; } t1 = now();
        double tn = (t1 - t0) / Qs * 1e9;
        /* correctness: trie count == array count */
        int bad = 0; for (int i = 0; i < 2000; i++) if (count_prefix(t, pref[i]) != upper_bound(names, n, pref[i]) - lower_bound(names, n, pref[i])) bad++;
        printf("%-8d %-10ld %-14ld %-14ld %-12.1f %-14.0f %-14.0f %-12.0f  (count mismatches %d)\n", n, nodes_alive, tb, ab, (double)tb / ab, tt, ta, tn, bad);
        destroy(t); free(pref); for (int i = 0; i < n; i++) free(names[i]); free(names);
    }

    puts("\n=== 3. Edge cases on a 5,000-name trie ===");
    char **names = gen_names(5000); nodes_alive = 0; Trie *t = node(); for (int i = 0; i < 5000; i++) insert(t, names[i]);
    printf("empty prefix: count = %d (all words), autocomplete first 5: ", count_prefix(t, "")); int k = autocomplete(t, "", out, 5); for (int j = 0; j < k; j++) printf("%s ", out[j]); puts("");
    const char *w = names[1234]; printf("prefix that is a whole word (\"%s\"): count = %d, autocomplete: ", w, count_prefix(t, w)); k = autocomplete(t, w, out, 5); for (int j = 0; j < k; j++) printf("%s ", out[j]); puts("");
    printf("prefix with no match (\"zzzq\"): count = %d, autocomplete returns %d words\n", count_prefix(t, "zzzq"), autocomplete(t, "zzzq", out, 5));
    int sorted_ok = 1; char (*all)[32] = malloc(5000 * sizeof *all); int na = 0; { char buf[64]; Trie *x = t; (void)x; static char big[5000][32]; na = 0; collect(t, buf, 0, big, &na, 5000); for (int i = 1; i < na; i++) if (strcmp(big[i - 1], big[i]) >= 0) sorted_ok = 0; }
    printf("autocomplete of the whole trie returns %d words in strictly increasing alphabetical order = %s\n", na, sorted_ok ? "YES" : "NO"); free(all);
    destroy(t); for (int i = 0; i < 5000; i++) free(names[i]); free(names);
    printf("memory still allocated after destroy: %ld nodes\n", nodes_alive);
    return 0;
}
