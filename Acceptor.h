#pragma once
#include "Channel.h"
#include "InetAddress.h"
#include "Socket.h"
#include "nocopyable.h"
#include "Eventloop.h"

#include<functional>


class Acceptor:nocopyable
{
public:
    using NewConnectionCallback=std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool setReusePort);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        newConnectionCallback_=cb;
    }
    bool listenning()const {return listenning_;}
    void listen();
private:
    void handleRead();
    EventLoop* loop_;//Acceptor用的就是用户定义得那个baseloop，也称为mainloop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};