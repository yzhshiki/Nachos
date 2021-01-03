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
#include "filesys.h"
#include "addrspace.h"
extern void StartProcess(char *fileName);
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

char* translateAddr(int vaddr) {
	int paddr = 0;
    machine->Translate(vaddr, &paddr, 4, FALSE);
    return machine->mainMemory + paddr;
}

void exec_func(char* fileName){
    printf("Entering exec_func.\n");

    printf("Try to open file %s.\n", fileName);
    OpenFile *executable = fileSystem->Open(fileName);
    AddrSpace *space;
    if(executable == NULL){
        printf("Unable to open file %s\n", fileName);
        return;
    }
    space = new AddrSpace(executable);
    currentThread->space = space;
    delete executable;

    space->InitRegisters();
    space->RestoreState();
    machine->Run();
}

void StartForkProcess(int func) {

	currentThread->space->InitRegisters(); // set the initial register values
	currentThread->space->RestoreState();  // load page table register
	currentThread->space->setPC(func);
	machine->Run(); // jump to the user progam
	ASSERT(FALSE);  // machine->Run never returns;
}

void SyscallHandler(int type){
    switch (type)
    {
    case SC_Halt:{
        DEBUG('a', "Shutdown, initiated by user program.\n");
    	interrupt->Halt();
        break;
    }
    case SC_Create:{
        printf("SC_Create called\n");
        int address = machine->ReadRegister(4); //从r4读到文件名变量的地址，然后读出文件名
        printf("Create address: %d\n", address);
        int len = 0 ,value = 1;
        while(value != '\0' ){ 
            machine->ReadMem(address++,1,&value);
			len++;
        }
        char* fileName = new char[len];
        address -= len;
		for(int i = 0; i < len; i++){
			machine->ReadMem(address+i,1,&value);
			fileName[i] = (char)value;
		}
        printf("Creating File: %s\n", fileName);
        fileSystem->Create(fileName, 0);
        machine->AdvancePC();   //一定要pc前进，不然会循环此syscall
        break;
    }
    case SC_Open:{
        printf("SC_Open called\n");
        int address = machine->ReadRegister(4);
        int len = 0 ,value = 1;
        while(value != '\0' ){ 
            machine->ReadMem(address++,1,&value);
			len++;
        }
        char* fileName = new char[len];
        address -= len;
		for(int i = 0; i < len; i++){
			machine->ReadMem(address+i,1,&value);
			fileName[i] = (char)value;
		}
        OpenFile *file = fileSystem->Open(fileName);
        machine->WriteRegister(2, int(file));   //将文件描述符写回r2
        machine->AdvancePC();
        break;
    }
    case SC_Close:{
        printf("SC_Close called\n");
        int fileId = machine->ReadRegister(4);
        OpenFile *file = (OpenFile*)fileId;
        delete file;
        machine->AdvancePC();
        break;
    }
    case SC_Read:{
        printf("SC_Read called\n");
        int address = machine->ReadRegister(4);     //要读的数据在内存的地址
        int len = machine->ReadRegister(5);         //数据长度
        int fd = machine->ReadRegister(6);          //文件描述符
        char *data = new char[len];                 //缓冲，先从文件取得数据然后写入内存
        int result;
        if(fd!=0){                                  //从文件到缓冲
            OpenFile *file = (OpenFile*) fd;
            result = file->Read(data, len);
        }
        else{
            for(int i = 0; i < len; i ++)
                data[i] = getchar();
            result = len;
        }
        for(int i = 0; i < len; i++)                //从缓冲到内存
            machine->WriteMem(address+i, 1, int(data[i]));
        machine->WriteRegister(2, result);
        machine->AdvancePC();
        break;
    }
    case SC_Write:{
        printf("SC_Write called\n");
        int address = machine->ReadRegister(4);
        int len = machine->ReadRegister(5);
        int fd = machine->ReadRegister(6);
        char* data = new char[len];
        int value;
        for(int i = 0; i < len; i ++){              //从内存到缓冲
            machine->ReadMem(address+i, 1, &value);
            data[i] = char(value);
        }
        if(fd!=1){                                  //从缓冲到文件
            OpenFile* file = (OpenFile*) fd;
            file->Write(data, len);
        }
        else{
            for(int i = 0; i < len; i ++)
                putchar(data[i]);
            printf("\n");
        }
        machine->AdvancePC();
        break;
    }
    case SC_Exec:{
        printf("SC_Exec called\n");
        int address = machine->ReadRegister(4);
        printf("Exec address: %d\n", address);
        int len = 0 ,value = 1;
        while(value != '\0' ){ 
            machine->ReadMem(address++,1,&value);
			len++;
        }
        char* fileName = new char[len];
        address -= len;
		for(int i = 0; i < len; i++){
			machine->ReadMem(address+i,1,&value);
			fileName[i] = (char)value;
		}
        printf("Exec File: %s\n", fileName);
        Thread *newThread = new Thread("new Thread");
        newThread->Fork(exec_func, (void *)fileName);
        machine->WriteRegister(2, newThread->getTid());
        machine->AdvancePC();
        break;
    }
    case SC_Exit:{
        IntStatus oldLevel = interrupt->SetLevel(IntOff);
		while (!currentThread->waitingList->IsEmpty()) {
			Thread *t = (Thread *) currentThread->waitingList->Remove();
			if (t != NULL) {
				t->exitCode = machine->ReadRegister(4);
				scheduler->ReadyToRun(t);
			}
		}
		interrupt->SetLevel(oldLevel);
        printf("Exiting userprog of thread: %s\n", currentThread->getName());
        // machine->freeMem();
		currentThread->Finish();
        if(currentThread->space!=NULL){
            delete currentThread->space;
        }
        machine->AdvancePC();
		break;
    }
    case SC_Join:{
        printf("SC_Join called\n");
        int tid = machine->ReadRegister(4);
        Thread *waitFor = scheduler->getThreadByTid(tid);
        waitFor->waitingList->Append((void *) currentThread);
        printf("thread %d join thread %d\n", currentThread->getTid(),
			waitFor->getTid());
        IntStatus oldLevel = interrupt->SetLevel(IntOff);
        currentThread->Sleep();
        interrupt->SetLevel(oldLevel);
        machine->WriteRegister(2, currentThread->exitCode);
        machine->AdvancePC();
        break;
    }
    case SC_Yield:{
        printf("SC_Yield Called for thread %s\n",currentThread->getName());
		currentThread->Yield();
        machine->AdvancePC();
        break;
    }
    case SC_Fork: {
        printf("SC_Fork Called for thread %s\n",currentThread->getName());
        int func = machine->ReadRegister(4);
        Thread* fork = new Thread("fork");
        fork->StackAllocate(StartForkProcess, func);
        AddrSpace* newspace = new AddrSpace(currentThread);
        fork->space = newspace;
        printf("forking func addr %x in thread : %d\n", func, currentThread->getTid());
        scheduler->ReadyToRun(fork);
        machine->AdvancePC();
        break;
    }
    default:
        break;
    }
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    //如果是异常是系统调用类型 且是halt调用
    
    if(which == SyscallException){
        SyscallHandler(type);
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
            // printf("Thread: %s\tRead Page %d from FILE to mainMemory\n", currentThread->getName(), ppn);
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
