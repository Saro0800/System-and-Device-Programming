#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <error.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct{
    int i, n;
    pthread_t tid;
    float *v1, *v2, *v, **mat;
}arg_th_t;

void *threadFunction(void *);

int main(int argc, char *argv[]){

    //check parameter
    if(argc < 2){
        printf("Missing parameter\n");
        exit(1);
    }

    int n = atoi(argv[1]), i, j;
    int seed = getpid();
    float val, res=0, *v1, *v2, *v, **mat;
    arg_th_t *args_array;
    

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

    for(i=0; i<n; i++){
        args_array[i].i = i;
        args_array[i].n = n;
        args_array[i].v1 = v1;
        args_array[i].v2 = v2;
        args_array[i].v = v;
        args_array[i].mat = mat;
        pthread_create(&args_array[i].tid, NULL, threadFunction, (void *)&args_array[i]);
    }

    //wait all threads
    for(i=0; i<n; i++)
        pthread_join(args_array[i].tid, NULL);
    
    //compute res = v * v2
    for(i=0; i<n; i++)
        res += v[i]*v2[i];

    printf("res: %f\n", res);

    return 0;
}

void *threadFunction(void *args){
    arg_th_t *arg = (arg_th_t *)args;
    int i, pos = arg->i;

    for(i=0; i<arg->n; i++)
        arg->v[pos] += arg->v1[i] * arg->mat[i][pos];

    pthread_exit(0);
}


