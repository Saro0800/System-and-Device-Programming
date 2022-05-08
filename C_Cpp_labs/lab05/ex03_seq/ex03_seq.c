#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define DBG_PRNT_V1V2MAT 0

int main(int argc, char *argv[]){

    //check parameter
    if(argc < 2){
        printf("Missing parameter\n");
        exit(1);
    }

    int n = atoi(argv[1]), i, j;
    int seed = getpid();
    float val, res=0, *v1, *v2, *v, **mat;
    

    v1  = (float *)malloc(n*sizeof(float));
    v2  = (float *)malloc(n*sizeof(float));
    v   = (float *)calloc(n, sizeof(float));
    mat = (float **)malloc(n*sizeof(float*));

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

    if(DBG_PRNT_V1V2MAT){
        printf("v1T: \n");
        for(i=0; i<n; i++)
            printf("%f ", v1[i]);

        printf("\nmat:\n");
        for(i=0; i<n; i++){
            for(j=0; j<n; j++)
                printf("%f ", mat[i][j]);
            printf("\n");
        }

        printf("v2: \n");
        for(i=0; i<n; i++)
            printf("%f\n", v2[i]);
    }
    
    //compute v = v1^T * mat
    for(i=0; i<n; i++)
        for(j=0; j<n; j++)
            v[i] += v1[j]*mat[j][i];
        
    //compute res = v * v2
    for(i=0; i<n; i++)
        res += v[i]*v2[i];

    printf("res: %f\n", res);

    return 0;
}