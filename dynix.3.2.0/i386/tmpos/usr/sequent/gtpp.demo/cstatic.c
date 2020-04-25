/* multiply two matrices, store results in third matrix,
   and print results */

#include <stdio.h>
#include <parallel/microtask.h> /* microtasking header */
#include <parallel/parallel.h>  /* parallel lib header */
#define SIZE 10                 /* size of matrices */

      /* Global shared memory data */

       shared float a[SIZE][SIZE]; /* first array */
       shared float b[SIZE][SIZE]; /* second array */
       shared float c[SIZE][SIZE]; /* result array */

main ()
{
	 void init_matrix(), m_fork(), m_kill_procs(),
	     matmul(), print_mats(); 
	 int nprocs;  /* number of parallel processes */

	 printf("Enter number of processes:"); 
	 scanf("%d",&nprocs);

	 init_matrix(a, b);			/* initialize data */
	 m_set_procs(nprocs);		/* set # of processes */
	 m_fork(matmul, a, b, c);	/* execute parallel loop */
	 m_kill_procs();			/* kill child processes */
	 print_mats(a, b, c);		/* print results */
}

/* initialize matrix function */

void 
init_matrix(a, b)
float a[][SIZE], b[][SIZE];
{
	int i, j;

	for (i = 0; i < SIZE; i ++) {
		for (j = 0; j < SIZE; j ++) {
			a[i][j] = (float)i + j;
			b[i][j] = (float)i - j;
		}
	}
}

/* matrix multiply function */

void 
matmul(a, b, c)
float a[][SIZE], b[][SIZE], c[][SIZE];
{
	int i, j, k, nprocs;

	nprocs = m_get_numprocs();	/* no. of processes */
	for (i = m_get_myid(); i < SIZE; i += nprocs) {
		for (j = 0; j < SIZE; j ++) {
			for (k = 0; k < SIZE; k ++)
				c[i][k] += a[i][j] * b[j][k];
		}
	}
}

/* print results function */

void 
print_mats(a, b, c)
float a[][SIZE], b[][SIZE], c[][SIZE];
{
	int i, j;

	for (i = 0; i < SIZE; i ++) {
		for (j = 0; j < SIZE; j ++) {
			printf("a[%d][%d] = %3.2f\tb[%d][%d] = %3.2f",
			   i, j, a[i][j], i, j, b[i][j]);
			printf("\tc[%d][%d] = %3.2f\n", i, j, 
			   c[i][j]); 
		}
	}
}
