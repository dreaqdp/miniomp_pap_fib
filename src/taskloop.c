#include "libminiomp.h"

#define GOMP_TASK_FLAG_UNTIED           (1 << 0)
#define GOMP_TASK_FLAG_FINAL            (1 << 1)
#define GOMP_TASK_FLAG_MERGEABLE        (1 << 2)
#define GOMP_TASK_FLAG_DEPEND           (1 << 3)
#define GOMP_TASK_FLAG_PRIORITY         (1 << 4)
#define GOMP_TASK_FLAG_UP               (1 << 8)
#define GOMP_TASK_FLAG_GRAINSIZE        (1 << 9)
#define GOMP_TASK_FLAG_IF               (1 << 10)
#define GOMP_TASK_FLAG_NOGROUP          (1 << 11)

// To generate tasks
void prepare_tasks_loop (void (*fn) (void *), char * buffer, int num_tasks, int it_per_task, long end) {
    for (int i = 0; i < ((num_tasks-1) << 1); i += 2) {
        miniomp_task_t * task = malloc(sizeof(miniomp_task_t));
        task->fn = fn;
        task->taskgroup = 1;
        //printf("it %d \n", i);
        ((long *)buffer)[i] = i*it_per_task;
        ((long *)buffer)[i+1] = (i + 1)*it_per_task - 1;
        task->data = &buffer[i];
        if (is_valid(task)) {
            //printf("TASKLOOP FOR: valid task, enqueueueueuing taskgroup %d\n", task->taskgroup);
            while (!enqueue(miniomp_taskqueue, task)); // try to enqueue
        }
    }
    miniomp_task_t * task = malloc(sizeof(miniomp_task_t));
    task->fn = fn;
    task->taskgroup = 1;
    ((long *)buffer)[num_tasks<<1] = it_per_task*num_tasks;
    ((long *)buffer)[(num_tasks<<1) + 1] = end;
    task->data = &buffer[num_tasks<<1];
    if (is_valid(task)) {
        //printf("TASKLOOP END: valid task, enqueueueueuing taskgroup %d\n", task->taskgroup);
        while (!enqueue(miniomp_taskqueue, task)); // try to enqueue
    }
} 

/* Called when encountering a taskloop directive. */

void GOMP_taskloop (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
                    long arg_size, long arg_align, unsigned flags,
                    unsigned long num_tasks, int priority,
                    long start, long end, long step)
{
    //printf("TASKLOOP: a taskloop has been encountered, with ");
    int it = (end - start)/step;
    if (flags & GOMP_TASK_FLAG_GRAINSIZE) {
        //printf("grainsize=%ld, ", num_tasks);
    } 
    else {
        if (num_tasks == 0) num_tasks = omp_get_num_threads() < it ? omp_get_num_threads(): it;
        //printf("num_tasks=%ld, ", num_tasks);
    }
    //printf("I am enqueueing it immediately\n");

    int it_per_task = ((end - start)/step) / num_tasks;

    GOMP_taskgroup_start();

    char * buf;
    //printf("TASKLOOP: start %ld end %ld step %ld\n", start, end, step);
    if (__builtin_expect (cpyfn != NULL, 0)) {
        //printf("premalloc\n");
        //char * buf =  malloc(sizeof(char) * (arg_size + arg_align - 1));
        buf =  malloc(sizeof(char) * (arg_size + arg_align - 1));
        //printf("postmalloc\n");
        char *arg = (char *) (((uintptr_t) buf + arg_align - 1)
                & ~(uintptr_t) (arg_align - 1));
        cpyfn (arg, data);
        /*
        for (int i = 0; i < (num_tasks<<1); i += 2) {
            miniomp_task_t task;
            task.fn = fn;
            ((long *)arg)[i] = i*it_per_task;
            ((long *)arg)[i+1] = (i + 1)*it_per_task; //- 1;
            task.data = &arg[i];
            if (is_valid(&task)) {
                printf("TASKLOOP FOR1: valid task, enqueueueueuing\n");
                while (!enqueue(miniomp_taskqueue, &task)); // try to enqueue
            }
        }
        */
        prepare_tasks_loop(fn, arg, num_tasks, it_per_task, end);
        //fn(arg);

    }
    else {
        //char * buf =  malloc(sizeof(char) * (arg_size + arg_align - 1));
        buf =  malloc(sizeof(char) * (arg_size + arg_align - 1));
        memcpy (buf, data, arg_size);

        prepare_tasks_loop(fn, buf, num_tasks, it_per_task, end);
    }

    GOMP_taskgroup_end();
    free(buf);
}

