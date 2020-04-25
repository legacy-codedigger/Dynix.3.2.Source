/* multiply two matrices, store results in third 
   matrix, and print results */

#include <stdio.h>
#include <parallel/microtask.h>
#include <parallel/parallel.h>

      /* Global shared memory data */

       shared float **a; /* first array */
       shared float **b; /* second array */
       shared float **c; /* result array */

main ()
{
  char *shmalloc();
  float ** setup_matrix(); 
  void init_matrix(), m_fork(), m_kill_procs(),
	matmul(), print_mats(); 
  int size ; /* loop end value and loop increment */

  printf("Enter array size:"); 
  scanf("%d",&size);

  a = setup_matrix (size, size);	/* allocate shared */ 
  b = setup_matrix (size, size);	/* memory */
  c = setup_matrix (size, size);
  init_matrix(a, b, size, size);	/* initialize data */
  m_set_procs(3);			/* set # of processes */
  m_fork(matmul, a, b, c, size, size); /* execute matmul */
  m_kill_procs(); 			/* kill childprocesses */
  print_mats(a, b, c, size, size); /* print results */
}

/* initialize matrix function */

float ** 
setup_matrix(nrows, ncols)
int nrows, ncols;
{
int i, j;
float **new_matrix;

  /*  allocate pointer arrays : set new_matrix to 
      address of newly allocated shared matrix */

new_matrix = (float**)shmalloc((unsigned)nrows*
	(sizeof(float*))); 

  /*  allocate data arrays : set first element of 
      new_matrix to address of first element of 
      newly allocated data array */

new_matrix[0] = (float*)shmalloc((unsigned)nrows * 
                 ncols * (sizeof(float))); 

  /*  initialize pointer arrays : set each element of
      new_matrix to address of corresponding element 
      of data array */

for (i = 1; i < nrows; i++) {
	new_matrix[i] = new_matrix[0] + (ncols * i);
  } 
return (new_matrix);
}
/* initialize matrix function */

void 
init_matrix(a, b, nrows, ncols)
float **a, **b, **c;
int nrows, ncols;
{
int i, j;
   
	for (i = 0; i < nrows; i ++) {
		for (j = 0; j < ncols; j ++) {
			a[i][j] = (float)i + j;
			b[i][j] = (float)i - j;
		}
	}
}
void 
matmul(a, b, c, nrows, ncols)
float **a, **b, **c;
int nrows, ncols;
{
int i, j, k, nprocs;
 
nprocs = m_get_numprocs();
	for (i = m_get_myid(); i < nrows; i += nprocs) {
		for (k = 0; k < ncols; k ++) {
			c[i][k] = 0.0;
			for (j = 0; j < ncols; j ++) {
				c[i][k] += a[i][j] * b[j][k];
			}
		}
	}
}
void 
print_mats(a, b, c, nrows, ncols)
float **a, **b, **c;
int nrows, ncols;
{
int i, j;
 
	for (i = 0; i < nrows; i ++) {
		for (j = 0; j < ncols; j ++) {
			printf("a[%d][%d] = %3.2f\tb[%d][%d] = %3.2f",
			i, j, a[i][j], i, j, b[i][j]);
			printf("\tc[%d][%d] = %3.2f\n", i, j, c[i][j]); 
		}
	}
}
