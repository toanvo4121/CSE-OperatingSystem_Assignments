#include <get_proc_info.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int main()
{
        pid_t mypid = getpid();
        printf("PID: %d\n",mypid);
        struct procinfos info;

        if (get_proc_info(mypid, &info) == 0) {
                printf("\n--StudenID %ld\n",info.studentID);
                printf("Current : name = '%s' pid = %d\n",info.proc.name,info.proc.pid);
                printf("Parent :name = '%s' pid = %d\n",info.parent_proc.name,info.parent_proc.pid);
                printf("Oldest child : name = '%s' %d\n",info.oldest_child_proc.name,info.oldest_child_proc.pid);
        } else {
                printf("Cannot get information from the process %d\n",mypid);
        }
        sleep(10);
        return 0;
}
