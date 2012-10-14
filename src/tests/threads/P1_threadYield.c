#include "threads/thread.h"
#include "threads/interrupt.h"


 	

void yielder (void);


void test_P1_threadYield(void){

enum intr_level prior = intr_set_level(INTR_OFF);

tid_t t1 = thread_create("first yielder", PRI_DEFAULT, yielder, NULL);
tid_t t2 = thread_create("second yielder", PRI_DEFAULT, yielder, NULL);
thread_yield();

intr_set_level(prior);
}


void yielder () { thread_yield(); }
