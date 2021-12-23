#include "get_proc_info.h"
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>

long get_proc_info(pid_t pid, struct procinfos * info) {
	long sysvalue;
	sysvalue = syscall(548, pid, info);
	return sysvalue;
}