#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>


int main()
{
        
	long sys_return_value;
	unsigned long info[200];
	sys_return_value = syscall(548, -1, &info);
	printf("My student ID: %lu\n",info[0]);
        return 0;
}
