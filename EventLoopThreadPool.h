#pragma once
#include "EventLoopThread.h"
#include "Eventloop.h"
#include "nocopyable.h"

#include<functional>
#include <memory>
#include<string>
#include <vector>

class EventLoopThreadPool:nocopyable
{
public:
    using ThreadIintCallback=std::function<void(EventLoop*)>;
    EventLoopThreadPool(EventLoop* baseloop,const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads){numThreads_=numThreads;}

    void start(const ThreadIintCallback& cb=ThreadIintCallback());

    //如果工作在多线程中baseloop_默认以轮询的方式分配给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool start()const {return start_;}
    const std::string name()const {return name_;}
private:
    
    EventLoop *baseLoop_;   //创建tcpserver时需要的传入的eventloop参数，当不设置参数时便只有这一个loop

    std::string name_;
    bool start_;
    int numThreads_;//线程数量
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};