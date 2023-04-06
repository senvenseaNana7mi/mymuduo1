#pragma once

#include "Timestamp.h"
#include "nocopyable.h"

#include <functional>
#include <memory>

class EventLoop;

/**
理解为通道，封装的sockfd和其感兴趣的event，还绑定了poller返回的具体事件
 * 理清楚  EventLoop、Channel、Poller之间的关系   《= Reactor模型上对应
 * Demultiplex Channel
 * 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件
 * 还绑定了poller返回的具体事件
 Eventloop包含Channel和poller两个模块，构成epoll实现的io多路复用
 */
// Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
class Channel : nocopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb)
    {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback cb)
    {
        writeCallback_ = std::move(cb);
    }
    void setCloseCallback(EventCallback cb)
    {
        closeCallback_ = std::move(cb);
    }
    void setErrorCallback(EventCallback cb)
    {
        errorCallback_ = std::move(cb);
    }
    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const
    {
        return fd_;
    }
    int events() const
    {
        return events_;
    }
    int set_revents(int revt)
    {
        revents_ = revt;
        return revents_;
    }

    // 设置fd相应的事件状态
    void enableReading()
    {
        events_ |= kReadEvent;
        update();//调用epollctl
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }


    // 返回fd当前的事件状态
    bool isNoneEvent() const
    {
        return events_ == kNoneEvent;
    }

    bool isWriting() const
    {
        return events_ & kWriteEvent;
    }

    bool isReading() const
    {
        return events_ & kReadEvent;
    }

    int index()
    {
        return index_;
    }
    void set_index(int idx)
    {
        index_ = idx;
    }

    // one loop per thread,一个eventloop一个线程，一个服务器有多个eventloop线程
    EventLoop *ownerLoop()
    {
        return loop_;
    }
    void remove();

private:
    void update();
    //保护的处理事件
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd, Poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // poller返回的具体发生的事件
    int index_;

    // 监听状态
    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channel通道里面能够获知fd最终发生的具体的事件revents，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};