#include "CurrentThread.h"
#include<sys/syscall.h>

namespace CurrentThread {
    __thread int t_chachedTid=0;
    void cacheTid()
    {
        if(t_chachedTid==0)
        {
            t_chachedTid=static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}