#pragma once
#include "Channel.h"
#include "Poller.h"
#include "Timestamp.h"
#include<vector>
#include<sys/epoll.h>


class Channel;
class EpollPoller:public Poller
{
public:
    EpollPoller(EventLoop* loop);//epoll_create
    ~EpollPoller() override;

    //重写基类Poller的抽象方法
    virtual Timestamp poll(int timeoutMs,ChannelList* activeChannels) override;//epoll_wait
    //epoll_ctl mod/add/del
    virtual void updateChannel(Channel *channel) override;
    virtual void removeChannel(Channel *channel) override;
private:
    static const int KInitEventListSize=16;
    //填写活跃的链接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    //更新Channel通道
    void update(int operation,Channel* channel);

    using EventList=std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};