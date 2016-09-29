/*
 * BY:
 * Mithun Kumar Ranganath & Chandra Prakash Gopalaiah
 * Date: Sep 30 2010
 * Advance OS assignment#2
 */

/*
 * queue.h - user/kernel level semaphores
 * When USER_LEVEL_SEMAPHORES is defined, 
 * user-level semaphores are used which have the same
 * API as POSIX kernel-level semaphores.
 *
 * If USER_LEVEL_SEMAPHORES is not defined, POSIX kernel-level
 * semaphores are used and code calling these functions must be linked
 * with the POSIX real time (-lrt) library.
 */


#ifndef QUEUE_H
#define QUEUE_H

#define Q_FAILURE	-1	/* operation failed */
#define Q_SUCCESS	0	/* operation succeeded */

#if defined(__cplusplus)
extern "C" {
#endif

//nothing done yet
//Queue starts
	struct node{
		void * tcb;
		struct node * next;
	};

	struct queue{
		struct node * head;
		struct node * tail;	
	};

	struct queue *ready_q,*completed_q;
	
	void add_last(struct queue *q,void * t);
	void * remove_first(struct queue *q);
	void q_init(struct queue * q);
	int queue_empty(struct queue * q);
	void show_q(struct queue * q);

#if defined(__cplusplus)
}
#endif 


#endif /* QUEUE_H */
