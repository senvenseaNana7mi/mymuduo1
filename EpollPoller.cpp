#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"
#include "Poller.h"
#include "Timestamp.h"

#include <cstring>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
// channel未添加
const int kNew = -1; // channel的成员index初始化就是-1

// channel已添加
const int kAdded = 1;

// channel已删除
const int kDel = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC)),     //EPOLL_CLOEXEC表示在多进程关闭该文件描述符
      events_(KInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EpollPoller::~EpollPoller()
{
    close(epollfd_);
}

// 通过epoll_wait给Eventloop
Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 实际应该用LOG_DEBUG
    LOG_INFO("func=%s => fd total count :%lu \n", __FUNCTION__,
             channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
                                 static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno; // 全局变量
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        //表示接收数组接受文件描述符到达上限
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_INFO("%s timeout!\n", __FUNCTION__);
    }
    else
    {
        errno = saveErrno; // 重置errno
        LOG_ERROR("EpollPoller::poll() err!");
    }
    return now;
}

// channel update remove -> EventLoop updateChannel removeChannel ->Poller
/*
Poller上的所有channel都注册在channel

*/
void EpollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func= %s => fd=%d events=%d index=%d\n", __FUNCTION__,
             channel->fd(), channel->events(), channel->index());
    if (index == kNew || index == kDel)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // channel已经注册过了-kAdded
    {
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDel);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 在poller中删除
void EpollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd);
    LOG_INFO("func=%s => fd=%d \n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 填写活跃的链接
void EpollPoller::fillActiveChannels(int numEvents,
                                     ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);

        // 更新channel的状态
        channel->set_revents(events_[i].events);
        // 加入eventloop的容器里
        activeChannels->push_back(channel);
    }
}

// 更新Channel通道
void EpollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof event);
    int fd = channel->fd();

    event.data.fd = fd;
    event.events = channel->events();
    //实际用的是指针，将channel指针放入event
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl mod/add error:%d\n", errno);
        }
    }
}