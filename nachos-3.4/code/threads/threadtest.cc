// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include <string>
// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);   //线程创建执行，标识为1（调用thread.cc的构造方法）
    SimpleThread(0);
}

//---------------------------------------------------------------------
//Lab1Exercise3Thread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
Lab1Exercise3Thread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d (userID = %d, tid = %d) looped %d times\n", which,currentThread->getUserID(),\
                            currentThread->getTid(),num);
        currentThread->Yield();
    }
}


//----------------------------------------------------------------------
// Lab1Exercise3
// 	线程测试，循环创建进程（可以直到线程池溢出
//----------------------------------------------------------------------

void
Lab1Exercise3()
{
    DEBUG('t', "Entering Lab1Exercise3");

    Thread *t = NULL;
    for(int i = 0; i < MAX_THREAD_NUMBER - 120; i++)
    {
        Thread *t = new Thread("test thread");
        t->Fork(Lab1Exercise3Thread, t->getTid());
    }

    Lab1Exercise3Thread(0);
}

//----------------------------------------------------------------------
// Lab1Exercise4
// 测试实现ts功能：显示所有线程的信息和状态
//----------------------------------------------------------------------

void
Lab1Exercise4()
{
    DEBUG('t', "Entering Lab1Exercise3");

    Thread *t = NULL;
    for(int i = 0; i < MAX_THREAD_NUMBER - 120; i++)
    {
        Thread *t = new Thread("Test Thread");
        // t->Fork(Lab1Exercise3Thread, t->getTid());
    }

    scheduler->TS();
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	    ThreadTest1();
	    break;
    case 2:
        Lab1Exercise3();
        break;
    case 3:
        Lab1Exercise4();
    default:
	    printf("No test specified.\n");
	    break;
    }
}

