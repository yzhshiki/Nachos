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

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//---------------------------------------------------------------------
// Lab1Exercise3Thread 打印线程userID tid
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
    
    for (num = 0; num < 2; num++) {
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
    for(int i = 0; i < 4; i++)
    {
        Thread *t = new Thread("test thread");
        t->Fork(Lab1Exercise3Thread, t->getTid());
    }

    Lab1Exercise3Thread(0);
}

//----------------------------------------------------------------------
// Lab1Exercise3_2
// 测试实现ts功能：显示所有线程的信息和状态
//----------------------------------------------------------------------

void
Lab1Exercise3_2()
{
    DEBUG('t', "Entering Lab1Exercise3_2");

    Thread *t = NULL;
    for(int i = 0; i < 4; i++)
        Thread *t = new Thread("Test Thread");
    scheduler->TS();
}

//----------------------------------------------------------------------
// Lab1Exercise5
// 测试实现抢占式优先级调度
//----------------------------------------------------------------------
void 
Lab1Exercise5()
{
    DEBUG('t', "Entering Lab1Exercise5");
    Thread* t3 = new Thread("thread 3", 3);
    t3->Fork(Lab1Exercise3Thread, 3);   //第二个参数是传给Lab1Exercise3Thread的，要与线程名一致，而不是tid

    Thread* t2 = new Thread("thread 2", 2);
    t2->Fork(Lab1Exercise3Thread, 2);

    Thread* t1 = new Thread("thread 1", 1);
    t1->Fork(Lab1Exercise3Thread, 1);

    // printf("thread main finished threadtest\n");
    // Lab1Exercise3Thread(0);
}

//----------------------------------------------------------------------
// Lab1Challenge1
// 测试实现时间片轮转调度
//----------------------------------------------------------------------
void Lab1Challenge1Thread(int which)
{
    for(int i = 0; i < 5; i++){
        printf("*** %s looped %d times with ticks: %d\n", currentThread->getName(), i + 1, 
                            scheduler->checkTicks(currentThread));
        interrupt->OneTick();
    }
}

void Lab1Challenge1()
{
    DEBUG('t', "Entering Lab1Challenge1");
    Thread* t3 = new Thread("Thread 3");
    t3->Fork(Lab1Challenge1Thread, 3);

    Thread* t2 = new Thread("Thread 2");
    t2->Fork(Lab1Challenge1Thread, 2);

    Thread* t1 = new Thread("Thread 1");
    t1->Fork(Lab1Challenge1Thread, 1);
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
        Lab1Exercise3_2();
        break;
    case 4:
        Lab1Exercise5();
        break;
    case 5:
        Lab1Challenge1();
        break;
    default:
	    printf("No test specified.\n");
	    break;
    }
}

