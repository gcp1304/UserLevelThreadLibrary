/*
 * BY:
 * Mithun Kumar Ranganath & Chandra Prakash Gopalaiah
 * Date: Sep 30 2010
 * Advance OS assignment#2
 */

#include "sem.h"
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#include "uthread.h"
/*
 * sem.h - user/kernel level semaphores
 * When USER_LEVEL_SEMAPHORES is defined, 
 * user-level semaphores are used which have the same
 * API as POSIX kernel-level semaphores.
 *
 * If USER_LEVEL_SEMAPHORES is not defined, POSIX kernel-level
 * semaphores are used and code calling these functions must be linked
 * with the POSIX real time (-lrt) library.
 */



#if defined(__cplusplus)
extern "C" {
#endif

    /* Initialize specified instance of a semaphore to value.
     * User must declare memory for the semaphore.
     * Returns USEM_SUCCESS or USEM_FAILURE
     */
	
    int usem_init(usem_t *semaphore, int value)
    {
	semaphore->counter=value;
	struct queue * temp ;
	temp = malloc(sizeof(struct queue));
	if(temp==NULL){
		printf("malloc failure\n");
		return USEM_FAILURE;	
	}
	semaphore->queue= temp;
	q_init(semaphore->queue);
	return USEM_SUCCESS;
    }

    /* 
     * int down(usem_t *semaphore) - down/P/wait operation
     *
     * Atomically decrement the counter and
     * block if counter below 0 after decrement
     * Returns USEM_SUCCESS or USEM_FAILURE
     */
    int down(usem_t *semaphore)
    {
	signal(SIGVTALRM,SIG_IGN);/* ignoring the timer interrupt signal */
	semaphore->counter--;
	if(semaphore->counter<-1){
		running_tcb->state=BLOCKED;
		running_tcb->num_blocked=running_tcb->num_blocked+1;
		add_last(semaphore->queue,running_tcb);
		scheduler(THR_JOIN);	
	}
	//setting the timer interrupt signal
	if(signal(SIGVTALRM,thr_yield)==SIG_ERR){
		return USEM_FAILURE;
		
	}
	
	
	
	return USEM_SUCCESS;
    }

    /* 
     * int up(usem_t *semaphore) - up/V/signal operation
     *
     * Atomically increment the counter.  If 
     * a thread is blocked on the semaphore, it
     * is moved to the ready queue.
     * Returns USEM_SUCCESS or USEM_FAILURE
     */
    int up(usem_t *semaphore)
    {
	signal(SIGVTALRM,SIG_IGN); /* ignoring the timer interrupt signal */
	semaphore->counter++;
	if(semaphore->counter<0){
		TCB * temp=remove_first(semaphore->queue);
		temp->state=READY;

		temp->num_ready=temp->num_ready+1;
		add_last(ready_q,temp);
		
		
	}
	//setting the timer interrupt signal
	if(signal(SIGVTALRM,thr_yield)==SIG_ERR){
		return USEM_FAILURE;
		
	}
	
	
	
	return USEM_SUCCESS;

    }

#if defined(__cplusplus)
}
#endif 

