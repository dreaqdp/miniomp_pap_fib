#include "libminiomp.h"

void omp_set_num_threads (int n) {
  miniomp_icv.nthreads_var = (n > 0 ? n : 1);
}

int omp_get_num_threads (void) {
  return(miniomp_icv.nthreads_var);
}

int omp_get_thread_num (void) {
  // this implementation should be changed in case you change the definition of miniomp_specifickey
  return((int)(long)pthread_getspecific(miniomp_specifickey));
}

// No need to implement this function, it is just involked by Extrae at some point
// and returns the current nesting for parallel regions
int omp_get_level (void) {
    printf("TBI: omp_get_level ... let say current nesting level is 1\n");
    return(1);
}

