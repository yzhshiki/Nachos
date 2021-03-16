#include "syscall.h"
int main(){
	char* name = "../test/fibonacci";
	int exitCode = -2;
    SpaceId id = Exec(name);
    exitCode = Join(id);
    Exit(exitCode);
}