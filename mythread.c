/*#########################################################################################
 *      This file contains the code for a local non-preemptive thread library `mythread`.
 *      Some code used here is picked up from various online resources.
 *      Please refer the REFERENCES file for the list of online resources used.
 *
 *      --------------------------------------
 *      Author: Ramya Vijayakumar
 *      --------------------------------------
 *#########################################################################################
*/

#define _GNU_SOURCE
#include <sched.h>
#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include<ucontext.h>
#include <linux/unistd.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "mythread.h"

// 8kB stack
#define STACK_SIZE 1024*8
//#define MAX_ACTIVE_THREADS 10

char *stack, *stack_top;

typedef struct Thread{
	ucontext_t context;
	int tid;
	int wait_for_all;
    	int to_exit;
    	struct Thread *parent;
    	struct threadQ *children;
    	struct Thread *waitingFor;
}Thread;

typedef struct queue{
        struct Thread *ready;
        struct queue *next;
}queue;

typedef struct threadQ{
        struct queue *head;
}threadQ;

int thread_id = 0;
int thread_count = 1;
int wait_count = 0;

Thread *curr_thread = NULL, *main_thread = NULL;
ucontext_t *main_context;
threadQ *readyQ = NULL, *blockedQ = NULL;

typedef struct semaphore{
        int semCount;
        int initValue;
      	struct threadQ *semBlocked;
        struct threadQ *waitingthreads;
}semaphore;

semaphore *semQ = NULL;

/*typedef struct queue{
	int pid;
	struct queue *next;
	
}readyQ;

readyQ *front,*rear;

static ucontext_t mainContext;*/

void enqueue(struct threadQ *threadQ, Thread *ready)
{
	/*readyQ *temp = (readyQ *)malloc(sizeof(readyQ));
	temp->pid = pid;
	temp->next = NULL;
	if(front == NULL)
        {
                front = temp;
                rear = temp;
        }
        else
        {
                readyQ *curr;
                curr = front;
                curr->next = temp;
                front = temp;
        }*/

	struct queue *tempQ = (queue *)malloc(sizeof(queue));
        tempQ->ready = ready;
        tempQ->next = NULL;
        if (threadQ->head == NULL)
	{
                threadQ->head = tempQ;
        }
	else
	{
                struct queue *traverse = threadQ->head;
                while(traverse->next != NULL)
		{
                        traverse = traverse->next;
                }
                traverse->next = tempQ;
        }
}

Thread* dequeue(struct threadQ *threadQ)
{
        /*int num;
        readyQ *curr;
        curr = front;
        if(rear == NULL)
        {
                printf("\nEmpty ready queue\n");
        }
        else
        {
                curr = rear;
                num = curr->pid;
                rear=rear->next;
                free(curr);
                printf("\nElement %d is deleted from the queue\n",num);
        }*/

	Thread *thread;
        if (threadQ->head == NULL)
	{
                thread = NULL;
        }
	else
	{
                thread = threadQ->head->ready;
                if (threadQ->head->next != NULL)
		{
                        threadQ->head = threadQ->head->next;
                }
		else
		{
                        free(threadQ->head);
                        threadQ->head = NULL;
                }
        }
        return thread;
}

int ScanQ(struct threadQ *threadQ, Thread *join)
{
        struct queue *temp = threadQ->head;
        if (temp == NULL)
	{
                return 0;
        }
	else
	{
                while(temp != NULL)
		{
                        if(temp->ready->tid == join->tid)
			{
                                return 1;
                        }
			temp = temp->next;
                }
        }
        return 0;
}

void dumpQ(struct threadQ *threadQ)
{
	/*printf("\n-----ready queue elements are------\n");
	if(front == NULL)
        {
                printf("\nEmpty ready queue!\n");
        }
        else
        {
                readyQ *curr;
                curr = rear;
                while(curr->next != NULL)
                {
                        printf("\n%d\t",curr->pid);
                        curr=curr->next;
                }
                printf("\n%d\n",curr->pid);
        }*/

	struct queue *temp;
        temp = threadQ->head;
        if(temp == NULL){
        }else{
                while (temp != NULL)
		{
                        temp = temp->next;
                }
        }
}

// Create a new thread.
MyThread MyThreadCreate(void(*start_funct)(void *), void *args)
{
	/*int status;
	int pid;
        stack = malloc(STACK_SIZE);
        stack_top = stack+STACK_SIZE;
        pid = clone((void *)*start_funct, stack_top, CLONE_THREAD | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_VM | SIGCHLD, &args);
        printf("\nnew thread pid=%d\n",pid);
        if(pid == -1)
        {
                printf("\nError in creating thread\n");
        }*/

	Thread *thread = malloc(sizeof(Thread));
	getcontext(&thread->context);
	thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
        thread->context.uc_stack.ss_size = STACK_SIZE;
        thread->context.uc_link = 0;
	thread->context.uc_stack.ss_flags = 0;
        makecontext(&thread->context, (void *)*start_funct, 1, args);

	/*enqueue(thread->tid);
	printf("\nppid=%d and tid=%d\n",thread->ppid, thread->tid);
	printf("\nNew thread created and is placed in ready queue\n");
	dumpQ();

	MyThread mythread = (MyThread)thread;
	return mythread;*/

	threadQ *child = (threadQ *)malloc(sizeof(threadQ));
        child->head = NULL;
        thread->children = child;
        thread->tid = thread_id;
        thread->wait_for_all = 0;
        thread->to_exit = 0;
        
	thread_id++;
        thread->parent = curr_thread;
        
	enqueue(curr_thread->children, thread);
        enqueue(readyQ, thread);
        
	//printf("\nEnd of create\n");
	return (MyThread)(thread);
}

// Terminate invoking thread
void MyThreadExit(void)
{
	/*Thread thread;
	thread.tid = gettid();
	printf("\nExiting thread tid is %d\n", thread.tid);
	*/

	Thread *parent = curr_thread->parent;
        curr_thread->to_exit = 1;
        Thread *toExit = curr_thread;
        int exitBlocked = 0;
        int gotFromBlocked = 0;
        gotFromBlocked = checkparentandclear(parent);
        removeparentlink(readyQ, curr_thread);
        removeparentlink(blockedQ, curr_thread);
        if (gotFromBlocked)
	{
                enqueue(readyQ, curr_thread);
                curr_thread = dequeue(readyQ);
                setcontext(&(curr_thread->context));
        }
	else
	{
                curr_thread = dequeue(readyQ);
                if (curr_thread == NULL)
		{
                        curr_thread = dequeue(blockedQ);
                        while(curr_thread != NULL)
			{
                                if(curr_thread->to_exit == 1 )
				{
                                        if(curr_thread->children->head == NULL)
					{
                                                free((curr_thread->context).uc_stack.ss_sp);
                                                free(curr_thread);
                                                toExit = NULL;
                                        }
					else
					{
                                                break;
                                        }
                                }
				else if(curr_thread->wait_for_all == 1){
                                }else if(curr_thread->waitingFor != NULL){
                                }else if(curr_thread->children->head == NULL){
                                        break;
                                }
                                curr_thread = dequeue(blockedQ);
                        }
                        if (curr_thread == NULL)
			{
                                setcontext((ucontext_t *) &main_context);
                        }
			else
			{
                                setcontext(&(curr_thread->context));
                        }
                }
		else
		{
			if(exitBlocked)
			{
                                swapcontext(&(toExit->context),&(curr_thread->context));
                        }
			else
			{
                                setcontext(&(curr_thread->context));
                        }
                }
        }
	//printf("\nEnd of exit\n");
}

// Yield invoking thread
void MyThreadYield(void)
{
	//sched_yield();
	Thread *thread = curr_thread;
        curr_thread = dequeue(readyQ);
        if (curr_thread == NULL)
	{
                curr_thread = thread;
                return;
        }
        enqueue(readyQ, thread);
        swapcontext(&(thread->context), &(curr_thread->context));
	//printf("\nEnd of yield\n");
}

// Join with a child thread
int MyThreadJoin(MyThread thread)
{
	Thread *child = (Thread*) thread;
        Thread *join = curr_thread;
        int isActive = 0;
        
	if(!(ScanQ(curr_thread->children, child)))
	{
                return -1;
        }
        if(ScanQ(readyQ, child))
	{
                isActive = 1;
        }
	else if(ScanQ(blockedQ, child))
	{
                isActive = 1;
        }
	/*else if(ScanQ(semaphoreQ->semBlocked, child))
	{
                isActive = 1;
        }*/
	else
	{
                isActive = 0;
        }

        if(isActive)
	{
                join->waitingFor = child;
                enqueue(blockedQ, join);
                curr_thread = dequeue(readyQ);
                if (curr_thread != NULL)
		{
                        swapcontext(&(join->context), &(curr_thread->context));
		}
                else
		{
                        return -1;
                }
        }
	//printf("\nEnd of join\n");
        return 0;
}

// Join with all children
void MyThreadJoinAll(void)
{
	Thread *join_all = curr_thread;
        if(join_all->children->head == NULL)
	{
                return;
        }
	else
	{
                join_all->wait_for_all = 1;
                enqueue(blockedQ, join_all);
                curr_thread = dequeue(readyQ);
                swapcontext(&(join_all->context), &(curr_thread->context));
        }
	//printf("\nEnd of joinAll\n");
}

// Create and run the "main" thread
void MyThreadInit(void(*start_funct)(void *), void *args)
{
	main_context = (ucontext_t *) malloc(sizeof(ucontext_t));
	Thread *thread = (Thread *)malloc(sizeof(Thread));

	getcontext(&thread->context);

	thread_id = 0;
        thread_count = 1;
        wait_count = 0;
        readyQ = (struct threadQ *) malloc(sizeof(threadQ));
        readyQ->head = NULL;
        blockedQ = (struct threadQ *) malloc(sizeof(threadQ));
        blockedQ->head = NULL;

        //makecontext(&thread->context, (void *)*start_funct, 1, args);

        thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
        thread->context.uc_stack.ss_size = STACK_SIZE;
        thread->context.uc_link = 0;
        thread->context.uc_stack.ss_flags = 0;
       
	makecontext(&thread->context, (void *)*start_funct, 1, args);
 
	thread->tid = thread_id;
	thread->parent = NULL;
        thread->wait_for_all = 0;
	thread->to_exit = 0;

	threadQ *child = (threadQ *)malloc(sizeof(threadQ));
        child->head = NULL;
        thread->children = child;
        
	thread_id++;
        main_thread = curr_thread = thread;
        swapcontext((ucontext_t *) &main_context, &(curr_thread->context));

        /*enqueue(thread->tid);
        printf("\nppid=%d and tid=%d\n",thread->ppid, thread->tid);
        printf("\nNew thread created and is placed in ready queue\n");
        dumpQ();*/
	//printf("\nEnd of init\n");
}

Thread* SearchandRemove(struct threadQ *threadQ, Thread *ready)
{
        Thread *thread = NULL;
        struct queue *curr, *prev;
        curr = prev = threadQ->head;
        int i = 0;
        while(curr != NULL)
        {
                if(curr->ready->tid == ready->tid)
                {
                        thread = curr->ready;
                        if (curr == threadQ->head)
                        {
                                threadQ->head = threadQ->head->next;
                                curr->next = NULL;
                        }
                        else
                        {
                                prev->next = curr->next;
                                curr->next = NULL;
                        }
                        return thread;
                }
                i++;
                if (i > 1){
                        prev = curr;
                        curr = curr->next;
                }else
                        curr = curr->next;
        }
        return thread;
}

int checkparentandclear(Thread *parent)
{
        int gotFromBlocked = 0;
        Thread *toExit = curr_thread;
        Thread *temp;
        if(parent != NULL)
	{
                temp = SearchandRemove(parent->children, toExit);
                toExit->parent = NULL;
                if(ScanQ(blockedQ, parent) == 1)
		{
                        temp = SearchandRemove(blockedQ, parent);
                        if(temp->waitingFor != NULL)
			{
                                if(temp->waitingFor->tid == toExit->tid)
				{
                                        curr_thread = temp;
                                        curr_thread->waitingFor = NULL;
                                        gotFromBlocked = 1;
                                }
				else
				{
                                        enqueue(blockedQ, temp);
                                        dumpQ(blockedQ);
                                }
                        }
			else if(temp->wait_for_all == 1)
			{
                                SearchandRemove(temp->children, toExit);
                                if(temp->children->head == NULL)
				{
                                        temp->wait_for_all = 0;
                                        curr_thread = temp;
                                        gotFromBlocked = 1;
                                }
				else
				{
                                        enqueue(blockedQ, temp);
                                }
                        }
			else if(temp->children->head == NULL)
			{
                                if(temp->to_exit == 1)
				{
                                        free((curr_thread->context).uc_stack.ss_sp);
                                        free(curr_thread);
                                        toExit = NULL;
                                        curr_thread = temp;
                                        gotFromBlocked = checkparentandclear(curr_thread->parent);
                                }
				else
				{
                                        exit(1);
                                }
                        }
			else
			{
                                enqueue(blockedQ, temp);
                        }
                }
        }
        if (toExit != NULL)
	{
                free((toExit->context).uc_stack.ss_sp);
                free(toExit);
                toExit = NULL;
        }
        return gotFromBlocked;
}

int removeparentlink(struct threadQ *threadQ, Thread *ready)
{
        struct queue *temp = threadQ->head;

        while(temp != NULL)
	{
                if(temp->ready->parent != NULL)
		{
                        if(temp->ready->parent->tid == ready->tid)
			{
                                temp->ready->parent = NULL;
			}
                }
                temp = temp->next;
        }
}

// Extra Credit: initializes the threads package
// and converts the UNIX process context 
// into a MyThread context
int MyThreadInitExtra(void)
{
	// code here runs in thread context.
  	// it is the root or "oldest ancestor" thread
        Thread *thread = (Thread *)malloc(sizeof(Thread));

        getcontext(&thread->context);

        thread_id = 0;
        thread_count = 1;
        wait_count = 0;
        readyQ = (struct threadQ *) malloc(sizeof(threadQ));
        readyQ->head = NULL;

	thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
        thread->context.uc_stack.ss_size = STACK_SIZE;
        thread->context.uc_link = (ucontext_t *) &main_context;
        thread->context.uc_stack.ss_flags = 0;

	thread->tid = thread_id;
        thread->parent = NULL;
        thread->wait_for_all = 0;
        thread->to_exit = 0;

	curr_thread = thread;
        swapcontext(&(curr_thread->context), &(curr_thread->context));

	return 0;
}

/*Create a semaphore*/
MySemaphore MySemaphoreInit(int initialValue)
{
	if(initialValue < 0)
	{
                return NULL;
        }
        
	semQ = (semaphore *)malloc(sizeof(semaphore));
        semQ->semCount = initialValue;
        semQ->initValue = initialValue;
        threadQ *blockThread = (threadQ *)malloc(sizeof(threadQ));
        blockThread->head = NULL;
        semQ->semBlocked = (threadQ *) blockThread;
        threadQ *waitThread = (threadQ *)malloc(sizeof(threadQ));
        waitThread->head = NULL;
        semQ->waitingthreads = (threadQ *) waitThread;
        return (MySemaphore)(semQ);
}

/*Signal a semaphore*/
void MySemaphoreSignal(MySemaphore sem)
{
	semQ = (semaphore *) sem;
        if(semQ == NULL)
	{
                return;
        }
        if(semQ->semCount <= 0)
	{
                Thread *temp = dequeue(semQ->semBlocked);
                if(temp != NULL)
		{
                        enqueue(readyQ, temp);
                }
        }
        (semQ->semCount)++;
}

/* Wait on a semaphore*/
void MySemaphoreWait(MySemaphore sem)
{
	semQ = (semaphore *) sem;
        if(semQ == NULL)
	{
                return;
        }
        if(semQ->semCount > 0){
        }
	else
	{
                enqueue(semQ->semBlocked, curr_thread);
                Thread *temp = curr_thread;
                curr_thread = dequeue(readyQ);
                if (curr_thread == NULL)
		{
                        setcontext((ucontext_t *) &main_context);
                        return;
                }
                swapcontext(&(temp->context), &(curr_thread->context));
        }
        (semQ->semCount)--;
}

/* Destroy on a semaphore*/
int MySemaphoreDestroy(MySemaphore sem)
{
	semQ = (semaphore *) sem;
        if(semQ != NULL)
	{
                if(semQ->semCount > 0)
		{
                        free(semQ->semBlocked);
                        free(semQ->waitingthreads);
                        semQ = NULL;
                        return 0;
                }
		else
		{
                        return -1;
		}
        }
        return -1;
}
