#include <get_proc_info.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

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
                        printf("first create child pid %d\n",getpid());
                        sleep(10);
                } else {
                        pid_t pid2;
                        pid2 = fork();
                        if (pid2 == 0) {
                                printf("last create child pid %d\n",getpid());
                                sleep(10);
                        } else {
				sleep (1);

                                if (get_proc_info(-1, &info) == 0) {
                                        // check
                                        printf("\n--StudenID %ld\n",info.studentID);
                                        printf("--current :'%s' [%d]\n",info.proc.name,info.proc.pid);
                                        printf("--parent :'%s' [%d]\n",info.parent_proc.name,info.parent_proc.pid);
                                        printf("--oldest child :'%s' [%d]\n",info.oldest_child_proc.name,info.oldest_child_proc.pid);
                                } else {
                                        printf("Cannot get information from the process %d\n",getpid());
                                }
                        }
                }
        }
        else {
                sleep(10);

        }
        return 0;
}
