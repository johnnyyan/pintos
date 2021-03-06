/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      list_insert_ordered (&sema->waiters, &thread_current ()->elem, less, NULL);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  sema->value++;
  if (!list_empty (&sema->waiters)) 
    thread_unblock (list_entry (list_pop_front (&sema->waiters),
                                struct thread, elem));
  
  intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* tom: I find it easier to think of a lock in its own terms,
 * rather than in terms of a semaphore.  A lock provides mutual exclusion.
 * There are two operations: acquire and release.  Acquire waits
 * until the lock is free, and then acquires it, atomically.
 * Only one thread can hold the lock at a time, and the lock holder
 * must be the one that calls release.
 *
 * In Pintos, locks are implemented in terms of semaphores.
 * One can also implement semaphores in terms of locks!
 * However, we need semaphores for interrupt handlers (since
 * we need a non-blocking wakeup to call within an interrupt handler),
 * so the choice here makes some sense, even if its more confusing than
 * it needs to be.
 */

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));
  
  enum intr_level old_level = intr_disable ();

  struct thread *cur = thread_current ();

  // If the lock acquire is going to fail, then lets donate our priority
  if (lock->semaphore.value == 0)
    {
      cur->blockingLock = lock;
      priorityDonate (cur);
    }
    
  sema_down (&lock->semaphore);
  
  lock->holder = cur;
  cur->blockingLock = NULL;
  // Add to the list of locks this thread holds
  list_push_back (&cur->locksHeld, &lock->elem);
  intr_set_level (old_level);
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    {
      struct thread *cur = thread_current ();
      lock->holder = cur;
      cur->blockingLock = NULL;
      list_push_back (&cur->locksHeld, &lock->elem);
    }
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));
  
  enum intr_level old_level = intr_disable ();

  struct thread *cur = thread_current ();
  lock->holder = NULL;
  
  // Remove the lock from the list of locks held by this thread
  list_remove (&lock->elem);
  
  // Update the priority of this thread in case it was using one donated
  // to it by a waiter of this lock.
  cur->priority = max_held_priority (cur);
  
  sema_up (&lock->semaphore);
  intr_set_level (old_level);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}


/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
/* tom: a note on the implementation.  It would be conceptually simpler
 * to implement this directly in terms of thread_block().
 * The key point is that you need to atomically wait for the condition
 * and release the lock.  You can do that by turning off interrrupts
 * and calling thread_block.  Or you can implement it in terms of semaphores,
 * as is done here.  This code is tricky since once you release the lock
 * someone else might grab the lock, and call condition_signal before 
 * you've finished condition_wait!  However, this will still work -- 
 * the signaller will call semaphore_up, so when wait calls semaphore_down,
 * it finds out it doesn't need to wait.  Work out for yourself why it
 * works in the other direction -- if wait calls "down" before signal 
 * calls "up".  We create a semaphore for each waiter so that we can
 * precisely deliver the signal to the correct waiter.
 */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  
  // Insert this waiter into the waiters list in an order that preserves priority
  waiter.priority = thread_current ()->priority;
  list_insert_ordered (&cond->waiters, &waiter.elem, condVarLess, NULL);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) 
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                          struct semaphore_elem, elem)->semaphore);
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}

/* Initialize the sentinel. */
void 
sentinel_init (struct sentinel *s, int64_t initialRemaining)
{
  ASSERT (s != NULL);
  s->remaining = initialRemaining;
  s->t = NULL;
}

/* Registers one's self as the waiting thread.  We coded a version of this
 * with a waiter's list, but in order to keep it no more complicated than
 * needed, we went with only 1 waiting thread.
 * This thread waits/blocks until the resources in this sentinel are exhauseted.*/
void 
sentinel_twiddle (struct sentinel *s)
{
  enum intr_level old_level = intr_disable ();
  ASSERT (s != NULL);

  if (s->remaining > 0 && s->t == NULL)
    {
      s->t = thread_current ();
      thread_block ();
    }
  intr_set_level (old_level);
}

/* Lower the amount of resources available and wake up the waiter if
 * we have exhausted all of them. */
bool 
sentinel_discharge (struct sentinel *s)
{
  ASSERT(s!=NULL);
  
  enum intr_level old_level = intr_disable ();
  (s->remaining)--;
  
  bool toReturn;
  
  /* Unblock all waiters if resources exhausted */
  if (s->remaining < 1 && s->t != NULL)
    {
      thread_unblock_no_schedule (s->t);
      toReturn = true;
    }
  else 
    toReturn = false;
	 
  intr_set_level (old_level);
  return toReturn;
}

/* Charge the sentinel.  That is - raise the count of resources available. */
void 
sentinel_charge (struct sentinel *s)
{
  enum intr_level old_level = intr_disable ();
  (s->remaining)++;
  intr_set_level (old_level);
}

/* Comparator function used for sorting waiters in a condition variable list
 * sorts by element priority. */
bool 
condVarLess (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  ASSERT (a != NULL);
  ASSERT (b != NULL);
  
  struct semaphore_elem * s1 = list_entry (a, struct semaphore_elem, elem);
  struct semaphore_elem * s2 = list_entry (b, struct semaphore_elem, elem);
  
  return s1->priority > s2->priority;
}

