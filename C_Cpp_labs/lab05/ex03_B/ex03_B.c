#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <error.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct{
    int i, n, *at_barr;
    pthread_t tid;
    float *res, *v1, *v2, *v, **mat;
    pthread_mutex_t *mutex;
}arg_th_t;

void *threadFunction(void *);

int main(int argc, char *argv[]){
    setbuf(stdout, 0);
    //check parameter
    if(argc < 2){
        printf("Missing parameter\n");
        exit(1);
    }

    int n = atoi(argv[1]), i, j, at_barr = 0;
    int seed = getpid();
    float val, res=0, *v1, *v2, *v, **mat;
    arg_th_t *args_array;
    pthread_mutex_t *mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    

    v1  = (float *)malloc(n*sizeof(float));
    v2  = (float *)malloc(n*sizeof(float));
    v   = (float *)calloc(n, sizeof(float));
    mat = (float **)malloc(n*sizeof(float*));
    args_array = (arg_th_t *)malloc(n*sizeof(arg_th_t));
    if(v1==NULL || v2==NULL || v==NULL || mat==NULL || args_array==NULL){
        perror("Main failed allocating one array");
        exit(1);
    }

    //create arrays and matrix
    for(i=0; i<n; i++){
        //randomize a value for v1
        val = rand_r((unsigned int*)&seed)%(1001) - 500;
        v1[i] = val/1000;

        //randomize a value for v2
        val = rand_r((unsigned int*)&seed)%(1001) - 500;
        v2[i] = val/1000;

        //allocate the i-th row of mat
        mat[i] = (float *)malloc(n*sizeof(float));
        for( j=0; j<n; j++){
            val = rand_r((unsigned int*)&seed)%(1001) - 500;
            mat[i][j] = val/1000;
        }
    }

    //init the mutex
    pthread_mutex_init(mutex, NULL);

    for(i=0; i<n; i++){
        args_array[i].i = i;
        args_array[i].n = n;
        args_array[i].res = &res;
        args_array[i].v1 = v1;
        args_array[i].v2 = v2;
        args_array[i].v = v;
        args_array[i].mat = mat;
        args_array[i].at_barr = &at_barr;
        args_array[i].mutex = mutex;
        pthread_create(&args_array[i].tid, NULL, threadFunction, (void *)&args_array[i]);
    }

    //wait the termination of all threads
    for(i=0; i<n; i++)
        pthread_join(args_array[i].tid, NULL);

    
    printf("res: %f\n", res);

    return 0;
}

void *threadFunction(void *args){
    arg_th_t *arg = (arg_th_t *)args;
    int i, pos = arg->i;

    for(i=0; i<arg->n; i++)
        arg->v[pos] += arg->v1[i] * arg->mat[i][pos];

    //access the barrier
    pthread_mutex_lock(arg->mutex);
    (*arg->at_barr)++;
    if(*(arg->at_barr) >= arg->n)
        //compute res = v * v2
        for(i=0; i<arg->n; i++)
            *arg->res += arg->v[i] * arg->v2[i];
    pthread_mutex_unlock(arg->mutex);

    pthread_exit(0);
}

