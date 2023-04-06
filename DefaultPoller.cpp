#include "EpollPoller.h"
#include "Poller.h"
#include <cstddef>
#include <cstdlib>
Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;//生成poll的实例
    }
    else {
        return new EpollPoller(loop);//生成epoll的实例
    }
}