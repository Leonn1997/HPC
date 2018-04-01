#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include <time.h>

#define TRYS 5000000

static int throw() {
  double x, y;
  x = (double)rand() / (double)RAND_MAX;
  y = (double)rand() / (double)RAND_MAX;
  if ((x*x + y*y) <= 1.0) return 1;
    
  return 0;
}

void print_results(int throw, int tid) 
{
  /*** use critical for clean output ***/
  #pragma omp critical
  {
    printf("\nThread %d: treffer %d \n", tid, throw);
  } 
}

int main(int argc, char **argv) {
  int globalCount = 0, globalSamples=TRYS, i = 0, threadResult = 0;
  clock_t begin = clock();

  #pragma omp parallel shared(globalCount, globalSamples) private(threadResult, i)
  {

  #pragma omp for nowait
    for(i = 0; i < globalSamples; ++i) {
      int throw_value = throw();
      threadResult += throw_value;
    }
    
    #pragma omp atomic
    globalCount += threadResult;
    
    print_results(threadResult, omp_get_thread_num());

  }
  clock_t end = clock();

  double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  double pi = 4.0 * (double)globalCount / (double)(globalSamples);
 
  printf("pi is %.9lf. execution took about %f \n", pi, time_spent);
  
  return 0;
}
