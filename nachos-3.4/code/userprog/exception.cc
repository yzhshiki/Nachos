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
                machine->RaiseException(PageFaultException, BadVAddr);
                machine->tlbReplace(BadVAddr);
            }
        }
    }
    else if(which == PageFaultException){
        
        //求出缺页的虚拟页号
        int BadVAddr = machine->ReadRegister(BadVAddrReg);
        unsigned int vpn = BadVAddr / PageSize;
        //获得一个物理页号，-1则需要执行页面替换
        int ppn = machine->allocMem();
        if(ppn == -1){
            //得到被替换页面属于哪个线程、对应vpn、对应磁盘位置
            ppn = machine->pageReplace();
            Thread* FormThread = machine->ppnToThread[ppn];
            int FormVpn = machine->ppnTovpn[ppn];   //不需要考虑FormVpn为-1，因为肯定已经建立映射
            if(FormThread->vpnTodpn[FormVpn] == -1){
                FormThread->vpnTodpn[FormVpn] = machine->allocDisk();
            }
            int FormDpn = FormThread->vpnTodpn[FormVpn];
            //将物理页内容写回磁盘
            memcpy(machine->disk + FormDpn * PageSize, machine->mainMemory + ppn * PageSize, PageSize);
            FormThread->setInvalid(FormVpn);
            printf("Write Thread: %s\tvpn %d\tppn %d back to disk\n", FormThread->getName(), FormVpn, ppn);
        }
        
        //将文件对应页读入物理内存
        //如果虚拟磁盘里有，则从虚拟磁盘读到内存
        int dpn = currentThread->vpnTodpn[vpn];
        if(dpn != -1){
            memcpy(machine->mainMemory + ppn * PageSize, machine->disk + dpn * PageSize, PageSize);
            printf("Thread: %s\tRead Page %d form Disk to mainMemory\n", currentThread->getName(), ppn);
        }
        else    //  虚拟磁盘里没有，则从文件读到内存
        {
            OpenFile *executable = fileSystem->Open(currentThread->filename);
            executable->ReadAt(&(machine->mainMemory[ppn * PageSize]), PageSize, vpn * PageSize + sizeof(NoffHeader));
            printf("Thread: %s\tRead Page %d form FILE to mainMemory\n", currentThread->getName(), ppn);
        }

        //修改页表与倒排页表
        machine->pageTable[vpn].valid = true;
        machine->pageTable[vpn].physicalPage = ppn;
        machine->ppnToThread[ppn] = currentThread;
        machine->ppnTovpn[ppn] = vpn;
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
