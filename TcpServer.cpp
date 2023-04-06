#include "TcpServer.h"
#include "Callback.h"
#include "Channel.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "Logger.h"
#include "Socket.h"
#include "Acceptor.h"
#include "TcpConnection.h"

#include <cstddef>
#include <cstdio>
#include <netinet/in.h>
#include <string>
#include<functional>
#include <strings.h>
#include <sys/socket.h>
EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop==nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null!\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &nameArg,
                     Option Option):
                     loop_(CheckLoopNotNull(loop)),
                     ipPort_(listenAddr.toIpPort()),
                     name_(nameArg),
                     acceptor_(new Acceptor(loop,listenAddr,Option==kReusePort)),//创建listen socket
                     threadPool_(new EventLoopThreadPool(loop,name_)), 
                     connectionCallback_(),
                     messageCallback_(),
                     nextConnId_(1),
                     started_(0)                                                              
{
    //有新用户连接时会执行该函数将fd分配给eventloop
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));
    
}

TcpServer::~TcpServer()
{
    for(auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second); //局部变量接受智能指针，出作用域直接析构
        item.second.reset();    //释放原智能指针

        //销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestoryed,conn)
        );
    }
}

//设置底层subloop个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

//开启服务器
void TcpServer::start()
{
    if(started_++==0)
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));//启动监听文件描述符
    }
}

//传给acceptor,在执行读事件也就是有新连接的时候会执行这个函数，作用是分配一个eventloop，将channel加入eventloop
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    // 轮询算法，选择一个subloop，来管理channel
    EventLoop* ioloop=threadPool_->getNextLoop();
    char buf[64]={0};
    snprintf(buf, sizeof buf, "-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName=name_+buf;

    LOG_INFO("TcpServer::new Connection [%s] - new connection [%s] from %s \n",
    name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());
    //通过sockfd获取其绑定的方法
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen=sizeof local;
    if(::getsockname(sockfd, (sockaddr*)&local, &addrlen))
    {
        LOG_ERROR("socks::getsockname");
    }
    InetAddress localAddr(local);

    //根据连接成功的sockfd，创建连接的TcpConnection对象

    TcpConnectionPtr conn(new TcpConnection(ioloop,connName,
                            sockfd,localAddr,peerAddr));
    connections_[connName]=conn;
    //下面的回调都是用户设置给TcpSerVer -> TcpConnection -> Channel ->poller -> notify channel调用回调  
    conn->setConnectionCallback(connectionCallback_);                      
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCallback(writeCallback_);
    //设置如何概念比连接的回调
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection,this,std::placeholders::_1)
    );

    //直接调用
    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));

}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop,this,conn)
    );
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s]-connection %s\n",
        name_.c_str(),conn->name().c_str());
    size_t n =connections_.erase(conn->name());
    EventLoop *ioloop=conn->getLoop();
    ioloop->queueInLoop(
        std::bind(&TcpConnection::connectDestoryed,conn)
    );
}