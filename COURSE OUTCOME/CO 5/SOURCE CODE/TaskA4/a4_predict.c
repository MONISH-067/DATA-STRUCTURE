#include <stdio.h>

#define V 4
#define X 999                           /* no direct road */

int main(void)
{
    int d[V][V] = {
        {0, 5, X, 10},
        {X, 0, 3, X},
        {X, X, 0, 1},
        {X, X, X, 0},
    };
    for (int k = 0; k < V; k++)
        for (int i = 0; i < V; i++)
            for (int j = 0; j < V; j++)
                if (d[i][k] + d[k][j] < d[i][j]) d[i][j] = d[i][k] + d[k][j];
    for (int i = 0; i < V; i++) {
        for (int j = 0; j < V; j++) printf("%4d", d[i][j]);
        printf("\n");
    }
    return 0;
}
