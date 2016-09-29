/* Pavan Kandepet
 * 9/15/08
 * Tanenbaum's algorithm for the Dining philosophers problem using a user
 * defined thread library.
 * cleaned up a bit 9/11/2010 - Marie Roch
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/errno.h>

#include <unistd.h>
#include "uthread.h"
#include "sem.h"
#include <signal.h>
 
#define N 5               /* Number of philosophers */
#define ENOUGHTIMES 20    /* Number of think/hungry/eat cycles before done */
#define LEFT(i) (i+N-1)%N /* Philosopher on the left side */  
#define RIGHT(i) (i+1)%N  /* Philosopher on the right side */

#define TRUE 1
#define RESOURCE 10	// unable to allocate resource
#define NOSUCHTHR 20
#define CATASTROPHIC	99	// Really bad failure

/* Adjust for shorter or longer delays when eating/thinking */
#define DELAYMAGNITUDE 10000000

using namespace std;

typedef enum {EATING, HUNGRY, THINKING} ACTION;

/* Information about a philosopher */
typedef struct {
  /* semaphore for this philosopher, 
   *	down when ready to eat
   *	up when satiated
   */
  usem_t sem;  

  ACTION state;
  /* count of times in each state */
  int history[THINKING+1]; /* # of times in each state */
  
} philosopher;

/* Common data across philosophers */
typedef struct {
  usem_t mutex;         /* critical section */
  usem_t stats;               /* Semaphore for gathering statistics */
  philosopher philo[N];	/* philosopher information */
} philosophers;

typedef struct {
  int i_am;   /* unique philosopher identifier */
  philosophers *shared; /* data shared across philosopher threads */
} a_philosopher;
   

/* time conversion */
#define USperMS	  1000
#define MStoUS(us) ((us) * USperMS)
#define MSperS    1000  
#define MStoS(ms) ((ms) / MSperS)
#define MSperNS	  1000000
#define MStoNS(ms) ((ms) * MSperNS)

/*
 * delay(N) - delay N units via loop
 * Using a loop is a bad way to deal with timing.
 * However, since we are using the virtual timer to handle our interrupts,
 * using a standard OS sleep call (e.g. nanosleep) would defeat the purpose.
 * So we'll just busy wait insted
 */
void delay(long Counter) {
  for (long n=0; n < Counter*DELAYMAGNITUDE; n++)
    ;
  return;
}


/* void think(void)
 * Philospher is thinking
 */
void think(void) {
  int think_mean = 20;  /* average think time */
  int think_jitter = (rand() % 20) - 10;  /* +/- 10 */

  delay(think_mean + think_jitter);
}

/* void eat(void)
 * Philosopher is eating
 */
void eat(void) {
  int eat_mean = 10;
  int eat_jitter = (rand() % 10) - 5;  /* +/- 5 */

  delay(eat_mean + eat_jitter);
}

/* 
 * void test(int i, philosophers *shared)
 * See if philopher i is hungry and eligible to eat
 * Note that this might not be called by the thread
 * associated with one of the philosopher i's neighbors
 * as opposed to the philosopher's thread.
 */
void test(int i, philosophers *shared) {          
  /*
   * philosopher i can eat if she is hungry and the philosopher to her
   * right and left are not engaged in eating (thus they are not
   * holding their chop sticks...  no back scratching with the chop
   * sticks allowed)
   */

  /* find philosophers to left and right */
  int left = LEFT(i);
  int right = RIGHT(i);
  if (shared->philo[i].state == HUNGRY &&
      shared->philo[left].state != EATING &&
      shared->philo[right].state != EATING) {

    /* wake up philosopher i's thread
     * Philosopher i will have done down(i) after becoming hungry
     */
    if (up(&shared->philo[i].sem) != USEM_SUCCESS) {
      perror("Unable to complete philosopher up");
      exit(CATASTROPHIC);
    }
  }
}

/*
 * void take_chopsticks(int i, philosophers *shared)
 * Have philosopher i attempt to take her or his
 * chopsticks.  
 */
void take_chopsticks(int i, philosophers *shared) {
  /* Enter critical section */
  if (down(&shared->mutex) == USEM_FAILURE) {
    perror("Unable to complete critical section down");
    exit(CATASTROPHIC);
  }

  /* critical section */

  /* hungry as we try to take the chopsticks */
  shared->philo[i].state = HUNGRY;
  shared->philo[i].history[HUNGRY]++;
  printf("Philosopher %d is hungry...\n", i);

  test(i, shared);

  /* Exit critical section */
  if (up(&shared->mutex) == USEM_FAILURE) {
    perror("Unable to complete critical section up");
    exit(CATASTROPHIC);
  }

  /* block until someone signals us that both utensils are ready */
  if (down(&shared->philo[i].sem) == USEM_FAILURE) {
    perror("Unable to complete philosopher down");
    exit(CATASTROPHIC);
  }

  /* put philosopher in the eating state */
  shared->philo[i].state = EATING;
  shared->philo[i].history[EATING]++;
  printf("Philosopher %d is eating...\n", i);
}

void put_chopsticks(int i, philosophers *shared) {
  /* Enter critical section */
  if (down(&shared->mutex) == USEM_FAILURE) {
    perror("Unable to complete critical section down");
    exit(CATASTROPHIC);
  }
  /* critical section */

  /* done eating */
  shared->philo[i].state = THINKING;
  shared->philo[i].history[THINKING]++;
  printf("Philosopher %d is thinking...\n", i);

  /* Check to see if the philosophers to our left and right
   * are hungry and their neighbors are not eating
   */
  test(LEFT(i), shared);
  test(RIGHT(i), shared);

  /* Exit critical section */
  if (up(&shared->mutex) == USEM_FAILURE) {
    perror("Unable to complete critical section up");
    exit(CATASTROPHIC);
  }
}

/*
 * void *philosopher_thread(void *thread_arg)
 * Starting routine for a philosopher
 */
void *philosopher_thread(void *thread_arg) {

  a_philosopher *me = static_cast<a_philosopher *>(thread_arg);
  int cycles = ENOUGHTIMES;

  printf("Philosopher %d starting\n", me->i_am);
  fflush(stdout);

  /* think/hungry/eat a limited number of times */
  for (int k=0; k < cycles; k++) {
    think();
    take_chopsticks(me->i_am, me->shared);
    eat();  
    put_chopsticks(me->i_am, me->shared);
  }

  /* Set return code to philosopher number.  Note that we allocat an
   * integer and return a pointer to it.  We cannot use a local
   * variable as our stack will be deallocated on thread exit.
   */
  int *philo_done = new int();
  *philo_done = me->i_am;
  
  return philo_done;
}

/*
 * void init_philosophers()
 * Set up semaphores needed for dining philopher problem
 */
void init_philosophers(philosophers *phil) {
  
  for (int i=0; i<N; i++) {
    // allocate semaphore for each philosopher
    if (usem_init(&phil->philo[i].sem, 0) == USEM_FAILURE) {
      perror("Philosoper semaphore allocation failed");
      exit(RESOURCE);
    }
    // start with a full belly...
    phil->philo[i].state = THINKING;
    phil->philo[i].history[THINKING] = 1;
    phil->philo[i].history[EATING] = 0;
    phil->philo[i].history[HUNGRY] = 0;
  }

  // allocate semaphore for critical section
  if (usem_init(&phil->mutex, 1) == USEM_FAILURE) {
    perror("Critical section semaphore allocation failed");
    exit(RESOURCE);
  }
}

/* int report_state(philosophers *p_ptr)
 * Care giver report for the philosophers, reports number of times
 * that they have been in each state and returns the sum of the number
 * of times that they have eaten.
 */
int report_state(philosophers *p_ptr) {
  int count = 0;

  printf("\n+-----------------+----------+--------+--------+\n");   
  printf("| Philisopher ID  | Thinking | Eating | Hungry |\n");
  printf("+-----------------+----------+--------+--------+\n");

  // Report state of each philosopher
  for (int id=0; id < N; id++) {
    // Sum to find out how many times philosphers have eaten
    count += p_ptr->philo[id].history[EATING]; 
    // Write summary information
    printf("| Philisopher %2d  | %3d\t     |%3d     |%3d     |\n", 
	   id,
	   p_ptr->philo[id].history[THINKING], 
	   p_ptr->philo[id].history[EATING], 
	   p_ptr->philo[id].history[HUNGRY]);
  }
  printf("+-----------------+----------+--------+--------+\n");

  return count;
}
    

int main(int argc, char **argv) {
  int i, result;
  ThreadId_t tid[N];
  a_philosopher philos[N];
  philosophers socratesAndCo;
  void *resultPtr;


  // Set up our data structures
  init_philosophers(&socratesAndCo);

  thr_init();  /* Start the user thread library */

  thr_diagnostics(THR_QUIET);  /* set thread library debug state */

  printf("\nCreating threads for philosophers...\n");
  for (i=0; i<N; i++) {
    /* set up data structure for new thread */
    philos[i].i_am = i;  /* need to know which philosopher we are */
    philos[i].shared = &socratesAndCo;  /* need to access neighbors */

    /* Set to call  philosopher_thread with point to philosopher id */
    tid[i] = thr_create(philosopher_thread, &philos[i]); 
    if(tid[i] == THR_FAILURE){
      printf("Failed to create thread\n");
      exit(RESOURCE);
    }
  }
  printf("Succesfully created philosopher threads...\n\n");
  fflush(stdout);
    
  bool done = false;
  int report_every_nth = 20;
  int iteration = 0;
  while (! done) {
    /* voluntary context switch - let the philosophers run */
    thr_yield();   
    
    /* only generate a report every nth time we're scheduled */
    iteration++;
    if (iteration % report_every_nth == 0) {
      int count = report_state(&socratesAndCo);
      done = count >= N*ENOUGHTIMES;
    }
  }
  
  printf("Everyone should be done, wait for them to tidy up...\n");

  /* Wait for threads to finish */
  for(i=0;i<N;i++){
    /* Wait for philosopher[i] to complete and set a pointer 
       to its result */
    if(thr_join(tid[i], &resultPtr) == THR_NOSUCHTHREAD){
      printf("Unable to find thread %d\n", tid[i]);
      return(NOSUCHTHR);
    }
    result = *(int *)resultPtr;
    
    printf("Thread: %d exited with return code %d ", i, result);
    if (i == result)
      printf("expected\n");
    else
      printf("UNEXPECTED\n");
  }
  
  int meals_served = report_state(&socratesAndCo);
  printf("Over %d meals served /\\/\\ \n", meals_served);
  printf("The world's problems are solved.  Go home.\n");
  return(0);
}   
