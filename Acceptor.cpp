#include "Acceptor.h"
#include "InetAddress.h"
#include "Logger.h"
#include"Eventloop.h"

#include <asm-generic/errno-base.h>
#include <cerrno>
#include<sys/types.h>
#include<sys/socket.h>

static int CreateNoblocking()
{
    int sockfd=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
    if(sockfd<0)
    {
        LOG_FATAL("%s:%s:%d create listen sockfd fail:ERR %d\n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    else
    {
        LOG_INFO("%s:%s:%d create listen sockfd:%d SUCC \n",__FILE__,__FUNCTION__,__LINE__,sockfd);
    }
    return sockfd;
}
Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool setReusePort)
    :loop_(loop),
    acceptSocket_(CreateNoblocking()),
    acceptChannel_(loop_,acceptSocket_.fd()),
    listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    //TCPserver::start()' Acceptor listen 有新用户链接执行回调connfd =>channel=>eventloop
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}
Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}


void Acceptor::listen()
{
    listenning_=true;
    acceptSocket_.listen();//listen
    //加入baseloop
    acceptChannel_.enableReading(); //acceptchannel_ => Poller
}

//listen有事件发生，有新用户链接了,此handler为监听channl的读事件文件回调函数
void Acceptor::handleRead()
{
    InetAddress peeraddr;
    int connfd=acceptSocket_.accept(&peeraddr);
    if(connfd>=0)
    {
        //回调，TCPServer
        if(newConnectionCallback_)
        {
            newConnectionCallback_(connfd,peeraddr);    //轮询，找到subloop唤醒分发当前得新客户chanbel
        }
        else 
        {
            ::close(connfd);
        }
    }
    else 
    {
        LOG_ERROR("%s:%s:%d accept sockfd fail:ERR %d\n",__FILE__,__FUNCTION__,__LINE__,errno);
        //文件描述符满了
        if(errno==EMFILE)
        {
            LOG_ERROR("%s:%s:%d accept sockfd reached limited\n",__FILE__,__FUNCTION__,__LINE__);

        }
    }
}