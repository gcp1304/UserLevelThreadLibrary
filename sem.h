/*
 * BY:
 * Mithun Kumar Ranganath & Chandra Prakash Gopalaiah
 * Date: Sep 30 2010
 * Advance OS assignment#2
 */
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


#ifndef SEM_H
#define SEM_H

#define USEM_FAILURE	-1	/* operation failed */
#define USEM_SUCCESS	0	/* operation succeeded */

#if defined(__cplusplus)
extern "C" {
#endif

    /* user-level semaphore type */
    typedef struct sem_structure {
      int counter;	/* semaphore value */
      /*
       * pointer to a queue structure of arbitrary type
       * Using a generic type permits the library developer 
       * to use pointers to any type of library data structure
       * they wish.  One can even develop the library in C++,
       * use a queue type from the standard template library,
       * and then link the library with C code.
       */
      void *queue;
    } usem_t;

    /* Initialize specified instance of a semaphore to value.
     * User must declare memory for the semaphore.
     * Returns USEM_SUCCESS or USEM_FAILURE
     */
    int usem_init(usem_t *semaphore, int value);

    /* 
     * int down(usem_t *semaphore) - down/P/wait operation
     *
     * Atomically decrement the counter and
     * block if counter below 0 after decrement
     * Returns USEM_SUCCESS or USEM_FAILURE
     */
    int down(usem_t *semaphore);

    /* 
     * int up(usem_t *semaphore) - up/V/signal operation
     *
     * Atomically increment the counter.  If 
     * a thread is blocked on the semaphore, it
     * is moved to the ready queue.
     * Returns USEM_SUCCESS or USEM_FAILURE
     */
    int up(usem_t *semaphore);

#if defined(__cplusplus)
}
#endif 


#endif /* USEM_H */
