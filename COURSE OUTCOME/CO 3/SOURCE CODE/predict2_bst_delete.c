#include <stdio.h>
#include <stdlib.h>
typedef struct N { int k; struct N *l, *r; } N;
N *ins(N *t, int k)
{
    if (!t) { t = calloc(1, sizeof *t); t->k = k; return t; }
    if (k < t->k) t->l = ins(t->l, k); else t->r = ins(t->r, k);
    return t;
}
N *del(N *t, int k)
{
    if (!t) return NULL;
    if (k < t->k) t->l = del(t->l, k);
    else if (k > t->k) t->r = del(t->r, k);
    else if (t->l && t->r) {
        N *s = t->r;
        while (s->l) s = s->l;               /* inorder successor */
        t->k = s->k;
        t->r = del(t->r, s->k);
    } else {
        N *c = t->l ? t->l : t->r;
        free(t);
        return c;
    }
    return t;
}
void pre(N *t) { if (t) { printf("%d ", t->k); pre(t->l); pre(t->r); } }
void kill(N *t) { if (t) { kill(t->l); kill(t->r); free(t); } }
int main(void)
{
    int a[] = {50, 30, 70, 20, 40, 60, 80, 65};
    N *t = NULL;
    for (int i = 0; i < 8; i++) t = ins(t, a[i]);
    t = del(t, 20);
    t = del(t, 30);
    t = del(t, 50);
    pre(t);
    printf("\n");
    kill(t);
    return 0;
}
