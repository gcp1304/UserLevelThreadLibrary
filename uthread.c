/*
 * BY:
 * Mithun Kumar Ranganath & Chandra Prakash Gopalaiah
 * Date: Sep 30 2010
 * Advance OS assignment#2
 */

#include "uthread.h"
//#include "sem.h"

//#include "queue.h"

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
TCB * running_tcb;
struct itimerval tout_val;
struct queue *ready_q,*completed_q; 
/* thread states*/
char * str[]={"BLOCKED","RUNNING","READY","COMPLETED","KILLED"};

/* set level of diagnostic chatter
     * Output information to standard error whenever a thread switch
     * occurs.  Amount of information is dependent upon the verbosity.
     */
	thr_diagnostic_t tdt;
	long count=1;

    /* provide information to user as per the specified
       thr_diagnostic_t */
    	void thr_diagnostics(thr_diagnostic_t Verbosity){
		tdt=Verbosity;	
	}
	
	void* wrapper(TCB * );
	void scheduler(int);

    	void thr_init()	/* Initialize thread package */
    	{
		/*initialisinf the Thread Ready Q */
		ready_q=malloc(sizeof(struct queue));
		q_init(ready_q);
		/*initialising the Thread Completed Q */
		completed_q=malloc(sizeof(struct queue));
		q_init(completed_q);
		
		/*creating a TCB for the main thread */
		if((running_tcb=malloc(sizeof(TCB)))==NULL){
			printf("cannot alloc memory for TCB in thr_init");
			exit(1);
		}
		/* setting the main thread ID to some number which is not equal to any of the other threads */
		running_tcb->id=-999; 
		running_tcb->state=RUNNING; //main thread is currently running
		usem_init(&running_tcb->barrier,0);
		running_tcb->barrier.queue=malloc(sizeof(struct queue));
		q_init(running_tcb->barrier.queue);

		
		getcontext(&running_tcb->context);

		running_tcb->num_blocked=0;
		running_tcb->num_run=1;
		running_tcb->num_ready=0;
		running_tcb->num_completed=0;
		running_tcb->ready_s=0;
		running_tcb->executed_s=0;
		running_tcb->last_change=0;
    	}
    

	/* setting time quantum and setting the timer up */
    	void thr_quantum(int N_ms)	  /* Set time quantum to N ms */
    	{
		tout_val.it_interval.tv_sec = 0;
		tout_val.it_interval.tv_usec = 0;
		tout_val.it_value.tv_sec = 0; 
		tout_val.it_value.tv_usec = N_ms*ONE_MS;  /* N_ms milli seconds */
		setitimer(ITIMER_VIRTUAL, &tout_val,0);   //setting the timer
    	}
    
    /* create a new thread
     * Expects pointer to thread type, the function to be started,
     * and the argument for the function.
     * Returns id of new thread, or THR_FAILURE if unable to create.
     */

    	ThreadId_t thr_create(void * (*start_fn)(void *), void *arg)
    	{
		if(tdt==THR_SWITCH)
			printf("creating/initialising a new thread with thread id= %d\n",*(int*)&arg);
		fflush(stdout);
		
		/*allocating memory for each thread */
		TCB * t=(TCB *)malloc(sizeof(TCB ));
		
		/* initialising the semaphore and the blocking Q */
		usem_init(&t->barrier,0);
		struct queue * bq=malloc(sizeof(struct queue));
		q_init(bq);
		t->barrier.queue=bq;
      		t->id=(*(int *)arg);	              /* unique identifier */
      		t->state=READY;			      /* ThreadState */
		t->fn_body_ptr=start_fn;      	      /* starting function */
		t->fn_arg_ptr=arg;	              /* argument to starting function */
		t->result=NULL;			      /* function result */
		getcontext(&t->context);	      /* registers, stack, signals, etc.  */
		if(sigemptyset(&t->context.uc_sigmask)==-1)
			return THR_FAILURE;
		t->context.uc_link=0;
		t->context.uc_stack.ss_flags=0;
		t->context.uc_stack.ss_size=SIGSTKSZ;
		t->context.uc_stack.ss_sp=calloc(SIGSTKSZ,sizeof(char));
		if(!t->context.uc_stack.ss_sp)
			return THR_FAILURE;

		/* Thread count statistics */
      		t->num_blocked=0;
      		t->num_run=0;
      		t->num_ready=1;
      		t->num_completed=0;
      	
		 /* time spent in ready queue */
		t->ready_s=0;

		/* spent executing */
		t->executed_s=0;

		/* time last enqueued or dispatched */
		t->last_change=0;
		makecontext(&t->context,(void(*)())wrapper,1,t);		
		
		/* adding TCB to the ready Q */
		add_last(ready_q,t);

		return t->id;
	}

/* Wrapper function is called when ever a new thread is initiated to 
	 * do some house keeping before and after the thread starts and finishes */
	void* wrapper(TCB * thread){
		void * result=thread->fn_body_ptr(thread->fn_arg_ptr); /* actual function call of the thread */
		wrapper_secondhalf(result);	/* to clean up stuffs like releasing the blocking Q, setting the result etc */
		scheduler(THR_COMPLETED);
		return NULL;
	} 

	/* second half of wrapper function
	 * will set the thread state to COMPLETED
	 * and will remove all threads being blocked in it and add them to ready Q
	 */
	void wrapper_secondhalf(void * res){
		signal(SIGVTALRM,SIG_IGN);/* ignoring the timer interrupt signal */
		
		running_tcb->state=COMPLETED;	
		if(tdt==THR_SWITCH)
			printf("Thread with id %d is COMPLETED\n",running_tcb->id);
		fflush(stdout);

		running_tcb->result=res;		//setting the result	
		add_last(completed_q,running_tcb);	//adding the completd thread to Completed Q
		while(!queue_empty((struct queue*)running_tcb->barrier.queue)){
			if(tdt==THR_SWITCH)
				printf("Thread with id=%d switching from BLOCKED to READY\n",((TCB * )((struct queue*)running_tcb->barrier.queue)->head->tcb)->id);
			fflush(stdout);	
			/* adding the blocked thread to ready Q */
			add_last(ready_q,running_tcb=remove_first(  (struct queue*)running_tcb->barrier.queue));
			
		}
	}

    /* exit the thread
     * returning a pointer to the result.
     * Note that the pointer should not be on the thread's
     * stack as the thread will have exited.
     */
    	void thr_exit(void *result){
		signal(SIGVTALRM,SIG_IGN);    	/* ignoring the timer interrupt signal */
		wrapper_secondhalf(result);	/* sacond half of wrapper for clean up process */
		scheduler(THR_EXIT);		/* will schedule the next thread in ready Q */
    	}
	
    
	/* To set the time quantum and to switch on the time and turn on timer interrupt signal */
	void switch_on_timer(){
		thr_quantum(THR_QUANTUM_DEFAULT);
		setitimer(ITIMER_VIRTUAL, &tout_val,0);
		signal(SIGVTALRM,thr_yield); /* set the Alarm signal capture */
	
	}

    /* Voluntarily requinquish remaining quantum.
     * Another thread is schedule with a full quantum.
     */
    void thr_yield()
    {
	signal(SIGVTALRM,SIG_IGN);	/* ignoring the timer interrupt signal */
	scheduler(THR_YIELD);		/* will put current thread to queue and swaps to the 1st thread in ready Q */
 	switch_on_timer();		/* set timer and Alarm signal capture */	
    }

    /* wait for specified thread to exit
     * Returns THR_NOSUCHTHREAD if unable to find thread.
     * A generic pointer is set to the return code of the routine
     */
    	int thr_join(ThreadId_t id, void **result)
    	{
		if(tdt==THR_SWITCH)
			printf("Thread id=%d  joining to thread id=%d\n",running_tcb->id,id);
		fflush(stdout);
		if( (running_tcb->id==id) || ( queue_empty(ready_q) && queue_empty(completed_q)  )  )
		{
			return THR_NOSUCHTHREAD;
		}

		/* to check if the thread to be joined has already 
		 * completed its work and is in completed Q */
		struct node * temp=completed_q->head;		
		if(!queue_empty(completed_q)){
			while(temp){
				if(((TCB *)temp->tcb)->id==id){
					*result=((TCB *)temp->tcb)->result;
					return id;
				}
				temp=temp->next;
			}
		}
		/* the thread is not in completed Q so before checking in ready Q, see if ready Q is empty */
		if( queue_empty(ready_q))
		{
			printf("the thread was not in completed Q nor in ready Q\n");
			return THR_NOSUCHTHREAD;
		}		

		/* searching thread is not in completed q may be its in ready Q so search in ready Q */;
		temp=ready_q->head;				
		while(((TCB *)temp->tcb)->id!=id){
			temp=temp->next;
		}

		if(temp==NULL){
			printf("the thread was not found in ready Q\n");
			return THR_NOSUCHTHREAD;
		}
		if(tdt==THR_SWITCH)		
			printf("changing state of current thread to BLOCKED\n");
		fflush(stdout);
		/*putting the caller into the Blocking Q of the thread to be joined*/
		running_tcb->state=BLOCKED;		
		running_tcb->num_blocked++;
		add_last(((TCB *)(struct queue*)temp->tcb)->barrier.queue,running_tcb);
		signal(SIGVTALRM,SIG_IGN);	/* ignoring the timer interrupt signal */
		scheduler(THR_JOIN);	/* schedules the next thread in ready Q */	
		
		/* caller thread will look for result of the joined thread in the completed Q */
		temp=completed_q->head;
		while(((TCB *)temp->tcb)->id!=id){
			temp=temp->next;
		}
		if(((TCB *)temp->tcb)->id==id){
				*result=((TCB *)temp->tcb)->result;
				//free(temp->tcb);	/*do not free because thr_summary() will use them
		}
		else
		{
			return THR_NOSUCHTHREAD;
		}
		if(queue_empty(ready_q)&&tdt==THR_SUMMARY){
			
			thr_summary(stdout);
		}
		return id;
	
    }


	
	void scheduler(int caller){
		count++;
		if(caller==THR_COMPLETED||caller==THR_EXIT){
			if(queue_empty(ready_q))
			{
				printf("ready_q is empty\n");
				return;
			}
			running_tcb->ready_s=(count-running_tcb->num_run)*THR_QUANTUM_DEFAULT;
			running_tcb->executed_s=(running_tcb->num_run)*THR_QUANTUM_DEFAULT;
			/* get the next thread in ready Q */
			running_tcb=remove_first(ready_q);
			running_tcb->state=RUNNING;		/*new thread status is running */
			if(tdt==THR_SWITCH)
				printf("Thread %d is now set to RUNNING from READY Q\n",running_tcb->id);
			running_tcb->num_run++;
			switch_on_timer(); 			/* set timer and Alarm signal capture */
			setcontext(&running_tcb->context);
		}else if(caller==THR_YIELD){
			if(running_tcb==NULL){
				printf("TCB not yet initialised\n");
				return;
			}
			running_tcb->ready_s=(count-running_tcb->num_run)*THR_QUANTUM_DEFAULT;
			running_tcb->executed_s=(running_tcb->num_run)*THR_QUANTUM_DEFAULT;
			if(queue_empty(ready_q)){
				return;
			}
			running_tcb->state=READY;	/*old thread status is ready */
			running_tcb->num_ready++;
			running_tcb->last_change=clock();
			add_last(ready_q,running_tcb); /*adding the old thread to ready Q */
			running_tcb=remove_first(ready_q);
			running_tcb->state=RUNNING;	/*new thread status is running */
			running_tcb->num_run++;
			switch_on_timer();		/* set timer and Alarm signal capture */
			swapcontext(&((TCB *)ready_q->tail->tcb)->context,&running_tcb->context);


		}else if(caller==THR_JOIN){
			running_tcb->ready_s=(count-running_tcb->num_run)*THR_QUANTUM_DEFAULT;
			running_tcb->executed_s=(running_tcb->num_run)*THR_QUANTUM_DEFAULT;
			TCB * blocked_thread=running_tcb;	/*current thread has to be blocked */
			running_tcb=remove_first(ready_q);	/*switch to next thread in ready Q */
			running_tcb->last_change=clock();
			running_tcb->state=RUNNING;
			running_tcb->num_run++;
			switch_on_timer();		/* set timer and Alarm signal capture */
			if(swapcontext(&blocked_thread->context,&running_tcb->context)){
				perror("unable to set the context");
			}

		}
		
	}
	

	
    
    /* report on threads
     * Generate thread table on specified output FILE.
     * For each thread:
     *	id, #times active, #time exectued (ms), #time in ready queue (ms)
     */
    	void thr_summary(FILE *FileHandle){
		if(FileHandle != NULL)
		{
			fprintf(FileHandle,"\n*****************************************THREADS SUMMARY***************************************\n");
			fprintf(FileHandle,"+-----------+-------------+---------+-------+---------+-------------+---------------+-------------------+\n");   
  			fprintf(FileHandle,"| Thread ID |   State     | Blocked | Ready | Running | Waiting(ms) | Execution(ms) | Enqued/Dispatched |\n");
  			fprintf(FileHandle,"+-----------+-------------+---------+-------+---------+-------------+---------------+-------------------+\n");
			/* summary of threads in completed Q */
			struct node * temp=completed_q->head;
			while(temp){
    				fprintf(FileHandle,"|    %d      | %s   |    %d    |   %d  |   %d    |  %d   |   %d    |   %.2f            |\n",
					((TCB*)temp->tcb)->id, str[((TCB*)temp->tcb)->state],((TCB*)temp->tcb)->num_blocked,((TCB*)temp->tcb)->num_ready,
					((TCB*)temp->tcb)->num_run,(int) ((TCB*)temp->tcb)->ready_s,
					(int)((TCB*)temp->tcb)->executed_s, (double)((TCB*)temp->tcb)->last_change/CLOCKS_PER_SEC);
	  			fprintf(FileHandle,"+-----------+-------------+---------+-------+---------+---------+-----------+-------------------+\n");
				temp=temp->next;
			}

			/* summary of threads in ready Q (if at all exists */
			temp=ready_q->head;
			while(temp){
				((TCB*)temp->tcb)->ready_s=(count-((TCB*)temp->tcb)->num_run)*THR_QUANTUM_DEFAULT;
				((TCB*)temp->tcb)->executed_s=(((TCB*)temp->tcb)->num_run)*THR_QUANTUM_DEFAULT;
    				fprintf(FileHandle,"|    %d      | %s   |    %d    |   %d  |   %d    |  %d   |   --    |   -----------     |\n",
					((TCB*)temp->tcb)->id, str[((TCB*)temp->tcb)->state],((TCB*)temp->tcb)->num_blocked,((TCB*)temp->tcb)->num_ready,
					((TCB*)temp->tcb)->num_run, (int)((TCB*)temp->tcb)->ready_s);
	  			fprintf(FileHandle,"+-----------+-------------+---------+-------+---------+---------+-----------+-------------------+\n");
				temp=temp->next;
			}
			running_tcb->ready_s=(count-running_tcb->num_run)*THR_QUANTUM_DEFAULT;
			running_tcb->executed_s=(running_tcb->num_run)*THR_QUANTUM_DEFAULT;
			fprintf(FileHandle,"|    %d      | %s   |    %d    |   %d  |   %d    |  %d   |   --    |    --------       |\n",
					((TCB*)running_tcb)->id, str[((TCB*)running_tcb)->state],((TCB*)running_tcb)->num_blocked,((TCB*)running_tcb)->num_ready,
					((TCB*)running_tcb)->num_run,(int)((TCB*)running_tcb)->ready_s);
	  			fprintf(FileHandle,"+-----------+-------------+---------+-------+---------+---------+-----------+-------------------+\n");
			fprintf(FileHandle,"**************************************** Thread Summary End ****************************************\n");
			fclose(FileHandle);	
  		}	
		fflush(stdout);
	}
	

#if defined(__cplusplus)
  }
}
#endif
