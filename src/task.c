#include "libminiomp.h"

miniomp_taskqueue_t * miniomp_taskqueue;

extern int in_taskgroup;
extern int taskgroup_cnt_tasks;

// Initializes the task queue
miniomp_taskqueue_t * init_task_queue (int max_elements) {
    miniomp_taskqueue_t * task_queue = malloc(sizeof(miniomp_taskqueue_t));
    task_queue->max_elements = max_elements;
    task_queue->count = 0;
    task_queue->count_executing = 0;
    task_queue->head = 0;
    task_queue->tail = 0;
    task_queue->first = 0;
    int ret_val = pthread_mutex_init(&(task_queue->lock_queue), NULL);
    printf("Init mutex queue %d\n", ret_val);
    pthread_mutex_init(&(task_queue->lock_consult), NULL);
    task_queue->queue = malloc(max_elements*sizeof(miniomp_task_t));

    return task_queue;
}

// Checks if the task descriptor is valid
bool is_valid(miniomp_task_t *task_descriptor) {
    return (task_descriptor->fn != NULL) && (task_descriptor->data != NULL);
}

// Checks if the task queue is empty
bool is_empty(miniomp_taskqueue_t *task_queue) {
    return __sync_bool_compare_and_swap(&task_queue->count, 0, 0);
}

// Checks if the task queue is full
bool is_full(miniomp_taskqueue_t *task_queue) {
    return __sync_bool_compare_and_swap(&task_queue->count, task_queue->max_elements, task_queue->max_elements);
}

// Checks if there are tasks which are being executed
bool is_executing(miniomp_taskqueue_t *task_queue) {
    return __sync_add_and_fetch(&task_queue->count_executing, 0);
}

// Enqueues the task descriptor at the tail of the task queue
bool enqueue(miniomp_taskqueue_t *task_queue, miniomp_task_t *task_descriptor) {
    pthread_mutex_lock(&(task_queue->lock_queue));

    bool ret = true;
    if (is_full(task_queue)) ret = false;
    //else if (!is_valid(task_descriptor)) ret = false;
    else {
        task_queue->queue[task_queue->tail] = task_descriptor;
        task_queue->tail = (task_queue->tail + 1)%task_queue->max_elements;
        __sync_fetch_and_add(&task_queue->count, 1);
    }

    pthread_mutex_unlock(&task_queue->lock_queue);
    return ret;
}

// Dequeue the task descriptor at the head of the task queue
bool dequeue(miniomp_taskqueue_t *task_queue) { 
    pthread_mutex_lock(&task_queue->lock_queue);

    bool ret = true;
    if (is_empty(task_queue)) ret = false;
    else {
        task_queue->head = (task_queue->head + 1)%task_queue->max_elements;
        __sync_fetch_and_sub(&task_queue->count, 1);
    }

    pthread_mutex_unlock(&task_queue->lock_queue);
    return ret;
}

// Returns the task descriptor at the head of the task queue
miniomp_task_t *first(miniomp_taskqueue_t *task_queue) {
    return task_queue->queue[task_queue->head];
}


#define GOMP_TASK_FLAG_UNTIED           (1 << 0)
#define GOMP_TASK_FLAG_FINAL            (1 << 1)
#define GOMP_TASK_FLAG_MERGEABLE        (1 << 2)
#define GOMP_TASK_FLAG_DEPEND           (1 << 3)
#define GOMP_TASK_FLAG_PRIORITY         (1 << 4)
#define GOMP_TASK_FLAG_UP               (1 << 8)
#define GOMP_TASK_FLAG_GRAINSIZE        (1 << 9)
#define GOMP_TASK_FLAG_IF               (1 << 10)
#define GOMP_TASK_FLAG_NOGROUP          (1 << 11)

// Called when encountering an explicit task directive. Arguments are:
//      1. void (*fn) (void *): the generated outlined function for the task body
//      2. void *data: the parameters for the outlined function
//      3. void (*cpyfn) (void *, void *): copy function to replace the default memcpy() from 
//                                         function data to each task's data
//      4. long arg_size: specify the size of data
//      5. long arg_align: alignment of the data
//      6. bool if_clause: the value of if_clause. true --> 1, false -->0; default is set to 1 by compiler
//      7. unsigned flags: untied (1) or not (0) 

void
GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
           long arg_size, long arg_align, bool if_clause, unsigned flags,
           void **depend, int priority)
{
    printf("TASK: a task has been encountered, I am enqueuing it immediately\n");
    miniomp_task_t task;
    task.fn = fn;
    task.taskgroup = 0;
    if (__builtin_expect (cpyfn != NULL, 0))
        {
	  char * buf =  malloc(sizeof(char) * (arg_size + arg_align - 1));
          char *arg = (char *) (((uintptr_t) buf + arg_align - 1)
                                & ~(uintptr_t) (arg_align - 1));
          cpyfn (arg, data);
          
          task.data = arg;
        }
    else
	{
          char * buf =  malloc(sizeof(char) * (arg_size + arg_align - 1));
          memcpy (buf, data, arg_size);
          task.data = buf;
	}

    if (in_taskgroup) {
        task.taskgroup = 1;
        __sync_fetch_and_add(&taskgroup_cnt_tasks, 1);
    }
    //pthread_mutex_lock(&miniomp_taskqueue->lock_consult); // crec que faria deadlock
    if (is_valid(&task)) {
        while (!enqueue(miniomp_taskqueue, &task)); // try to enqueue
    }
    //pthread_mutex_unlock(&miniomp_taskqueue->lock_consult);
}
