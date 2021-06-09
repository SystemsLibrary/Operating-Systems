/*
 * locking.c
 *
 *  Created on: 26 May 2021
 *      Author: Grant
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <threads.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#define MIN_DELAY 500
#define MAX_DELAY 200000

static inline unsigned long tas(volatile unsigned long* ptr)//Test and Set
{
return __sync_lock_test_and_set(ptr, 1);
}


static inline unsigned long cas(volatile unsigned long* ptr, unsigned long old, unsigned long _new)//Compare and Swap
{
    return __sync_val_compare_and_swap(ptr, old, _new);
}

static inline unsigned long fetch_and_add(volatile unsigned long* ptr, unsigned long _increment)
{
  return __sync_fetch_and_add(ptr,_increment);
}

struct my_spinlock_struct {
  volatile unsigned long lock_bit;
  unsigned int thread_ID;
  int recursive_counter;
};

typedef struct my_spinlock_struct my_spinlock_t;

void my_spinlock_init(my_spinlock_t *lock)
{
	lock->lock_bit = 0;
	lock->thread_ID = 0;
	lock->recursive_counter = 0;
}

void my_spinlock_unlock(my_spinlock_t *lock)
{
  if(lock->recursive_counter == 1 || lock->recursive_counter ==  0){
    lock->lock_bit = 0;
    lock->thread_ID = 0;
    lock->recursive_counter = 0;
  }
  else{
    lock->recursive_counter = lock->recursive_counter - 1;
  }
}

void my_spinlock_lockTAS(my_spinlock_t *lock)
{
    while(tas(&(lock->lock_bit))==1 && lock->thread_ID != pthread_self()) {}
    if(lock->thread_ID == pthread_self())
    {
    	lock->recursive_counter++;
    }
    lock->thread_ID = pthread_self();

    return;
}

void my_spinlock_lockTTAS(my_spinlock_t *lock)
{
  while(1){
    while(lock->lock_bit==1 && lock->thread_ID != pthread_self()){}; // spin until lock_bit == 0
    if(tas(&(lock->lock_bit))==0){
      lock->thread_ID = pthread_self();
      return;
    }
    else if(lock->thread_ID == pthread_self()){
      lock->recursive_counter++;
      return;
    }
  }
}

struct my_mutex_struct {
  volatile unsigned long lock_bit;
  unsigned int thread_ID;
  int recursive_counter;
};

typedef struct my_mutex_struct my_mutex_t;

void my_mutex_init(my_mutex_t *lock)
{
	lock->lock_bit = 0;
	lock->thread_ID = 0;
	lock->recursive_counter = 0;
}

void my_mutex_unlock(my_mutex_t *lock)
{
    lock->lock_bit = 0;
    lock->thread_ID = 0;
    lock->recursive_counter = 0;
}

void my_mutex_lock(my_spinlock_t *lock)
{
	int delay = MIN_DELAY;
	  //printf("%i ",delay);
	  while(1){
	    while(lock->lock_bit==1){}; // spin until lock_bit == 0
	    if(tas(&(lock->lock_bit))==0){
	      lock->thread_ID = pthread_self();
	      return;
	    }
	    usleep(rand() % delay);
	    if(delay < MAX_DELAY) delay = 2*delay;
	  }
}

my_spinlock_t count_myspin;
my_mutex_t count_mymutex;

volatile long cnt = 0;
volatile long ent = 0;

void *mypthreadSpinTTASTest(void *vargp)
{
 	long i, niters = *((long *)vargp);

 	my_spinlock_lockTTAS(&count_myspin);
 	cnt++;
 	for (i = 0; i < niters; i++)
	{
 		ent += cnt;
	}

 	printf("ent is %d\n", ent);
 	my_spinlock_unlock(&count_myspin);
}

void *mypthreadMutexTest(void *vargp)
{
 	long i, niters = *((long *)vargp);

 	my_mutex_lock(&count_mymutex);
 	cnt++;
 	for (i = 0; i < niters; i++)
	{
 		ent += cnt;
	}

 	printf("ent is %d\n", ent);
 	my_mutex_unlock(&count_mymutex);
}

int main()
{
	long niters;

	pthread_t tid1, tid2, tid3, tid4, tid5;
 	niters = 5;

 	my_spinlock_init(&count_myspin);

 	pthread_create(&tid3, NULL, &mypthreadSpinTTASTest, &niters);
	pthread_create(&tid1, NULL, &mypthreadSpinTTASTest, &niters);
 	pthread_create(&tid2, NULL, &mypthreadSpinTTASTest, &niters);
	pthread_create(&tid4, NULL, &mypthreadSpinTTASTest, &niters);
	pthread_create(&tid5, NULL, &mypthreadSpinTTASTest, &niters);
	pthread_join(tid5, NULL);
	pthread_join(tid2, NULL);
	pthread_join(tid1, NULL);
	pthread_join(tid3, NULL);
	pthread_join(tid4, NULL);

	printf("ent=%ld\n", ent);
}
