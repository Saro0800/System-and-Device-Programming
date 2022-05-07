#include <pthread.h>

typedef struct thread_arg_s{
    unsigned int id, n_elem;
    pthread_t tid;
    int *file_content, *elements;
    char *filePath;
    int *th_finished;
    pthread_mutex_t *mutex_thSignaling;
}thread_arg_t;


void *threadFunc(void *arg);