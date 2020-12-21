/**********************************************************************
 * Copyright (c) 2020
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "types.h"
#include "locks.h"
#include "atomic.h"
#include "list_head.h"

/*********************************************************************
 * Spinlock implementation
 *********************************************************************/
struct spinlock {
	int held;
};

/*********************************************************************
 * init_spinlock(@lock)
 *
 * DESCRIPTION
 *   Initialize your spinlock instance @lock
 */
void init_spinlock(struct spinlock *lock)
{
	lock->held = 0;
	return;
}

/*********************************************************************
 * acquire_spinlock(@lock)
 *
 * DESCRIPTION
 *   Acquire the spinlock instance @lock. The returning from this
 *   function implies that the calling thread grapped the lock.
 *   In other words, you should not return from this function until
 *   the calling thread gets the lock.
 */
void acquire_spinlock(struct spinlock *lock)
{
	while(compare_and_swap(&lock->held,0,1));
	return;
}

/*********************************************************************
 * release_spinlock(@lock)
 *
 * DESCRIPTION
 *   Release the spinlock instance @lock. Keep in mind that the next thread
 *   can grap the @lock instance right away when you mark @lock available;
 *   any pending thread may grap @lock right after marking @lock as free
 *   but before returning from this function.
 */
void release_spinlock(struct spinlock *lock)
{
	lock->held = 0;
	return;
}


/********************************************************************
 * Blocking mutex implementation
 ********************************************************************/
struct thread {
	pthread_t pthread;
	struct list_head list;
};

struct mutex {
	struct spinlock lock;
	struct thread t;
	struct list_head waitqueue;
	int S;
//	struct sigaction sigact;
};


/*********************************************************************
 * init_mutex(@mutex)
 *
 * DESCRIPTION
 *   Initialize the mutex instance pointed by @mutex.
 */
void init_mutex(struct mutex *mutex)
{
	mutex->lock.held= 0;
	mutex->S = 1;
	INIT_LIST_HEAD(&(mutex->waitqueue));
//	sigemptyset(&mutex->sigact.sa_mask);
//	sigaddset(&mutex->sigact.sa_mask,SIGINT);
//	sigprocmask(SIG_BLOCK,&mutex->sigact.sa_mask,NULL);
	return;
}

/*********************************************************************
 * acquire_mutex(@mutex)
 *
 * DESCRIPTION
 *   Acquire the mutex instance @mutex. Likewise acquire_spinlock(), you
 *   should not return from this function until the calling thread gets the
 *   mutex instance. But the calling thread should be put into sleep when
 *   the mutex is acquired by other threads.
 *
 * HINT
 *   1. Use sigwaitinfo(), sigemptyset(), sigaddset(), sigprocmask() to
 *      put threads into sleep until the mutex holder wakes up
 *   2. Use pthread_self() to get the pthread_t instance of the calling thread.
 *   3. Manage the threads that are waiting for the mutex instance using
 *      a custom data structure containing the pthread_t and list_head.
 *      However, you may need to use a spinlock to prevent the race condition
 *      on the waiter list (i.e., multiple waiters are being inserted into the 
 *      waiting list simultaneously, one waiters are going into the waiter list
 *      and the mutex holder tries to remove a waiter from the list, etc..)
 */

//pthread_t alarmT;
//void signal_handler(int signum)
//{
//	kill((pid_t)alarmT,SIGINT);
//}
//struct sigaction sa= {
//	.sa_handler = signal_handler,
//	.sa_flags = 0,
//},old_sa;

//struct sigaction sigact;
//sigemptyset(&sigact.sa_mask);
//sigaddset(&sigact.sa_mask.SIGINT);

void acquire_mutex(struct mutex *mutex)
{
//	int signum;
	siginfo_t info;
	struct sigaction sigact;
//	struct sigaction sigact2;
//	sigemptyset(&sa.sa_mask);
//	sigaddset(&sa.sa_mask,SIGUSR1);
//	sigemptyset(&sigact.sa_mask);
//	sigaddset(&sigact.sa_mask,SIGINT);
//	sigemptyset(&sigact2.sa_mask);
//	sigaddset(&sigact2.sa_mask,SIGUSR1);
//	sigaction(SIGUSR1,&sa,&old_sa);
//	struct thread tmp;
	acquire_spinlock(&mutex->lock);
	mutex->S--;
//	struct thread*tmp = malloc(sizeof(struct thread));
//	tmp->pthread = pthread_self();
	struct thread tmp;
	tmp.pthread=pthread_self();
//	acquire_spinlock(&mutex->lock);
//	mutex->S--;
//	printf("I'm thread %ld\n",tmp->pthread);
	
	if(!(mutex->S<0))
	{
		mutex->t = tmp;
		release_spinlock(&mutex->lock);
		return;
	}
//	list_add_tail(&tmp->list, &mutex->waitqueue);
//	release_spinlock(&mutex->lock);
	list_add_tail(&tmp.list, &mutex->waitqueue);
	release_spinlock(&mutex->lock);
	//for debugging
//	printf("I'll waiting %ld\n",tmp->pthread);
//	struct thread*temp;
//	printf("waiting list : ");
//	list_for_each_entry(temp,&mutex->waitqueue,list)
//	{
//		printf("%ld, ",temp->pthread);
//	}
//	printf("\n");
	
//	release_spinlock(&mutex->lock);
//	sigprocmask(SIG_BLOCK,&sa.sa_mask,NULL);
	sigemptyset(&sigact.sa_mask);
	sigaddset(&sigact.sa_mask,SIGINT);
	sigprocmask(SIG_BLOCK,&sigact.sa_mask,NULL);
//	printf("Helloworl\n");

//	release_spinlock(&mutex->lock);
//	signum = sigwaitinfo(&sa.sa_mask,&info);
//	signum = sigwaitinfo(&sigact1.sa_mask,&info);
	sigwaitinfo(&sigact.sa_mask,&info);
//	acquire_spinlock(&mutex->lock);
//	printf("%d\n",signum);
//	sigprocmask(SIG_UNBLOCK,&sa.sa_mask,NULL);
//	sigprocmask(SIG_UNBLOCK,&sigact1.sa_mask,NULL);	
//	acquire_spinlock(&mutex->lock);
//	if(signum==SIGUSR1)
//	{
//		acquire_spinlock(&mutex->lock);
//		goto again;
		mutex->t = tmp;
//		list_del_init(&tmp->list);
//		mutex->t = *tmp;
//		printf("I'm new owner %ld\n",tmp->pthread);
//		list_del_init(&tmp->list);
//	}
//	release_spinlock(&mutex->lock);
	return;
/*
	if(mutex->S<0)
	{
		list_add_tail(&tmp->list,&mutex->waitqueue);
		release_spinlock(&mutex->lock);
		signum = sigwaitinfo(&sa.sa_mask,&info);
		if(signum==SIGUSR1)
		{
			mutex->t = *tmp;
		}
		return;
	}
	mutex->t = *tmp;
	release_spinlock(&mutex->lock);
	return;
*/
}



/*********************************************************************
 * release_mutex(@mutex)
 *
 * DESCRIPTION
 *   Release the mutex held by the calling thread.
 *
 * HINT
 *   1. Use pthread_kill() to wake up a waiter thread
 *   2. Be careful to prevent race conditions while accessing the waiter list
 */
void release_mutex(struct mutex *mutex)
{
//	printf("Bye mutex %ld\n",mutex->t->pthread);
//	sigprocmask(SIG_UNBLOCK,&sigact.sa_mask,NULL);
	acquire_spinlock(&mutex->lock);
	mutex->S++;
	if(mutex->S<=0)
	{
		struct thread*t = list_first_entry(&mutex->waitqueue, struct thread, list);
//		printf("I'll delete : %ld\n\n",t->pthread);
//		acquire_spinlock(&mutex->lock);
//		release_spinlock(&mutex->lock);
//		list_del_init(&t->list);
//		kill((pid_t)t->pthread,SIGINT);
//		alarmT = t->pthread;
		pthread_kill(t->pthread,SIGINT);
//		mutex->t = NULL;
//		mutex->flag=0;
//		release_spinlock(&mutex->lock);
//		mutex->t = malloc(sizeof(struct thread));
//		mutex->t= t;
		list_del_init(&t->list);
	}
//	signal_sem(&mutex->sem2);
	release_spinlock(&mutex->lock);
	return;
}



/*********************************************************************
 * Ring buffer
 *********************************************************************/
struct ringbuffer {
	/** NEVER CHANGE @nr_slots AND @slots ****/
	/**/ int nr_slots;                     /**/
	/**/ int *slots;                       /**/
	/*****************************************/
	int in;
	int out;
//	int count;	
	struct mutex mutex;
	struct mutex empty;
	struct mutex full;
};

struct ringbuffer ringbuffer = {
};

/*********************************************************************
 * enqueue_into_ringbuffer(@value)
 *

 * DESCRIPTION
 *   Generator in the framework tries to put @value into the buffer.
 */
//int count = 0;
void enqueue_into_ringbuffer(int value)
{
//printf("%d\n",value);
//again:
	/*
	acquire_spinlock(&ringbuffer.lock);
	if(ringbuffer.count == ringbuffer.nr_slots-1)
	{
		release_spinlock(&ringbuffer.lock);
//		goto again;
		while(ringbuffer.count > ringbuffer.nr_slots-1);
//		release_spinlock(&ringbuffer.lock);
	}
	ringbuffer.slots[ringbuffer.in] = value;
	ringbuffer.in = (ringbuffer.in+1)%ringbuffer.nr_slots;
	ringbuffer.count++;
	printf("1 : %d\n",value);
	release_spinlock(&ringbuffer.lock);
	*/
/*	acquire_mutex(&ringbuffer.full);
	acquire_mutex(&ringbuffer.mutex);
	ringbuffer.slots[ringbuffer.in] = value;
	ringbuffer.in = (ringbuffer.in+1)%ringbuffer.nr_slots;
	release_mutex(&ringbuffer.mutex);
	release_mutex(&ringbuffer.empty);*/
//	printf("I'm acquire\n");
	
//	if(count==ringbuffer.nr_slots)
//	{
//		release_spinlock(&ringbuffer.lock);
//		while(count>=ringbuffer.nr_slots);
//	}	
	
	acquire_mutex(&ringbuffer.full);
	acquire_mutex(&ringbuffer.mutex);
	
	ringbuffer.slots[ringbuffer.in] = value;
	ringbuffer.in = (ringbuffer.in+1)%ringbuffer.nr_slots;
//	printf("%d %d\n",ringbuffer.in,value);
//	count++;
	
	release_mutex(&ringbuffer.mutex);
	release_mutex(&ringbuffer.empty);
	
//	release_spinlock(&ringbuffer.lock);
}


/*********************************************************************
 * dequeue_from_ringbuffer(@value)
 *
 * DESCRIPTION
 *   Counter in the framework wants to get a value from the buffer.
 *
 * RETURN
 *   Return one value from the buffer.
 */
int dequeue_from_ringbuffer(void)
{
//again :
/*
	printf("Hi\n");
	acquire_spinlock(&ringbuffer.lock);
	if (ringbuffer.count == 0)
	{
		release_spinlock(&ringbuffer.lock);
		while(ringbuffer.count <= 0);
//		release_spinlock(&ringbuffer.lock);
//		goto again;
	}
	int n = ringbuffer.slots[ringbuffer.out];
	printf("2 : %d",n);
	ringbuffer.out = (ringbuffer.out+1)%ringbuffer.nr_slots;
	ringbuffer.count--;
	release_spinlock(&ringbuffer.lock);
//	printf("I'm dequeue");
*/
/*	acquire_mutex(&ringbuffer.empty);
	acquire_mutex(&ringbuffer.mutex);
	int n = ringbuffer.slots[ringbuffer.out];
	ringbuffer.out = (ringbuffer.out + 1)%ringbuffer.nr_slots;
	release_mutex(&ringbuffer.mutex);
	release_mutex(&ringbuffer.full);*/
//	acquire_spinlock(&ringbuffer.lock2);
//	if(count==0)
//	{
//		release_spinlock(&ringbuffer.lock2);
//		while(count<=0);
	
	acquire_mutex(&ringbuffer.empty);

	acquire_mutex(&ringbuffer.mutex);
	
//	}
	int n = ringbuffer.slots[ringbuffer.out];
	ringbuffer.out = (ringbuffer.out + 1) % ringbuffer.nr_slots;
//	printf("%d %d\n", ringbuffer.out, n);
//	count--;
	
	release_mutex(&ringbuffer.mutex);
	release_mutex(&ringbuffer.full);
	
//	release_spinlock(&ringbuffer.lock2);

	return n;
}


/*********************************************************************
 * fini_ringbuffer
 *
 * DESCRIPTION
 *   Clean up your ring buffer.
 */
void fini_ringbuffer(void)
{
	free(ringbuffer.slots);
}

/*********************************************************************
 * init_ringbuffer(@nr_slots)
 *
 * DESCRIPTION
 *   Initialize the ring buffer which has @nr_slots slots.
 *
 * RETURN
 *   0 on success.
 *   Other values otherwise.
 */
int init_ringbuffer(const int nr_slots)
{
	/** DO NOT MODIFY THOSE TWO LINES **************************/
	/**/ ringbuffer.nr_slots = nr_slots;                     /**/
	/**/ ringbuffer.slots = malloc(sizeof(int) * nr_slots);  /**/
	/***********************************************************/
	ringbuffer.in = 0;
	ringbuffer.out = 0;
//	ringbuffer.count = 0;
//	ringbuffer.lock.held=0;
//	ringbuffer.lock2.held=0;
	ringbuffer.empty.S = 0;
	ringbuffer.full.S = nr_slots;
	ringbuffer.mutex.S = 1;
	ringbuffer.empty.lock.held=0;
	ringbuffer.full.lock.held = 0;
	ringbuffer.mutex.lock.held=0;
	INIT_LIST_HEAD(&(ringbuffer.empty.waitqueue));
	INIT_LIST_HEAD(&(ringbuffer.full.waitqueue));
	INIT_LIST_HEAD(&(ringbuffer.mutex.waitqueue));
	return 0;
}
