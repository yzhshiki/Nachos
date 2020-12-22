#include "synchconsole.h"

//----------------------------------------------------------------------
// Console RequestDone
// 	Console interrupt handler.  Need this to be a C routine, because
//	C++ can't handle pointers to member functions.
//----------------------------------------------------------------------

static void ConsoleReadAvail(int arg){
    Console *console = (Console *)arg;
    console->CheckCharAvail();
}

static void ConsoleWriteDone(int arg){
    Console *console = (Console *)arg;
    console->WriteDone();
}


SynchConsole::SynchConsole(char *readFile, char *writeFile){
    readSemaphore = new Semaphore("readSemaphore", 0);
    writeSemaphore = new Semaphore("writeSemaphore", 0);
    lock = new Lock("synch console lock");
    console = new Console(readFile, writeFile, ConsoleReadAvail, ConsoleWriteDone, (int)this);
}

SynchConsole::~SynchConsole(){
    delete console;
    delete lock;
    delete readSemaphore;
    delete writeSemaphore;
}


//----------------------------------------------------------------------
// Console::PutChar()
// 	Write a character to the simulated display, schedule an interrupt 
//	to occur in the future, and return.
//----------------------------------------------------------------------
void SynchConsole::PutChar(char ch){
    lock->Acquire();
    console->PutChar(ch);
    writeSemaphore->P();
    lock->Release();
}

//注意这里调用信号量的时间在Getchar之前
char SynchConsole::GetChar(){
    lock->Acquire();
    readSemaphore->P();
    char ch = console->GetChar();
    lock->Release();
    return ch;
}

void SynchConsole::CheckCharAvail(){
    readSemaphore->V();
}

void SynchConsole::WriteDone(){
    writeSemaphore->V();
}