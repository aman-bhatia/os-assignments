#include "uthread.h"

void 
thread_schedule(void)
{
  /*HW: 
      Read the schedular code below to understand what it is doing and the order 
        in which it schedules the threads.
   
    */

    /* To be improved and implemented in the HW: Change the schedular to implement the below behavior:
       The highest priority thread has to be scheduled first (priority=1 is higher than priority =2).
       If there are more than one thread with same priority, then they need to be scheduled in round-robin manner.

     stop search when all threads are checked and none can be scheduled.
    */

  /* Find another runnable thread. */

  int temp_priority = MAX_PRIORITY;
  
  thread_p t;
  t = current_thread + 1;

  int i;
  for (i=0; i < MAX_THREAD; i++) {

    if (t >= all_thread + MAX_THREAD)
      t = all_thread;

    if (t->state == RUNNABLE && t->priority < temp_priority) {
        next_thread = t;
        temp_priority = t->priority;
    }

    t = t+1;
  }


// We dont need this anymore because if current_thread is the only runnable thread
// next_thread will be assigned to it in above loop
/*
  if (t >= all_thread + MAX_THREAD && current_thread->state == RUNNABLE) {
    // The current thread is the only runnable thread; run it.
    next_thread = current_thread;
  }
*/

  if (next_thread == 0) {
    printf(2, "thread_schedule: no runnable threads; deadlock\n");
    exit();
  }

  if (current_thread != next_thread) {         /* switch threads?  */
    next_thread->state = RUNNING;
    thread_switch();
  } else
    next_thread = 0;
}

void 
thread_create(void (*func)(), int priority)
{
  thread_p t;

  //HW: Your code here
  //starting from all_thread, scan and find out which slot is FREE
  //use that slot to create a new thread
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) {
        break;
    }
  }

  // set priority
  t->priority = priority;

  // set esp to the top of the stack
  t->sp = (int)(t->stack + STACK_SIZE);

  // leave space for return address
  t->sp -= 4;

  // push return address on stack
  int * pointer_to_ret_addr = (int *) t->sp;
  *pointer_to_ret_addr = (int) func;  

  // leave space for registers that thread_switch will push
  t->sp -= 32;

  t->state = RUNNABLE;
}

