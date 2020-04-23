#include <stdio.h>
#include <stdlib.h>
#include <omp.h>	/* OpenMP */

long result=0;

void foo() {
#pragma omp parallel 
//#pragma omp single
    {
    int argum = 0;
    #pragma omp task  shared(result) firstprivate(argum)
    for (long i = 0; i < 10; i++) {
	#pragma omp atomic
        result++;
    }
    #pragma omp taskwait
    printf("1st loop\n");
    argum++;
    #pragma omp taskloop shared(result) firstprivate(argum)
    for (long i = 0; i < 10; i++) {
	#pragma omp atomic
        result+=argum;
    }

    printf("2nd loop\n");
    argum++;
    #pragma omp taskloop shared(result) num_tasks(2) firstprivate(argum)
    for (long i = 0; i < 10; i++) {
	#pragma omp atomic
        result+=argum;
    }

    printf("3rd loop\n");
    argum++;
    #pragma omp taskloop shared(result) grainsize(3) firstprivate(argum)
    for (long i = 9; i >= 0; i--) {
	#pragma omp atomic
        result+=argum;
    }

    printf("4th loop\n");
    argum++;
    #pragma omp task firstprivate(result) firstprivate(argum)
    printf("Hello from second task, up to now result=%ld and argum = %d\n", result, argum);
    }
}

int main(int argc, char *argv[]) {
    foo();
    printf("Back in main ... result = %ld\n", result);
}
