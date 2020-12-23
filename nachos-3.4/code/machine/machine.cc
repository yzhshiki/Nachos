// machine.cc 
//	Routines for simulating the execution of user programs.
//
//  DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include <stdlib.h>
#include <time.h>
#include "copyright.h"
#include "machine.h"
#include "system.h"

// Textual names of the exceptions that can be generated by user program
// execution, for debugging.
static char* exceptionNames[] = { "no exception", "syscall", 
				"page fault/no TLB entry", "page read only",
				"bus error", "address error", "overflow",
				"illegal instruction" };

//----------------------------------------------------------------------
// CheckEndian
// 	Check to be sure that the host really uses the format it says it 
//	does, for storing the bytes of an integer.  Stop on error.
//----------------------------------------------------------------------

static
void CheckEndian()
{
    union checkit {
        char charword[4];
        unsigned int intword;
    } check;

    check.charword[0] = 1;
    check.charword[1] = 2;
    check.charword[2] = 3;
    check.charword[3] = 4;

#ifdef HOST_IS_BIG_ENDIAN
    ASSERT (check.intword == 0x01020304);
#else
    ASSERT (check.intword == 0x04030201);
#endif
}

//----------------------------------------------------------------------
// Machine::Machine
// 	Initialize the simulation of user program execution.
//
//	"debug" -- if TRUE, drop into the debugger after each user instruction
//		is executed.
//----------------------------------------------------------------------

Machine::Machine(bool debug)
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
        registers[i] = 0;
    mainMemory = new char[MemorySize];
    for (i = 0; i < MemorySize; i++)
      	mainMemory[i] = 0;

    disk = new char[DiskSize];
    for(i = 0; i < DiskSize; i++)
        disk[i] = 0;
    memBitMap = new BitMap(NumPhysPages);
    diskBitMap = new BitMap(DiskPages);
    
    for(i = 0; i < NumPhysPages; i++)
      	ppnToThread[i] = NULL;
    ppnTovpn = new int[NumPhysPages];
    for(i = 0; i < NumPhysPages; i++)
        ppnTovpn[i] = -1;

#ifdef USE_TLB
    tlbSize = TLBSize;
    tlb = new TranslationEntry[TLBSize];
    for (i = 0; i < TLBSize; i++){
        tlb[i].valid = FALSE;
        tlb[i].lastUsedTime = 0;
    }
	
    pageTable = NULL;
    tlbAlgo = LRU;
    
#else	// use linear page table
    tlb = NULL;
    pageTable = NULL;
#endif

    singleStep = debug;
    CheckEndian();
}

//----------------------------------------------------------------------
// Machine::~Machine
// 	De-allocate the data structures used to simulate user program execution.
//----------------------------------------------------------------------

Machine::~Machine()
{
    delete [] mainMemory;
    // printf("Thread: %s\ttlbtimes: %d\ttlbhits: %d\taccuracy: %lf\n", 
    //         currentThread->getName(), currentThread->getTlbTimes(), currentThread->getTlbHits(), double(currentThread->getTlbHits())/double(currentThread->getTlbTimes()));
    if (tlb != NULL)
        delete [] tlb;
    delete memBitMap;
}

//----------------------------------------------------------------------
// Machine::RaiseException
// 	Transfer control to the Nachos kernel from user mode, because
//	the user program either invoked a system call, or some exception
//	occured (such as the address translation failed).
//
//	"which" -- the cause of the kernel trap
//	"badVaddr" -- the virtual address causing the trap, if appropriate
// 抛出异常，在oneinstruction里常用
//----------------------------------------------------------------------

void
Machine::RaiseException(ExceptionType which, int badVAddr)
{
    DEBUG('m', "Exception: %s\n", exceptionNames[which]);
    
//  ASSERT(interrupt->getStatus() == UserMode);
    registers[BadVAddrReg] = badVAddr;
    DelayedLoad(0, 0);			// finish anything in progress
    interrupt->setStatus(SystemMode);
    ExceptionHandler(which);		// interrupts are enabled at this point
    interrupt->setStatus(UserMode);
}

//----------------------------------------------------------------------
// Machine::Debugger
// 	Primitive debugger for user programs.  Note that we can't use
//	gdb to debug user programs, since gdb doesn't run on top of Nachos.
//	It could, but you'd have to implement *a lot* more system calls
//	to get it to work!
//
//	So just allow single-stepping, and printing the contents of memory.
//----------------------------------------------------------------------

void Machine::Debugger()
{
    char *buf = new char[80];
    int num;

    interrupt->DumpState();
    DumpState();
    printf("%d> ", stats->totalTicks);
    fflush(stdout);
    fgets(buf, 80, stdin);
    if (sscanf(buf, "%d", &num) == 1)
	runUntilTime = num;
    else {
	runUntilTime = 0;
	switch (*buf) {
	  case '\n':
	    break;
	    
	  case 'c':
	    singleStep = FALSE;
	    break;
	    
	  case '?':
	    printf("Machine commands:\n");
	    printf("    <return>  execute one instruction\n");
	    printf("    <number>  run until the given timer tick\n");
	    printf("    c         run until completion\n");
	    printf("    ?         print help message\n");
	    break;
	}
    }
    delete [] buf;
}
 
//----------------------------------------------------------------------
// Machine::DumpState
// 	Print the user program's CPU state.  We might print the contents
//	of memory, but that seemed like overkill.
//----------------------------------------------------------------------

void
Machine::DumpState()
{
    int i;
    
    printf("Machine registers:\n");
    for (i = 0; i < NumGPRegs; i++)
	switch (i) {
	  case StackReg:
	    printf("\tSP(%d):\t0x%x%s", i, registers[i],
		   ((i % 4) == 3) ? "\n" : "");
	    break;
	    
	  case RetAddrReg:
	    printf("\tRA(%d):\t0x%x%s", i, registers[i],
		   ((i % 4) == 3) ? "\n" : "");
	    break;
	  
	  default:
	    printf("\t%d:\t0x%x%s", i, registers[i],
		   ((i % 4) == 3) ? "\n" : "");
	    break;
	}
    
    printf("\tHi:\t0x%x", registers[HiReg]);
    printf("\tLo:\t0x%x\n", registers[LoReg]);
    printf("\tPC:\t0x%x", registers[PCReg]);
    printf("\tNextPC:\t0x%x", registers[NextPCReg]);
    printf("\tPrevPC:\t0x%x\n", registers[PrevPCReg]);
    printf("\tLoad:\t0x%x", registers[LoadReg]);
    printf("\tLoadV:\t0x%x\n", registers[LoadValueReg]);
    printf("\n");
}

//----------------------------------------------------------------------
// Machine::ReadRegister/WriteRegister
//   	Fetch or write the contents of a user program register.
//----------------------------------------------------------------------

int Machine::ReadRegister(int num)
    {
	ASSERT((num >= 0) && (num < NumTotalRegs));
	return registers[num];
    }

void Machine::WriteRegister(int num, int value)
    {
	ASSERT((num >= 0) && (num < NumTotalRegs));
	// DEBUG('m', "WriteRegister %d, value %d\n", num, value);
	registers[num] = value;
    }

void Machine::tlbReplace(int BadVAddr){
    switch (tlbAlgo)
    {
    case TLBFIFO:
        tlbReplaceTLBFIFO(BadVAddr);
        break;
    case LRU:
        tlbReplaceLRU(BadVAddr);
        break;
    default:
        break;
    }
}

void Machine::tlbReplaceTLBFIFO(int BadVAddr){
    unsigned int vpn = BadVAddr / PageSize;
    int position = -1;

    for(int i = 0; i < TLBSize; i ++){
        if(tlb[i].valid == false){
            position = i;
            break;
        }
    }
    if(position == -1){
        for(int i = 0; i < TLBSize - 1; i ++)
            tlb[i] = tlb[i + 1];
        position = TLBSize - 1;    
    }
    tlb[position].virtualPage = pageTable[vpn].virtualPage;
    tlb[position].physicalPage = pageTable[vpn].physicalPage;
    tlb[position].valid = true;
    tlb[position].use = false;
    tlb[position].dirty = false;
}

void Machine::tlbReplaceLRU(int BadVAddr){
    unsigned int vpn = BadVAddr / PageSize;
    int position = -1;

    for(int i = 0; i < TLBSize; i ++){
        if(tlb[i].valid == false){
            position = i;
            break;
        }
    }

    if(position == -1){
        int maxLUT = 0;
        for(int i = 0; i < TLBSize; i ++){
            if(tlb[i].lastUsedTime >= maxLUT){      //这里要是大于等于，不能是大于
                position = i;
                maxLUT = tlb[i].lastUsedTime;
            }
            tlb[i].lastUsedTime++;
        }
    }
    tlb[position].virtualPage = pageTable[vpn].virtualPage;
    tlb[position].physicalPage = pageTable[vpn].physicalPage;
    tlb[position].valid = true;
    tlb[position].use = false;
    tlb[position].dirty = false;
    tlb[position].lastUsedTime = 0;
}

void Machine::freeMem(){
    for(int i = 0; i < NumPhysPages; i++){
        if(pageTable[i].valid && memBitMap->Test(i))
            memBitMap->Clear(i);
    }
}

int Machine::pageReplace(){
    srand((unsigned)time(NULL));
    return rand() % NumPhysPages;
}