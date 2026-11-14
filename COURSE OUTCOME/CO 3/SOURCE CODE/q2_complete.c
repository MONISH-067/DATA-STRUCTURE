#include <stdio.h>
#include <stdlib.h>
typedef struct Node {
    int key, h;                             /* h = height, leaf = 1 */
    struct Node *l, *r;
} Node;
int ht(Node *n) { return n ? n->h : 0; }
int max(int a, int b) { return a > b ? a : b; }
void fix(Node *n) { n->h = 1 + max(ht(n->l), ht(n->r)); }
Node *rotate_right(Node *y)
{
    /* TODO 1 done: x = y->l; y becomes right child of x; x's old right subtree becomes y's left */
    Node *x = y->l;
    y->l = x->r;
    x->r = y;
    fix(y);                                 /* y first (now lower), then x */
    fix(x);
    return x;
}
Node *rotate_left(Node *x)
{
    /* TODO 2 done: mirror image */
    Node *y = x->r;
    x->r = y->l;
    y->l = x;
    fix(x);
    fix(y);
    return y;
}
Node *insert(Node *n, int key)
{   if (!n) {
        n = malloc(sizeof *n);
        if (!n) return NULL;
        n->key = key; n->h = 1; n->l = n->r = NULL;
        return n;
    }
    if (key < n->key) n->l = insert(n->l, key);
    else if (key > n->key) n->r = insert(n->r, key);
    else return n;
    fix(n);
    int bal = ht(n->l) - ht(n->r);
    if (bal > 1 && key < n->l->key) return rotate_right(n);              /* LL */
    if (bal < -1 && key > n->r->key) return rotate_left(n);             /* RR */
    if (bal > 1) { n->l = rotate_left(n->l); return rotate_right(n); }  /* LR */
    if (bal < -1) { n->r = rotate_right(n->r); return rotate_left(n); } /* RL */
    return n;
}
void preorder(Node *n)
{   if (!n) return;
    printf("%d ", n->key);
    preorder(n->l);
    preorder(n->r);
}
void destroy(Node *n) { if (n) { destroy(n->l); destroy(n->r); free(n); } }
int main(void)
{   int prices[] = {10, 20, 30, 40, 50, 25};
    Node *root = NULL;
    for (int i = 0; i < 6; i++) root = insert(root, prices[i]);
    preorder(root);
    printf("\nheight = %d\n", ht(root));
    destroy(root);
    return 0;
}
