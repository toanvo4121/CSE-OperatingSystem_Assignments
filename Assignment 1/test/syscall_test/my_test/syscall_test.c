#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>


struct proc_info {
        pid_t pid;
        char name[16];
};

struct procinfos {
        long studentID;
        struct proc_info proc;
        struct proc_info parent_proc;
        struct proc_info oldest_child_proc;
};

int main()
{
        struct procinfos info;
        printf("parent pid %d\n",getpid());
        pid_t pid;
        //info = syscall(548, -1, &info);
        pid = fork();
        if (pid == 0) {
                printf("current pid %d\n",getpid());
                pid_t pid1;
                pid1 = fork();
                if (pid1 == 0) {
                        printf("first created child pid %d\n",getpid());
                        sleep(10);
                } else {
                        pid_t pid2;
                        pid2 = fork();
                        if (pid2 == 0) {
                                printf("last created child pid %d\n",getpid());
                                sleep(10);
                        } else {
				sleep (1);
                               syscall(548, getpid(), &info);
                               // check
                               printf("\n--StudenID %ld\n",info.studentID);
                               printf("--current :'%s' [%d]\n",info.proc.name,info.proc.pid);
                               printf("--parent :'%s' [%d]\n",info.parent_proc.name,info.parent_proc.pid);
                               printf("--oldest child :'%s' [%d]\n",info.oldest_child_proc.name,info.oldest_child_proc.pid);
                        }
                }
        }
        else {
                sleep(10);

        }
        return 0;
}
