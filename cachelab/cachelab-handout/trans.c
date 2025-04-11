/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l;
    

    if (M == 32 && N == 32) // case 1
    {
        for (i = 0; i < N; i += 8)
        {
            for (j = 0; j < M; j += 8)
            {
                for (k = 0; k < 8; k++) 
                {
                    for ( l = 0; l < 8; l++) 
                    {
                        if (i + k== j + l) 
                        {
                            continue;
                        }
                        B[j + l][i + k] = A[i + k][ j + l];
                        }
                        if (i == j){
                        B[j + k][i + k] = A[i + k][ j + k];
                    }
                }
            }
        }
    }
    else if (M == 64 && N == 64) // case 2 줄여야 한다...
    {
        for (i = 0; i < N; i += 4)
        {
            for (j = 0; j < M; j += 4)
            {
                for (k = i; k < i + 4; k++)
                {
                    for (l = j; l < j + 4; l++)
                    {
                        B[l][k] = A[k][l];
                    }
                }
            }
        }
    }

    else if (M == 61 && N == 67) // case 3
{
	for (i = 0; i < N; i += 16)
	{
		for (j = 0; j < M; j += 16)
		{
			for (k = i; k < i + 16 && k < N; k++)
			{
				for (l = j; l < j + 16 && l < M; l++)
				{
					if (k == l && k == 0)
						continue;
						 
					B[l][k] = A[k][l];
				}
				
				if (i == j && k == 0)
					B[0][0] = A[0][0];
			}
		}
	}
}
    else
    {
        for (i = 0; i < N; i += 8)
        {
            for (j = 0; j < M; j += 8)
            {
                for (k = i; k < i + 8 && k < N; k++)
                {
                    for (l = j; l < j + 8 && l < M; l++)
                    {
                        B[l][k] = A[k][l];
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
