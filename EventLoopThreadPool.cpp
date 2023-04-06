#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "Eventloop.h"
#include <cstdio>
#include <vector>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop,const std::string &nameArg):
    baseLoop_(baseloop),
    name_(nameArg),
    start_(false),
    numThreads_(0),
    next_(0)
{
}
//EventLoopThreadPool的析构函数不需要释放每个eventloop资源，因为每个Eventloop都
//在栈区定义，出作用域了会自动释放
EventLoopThreadPool::~EventLoopThreadPool()
{
}


void EventLoopThreadPool::start(const ThreadIintCallback& cb)
{
    start_=true;
    for(int i=0;i<numThreads_;++i)
    {
        char buf[name_.size()+32];
        snprintf(buf, sizeof buf ,"%s%d",name_.c_str(),i);
        EventLoopThread* t=new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());//底层创建线程，绑定一个新的loop,返回一个新的eventloop
    }
    //说明整个服务端只有一个线程运行
    if(numThreads_==0)
    {
        cb(baseLoop_);
    }
}

//如果工作在多线程中baseloop_默认以轮询的方式分配给subloop
EventLoop* EventLoopThreadPool::getNextLoop(){
    EventLoop* loop=baseLoop_;
    if(!loops_.empty())//通过轮询获取下一个处理事件的loops
    {
        loop=loops_[next_];
        ++next_;
        if(next_>=loops_.size())next_=0;
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops(){
    if(loops_.empty())
    {
        return std::vector<EventLoop*>(1,baseLoop_);
    }
    return loops_;
}