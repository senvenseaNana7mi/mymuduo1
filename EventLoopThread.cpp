#include "EventLoopThread.h"
#include "Eventloop.h"
#include "Thread.h"

#include <cstddef>
#include<memory>
#include <condition_variable>
#include <functional>
#include <mutex>

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
        const std::string& name)
        :loop_(nullptr),
        exiting_(false),
        thread_(std::bind(&EventLoopThread::threadFunc,this),name),
        mutex_(),
        cond_(),
        callback_()
{
}
EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if(loop_!=nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    thread_.start();    //启动底层新线程
    EventLoop* loop =nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_==nullptr)//线程还未初始化好，如果初始化好了直接不进入循环
        {
            cond_.wait(lock);
        }
        loop=loop_;
    }
    return loop;
}

// 下面这个方法，在单独的新线程运行的,作为参数传入新线程
void EventLoopThread::threadFunc()
{
    EventLoop loop; //创建一个独立的eventloop和上面的线程是一一对应的

    if(callback_)
    {
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_=&loop;
        cond_.notify_one();
    }
    loop.loop();//eventloop的循环函数，一直执行直到关闭
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}