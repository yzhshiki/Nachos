// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_THREAD_NUMBER 128   //定义线程池容量

#include "copyright.h"
#include "list.h"
#include "thread.h"
#include <vector>
#include <queue>

// 选择调度算法
enum ThreadSchedulingMethod{
    FIFO,
    PRIORIY
};

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class Scheduler {
  public:
    Scheduler();			// Initialize list of ready threads 
    ~Scheduler();			// De-allocate ready list

    void ReadyToRun(Thread* thread);	// Thread can be dispatched.
    Thread* FindNextToRun();		// Dequeue first thread on the ready 
					// list, if any, and return thread.
    void Run(Thread* nextThread);	// Cause nextThread to start running
    void Print();			// Print contents of ready list
    
  private:
    List *readyList;  		// queue of threads that are ready to run,
				// but not running
  
  
  private:
    std::vector<Thread *> threadPool;   //线程池
    std::queue<int> tids;   //可用的tid队列
    int tid_Ticks[MAX_THREAD_NUMBER]; //一个tid对应一个ticks计数
    ThreadSchedulingMethod scheduleMethod;
  
  public:
    int acquireTid(Thread *t);  //用于给线程分配tid
    void releaseTid(Thread *t); //给线程释放tid
    void TS();  //打印所有进程信息
    ThreadSchedulingMethod getScheduleMethod(){ return scheduleMethod; }
    void preemptive_sched(Thread *t);
    void initTicks(Thread *t){ tid_Ticks[t->getTid()] = 0; }
    int checkTicks(Thread *t){ return tid_Ticks[t->getTid()]; }
    void addTicks(Thread *t, int ticks){ tid_Ticks[t->getTid()] += ticks; } //onetick时调用
    void resetTicks(Thread *t){ tid_Ticks[t->getTid()] = 0; }
    Thread* getThreadByTid(int tid);

};

#endif // SCHEDULER_H
