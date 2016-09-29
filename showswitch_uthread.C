/*
 * Simple program to demonstrate thread switching with
 * the uthread user-level thread library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/errno.h>

#include <unistd.h>
#include "uthread.h"
#include <signal.h>
 
#define N 3

#define SUCCESS 0
#define TRUE 1
#define RESOURCE 10	// unable to allocate resource
#define NOSUCHTHR 20
#define CATASTROPHIC	99	// Really bad failure

using namespace std;



/*
 * delay(N) - delay N units via loop
 * Using a loop is a bad way to deal with timing.
 * However, since we are using the virtual timer to handle our interrupts,
 * using a standard OS sleep call (e.g. nanosleep) would defeat the purpose.
 * So we'll just busy wait insted
 */
void delay(long Counter) {
  for (long n=0; n < Counter; n++)
    ;
  return;
}

/*
 * void *dumbold_thread(void *thread_arg)
 * demonstration thread with an algorithm barely worthy of an
 * intro to CS course
 */
void *dumbold_thread(void *thread_arg) {
  int thread_num = * static_cast<int *>(thread_arg);
  int *retval = new int();

  printf("thread %d starting\n", thread_num);
  fflush(stdout);

  for (int i=0; i < 25; i++) {
    delay(10000000l);  //1000 + 1000*rand());
    printf("thread %d iteration %d\n", thread_num, i);
    fflush(stdout);
  }
  
  printf("Thread %d exiting\n", thread_num);

  // Return the argument that was passed in so that we can
  // keep these things straight.
  *retval = thread_num;
  return retval;
}


int main(int argc, char **argv) {
  int i;
  int thread_arg[N];
  ThreadId_t thread_id[N];

  void *resultPtr;

  thr_init();  /* Start the user thread library */
  thr_diagnostics(THR_QUIET);  /* set debug state */

  printf("\nCreating threads ...\n");
  for (i=0;i<N;i++) {
    thread_arg[i] = i+1;
    /* Set to call  philosopher_thread with point to philosopher id */
    thread_id[i] = thr_create(dumbold_thread, &thread_arg[i]); 
    if(thread_id[i] == THR_FAILURE){
      printf("Failed to create thread\n");
      exit(RESOURCE);
    }
  }
  printf("Threads created...\n\n");
  fflush(stdout);
    
  /* Wait for threads to finish */
  for(i=0;i<N;i++){
    if(thr_join(thread_id[i], &resultPtr) == THR_NOSUCHTHREAD){
      printf("Unable to find thread %d\n", thread_id[i]);
      return(NOSUCHTHR);
    }
    int result = * static_cast<int *>(resultPtr);
    printf("thread %d: exited return code %d\n", i, result);
  }
  
  printf("All threads exited gracefully (we hope)\n");
  return(SUCCESS);
}   
