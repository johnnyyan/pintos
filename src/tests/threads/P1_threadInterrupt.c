#include "threads/thread.h"
#include "threads/interrupt.h"


 	
void runner (void);

void test_P1_threadInterrupt(void){

tid_t t1 = thread_create("first runner", PRI_DEFAULT, runner, NULL);
tid_t t2 = thread_create("second runner", PRI_DEFAULT, runner, NULL);
thread_yield();

}

void runner(){
	while(true);
}
