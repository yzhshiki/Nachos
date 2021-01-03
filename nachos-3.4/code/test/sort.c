/* sort.c 
 *    Test program to sort a large number of integers.
 *
 *    Intention is to stress virtual memory system.
 *
 *    Ideally, we could read the unsorted array off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"
int A[100];	/* size of physical memory; with code, we'll run out of space!*/
void func(){
	Create("1.txt");
}
int
main()
{
    // int i, j, tmp;

    // /* first initialize the array, in reverse sorted order */
    // for (i = 0; i < 100; i++)		
    //     A[i] = 100 - i;

    // /* then sort! */
    // for (i = 0; i < 99; i++){
    //     for (j = i; j < (99 - i); j++)
	//    if (A[j] > A[j + 1]) {	/* out of order -> need to swap ! */
	//       tmp = A[j];
	//       A[j] = A[j + 1];
	//       A[j + 1] = tmp;
    // 	   }
    // }

    OpenFileId fd;
    // char* input;
    // char* output;
    // Read(input,10,0);
    // Create("testFile");
    // fd = Open("testFile");
    // Write(input,10,fd);
    // Close(fd);
    // fd = Open("testFile");
    // Read(output,10,fd);
    // Close(fd);
    // Write(output,10,1);

    Create("2.txt");
	Fork(func);
	fd = Exec("../test/halt");
	Join(fd);

    Exit(A[0]);		/* and then we're done -- should be 0! */
    // Halt();
}
