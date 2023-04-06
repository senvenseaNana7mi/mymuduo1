#include "Channel.h"
#include "Logger.h"
#include <memory>
#include <sys/epoll.h>
#include "Eventloop.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

//eventloop包含很多channel
Channel::Channel(EventLoop *loop, int fd):loop_(loop),fd_(fd)
,events_(0),revents_(0),index_(-1),tied_(false)
{
}

Channel::~Channel()
{
    
}

// 防止当channel被手动remove掉，channel还在执行回调操作
//Channel的tie方法什么时候调用 TcpConnection => channel
//新连接创建的时候
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_=obj;
    tied_=true;
}

//当改变channel的fd的事件后，update负责更改事件
void Channel::update()
{
    //通过channel所属的eventloop，调用poller的相应方法
    //add code
    loop_->updateChannel(this);
}

//在eventloop里面删除该Channel
void Channel::remove()
{
    // add code...
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    
    if(tied_)
    {
        std::shared_ptr<void> guard;
        guard=tie_.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else 
    {
        handleEventWithGuard(receiveTime);
    } 
}

//根据发生的具体事件
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("chanel handleEvent revents:%d\n",revents_);
    if((revents_&EPOLLHUP)&&!(revents_&EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }
    if(revents_&EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }
    if(revents_&(EPOLLIN|EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    if(revents_&EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
}