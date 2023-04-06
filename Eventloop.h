#pragma once

// 时间循环类 主要包含了两个大模块 channel poller(Epoll的抽象)
#include<functional>

#include "CurrentThread.h"
#include "Poller.h"
#include "nocopyable.h"
#include"Timestamp.h"

#include <iostream>
#include <memory>
#include <sched.h>
#include<vector>
#include<atomic>
#include<mutex>

class Channel;
class Poller;

class EventLoop:nocopyable
{
public:
    using Functor=std::function<void()>;

    EventLoop();
    ~EventLoop();

    //开启事件循环
    void loop();
    //退出事件循环
    void quit();

    Timestamp pollReturnTime()const {return pollReturnTime_;}
    //在当前loop中执行cb
    void runInLoop(Functor cb);
    //在cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    //唤醒线程
    void wakeup();

    //操作channel，调用poller的接口，channel更新会调用该接口
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    //判断eventloop是否在该线程
    bool isInLoopThread()const {return threadId_==CurrentThread::tid();}//判断该线程是否对应该eventloop

private:
    void handleRead();
    //执行回调
    void doPendingFunctors();

    using ChannelList=std::vector<Channel*>;
    
    std::atomic_bool looping_; //原子操作，底层通过CAS
    std::atomic_bool quit_;     //标识退出loop循环
    
    const int threadId_;  //记录当前loop所在的线程id
    Timestamp pollReturnTime_;  //  poller返回channel事件发生变换的时间点
    std::unique_ptr<Poller> poller_;

    int wakeFd_;    //当mainloop获取一个新用户的channel通过轮询算法选择一个eventloop，通过eventfd函数
    std::unique_ptr<Channel> wakeupChannel_;//通过wakeupchannel通信

    ChannelList activeChannels_;
    Channel* currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_;   //标识当前loop是否需要执行的回溯操作
    std::vector<Functor> PendingFunctors_;  //存储loop所有的回调操作
    std::mutex mutex_;  //vector容器的线程安全
};