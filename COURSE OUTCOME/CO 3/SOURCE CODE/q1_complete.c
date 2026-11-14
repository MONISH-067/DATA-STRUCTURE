#include <stdio.h>
#include <stdlib.h>
typedef struct Book {
    int acc;                        /* accession number (key) */
    struct Book *left, *right;
} Book;

Book *insert(Book *root, int acc)
{
    /* TODO 1 done: NULL -> new node; smaller -> left; larger -> right; duplicates ignored */
    if (root == NULL) {
        Book *n = malloc(sizeof *n);
        if (!n) return NULL;
        n->acc = acc; n->left = n->right = NULL;
        return n;
    }
    if (acc < root->acc) root->left = insert(root->left, acc);
    else if (acc > root->acc) root->right = insert(root->right, acc);
    return root;
}
int height(const Book *r)           /* height of an empty tree is -1 */
{
    /* TODO 2 done */
    if (r == NULL) return -1;
    int hl = height(r->left), hr = height(r->right);
    return 1 + (hl > hr ? hl : hr);
}
int search(const Book *r, int acc, int *cmp)
{
    *cmp = 0;
    while (r) {
        (*cmp)++;
        if (acc == r->acc) return 1;
        r = acc < r->acc ? r->left : r->right;
    }
    return 0;
}
void inorder(const Book *r)
{
    if (!r) return;
    inorder(r->left);
    printf("%d ", r->acc);
    inorder(r->right);
}
void destroy(Book *r)
{
    if (!r) return;
    destroy(r->left);
    destroy(r->right);
    free(r);
}
int main(void)
{
    int a[] = {500, 300, 700, 200, 400, 600, 800, 350};
    int s[] = {10, 20, 30, 40, 50, 60, 70, 80};
    Book *random = NULL, *sorted = NULL;
    for (int i = 0; i < 8; i++) random = insert(random, a[i]);
    for (int i = 0; i < 8; i++) sorted = insert(sorted, s[i]);
    inorder(random);
    printf("\nheight: random order %d, sorted order %d\n",
           height(random), height(sorted));
    int c1, c2;
    search(random, 350, &c1);
    search(sorted, 80, &c2);
    printf("comparisons: %d and %d\n", c1, c2);
    destroy(random);
    destroy(sorted);
    return 0;
}
