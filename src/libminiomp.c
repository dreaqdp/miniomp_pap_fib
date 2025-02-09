#include "libminiomp.h"
//#include "intrinsic.h"

// Library constructor and desctructor
void init_miniomp(void) __attribute__((constructor));
void fini_miniomp(void) __attribute__((destructor));

// Function to parse OMP_NUM_THREADS environment variable
void parse_env(void);

extern miniomp_taskqueue_t * miniomp_taskqueue;
miniomp_taskqueue_t * init_task_queue (int max_elements);

extern int in_taskgroup;

void init_miniomp(void) {
    printf ("mini-omp is being initialized\n");
    // Parse OMP_NUM_THREADS environment variable to initialize nthreads_var internal control variable
    parse_env();
    // Initialize Pthread data structures 
    // Initialize Pthread thread-specific data, now just used to store the OpenMP thread identifier
    pthread_key_create(&miniomp_specifickey, NULL);
    pthread_setspecific(miniomp_specifickey, (void *) 0); // implicit initial pthread with id=0
    // Initialize OpenMP default lock and default barrier
    // Initialize OpenMP workdescriptors for for and single 
    // Initialize OpenMP task queue for task and taskloop
    miniomp_taskqueue = init_task_queue(MAXELEMENTS_TQ);
    in_taskgroup = 0;

}

void fini_miniomp(void) {
    // delete Pthread thread-specific data
    pthread_key_delete(miniomp_specifickey);

    // free other data structures allocated during library initialization
    pthread_mutex_destroy(&miniomp_taskqueue->lock_consult);
    pthread_mutex_destroy(&miniomp_taskqueue->lock_queue);
    free(miniomp_taskqueue->queue);
    free(miniomp_taskqueue);
    free(miniomp_threads);
    free(miniomp_parallel);
    printf ("mini-omp is finalized\n");
}
