#include "libminiomp.h"

int in_taskgroup;
int taskgroup_cnt_tasks;

// Called when encountering taskwait and taskgroup constructs

void GOMP_taskwait (void) {
    /*
    pthread_mutex_lock(&miniomp_taskqueue->lock_consult);
    //int tasks_not_exec = get_count(miniomp_taskqueue);
    pthread_mutex_unlock(&miniomp_taskqueue->lock_consult); // possible dead lock or data race, enqueuing and getting count value
    */

    __sync_synchronize();
        printf("TASKSYNC: tasks to execute %d\n", miniomp_taskqueue->count);
    while (!is_empty(miniomp_taskqueue) || is_executing(miniomp_taskqueue)) { // to check if there are no remaining tasks to execute and there are no tasks being executed
        __sync_synchronize();
    }

    printf("TBI: Entered in taskwait, there should be no pending tasks, so I proceed\n");
}

void GOMP_taskgroup_start (void) {
    in_taskgroup = 1;
    taskgroup_cnt_tasks = 0;
    __sync_synchronize();

    printf("TBI: Starting a taskgroup region, at the end of which I should wait for tasks created here\n");
}

void GOMP_taskgroup_end (void) {
    __sync_synchronize();
    while(__sync_add_and_fetch(&taskgroup_cnt_tasks, 0)); // consult value
    __sync_fetch_and_sub(&in_taskgroup, 1);
    __sync_synchronize();
    printf("TBI: Finished a taskgroup region, there should be no pending tasks, so I proceed\n");
}
