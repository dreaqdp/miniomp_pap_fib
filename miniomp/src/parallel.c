#include "libminiomp.h"

// This file implements the PARALLEL construct

// Declaration of array for storing pthread identifier from pthread_create function
pthread_t *miniomp_threads;

// Global variable for parallel descriptor
miniomp_parallel_t *miniomp_parallel;

// Declaration of per-thread specific key
pthread_key_t miniomp_specifickey;

// This is the prototype for the Pthreads starting function
void *worker(void *args) {
    // insert all necessary code here for:
    //   1) save thread-specific data
    ulong *tid = (ulong *) args;
    pthread_setspecific(miniomp_specifickey, tid);

    //   2) invoke the per-tkhreads instance of function encapsulating the parallel region
    //miniomp_parallel[*tid].fn(miniomp_parallel[*tid].fn_data);

    if (*tid == 0) 
        miniomp_parallel[*tid].fn(miniomp_parallel[*tid].fn_data);
    else
        printf("Worker id %lu created\n", *tid);
    //   3) exit the function
    pthread_exit(NULL);
    return(NULL);
}

void GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags) {
    if(!num_threads) num_threads = omp_get_num_threads();
    printf("Starting a parallel region using %d threads\n", num_threads);

    miniomp_threads = malloc(num_threads*sizeof(pthread_t));

    miniomp_parallel = malloc(num_threads*sizeof(miniomp_parallel_t));

    pthread_key_create(&miniomp_specifickey, NULL);

    for (int i = 0; i < num_threads; i++) {
        miniomp_parallel[i].fn = fn;
        miniomp_parallel[i].fn_data = data;
        miniomp_parallel[i].id = i;
        pthread_create(&miniomp_threads[i], NULL, worker, (void *) &miniomp_parallel[i].id);
    }
    
    for (int i = 0; i < num_threads; i++) 
        pthread_join(miniomp_threads[i], NULL);

    pthread_key_delete(miniomp_specifickey);
    /*
    pthread_t thread_ids[num_threads];

    for (int i=0; i<num_threads; i++) {
        //      pthread_create(thread_ids[i], NULL, fn, data);
        miniomp_parallel_t ptr_arg;
        ptr_arg.fn = fn;
        ptr_arg.fn_data = data;
        ptr_arg.id = i;
        pthread_create(thread_ids[i], NULL, worker, ptr_arg);
    }
    */

}
