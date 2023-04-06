#pragma once

#include "Channel.h"
#include "Timestamp.h"
#include "nocopyable.h"

#include<vector>
#include<unordered_map>

class Channel;
class EventLoop;

//muduo库中多路事件分发器的IO复用模块
class Poller:nocopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller()=default;

    //给所有IO复用保留统一接口
    virtual Timestamp poll(int timeoutMs,ChannelList* activeChannels)=0;
    virtual void updateChannel(Channel *channel)=0;
    virtual void removeChannel(Channel *chanel)=0;

    //判断参数channel是否在这当前Poller当中
    bool hasChannel(Channel *channel) const;

    //EventLoop可以通过该接口获取默认IO多路复用的具体实现
    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    //map的key：sockfd  value：sockfd所属的channel
    using ChannelMap=std::unordered_map<int, Channel*>;
    ChannelMap channels_;

private:
    EventLoop *ownerLoop_;//Poller所属的loop对象
};

