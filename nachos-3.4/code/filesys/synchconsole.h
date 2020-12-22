#include "copyright.h"
#include "utility.h"
#include "console.h"
#include "synch.h"

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

class SynchConsole {
  public:
    SynchConsole(char *readFile, char *writeFile);
				// initialize the hardware console device
    ~SynchConsole();			// clean up console emulation

// external interface -- Nachos kernel code can call these
    void PutChar(char ch);	// Write "ch" to the console display, 
				// and return immediately.  "writeHandler" 
				// is called when the I/O completes. 

    char GetChar();	   	// Poll the console input.  If a char is 
				// available, return it.  Otherwise, return EOF.
    				// "readHandler" is called whenever there is 
				// a char to be gotten

// internal emulation routines -- DO NOT call these. 
    void WriteDone();	 	// internal routines to signal I/O completion
    void CheckCharAvail();

  private:
    Console *console;
    Lock *lock;
    Semaphore *readSemaphore;
    Semaphore *writeSemaphore;
};

#endif