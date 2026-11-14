#include <stdio.h>

/* Tree stored in an array: children of i are 2i+1 and 2i+2; '.' means empty. */
const char T[] = "ABCDE.F";
int N = 7;

void pre(int i)
{
    if (i >= N || T[i] == '.') return;
    printf("%c", T[i]); pre(2 * i + 1); pre(2 * i + 2);
}

void in(int i)
{
    if (i >= N || T[i] == '.') return;
    in(2 * i + 1); printf("%c", T[i]); in(2 * i + 2);
}

void post(int i)
{
    if (i >= N || T[i] == '.') return;
    post(2 * i + 1); post(2 * i + 2); printf("%c", T[i]);
}

int main(void)
{
    pre(0);  printf(" ");
    in(0);   printf(" ");
    post(0); printf(" ");
    for (int i = 0; i < N; i++) if (T[i] != '.') printf("%c", T[i]);
    printf("\n");
    return 0;
}
