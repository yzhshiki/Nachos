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
#include "synch.h"
#include <string>
// testnum is set in main.cc
int testnum = 1;
int readerCount = 0;
Lock *lock; //普通互斥锁，
Lock *rclock; //用于生产消费问题或者读写问题的读者内部
Lock *rwlock; //读写锁，用于读者与写者之间
Condition *condition;
Condition *produceCondition;
Condition *consumeCondition;
int product = 0;
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

//---------------------------
//用锁和条件变量实现生产者消费者问题
//命令行运行 ./nachos -rs -q 6
//---------------------------
void L3E4_Con_condition(int which){
    for(int i = 0; i < 6; i ++){
        lock->Acquire();
        //要用while，而不是if，因为线程从wait中被唤醒后，再上cpu后应该重新判断资源情况
        while(product == 0){
            printf("There is no more product!\n");
            consumeCondition->Wait(lock);
        }
        product--;
        printf("Thread %s consumed a product, %d products now\n", currentThread->getName(), product);
        produceCondition->Signal(lock); //只要消费一个就可以唤醒生产者了。
        lock->Release();
    }
}

void L3E4_Pro_condition(int which){
    for(int i = 0; i < 10; i ++){
        lock->Acquire();
        while(product == 3){
            printf("There are too much products!\n");
            produceCondition->Wait(lock);
        }
        product++;
        printf("Thread %s produced a product, %d products now\n", currentThread->getName(), product);
        consumeCondition->Signal(lock); //只要生产一个就可以唤醒消费者了。
        lock->Release();
    }
}

void Lab3Exercise4()
{
    DEBUG('t', "Entering Lab3Exeercise4");
    lock = new Lock("Pro_cons_Lock");
    produceCondition = new Condition("ProduceCondition");
    consumeCondition = new Condition("ConsumeCondition");

    Thread* consumer1= new Thread("Consumer1");
    consumer1->Fork(L3E4_Con_condition, consumer1->getTid());

    Thread* consumer2= new Thread("Consumer2");
    consumer2->Fork(L3E4_Con_condition, consumer2->getTid());

    Thread* producer= new Thread("Producer");
    producer->Fork(L3E4_Pro_condition, producer->getTid());

}

//---------------------------
//用锁实现读者写者问题
//命令行运行 ./nachos -rs -q 7
//---------------------------

void L3C2_read(int which){
    for(int i = 0; i < 6; i++){
        rclock->Acquire();
        readerCount++;
        if(readerCount == 1)
            rwlock->Acquire();
        rclock->Release();

        printf("%s is reading with %d readers\n", currentThread->getName(), readerCount - 1);
        interrupt->OneTick();   //这里使时钟前进，制造多读者共存的机会
        rclock->Acquire();
        readerCount--;
        if(readerCount == 0)
            rwlock->Release();
        rclock->Release();
    }
}

void L3C2_write(int which){
    for(int i = 0; i < 10; i++){
        rwlock->Acquire();
        printf("%s is writing\n", currentThread->getName());
        rwlock->Release();
    }
}

void Lab3Challenge2()
{
    DEBUG('t', "Entering Lab3Challenge2");
    rclock = new Lock("ReaderCountLock");
    rwlock = new Lock("ReaderWriterLock");
    Thread* reader1= new Thread("Reader1");
    reader1->Fork(L3C2_read, reader1->getTid());

    Thread* reader2= new Thread("Reader2");
    reader2->Fork(L3C2_read, reader2->getTid());

    Thread* reader3= new Thread("Reader3");
    reader3->Fork(L3C2_read, reader3->getTid());

    Thread* writer= new Thread("Writer");
    writer->Fork(L3C2_write, writer->getTid());

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
    case 6:
        Lab3Exercise4();
        break;
    case 7:
        Lab3Challenge2();
        break;
    default:
	    printf("No test specified.\n");
	    break;
    }
}

