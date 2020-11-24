// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) {
    name = debugName;
    semaphore = new Semaphore("Mutex",1);
}
Lock::~Lock() {}

//获得锁，若被其他进程占用则sleep
void Lock::Acquire() {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    semaphore->P();
    holdingThread = currentThread;
    // printf("Thread %s acquired the %s\n", currentThread->getName(), name);
    (void) interrupt->SetLevel(oldLevel);
}

//释放锁，若有进程在等待锁则唤醒他
void Lock::Release() {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    // printf("%s holding the %s\n", holdingThread->getName(), name);
    // ASSERT(holdingThread == currentThread);
    // if(holdingThread == currentThread){
    semaphore->V();
    holdingThread = NULL;
    // printf("Thread %s released the %s\n", currentThread->getName(), name);
    (void) interrupt->SetLevel(oldLevel);
    // }
    // else{
    //     printf("%s does not belongs to %s, yield\n", name, currentThread->getName());
    //     currentThread->Yield();
    //     (void) interrupt->SetLevel(oldLevel);
    // }
    
}

bool Lock::isHeldByCurrentThread(){
    return holdingThread == currentThread;
}

Condition::Condition(char* debugName) {
    waitList = new List;
}
Condition::~Condition() {
    delete waitList;
}

//释放锁，睡眠并进入等待队列，被唤醒后取得锁
void Condition::Wait(Lock* conditionLock) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    conditionLock->Release();
    waitList->Append((Thread *)currentThread);

    printf("Thread %s sleeping\n", currentThread->getName());
    currentThread->Sleep();
    printf("Thread %s waking up\n", currentThread->getName());
    conditionLock->Acquire();
    (void) interrupt->SetLevel(oldLevel);
}

//从等待队列唤醒一个线程
void Condition::Signal(Lock* conditionLock) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(conditionLock->isHeldByCurrentThread());
    if(conditionLock->isHeldByCurrentThread()){
        Thread *thread = (Thread *)waitList->Remove();
        if(thread){
            scheduler->ReadyToRun(thread);
            printf("Thread %s signaled %s\n", currentThread->getName(), thread->getName());
        }
            
    }
    (void) interrupt->SetLevel(oldLevel);
}

//唤醒所有等待队列中的线程
void Condition::Broadcast(Lock* conditionLock) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    while(!waitList->IsEmpty()){
        Signal(conditionLock);
    }
    (void) interrupt->SetLevel(oldLevel);
}
