/*
 * BY:
 * Mithun Kumar Ranganath & Chandra Prakash Gopalaiah
 * Date: Sep 30 2010
 * Advance OS assignment#2
 */

#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
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




#if defined(__cplusplus)
extern "C" {
#endif
	/*
	 *initializing the queue, by setting the head and tail pointers to NULL
	 */
	void q_init(struct queue *q){
		q->head=NULL;
		q->tail=NULL;
	}

	/*
	 *adding the sent block or element to the end of the queue
	 */
	void add_last(struct queue *q,void * t){
		struct node * temp=malloc(sizeof(struct node));
		temp->tcb=t;
		temp->next=NULL;
		if(q->head==NULL){
			q->head=temp;
			q->tail=temp;
			return;
		}
		q->tail->next=temp;
		q->tail=temp;
		return;
	}
	
	/*
	 * removing the first block or element from the queue and will return the removed element
	 */
	void * remove_first(struct queue *q){
		if(q->head==NULL){
			printf("Q is empty");
			return NULL;
		}
		struct node * temp;
		temp=q->head;
		if(q->head==q->tail){
			q->head=NULL;
			q->tail=NULL;
			return temp->tcb;
		}
		q->head=q->head->next;
		return temp->tcb;
	}
	
	void show_q(struct queue * q){
		if(q==NULL){
			printf("Q not initialised\n");
			return;
		}
		if(q->head==NULL){
			printf("Q is empty\n");
			return;
		}
		struct node * temp=q->head;				
		
		while(temp){
		//	fprintf(stdout,"  %d \t",temp->tcb->id ,str[temp->tcb->state]);
			temp=temp->next;
		}
			printf("\n");
	}

	int queue_empty(struct queue * q){
		if(q==NULL||q->head==NULL){
			return 1;
		}
		return 0;
	}
	
	//queue ends


//////////////////////////////////////////////////////////////

#if defined(__cplusplus)
}
#endif 



