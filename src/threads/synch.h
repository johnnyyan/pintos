#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* A counting semaphore. */
struct semaphore 
  {
    unsigned value;             /* Current value. */
    struct list waiters;        /* List of waiting threads. */
  };

void sema_init (struct semaphore *, unsigned value);
void sema_down (struct semaphore *);
bool sema_try_down (struct semaphore *);
void sema_up (struct semaphore *);
void sema_self_test (void);

/* Lock. */
struct lock 
  {
    struct thread *holder;      /* Thread holding lock (for debugging). */
    struct semaphore semaphore; /* Binary semaphore controlling access. */
  };

void lock_init (struct lock *);
void lock_acquire (struct lock *);
bool lock_try_acquire (struct lock *);
void lock_release (struct lock *);
bool lock_held_by_current_thread (const struct lock *);

/* Condition variable. */
struct condition 
  {
    struct list waiters;        /* List of waiting threads. */
  };

void cond_init (struct condition *);
void cond_wait (struct condition *, struct lock *);
void cond_signal (struct condition *, struct lock *);
void cond_broadcast (struct condition *, struct lock *);


/* Resource sentinel - wake a thread when all resources exhausted */
struct sentinel{
	int64_t remaining;
	struct thread * t;
};

void sentinel_init(struct sentinel *, int64_t);
bool sentinel_discharge(struct sentinel *);
void sentinel_charge(struct sentinel *);
void sentinel_twiddle(struct sentinel *);

/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    int priority;
    struct semaphore semaphore;         /* This semaphore. */
  };


bool condVarLess (const struct list_elem *, const struct list_elem *, void * UNUSED);


/* tom: DO NOT USE THIS ROUTINE UNDER ANY CIRCUMSTANCES!!!!!
 * It is not an "optimization" nor is it a "barrier" -- it is dangerous code. 
 * It is only included here because its needed for some configuration
 * code in one of the device drivers, and even there it could have
 * been engineered around.  This should be a private routine inside
 * a class file, but we're not in C++ so we can't do that. 
 *
 * OK, now that I've piqued your interest, what this does is to prevent
 * the compiler from reordering instructions across the call to this
 * routine.  But if your code depends on that, it means you've badly 
 * designed it. Use structured synchronization code -- locks and 
 * condition variables instead.  Ignore the reference guide. 
 */

/* Optimization barrier.

   The compiler will not reorder operations across an
   optimization barrier.  See "Optimization Barriers" in the
   reference guide for more information.*/
#define barrier() asm volatile ("" : : : "memory")

#endif /* threads/synch.h */
