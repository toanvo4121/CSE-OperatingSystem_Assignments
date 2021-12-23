#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/sched.h>  
#include <linux/string.h>
#include <linux/syscalls.h> 
#include <linux/sched/signal.h> 
#include <asm/current.h> 
#include <asm/uaccess.h> 

struct proc_info
{
	pid_t pid;
	char name[16];
};

struct procinfos
{
	long studentID;
	struct proc_info proc;
	struct proc_info parent_proc;
	struct proc_info oldest_child_proc;
};

SYSCALL_DEFINE2(get_proc_info, pid_t, pid, struct procinfos *, info)
{
	struct task_struct *process = NULL, *child_process = NULL;
	struct procinfos process_infos;
	
	process_infos.studentID = 1913944;
	if (pid == -1)
	{
		pid = current->pid;
	}
	for_each_process(process)
	{
		if (process->pid == pid)
		{
			process_infos.proc.pid = process->pid;
			strcpy(process_infos.proc.name, process->comm);

			// PARENT
			if (process->real_parent != NULL)
			{
				process_infos.parent_proc.pid = process->real_parent->pid;
				strcpy(process_infos.parent_proc.name, process->real_parent->comm);
			}
			else
			{
				process_infos.parent_proc.pid = 0;
				strcpy(process_infos.parent_proc.name, "\0");
			}

			// CHILD
			child_process = list_first_entry_or_null (&process->children, 
                                                    struct task_struct, sibling);

			if (child_process != NULL)
			{
				process_infos.oldest_child_proc.pid = child_process->pid;
				strcpy(process_infos.oldest_child_proc.name, child_process->comm);
			}
			else
			{
				process_infos.oldest_child_proc.pid = 0;
				strcpy(process_infos.oldest_child_proc.name, "\0");
			}

			copy_to_user(info, &process_infos, sizeof(struct procinfos));
			return 0;
		}
	}
	return EINVAL;
}