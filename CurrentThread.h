#pragma once

#include<pthread.h>
#include <unistd.h>

namespace CurrentThread {
        extern __thread int t_chachedTid;

        void cacheTid();

        inline int tid()
        {
            if(__builtin_expect(t_chachedTid==0,0))
            {
                cacheTid();
            }
            return t_chachedTid;
        }
}