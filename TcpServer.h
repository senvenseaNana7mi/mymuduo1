#pragma once
#include "Acceptor.h"
#include "Callback.h"
#include "EventLoopThreadPool.h"
#include "Eventloop.h"
#include "InetAddress.h"
#include "nocopyable.h"
#include "TcpConnection.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include<unordered_map>
/*
用户使用muduo库编写服务器程序
*/
// 对外的服务器编程使用的类
class TcpServer : nocopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option{
        kNoReusePort,
        kReusePort,
    };
    TcpServer(EventLoop* loop ,
        const InetAddress& listenAddr,
        const std::string &nameArg,
        Option Option=kNoReusePort);
    ~TcpServer();

    void setThreadInitcallback(const ThreadInitCallback& cb)
    {
        threadInitCallback_=cb;
    }
    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connectionCallback_=cb;
    }
    void setMessageCallback(const MessageCallback& cb)
    {
        messageCallback_=cb;
    }
    void setWriteCallback(const WriteCallback& cb)
    {
        writeCallback_=cb;
    }
    //设置底层subloop数量
    void setThreadNum(int numThreads);

    //开启服务器监听
    void start();
private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap=std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_; // baseloop用户
    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;//运行在mainloop，监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_;   //有读写消息的回调
    WriteCallback writeCallback_;   //消息发送完成以后的就回调

    ThreadInitCallback threadInitCallback_;//loop线程初始化回调
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; //保存所有的连接
};