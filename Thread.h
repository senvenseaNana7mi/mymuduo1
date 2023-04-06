#pragma once

#include "nocopyable.h"

#include <atomic>
#include<functional>
#include <sched.h>
#include <unistd.h>
#include<string>
#include<thread>
#include<memory>

class Thread:nocopyable
{
public:
    using ThreadFunc=std::function<void()>;

    explicit Thread(ThreadFunc,const std::string& name=std::string());
    ~Thread();
    
    void start();
    void join();

    bool started()const {return started_;}
    pid_t tid() const {return tid_;}
    const std::string& name()const {return name_;}

    static int numCreated(){return numCreated_;}
private:
    void setDefaultName();
    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;   //用智能指针储存线程，以防线程直接启动
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int32_t numCreated_;
};