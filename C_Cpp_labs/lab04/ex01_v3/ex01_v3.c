/*  in this implementation i neglected the fifth rule written in lab04.txt.
    it says the following:
        5. Some threads terminate before the n-th step.
        In particular thread i terminates after step i, with i in the
        range [1–n].
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

unsigned int count = 0;         //number of element that reached the barrier
int term = 0;                   //number of threads terminated
pthread_mutex_t *mutex;         //mutex to access and modify count
pthread_mutex_t mutex_term = PTHREAD_MUTEX_INITIALIZER;
sem_t *in_barrier;              //semaphore to create an entrance barrier
sem_t *out_barrier;             //semaphore to create an entrance barrier

typedef struct thread_arg_s{
    int id;                     //id assigned by the father
    int n_threads;     //max number of threads
    unsigned int n_iter;        //num of iterations to be done by a thread
    int *elements;              //elements to be summed
    pthread_t tid;              //real TID
} thread_arg_t;     //arg to be passed to each thread    

int *gen_elements(int);
void *thread_function(void *);

int main(int argc, char *argv[]){
    setbuf(stdout, 0);

    unsigned int exp;
    unsigned long int n_elem;
    int *elements;
    pthread_mutex_t *mutex_temp;    //mutex to access and modify count
    sem_t *barrier_temp;            //semaphore to create an entrance/exit barrier
    thread_arg_t *thread_args;      //array of structure

    if(argc < 2){
        fprintf(stdout, "Missing parameter\n");
        exit(1);
    }

    exp = atoi(argv[1]);            //take n
    n_elem = 1 << exp;              //compute the total number of elements
    elements = gen_elements(exp);   //generate the sequence of random elements

    //allocate ad initialize the mutex
    mutex_temp = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if(mutex_temp == NULL ){
        printf("Failed allocating the mutex\n");
        exit(1);
    }
    if(pthread_mutex_init(mutex_temp, 0) > 0){
        printf("Error creating the mutex\n");
        exit(1);
    }
    mutex = mutex_temp;

    //allocate and initialize the sempahore for the entrance barrier barrier
    barrier_temp = (sem_t *)malloc(sizeof(sem_t));
    if(barrier_temp == NULL ){
        printf("Failed allocating the mutex\n");
        exit(1);
    }
    if(sem_init(barrier_temp, 0, 0) > 0){
        printf("Error creating the sempahore for the barrier\n");
        exit(1);
    }
    in_barrier = barrier_temp;


    //allocate and initialize the sempahore for the entrance barrier barrier
    barrier_temp = (sem_t *)malloc(sizeof(sem_t));
    if(barrier_temp == NULL ){
        printf("Failed allocating the mutex\n");
        exit(1);
    }
    if(sem_init(barrier_temp, 0, 0) > 0){
        printf("Error creating the sempahore for the barrier\n");
        exit(1);
    }
    out_barrier = barrier_temp;

    //allocate and initialize the array of structures for the threads
    thread_args = (thread_arg_t *)malloc((n_elem-1) * sizeof(thread_arg_t));
    for(int i=0; i<n_elem-1; i++){
        thread_args[i].id = i+1;
        //printf("%d ", i+1);
        thread_args[i].n_threads = n_elem-1;
        thread_args[i].n_iter = exp;
        thread_args[i].elements = elements;
        pthread_create(&thread_args[i].tid, NULL, thread_function, (void *)&thread_args[i]);
    }

    printf("%d ", elements[0]);
    for(int i=0; i<n_elem-1; i++){
        pthread_join(thread_args[i].tid, NULL);
        printf("%d ", elements[thread_args[i].id]);
    }
    printf("\n");

    return 0;
}

void *thread_function(void *args){
    thread_arg_t *arg = (thread_arg_t *)args;
    int num_iter = 0;
    int i, gap = 1, prev;

    while(num_iter < arg->n_iter){
        sleep(1);
        
        prev = arg->elements[arg->id - gap];

        pthread_mutex_lock(mutex);      //trying to acquire the mutex
        count++;                        //update number of threads stuck at the barrier
        if(count == (arg->n_threads - term)){    //the last thread unlocks all the others
            for(i=0; i<(arg->n_threads - term); i++)
                sem_post(in_barrier);
            count = 0;                  //update number of threads stuck at the barrier
        }
        pthread_mutex_unlock(mutex);    //release the mutex
        //printf("iter:%d  T:%d stuck at entrance barrier\n", num_iter, arg->id);
        sem_wait(in_barrier);           //wait at the barrier to be unstucked

        arg->elements[arg->id] = arg->elements[arg->id] + prev;

        gap = 1 << (num_iter + 1);    //distance between the two elements
        if( (arg->id - gap) < 0 ){
            pthread_mutex_lock(&mutex_term);
            term++;
            pthread_mutex_unlock(&mutex_term);
            pthread_exit(NULL);
        }

        pthread_mutex_lock(mutex);      //trying to acquire the mutex
        count++;                        //update number of threads stuck at the barrier
        if(count == (arg->n_threads - term)){    //the last thread unlocks all the others
            for(i=0; i<(arg->n_threads - term); i++)
                sem_post(out_barrier);
            count = 0;                  //update number of threads stuck at the barrier
        }
        pthread_mutex_unlock(mutex);    //release the mutex
        //printf("iter:%d  T:%d stuck at exit barrier\n", num_iter, arg->id);
        sem_wait(out_barrier);           //wait at the barrier to be unstucked

        num_iter++;
        //printf("\n");
    }

    pthread_exit(NULL);
}

int *gen_elements(int exp){
    unsigned int seed = getpid();
    unsigned long int n_elem = 1 << exp;
    int *array = (int *)malloc(n_elem * sizeof(int));

    printf("%ld elements will be generated:\n", n_elem);

    for(int i=0; i<n_elem; i++){
        array[i] = rand_r(&seed) % 9 + 1;
        printf("%d ",array[i]);
    }
    printf("\n");

    return array;
}
