#include <qpalm.h>
#include <stdio.h>

#define N 2
#define M 3
#define ANZMAX 4
#define QNZMAX 2

#define TRUE 1
#define FALSE 0

c_float *random_vector(c_int n) {
    c_float *X = (c_float *)qpalm_calloc(n, sizeof(c_float));
    for (int i = 0; i < n; i++)
        X[i] = (c_float)10 * rand() / (c_float)RAND_MAX;
    return X;
}

c_float *constant_vector(c_float c, c_int n) {
    c_float *X = (c_float *)qpalm_calloc(n, sizeof(c_float));
    for (int i = 0; i < n; i++)
        X[i] = c;
    return X;
}
int main() {

    // Load problem data
    c_int n = N;
    c_int m = M;

    // Problem settings
    QPALMSettings *settings =
        (QPALMSettings *)qpalm_malloc(sizeof(QPALMSettings));

    // Structures
    QPALMWorkspace *work; // Solver workspace
    QPALMData *data;      // Problem matrices

    // Populate data
    srand(12345);
    data       = (QPALMData *)qpalm_malloc(sizeof(QPALMData));
    data->n    = n;
    data->m    = m;
    data->q    = random_vector(data->n);
    data->c    = 0;
    data->bmin = constant_vector(-2, data->m);
    data->bmax = constant_vector(2, data->m);

    solver_sparse *A, *Q;
    A = ladel_sparse_alloc(M, N, ANZMAX, UNSYMMETRIC, TRUE, FALSE);
    Q = ladel_sparse_alloc(N, N, QNZMAX, UPPER, TRUE, FALSE);

    // clang-format off
    A->x[0] = 1; A->x[1] = 1; A->x[2] = 1; A->x[3] = 1;
    A->p[0] = 0; A->p[1] = 2; A->p[2] = 4;
    A->i[0] = 0; A->i[1] = 2; A->i[2] = 1; A->i[3] = 2;
    Q->x[0] = 1.0; Q->x[1] = 1.5;
    Q->p[0] = 0; Q->p[1] = 1; Q->p[2] = 2;
    Q->i[0] = 0; Q->i[1] = 1;
    // clang-format on
    data->A = A;
    data->Q = Q;

    // Define Solver settings as default
    qpalm_set_default_settings(settings);

    // Setup workspace
    work = qpalm_setup(data, settings);

    // Solve Problem
    qpalm_solve(work);

    printf("Solver status: ");
    puts(work->info->status);
    printf("Iter: %d\n", (int)work->info->iter);
    printf("Iter Out: %d\n", (int)work->info->iter_out);
    printf("Solution:");
    for (c_int i = 0; i < n; i++)
        printf(" %.10f", work->solution->x[i]);
    printf("\n");
#ifdef QPALM_TIMING
    printf("Setup time: %f\n", work->info->setup_time);
    printf("Solve time: %f\n", work->info->solve_time);
    printf("Run time: %f\n", work->info->run_time);
#endif

    // Clean workspace
    ladel_sparse_free(data->Q);
    ladel_sparse_free(data->A);

    qpalm_cleanup(work);
    qpalm_free(data->q);
    qpalm_free(data->bmin);
    qpalm_free(data->bmax);
    qpalm_free(data);
    qpalm_free(settings);

    return 0;
}
