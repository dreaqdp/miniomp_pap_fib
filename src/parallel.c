#include "libminiomp.h"

// This file implements the PARALLEL construct

// Declaration of array for storing pthread identifier from pthread_create function
pthread_t *miniomp_threads;

// Global variable for parallel descriptor
miniomp_parallel_t *miniomp_parallel;

// Declaration of per-thread specific key
pthread_key_t miniomp_specifickey;

//extern int taskgroup;
extern int taskgroup_cnt_tasks;
extern int taskgroup_exec_tasks;
int in_parallel;

// This is the prototype for the Pthreads starting function
void *worker(void *args) {
    // insert all necessary code here for:
    //   1) save thread-specific data
    ulong *tid = (ulong *) args;
    pthread_setspecific(miniomp_specifickey, tid);

    //   2) invoke the per-tkhreads instance of function encapsulating the parallel region

    if (*tid == 0) {
        miniomp_parallel[*tid].fn(miniomp_parallel[*tid].fn_data);
        __sync_fetch_and_sub(&in_parallel, 1);
    }
    else
        printf("PARALLEL: Worker id %lu created\n", *tid);

    while (in_parallel || !is_empty(miniomp_taskqueue)) {
        //pthread_mutex_lock(&miniomp_taskqueue->lock_consult);
        if (!is_empty(miniomp_taskqueue)) {
            miniomp_task_t *task = first(miniomp_taskqueue);
            bool execute_task = dequeue(miniomp_taskqueue);
            //printf("thread %lu executing? %d task %d", *tid, execute_task, miniomp_taskqueue->head);
            //pthread_mutex_unlock(&miniomp_taskqueue->lock_consult);

            if (execute_task) {
                __sync_fetch_and_add(&miniomp_taskqueue->count_executing, 1);
                task->fn(task->data);
                __sync_fetch_and_sub(&miniomp_taskqueue->count_executing, 1);
                __sync_synchronize();

                printf("PARALLEL: thread %lu executed task \n", *tid);
                int tmp;
                if (task->taskgroup == 1) {
                    //tmp =__sync_sub_and_fetch(&taskgroup_cnt_tasks, 1);
                    tmp = __sync_add_and_fetch(&taskgroup_exec_tasks, 1);
                    printf("PARALLEL: thread %lu executing taskgroup %d, %d tasks executed\n", *tid, task->taskgroup, tmp);
                }
            //pthread_mutex_unlock(&miniomp_taskqueue->lock_consult);
             //  sleep(1); 
            }
            //free(task);
        }
        //else pthread_mutex_unlock(&miniomp_taskqueue->lock_consult);
    }

    //   3) exit the function
    pthread_exit(NULL);
}

void GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags) {
    if(!num_threads) num_threads = omp_get_num_threads();
    printf("Starting a parallel region using %d threads\n", num_threads);

    miniomp_threads = malloc(num_threads*sizeof(pthread_t));

    miniomp_parallel = malloc(num_threads*sizeof(miniomp_parallel_t));

    in_parallel = 1;
    for (int i = 0; i < num_threads; i++) {
        miniomp_parallel[i].fn = fn;
        miniomp_parallel[i].fn_data = data;
        miniomp_parallel[i].id = i;
        pthread_create(&miniomp_threads[i], NULL, worker, (void *) &miniomp_parallel[i].id);
    }


    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(miniomp_threads[i], NULL);
        printf("PARALLEL: Thread %d joined\n", i);
    }

}
