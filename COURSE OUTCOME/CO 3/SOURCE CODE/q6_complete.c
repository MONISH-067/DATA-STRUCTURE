#include <stdio.h>
#include <stdlib.h>
typedef struct Trie {
    struct Trie *child[26];
    int words;                      /* number of words passing through here */
    int end;                        /* 1 if a word ends here */
} Trie;
Trie *node(void) { return calloc(1, sizeof(Trie)); }
void insert(Trie *t, const char *w)
{
    /* TODO 1 done: create missing child, move to it, count the word, mark end at last node */
    for (; *w; w++) {
        int c = *w - 'a';
        if (!t->child[c]) t->child[c] = node();
        t = t->child[c];
        t->words++;
    }
    t->end = 1;
}
int count_prefix(const Trie *t, const char *p)
{
    /* TODO 2 done: follow the letters; 0 if a letter is missing, else the words count of the last node */
    for (; *p; p++) {
        int c = *p - 'a';
        if (!t->child[c]) return 0;
        t = t->child[c];
    }
    return t->words;
}
void destroy(Trie *t)
{
    if (!t) return;
    for (int i = 0; i < 26; i++) destroy(t->child[i]);
    free(t);
}
static int nodes(const Trie *t){ int n = 1; for (int i = 0; i < 26; i++) if (t->child[i]) n += nodes(t->child[i]); return n; }
int main(void)
{
    const char *contacts[] = {"anand", "anitha", "anu", "arun", "bala", "anusuya"};
    Trie *root = node();
    for (int i = 0; i < 6; i++) insert(root, contacts[i]);
    const char *typed[] = {"a", "an", "anu", "b", "z"};
    for (int i = 0; i < 5; i++)
        printf("%s:%d ", typed[i], count_prefix(root, typed[i]));
    printf("\n");
    int nd = nodes(root);
    printf("nodes created (including the root) = %d, memory = %d x %zu bytes = %zu bytes\n", nd, nd, sizeof(Trie), nd * sizeof(Trie));
    destroy(root);
    return 0;
}
