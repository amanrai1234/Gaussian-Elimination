

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

/* #define DEBUG */

int num_threads = 8;

#define SWAP(a,b)       {double tmp; tmp = a; a = b; b = tmp;}

/* Solve the equation:
 *   matrix * X = R
 */

double **matrix, *X, *R;

/* Pre-set solution. */

double *X__;

/* Initialize the matirx. */

pthread_t *threadsInt;

int initMatrix(const char *fname)
{
    FILE *file;
    int l1, l2, l3;
    double d;
    int nsize;
    int i, j;
    double *tmp;
    char buffer[1024];

    if ((file = fopen(fname, "r")) == NULL) {
	fprintf(stderr, "The matrix file open error\n");
        exit(-1);
    }


    /* Parse the first line to get the matrix size. */
    fgets(buffer, 1024, file);
    sscanf(buffer, "%d %d %d", &l1, &l2, &l3);
    nsize = l1;
#ifdef DEBUG
    fprintf(stdout, "matrix size is %d\n", nsize);
#endif

    /* Initialize the space and set all elements to zero. */
    matrix = (double**)malloc(nsize*sizeof(double*));
    assert(matrix != NULL);
    tmp = (double*)malloc(nsize*nsize*sizeof(double));
    assert(tmp != NULL);
    for (i = 0; i < nsize; i++) {
        matrix[i] = tmp;
        tmp = tmp + nsize;
    }
    for (i = 0; i < nsize; i++) {
        for (j = 0; j < nsize; j++) {
            matrix[i][j] = 0.0;
        }
    }

    /* Parse the rest of the input file to fill the matrix. */
    for (;;) {
	fgets(buffer, 1024, file);
	sscanf(buffer, "%d %d %lf", &l1, &l2, &d);
	if (l1 == 0) break;

	matrix[l1-1][l2-1] = d;
#ifdef DEBUG
	fprintf(stdout, "row %d column %d of matrix is %e\n", l1-1, l2-1, matrix[l1-1][l2-1]);
#endif
    }

    fclose(file);
    return nsize;
}

/* Initialize the right-hand-side following the pre-set solution. */

void initRHS(int nsize)
{
    int i, j;

    X__ = (double*)malloc(nsize * sizeof(double));
    assert(X__ != NULL);
    for (i = 0; i < nsize; i++) {
	X__[i] = i+1;
    }

    R = (double*)malloc(nsize * sizeof(double));
    assert(R != NULL);
    for (i = 0; i < nsize; i++) {
	R[i] = 0.0;
	for (j = 0; j < nsize; j++) {
	    R[i] += matrix[i][j] * X__[j];
	}
    }
}

/* Initialize the results. */

void initResult(int nsize)
{
    int i;

    X = (double*)malloc(nsize * sizeof(double));
    assert(X != NULL);
    for (i = 0; i < nsize; i++) {
	X[i] = 0.0;
    }
}

/* Get the pivot - the element on column with largest absolute value. */




void getPivot(int nsize, int currow)
{
    int i, pivotrow;

    pivotrow = currow;
    for (i = currow+1; i < nsize; i++) {
	if (fabs(matrix[i][currow]) > fabs(matrix[pivotrow][currow])) {
	    pivotrow = i;
	}
    }

    if (fabs(matrix[pivotrow][currow]) == 0.0) {
        fprintf(stderr, "The matrix is singular\n");
        exit(-1);
    }

    if (pivotrow != currow) {
#ifdef DEBUG
	fprintf(stdout, "pivot row at step %5d is %5d\n", currow, pivotrow);
#endif
        for (i = currow; i < nsize; i++) {
            SWAP(matrix[pivotrow][i],matrix[currow][i]);
        }
        SWAP(R[pivotrow],R[currow]);
    }
}

/* For all the rows, get the pivot and eliminate all rows and columns
 * for that particular pivot row. */

struct arg_struct
{
    int g;
    int nsize;
    int threadid;
   
};

void *computeGausss(void *arguments)
{
    int i, j, k;
    double pivotval;
    struct arg_struct *args = (struct arg_struct *)arguments;
    int g = args->g;
    int nsize = args->nsize;
    int tid = args->threadid;
    int rc, verify;
    long t;
    void *status;
    int l, m, llim, mlim;
    int n, o;
    int bs = 16;
    /* Scale the main row. */
    pivotval = matrix[g][g];

    for (j = g + 1 + tid; j < nsize; j = j + num_threads)
    {
        R[j] -= matrix[j][g] * R[g];
}

  for (l = g+1; l < nsize; l+=bs)
    {
        llim = l + bs;
        if (llim > nsize)
            llim = nsize;
        for (m = g+1; m < nsize; m+=bs)
        {
            mlim = m + bs;
            if (mlim > nsize)
                mlim = nsize;
            for (n = l + tid; n < llim; n += num_threads)
            {
                pivotval = matrix[n][g];

                for (o = m; o < mlim; o++)
                {

                    matrix[n][o] -= pivotval * matrix[g][o];
                }
                
            }
        }
    }
for (j = g + 1 + tid; j < nsize; j = j + num_threads)
    {
        matrix[j][g] = 0.0;
    }

       pthread_exit(NULL);
}





        /* Factorize the rest of the matrix. 
        for (j = g + 1 + tid; j < nsize; j=j+num_threads)
        {
            pivotval = matrix[j][g];
            matrix[j][g] = 0.0;
            for (k = g + 1; k < nsize; k++)
            {
                matrix[j][k] -= pivotval * matrix[g][k];
            }
            R[j] -= pivotval * R[g];
        }
        pthread_exit(NULL);
    }*/
    

void computeGauss(int nsize)
{
    int i, j, k;
    double pivotval;
    struct arg_struct args[nsize];
    void *status;
    long t;

    for (i = 0; i < nsize; i++) {
	getPivot(nsize,i);

	/* Scale the main row. */
        pivotval = matrix[i][i];
	if (pivotval != 1.0) {
	    matrix[i][i] = 1.0;
	    for (j = i + 1; j < nsize; j++) {
		matrix[i][j] /= pivotval;
	    }
	    R[i] /= pivotval;
	}

    for (t = 0; t < num_threads; t++)
    {
        args[t].g = i;
        args[t].nsize = nsize;
        args[t].threadid = t;
        pthread_create(&threadsInt[t], NULL, computeGausss, (void *)&args[t]);
    }

    // Wait for thread termination
    for (t = 0; t < num_threads; t++)
        pthread_join(threadsInt[t], &status);

    /* Factorize the rest of the matrix. */
        
    }
}

/* Solve the equation. */

void solveGauss(int nsize)
{
    int i, j;

    X[nsize-1] = R[nsize-1];
    for (i = nsize - 2; i >= 0; i --) {
        X[i] = R[i];
        for (j = nsize - 1; j > i; j--) {
            X[i] -= matrix[i][j] * X[j];
        }
    }

#ifdef DEBUG
    fprintf(stdout, "X = [");
    for (i = 0; i < nsize; i++) {
        fprintf(stdout, "%.6f ", X[i]);
    }
    fprintf(stdout, "];\n");
#endif
}

int main(int argc, char *argv[])
{
    int i;
    struct timeval start, finish;
    int nsize = 0;
    double error;
    threadsInt = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    void *status;
    /*
    if (argc != 2) {
	fprintf(stderr, "usage: %s <matrixfile>\n", argv[0]);
	exit(-1);
    }

    nsize = initMatrix(argv[1]);
    */
    nsize = initMatrix("matrices_dense/jpwh_991.dat");
    initRHS(nsize);
    initResult(nsize);

    gettimeofday(&start, 0);
    computeGauss(nsize);
    gettimeofday(&finish, 0);

    solveGauss(nsize);

    fprintf(stdout, "Time:  %f seconds\n", (finish.tv_sec - start.tv_sec) + (finish.tv_usec - start.tv_usec)*0.000001);

    error = 0.0;
    for (i = 0; i < nsize; i++) {
	double error__ = (X__[i]==0.0) ? 1.0 : fabs((X[i]-X__[i])/X__[i]);
	if (error < error__) {
	    error = error__;
	}
    }
    fprintf(stdout, "Error: %e\n", error);

    return 0;
}
