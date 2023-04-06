#include "Eventloop.h"
#include "Channel.h"
#include "CurrentThread.h"
#include "Logger.h"
#include "Poller.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <mutex>
#include<sys/eventfd.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<stdio.h>
#include<memory>
#include <vector>

//防止一个线程创建多个Eventloop thread_local
__thread EventLoop *t_loopInthisThread=nullptr;

//定义默认的poller IO复用接口的超时事件
const int kPollTimeMS=10000;

//创建wakefd，用来唤醒subreactor处理channel事件
int createEventfd()
{
    int evtfd=::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd<0)
    {
        LOG_FATAL("eventfd error : %d \n",errno);
    }
    return evtfd;
}

EventLoop::EventLoop():looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    wakeFd_(createEventfd()),
    wakeupChannel_(new Channel(this,wakeFd_)),
    currentActiveChannel_(nullptr)
{
    LOG_INFO("EventLoop created %p in thread %d \n", this, threadId_);
    if(t_loopInthisThread)
    {
        LOG_FATAL("Another Eventloop %p in this thread %d\n",t_loopInthisThread,threadId_);
    }
    else 
    {
        t_loopInthisThread=this;
    }

    //设置wakefd监听的事件
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    //每一个eventloop将监听epollin读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    //禁止wakeupchannel的所有事件
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();

    ::close(wakeFd_);
    t_loopInthisThread=nullptr;
}

//wakeupchannel的读事件回调函数
void EventLoop::handleRead()
{
    uint64_t one=1;
    ssize_t n=read(wakeFd_,&one,sizeof one);
    if(n!=sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead og 8",n);
    }
}

//开启事件循环
void EventLoop::loop()
{
    looping_=true;
    quit_=false;

    LOG_INFO("EventLoop %p start looping \n",this);
    while(!quit_)
    {
        activeChannels_.clear();
        //监听两种fd,clientfd和wakeupfd，唤醒好像在这一行，唤醒epollwait
        pollReturnTime_=poller_->poll(kPollTimeMS, &activeChannels_);
        for(Channel *channel:activeChannels_)
        {
            //通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前eventloop事件循环处理的回调操作
        /*
        mainloop accept df => channel =>subloop
        mainloop 事先注册一个回调cb，需要subloop执行 wakeup subloop后执行下面的方法
        */
        //注册回调
        doPendingFunctors();
    }
    LOG_INFO("Eventloop %p stop looping",this);
    looping_=false;

}

//退出事件循环 1.在自己的线程中调用quit 2.在非loop的线程中调用Loop的quit
void EventLoop::quit()
{
    quit_=true;
    
    if(!isInLoopThread())       //如果在其他线程中调用quit
    {
        wakeup();       //要唤醒poller_->poll，防止阻塞,调用了mainloop(IO)的quit
    }
}
// 退出事件循环  1.loop在自己的线程中调用quit  2.在非loop的线程中，调用loop的quit
/**
 *              mainLoop
 * 
 *       no ==================== 生产者-消费者的线程安全的队列
 * 
 *  subLoop1     subLoop2     subLoop3
 */ 

 
//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())    //在当前的loop线程中执行callback
    {
        cb();
    }
    else //非当前loop线程,唤醒loop所在线程执行cb
    {
        queueInLoop(cb);
    }
}

//在cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        PendingFunctors_.emplace_back(std::move(cb));
    }

    //唤醒相应的，需要执行上面回调操作的loop线程，第二个逻辑是为了下次执行回调做准备，
    //因为已经有了新的待执行cb加入队列，现在正在执行上一轮回调，为了直接执行下一轮回调，提前唤醒工
    //作线程，即往fd里写数据，看doPendingFunctors的最后一行
    if(!isInLoopThread()||callingPendingFunctors_)  
    {
        wakeup();//唤醒loop所在线程
    }
}

//唤醒线程，向wakeupfd写数据
void EventLoop::wakeup()
{
    uint64_t one=1;
    ssize_t n=write(wakeFd_, &one, sizeof one);
    if(n!=sizeof one)
    {
        LOG_ERROR("Eventloop ::wakeup() writes %lu bytes instead of 8",n);
    }
}

//操作channel，调用poller的接口，channel更新会调用该接口
void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_=true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        std::swap(functors, PendingFunctors_);
    }
    for(const Functor& functor:functors)
    {
        functor();  //执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_=false;
}