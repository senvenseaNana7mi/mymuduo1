#pragma once 

#include "Buffer.h"
#include "Channel.h"
#include "Eventloop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Timestamp.h"
#include "nocopyable.h"
#include "Callback.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
class Channel;
class EventLoop;
class Socket;

/*
Tcpserver -> Accpertor -> 有新用户连接，通过accept函数拿到connfd
->Tcpconnection 设置回调 -> channel ->poll -> Channel的回调操作
*/
class TcpConnection:nocopyable,public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                const std::string &name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop()const {return loop_;}
    const std::string& name()const {return name_;}
    const InetAddress& localAddress()const{return localAddr_;}
    const InetAddress& peerAddress()const{return peerAddr_;}

    bool connected() const {return state_==kConnected;}

    //发送数据
    void send(const void* message,int len);

    //关闭连接
    void shutdown();

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
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,size_t hightWaterMark)
    {
        highWaterMarkCallback_ =cb,highWaterMark_=hightWaterMark;
    }   
    void setCloseCallback(const CloseCallback& cb)
    {
        closeCallback_=cb;
    }

    void send(const std::string& buf);
    //连接建立
    void connectEstablished();
    //连接销毁
    void connectDestoryed();    

private:
    //状态
    enum StateE {kDisconnected, kConnecting, 
        kConnected, kDisconnecting};
    void setState(StateE state){ state_=state; }
    //channel的回调函数
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    
    

    void sendInLoop(const void* message, size_t len);

    void shutdownInLoop();

    EventLoop *loop_; //这里不是baseloop,因为tcpconnection都是在subloop里管理
    const std::string name_;
    std::atomic_int state_;
    bool reading_;
    //和acceptor类似
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_; //有连接关闭的回调
    MessageCallback messageCallback_;   //有读写消息的回调
    WriteCallback writeCallback_;   //消息发送完成以后的就回调
    HighWaterMarkCallback highWaterMarkCallback_ ;
    CloseCallback closeCallback_;   //关闭连接的回调

    size_t highWaterMark_;

    Buffer inputBuffer_;    //读buffer
    Buffer outputBuffer_;   //写buffer
};