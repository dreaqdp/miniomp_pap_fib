/* This structure describes a "task" to be run by a thread.  */
typedef struct {
    void (*fn)(void *);
    void (*data);
    bool taskgroup;
    // complete with additional field if needed
} miniomp_task_t;

typedef struct {
    int max_elements;
    int count;
    int count_executing;
    int head;
    int tail;
    int first;
    pthread_mutex_t lock_queue;
    //pthread_mutex_t lock_consult; // another mutex to protect consults such as empty and full
    miniomp_task_t **queue;
    // complete with additional field if needed
} miniomp_taskqueue_t;

extern miniomp_taskqueue_t * miniomp_taskqueue;
#define MAXELEMENTS_TQ 128

// funtions to implement basic management operations on taskqueue
bool is_empty(miniomp_taskqueue_t *task_queue);
bool is_full(miniomp_taskqueue_t *task_queue) ;
bool is_executing(miniomp_taskqueue_t *task_queue);
bool is_valid(miniomp_task_t *task_descriptor);
bool enqueue(miniomp_taskqueue_t *task_queue, miniomp_task_t *task_descriptor); 
bool dequeue(miniomp_taskqueue_t *task_queue);
miniomp_task_t *first(miniomp_taskqueue_t *task_queue); 
