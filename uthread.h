/*
 * BY:
 * Mithun Kumar Ranganath & Chandra Prakash Gopalaiah
 * Date: Sep 30 2010
 * Advance OS assignment#2
 */
#ifndef UTHREAD_H
#define UTHREAD_H

#include <ucontext.h>
#include "sem.h"
#include "queue.h"
#include <stdio.h>
#include <sys/times.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include<time.h>

#define THR_QUANTUM_DEFAULT	5	/* default time quantum ms */
#define THR_STACK_DEFAULT	65536	/* default stack bytes */
#define THR_FAILURE		-1
#define THR_NOSUCHTHREAD	-1
#define THR_LIBFAILURE		10	/* lib failure - exit w/ return code */

#define ONE_MS			1000
#define THR_EXIT		101
#define THR_COMPLETED		102
#define THR_JOIN		103
#define THR_YIELD		104

#if defined(__cplusplus)

/* The above test checks to see if a C++ compiler is being used.  If it is,
 * we tell the compiler to use C style linkage so that C programs can
 * call us.  Any functions with C style linkages cannot be overloaded.
 * 
 * Note that if you compile with a C compiler, the lines
 * in this #if construct are not inluded and thus will not
 * cause compilation problems.
 *
 * A similar #if below closes off the extern "C" block.  Note that when
 * defining the functions for C++, they will need to be declared
 * extern "C".
 */

namespace std {
  extern "C" {
#endif

    typedef int ThreadId_t;
	


    /* thread states */
    typedef enum {
      BLOCKED,		/* blocked */
      RUNNING,		/* currently dispatched */
      READY,		/* in ready queue */
      COMPLETED,	/* returned from thread */
      KILLED,		/* killed by another thread */
    } ThreadState_t;
  
    /* Thread control block */
    typedef struct {
      usem_t barrier;	              /* for supporting thr_join */
      ThreadId_t  id;	              /* unique identifier */
      ThreadState_t state;
      void * (*fn_body_ptr)(void *); /* starting function */ // fn_body_ptr is a function pointer which accepts void pointer as parameter and returns void pointer
      void * fn_arg_ptr;	          /* argument to starting function */
      void * result;	              /* function result */
      ucontext_t context;	          /* registers, stack, signals, etc.  */

      /* Thread count statistics */
      int num_blocked;
      int num_run;
      int num_ready;
      int num_completed;

      /* bean counter material - see assignment if you are interested
       * in extra credit.
       *
       * Track how many clock ticks the thread has spent waiting in the
       * ready queue.  Note that this assume that whenever virtual time is
       * incremented that a thread is either executing or in the ready queue.
       *
       * CAVEATS:
       *
       * This accounting scheme will not work with a multiple kernel level
       * threads as the virtual time can be incremented while a thread is
       * blocked.
       *
       * We assume that there is no wrap around.
       */
      clock_t ready_s;	  /* time spent in ready queue */
      clock_t executed_s;     /* spent executing */
      clock_t last_change;	  /* time last enqueued or dispatched */
    } TCB;


	
	extern TCB * running_tcb;
	extern struct itimerval tout_val;
	extern struct queue *ready_q,*completed_q; 
	//struct sigaction timer_mask;
	//sigset_t t_sig;

    void thr_init();	/* Initialize thread package */
    
    void thr_quantum(int N_ms);	  /* Set time quantum to N ms */

    /* create a new thread
     * Expects pointer to thread type, the function to be started,
     * and the argument for the function.
     * Returns id of new thread, or THR_FAILURE if unable to create.
     */
    ThreadId_t thr_create(void * (*start_fn)(void *), void *arg);

    /* exit the thread
     * returning a pointer to the result.
     * Note that the pointer should not be on the thread's
     * stack as the thread will have exited.
     */
    void thr_exit(void *result);
    
    /* wait for specified thread to exit
     * Returns THR_NOSUCHTHREAD if unable to find thread.
     * A generic pointer is set to the return code of the routine
     */
    int thr_join(ThreadId_t id, void **result);

    /* set level of diagnostic chatter
     * Output information to standard error whenever a thread switch
     * occurs.  Amount of information is dependent upon the verbosity.
     */
    typedef enum {
      THR_QUIET, /* no information */
      THR_SWITCH, /* report thread switch */
      THR_SUMMARY /* provide the same information as in thr_summary */
    } thr_diagnostic_t;

    /* provide information to user as per the specified
       thr_diagnostic_t */
    void thr_diagnostics(thr_diagnostic_t Verbosity);

    /* Voluntarily requinquish remaining quantum.
     * Another thread is schedule with a full quantum.
     */
    void thr_yield();


    /* report on threads
     * Generate thread table on specified output FILE.
     * For each thread:
     *	id, #times active, #time exectued (ms), #time in ready queue (ms)
     */
    void thr_summary(FILE *FileHandle);

#if defined(__cplusplus)
  }
}
#endif
  
#endif
