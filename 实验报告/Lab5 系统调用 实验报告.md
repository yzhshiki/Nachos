# Lab5 系统调用 实验报告

## 内容一：总体概述

本次实习希望在阅读Nachos系统调用相关源代码的基础上，通过修改Nachos系统的底层源代码，达到“实现系统调用”的目标。

系统调用（system call），指运行在用户空间的程序向操作系统内核请求需要更高权限运行的服务。系统调用提供用户程序与操作系统之间的接口。大多数系统交互式操作需求在内核态运行。如设备IO操作或者进程间通信。Nachos的初始版本只为用户程序提供了Halt系统调用，本次实习将提供10个系统调用(文件系统相关5个，用户程序相关5个)

## 内容二：任务完成情况

### 任务完成列表

| Exercise1 | Exercise2 | Exercise3 | Exercise4 | Exercise5 |
| :-------: | :-------: | :-------: | :-------: | :-------: |
|     Y     |     Y     |     Y     |     Y     |     N     |



### Exercise 1  源代码阅读

> 阅读与系统调用相关的源代码，理解系统调用的实现原理。
>
> code/userprog/syscall.h
>
> code/userprog/exception.cc
>
> code/test/start.s

- `syscall.h`

  定义了11个系统调用号并声明了相应的系统调用函数，实现在start.s里（把syscall种类放进2号寄存器）。

  ```
  #define SC_Halt		0
  #define SC_Exit		1
  #define SC_Exec		2
  #define SC_Join		3
  #define SC_Create	4
  #define SC_Open		5
  #define SC_Read		6
  #define SC_Write	7
  #define SC_Close	8
  #define SC_Fork		9
  #define SC_Yield	10
  ```

- `exception.cc`

  定义`ExceptionHandler`，根据异常类型和2号寄存器保存的具体类型进行相应的处理。初始版本只实现了`Halt`和`Exit`系统调用的处理，之前的虚拟内存Lab中实现了`PageFault`的处理，这次要增加10个系统调用的处理。从注释可以看出，系统调用号由寄存器`r2`传递，`r4`、`r5`、`r6`、`r7`传递系统调用的第1-4个参数。如果系统调用有返回值，要写回`r2`寄存器。为了避免反复执行系统调用，在返回前还要将PC+4.

  ```
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
  ```

- `start.s`

  汇编代码，定义了用户程序`main`函数启动的入口和系统调用的入口，需要实现的10个系统调用启动对应的汇编代码已经完成。

通过阅读以上源代码，可以梳理出Nachos系统调用的过程：

- `OneInstruction`函数判断当当前指令是系统调用，转入`start.s`
- 通过过`start.s`确定系统调用入口，通过寄存器`r2`传递系统调用号，转入`exception. cc` (此时系统调用参数位于相应寄存器）
- `exception .cc`通过系统调用识別号识别系统调用，进行相关处理，此处理过程中调用内核系统调用函数（实现在`syscall.h`)，返回前更新`PCReg`的值。
- 系统调用结束，程序继续执行



### Exercise 2 系统调用实现

> 类比Halt的实现，完成与文件系统相关的系统调用：Create, Open，Close，Write，Read。Syscall.h文件中有这些系统调用基本说明。

```c++
case SC_Create:{
        printf("SC_Create called\n");
        int address = machine->ReadRegister(4); //从r4读到文件名变量的地址，然后读出文件名
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
```



### Exercise 3  编写用户程序

> 编写并运行用户程序，调用练习2中所写系统调用，测试其正确性。

测试：修改sort.c文件，预期效果是控制台输入input——input写入testFile——testFile写入output——output输出到控制台

```c++
OpenFileId fd;
    char* input;
    char* output;
    Read(input,10,0);
    Create("testFile");
    fd = Open("testFile");
    Write(input,10,fd);
    Close(fd);
    fd = Open("testFile");
    Read(output,10,fd);
    Close(fd);
    Write(output,10,1);
    Exit(A[0]);
```

效果符合预期：

```
vagrant@precise32:/vagrant/nachos/nachos-3.4/code/userprog$ ./nachos -x ../test/sort
SC_Read called
1111111111111
SC_Create called
Creating File: testFile
SC_Open called
SC_Write called
SC_Close called
SC_Open called
SC_Read called
SC_Close called
SC_Write called
1111111111
Exiting userprog of thread: main
No threads ready or runnable, and no pending interrupts.
Assuming the program completed.
Machine halting!
```



### Exercise 4 系统调用实现

> 实现如下系统调用：Exec，Fork，Yield，Join，Exit。Syscall.h文件中有这些系统调用基本说明。

线程相关的系统调用较为复杂，需要考虑到用户程序的执行流程是逐条指令的翻译执行，其内存地址和nachos内核的内存地址是相互独立不可见的，在nachos内核中很难再程序执行中间改变程序走向。因此需要在执行用户程序的进程创建之初就设定好相关的信息。

1. `Exec`函数较为简单，创建新的线程并执行一个新的用户程序，将新进程的`tid`写回`r2`寄存器即可

   ```c++
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
   ```

   ```c++
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
   ```

   

2. `Yield`函数较为简单，直接执行`Thread->Yield()`函数即可

   ```c++
   case SC_Yield:{
           printf("SC_Yield Called for thread %s\n",currentThread->getName());
   		currentThread->Yield();
           machine->AdvancePC();
           break;
       }
   ```

   

3. `Exit`函数同样较为简单，为了方便调试，打印`exitCode`并终止当前线程即可

   ```c++
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
   ```

   

4. Fork函数相对较为复杂，该函数要求新的线程fork在当前线程current地址空间内执行，并且需要执行指定的函数，因此作出如下处理

   - 针对需要执行指定函数的要求，作出如下修改。创建一个新的函数为`startForkProcess(addr)`，其作用类似于`progtest.cc::startProgess(name)`函数，该函数针对传入的地址，设置最初的`PC`寄存器值为指定位置，并调用`machine::RUn()`函数执行，以达直接执行指定函数的目的。

   - 而在系统调用的处理阶段，则需要手动为`fork`线程分配空间，剩余的`PC`寄存器的设置在程序执行最初初始化时设置，相关代码如下：

     ```c++
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
     ```

     

5. join函数要求线程等待指定的线程结束，并获得其exit code因此添加如下数据结构的支持

   - 在`Thread`结构体中添加变量`exit code`，当等待的线程退出后，内核将其`exit code`保存至本线程的`exit code`变量中

   - 在`Thread`结构体中添加`waitingList`变量，维护等待当前进程结束的进程地址 然后在`join`函数调用时，将当前进程加入指定进程的`waitingList`中，并`Sleep()`。在某个线程调用`Exit`时，检查该线程的`waitingList`，唤醒其中的额所有线程并设置对应线程的`exit code`为当前的`exit code`，实现如下：

     ```c++
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
     ```

     

### Exercise 5  编写用户程序

> 编写并运行用户程序，调用练习4中所写系统调用，测试其正确性。

segmentation Fauuuuuuult！

## 内容三 遇到的困难及解决方法

系统调用时参数是什么时候传到r4～r7的呢？

filename不知如何传入exec_func，其实只要强制转换为int就可以。



## 内容四 收获及感想

本次Lab在Nachos提供好的接口基础上完成并测试了十个系统调用，起到了用户态和内核态之间的桥梁作用。通过本次Lab,我对Nachos的内核和用户程序加载方式有了更深的认识，并领悟到了系统调用在操统系统中的重要作用——用户通过内核执行关键操作，以保证系统的安全性和鲁棒性。

## 内容五 对课程的意见或建议

无
