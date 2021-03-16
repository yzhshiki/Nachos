# Lab6-shell实现实验报告

【实习内容】

​     设计实现一个用户程序shell，通过./nachos -x shell进入用户交互界面中。在该界面中可以查询支持的功能、可以创建删除文件或目录、可以执行另一个用户程序并输出运行结果，类似Linux上跑的bash。

​     你实现的越完善，碰到的问题越多，学到的也会越多。

​     本实验所修改的代码包括内核和用户程序两部分。



nachos原始代码已经给出了shell.c用户程序的代码，给出了基本命令读入功能：

```c
#include "syscall.h"

int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60];
    int i;

    prompt[0] = '-';
    prompt[1] = '-';

    while( 1 )
    {
	Write(prompt, 2, output);

	i = 0;
	
	do {
	
	    Read(&buffer[i], 1, input); 

	} while( buffer[i++] != '\n' );

	buffer[--i] = '\0';

    
	if( buffer[0]=='x' && buffer[1]==' ' ) {
		newProc = Exec(buffer);
		Join(newProc);
	}
    }
}


```

给此文件增加一些特定命令的解析：

```c
//shell.c
......
else if( buffer[0]=='p' && buffer[1]=='w' && buffer[2]=='d' && buffer[3]=='\0'){
        Pwd();
    }
    else if( buffer[0]=='l' && buffer[1]=='s' && buffer[2]=='\0'){
        Ls();
    }
    else if( buffer[0]=='c' && buffer[1]=='d' && buffer[2]==' '){
        Cd(buffer+3);
    }
    else if( buffer[0]=='n' && buffer[1]=='f' && buffer[2]==' '){
        Create(buffer+3);
    }
    else if( buffer[0]=='d' && buffer[1]=='f' && buffer[2]==' '){
        Remove(buffer+3);
    }
    else if( buffer[0]=='n' && buffer[1]=='d' && buffer[2]==' '){
        CreateDir(buffer+3);
    }
    else if( buffer[0]=='d' && buffer[1]=='d' && buffer[2]==' '){
        RemoveDir(buffer+3);
    }
    else if( buffer[0]=='h' && buffer[1]=='\0'){
        Help();
    }
    else if( buffer[0]=='q' && buffer[1]=='\0'){
        Exit(0);
    }
```

为了完成这些命令的处理，需要增加相应系统调用的声明与实现。添加系统调用的步骤：在syscall.h中增加系统调用号和系统调用函数声明；在start.s里增加系统调用函数的定义（寄存器操作）；在exception.cc里增加系统调用处理函数：

```c
//syscall.c
#define SC_Pwd      11
#define SC_Ls       12
#define SC_Cd       13
#define SC_Remove   14
#define SC_CreateDir    15
#define SC_RemoveDir    16
#define SC_Help        17
......
void Pwd();
void Cd();
void Ls();
void Remove(char *name);
void CreateDir(char *name);
void RemoveDir(char *name);
void Help();
```

```c
//exception.cc
void PwdSyscallHandler(){
    system("pwd");
}

void LsSyscallHandler(){
    system("ls");
}

void CdSyscallHandler(){
    int address = machine->ReadRegister(4);
    char *name = getFileNameFromAddress(address);
    chdir(name);
}

void RemoveSyscallHandler(){
    int address = machine->ReadRegister(4);
    char *name = getFileNameFromAddress(address);
    fileSystem->Remove(name);
}

void CreateDirSyscallHandler(){
    int address = machine->ReadRegister(4);
    char *name = getFileNameFromAddress(address);
    mkdir(name,0777);
}

void RemoveDirSyscallHandler(){
    int address = machine->ReadRegister(4);
    char *name = getFileNameFromAddress(address);
    rmdir(name);
}

void HelpSyscallHandler(){
    printf("-------------help-------------\n");
    printf("x [userprog]: execute a uesr program\n");
    printf("pwd: get current path\n");
    printf("ls: list the files and folders in current path\n");
    printf("nf [filename]: create a new file\n");
    printf("nd [foldername]: create a new folder\n");
    printf("df [filename]: delete a file\n");
    printf("dd [foldername]: delete a folder\n");
    printf("h:print the help information\n");
    printf("q: leave the shell\n");
    printf("------------------------------\n");
}
......
......
    //在exceptionHandler函数中：
else if(type == SC_Pwd) {
        PwdSyscallHandler();
    }
    else if(type == SC_Ls) {
        LsSyscallHandler();
    }
    else if(type == SC_Cd) {
        CdSyscallHandler();
    }
    else if(type == SC_Remove) {
        RemoveDirSyscallHandler();
    }
    else if(type == SC_CreateDir) {
        CreateDirSyscallHandler();
    }
    else if(type == SC_RemoveDir) {
        RemoveDirSyscallHandler();
    }
    else if(type == SC_Help) {
        HelpSyscallHandler();
    }
    IncrementPCRegs(); 
```

至此，ls、pwd、cd、makedir、remove、removedir、help、exec功能的调用和实现都已经完成。

测试：

```
vagrant@precise32:/vagrant/nachos/nachos-3.4/code/userprog$ ./nachos -x ../test/shell
h
-------------help-------------
x [userprog]: execute a uesr program
pwd: get current path
ls: list the files and folders in current path
nf [filename]: create a new file
nd [foldername]: create a new folder
df [filename]: delete a file
dd [foldername]: delete a folder
h:print the help information
q: leave the shell
------------------------------
pwd
/vagrant/nachos/nachos-3.4/code/userprog
ls
1.txt         addrspace.h  bitmap.h   exception.cc  interrupt.o  main.o     nachos       scheduler.o  swtch.s      syscall.h  testFile   threadtest.o  utility.o
2.txt         addrspace.o  bitmap.o   exception.o   list.o       Makefile   progtest.cc  stats.o      synchlist.o  sysdep.o   testMkdir  timer.o
addrspace.cc  bitmap.cc    console.o  hello.o       machine.o    mipssim.o  progtest.o   switch.o     synch.o      system.o   thread.o   translate.o
nd 123
ls
123    addrspace.cc  bitmap.cc  console.o     hello.o      machine.o  mipssim.o    progtest.o   switch.o     synch.o    system.o   thread.o      translate.o
1.txt  addrspace.h   bitmap.h   exception.cc  interrupt.o  main.o     nachos       scheduler.o  swtch.s      syscall.h  testFile   threadtest.o  utility.o
2.txt  addrspace.o   bitmap.o   exception.o   list.o       Makefile   progtest.cc  stats.o      synchlist.o  sysdep.o   testMkdir  timer.o
cd 123
pwd
/vagrant/nachos/nachos-3.4/code/userprog/123
nf huaboshell
SC_Create called
Create address: 2211
Thread main Creating File: huaboshell
ls
huaboshell
cd ..
cd 123
ls
huaboshell
df huaboshell
ls
cd ..
ls
123    addrspace.cc  bitmap.cc  console.o     hello.o      machine.o  mipssim.o    progtest.o   switch.o     synch.o    system.o   thread.o      translate.o
1.txt  addrspace.h   bitmap.h   exception.cc  interrupt.o  main.o     nachos       scheduler.o  swtch.s      syscall.h  testFile   threadtest.o  utility.o
2.txt  addrspace.o   bitmap.o   exception.o   list.o       Makefile   progtest.cc  stats.o      synchlist.o  sysdep.o   testMkdir  timer.o
dd123
ls
123    addrspace.cc  bitmap.cc  console.o     hello.o      machine.o  mipssim.o    progtest.o   switch.o     synch.o    system.o   thread.o      translate.o
1.txt  addrspace.h   bitmap.h   exception.cc  interrupt.o  main.o     nachos       scheduler.o  swtch.s      syscall.h  testFile   threadtest.o  utility.o
2.txt  addrspace.o   bitmap.o   exception.o   list.o       Makefile   progtest.cc  stats.o      synchlist.o  sysdep.o   testMkdir  timer.o
x ../test/halt
SC_Exec called
Exec address: 2210
Exec File: ../test/halt
SC_Join called
thread 0 join thread 1
Thread main sleep and Thread Thread 1 start to run
Entering exec_func.
Try to open file ../test/halt.
**********Haaalting*******
Machine halting!

Ticks: total 4258, idle 0, system 30, user 4228
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...
```

过程中遇到的问题：

为顺利调用system函数（PwdSyscallHandler()），需要#include <stdlib.h>此时会与sysdep.h中的一些函数重复定义，把这些函数注释掉，用#include <stdlib.h>代替即可。

start.s修改后make没有及时更新新函数，可行的操作流程：start.c也像start.s一样修改，去掉start.s中原有的某个函数，make一下，这时新写的函数成功添加。