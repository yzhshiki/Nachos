// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "machine.h"
#include "noff.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    //如果是异常是系统调用类型 且是halt调用
    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
    else if(which == SyscallException && type == SC_Exit){
        printf("Exiting userprog of thread: %s\n", currentThread->getName());
        machine->freeMem();
        currentThread->Finish();
        if(currentThread->space!=NULL){
            delete currentThread->space;
        }
    }
    else if(which == TLBMissException) {
        int BadVAddr = machine->ReadRegister(BadVAddrReg);
        unsigned int vpn = BadVAddr / PageSize;
        if(machine->tlb != NULL){
            DEBUG('a', "=> TLB miss (no TLB entry)\n");
            if(machine->pageTable[vpn].valid == true)
                machine->tlbReplace(BadVAddr);
            else{
                machine->tlbReplace(BadVAddr);
                machine->RaiseException(PageFaultException, BadVAddr);
            }
        }
    }
    else if(which == PageFaultException){
        //求出缺页的虚拟页号
        int BadVAddr = machine->ReadRegister(BadVAddrReg);
        unsigned int vpn = BadVAddr / PageSize;
        int PhysPage = machine->pageTable[vpn].physicalPage;
        
        //获得一个物理页号
        int ppn = machine->allocMem();
        if(ppn = -1){
            ppn = machine->pageReplace();
        }
        //修改页表,将对应物理页内容写到其对应原本线程的交换区（虚拟磁盘）
        machine->pageTable[vpn].physicalPage = ppn;
        Thread *former = machine->MemToThread[ppn];
        if(former != NULL){
            memcpy(machine->disk[former->UserProgPosInDisk + ppn * PageSize], machine->mainMemory + ppn * PageSize, PageSize);
        }
        //将文件对应页读入物理内存
        //如果虚拟磁盘里有
        if((machine->disk[former->UserProgPosInDisk + ppn * PageSize]) != 0){
            memcpy(machine->mainMemory + ppn * PageSize, machine->disk[former->UserProgPosInDisk + ppn * PageSize], PageSize);
            printf("Thread: %s\tRead exspace to mainmemory\n", currentThread->getName());
        }
        else    //  虚拟磁盘里没有，则先读到虚拟磁盘，再读到内存
        {
            OpenFile *executable = fileSystem->Open(currentThread->filename);
            executable->ReadAt(&(machine->disk[former->UserProgPosInDisk + ppn * PageSize]), PageSize, vpn * PageSize + sizeof(NoffHeader));
            memcpy(machine->mainMemory + ppn * PageSize, machine->disk[former->UserProgPosInDisk + ppn * PageSize], PageSize);
            printf("Thread: %s\tRead file to exspace to mainmemory\n", currentThread->getName());
        }

        /*下面是使用交换区但不实现页面替换算法、不支持单个程序大小超过的物理内存大小的版本
        if(machine->pageTable[vpn].dirty){
            memcpy(machine->pageTable[vpn].BelongToThread->ExSpace + PhysPage * PageSize, machine->mainMemory + PhysPage * PageSize, PageSize);
        }
        //将文件对应页读入物理内存
        printf("PyhsPage: %d\n", PhysPage);
        if((currentThread->ExSpace[PhysPage * PageSize]) != 0){
            memcpy(machine->mainMemory + PhysPage * PageSize, currentThread->ExSpace + PhysPage * PageSize, PageSize);
            printf("Thread: %s\tRead exspace to mainmemory\n", currentThread->getName());
        }
        else
        {
            OpenFile *executable = fileSystem->Open(currentThread->filename);
            executable->ReadAt(&(currentThread->ExSpace[PhysPage * PageSize]), PageSize, vpn * PageSize + sizeof(NoffHeader));
            memcpy(machine->mainMemory + PhysPage * PageSize, currentThread->ExSpace + PhysPage * PageSize, PageSize);
            printf("Thread: %s\tRead file to exspace to mainmemory\n", currentThread->getName());
        }
        */
        machine->MemToThread[ppn] = currentThread;
        machine->pageTable[vpn].dirty = false;
        machine->pageTable[vpn].valid = true;
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
