#include "TcpConnection.h"
#include "Callback.h"
#include "Channel.h"
#include "Logger.h"
#include "Socket.h"
#include "Timestamp.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <cerrno>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include <netinet/tcp.h>

extern EventLoop* CheckLoopNotNull(EventLoop* loop);
TcpConnection::TcpConnection(EventLoop* loop,
            const std::string &name,
            int sockfd,
            const InetAddress& localAddr,
            const InetAddress& peerAddr):
            loop_(CheckLoopNotNull(loop)),
            name_(name),
            state_(kConnecting),
            reading_(true),
            socket_(new Socket(sockfd)),
            channel_(new Channel(loop,sockfd)),
            localAddr_(localAddr),
            peerAddr_(peerAddr),
            highWaterMark_(64*1024*1024)    //64m
{
    //给channel设置相应的回调函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose,this));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite,this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError,this));
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n",name_.c_str(),sockfd);
    //设置保活机制
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n state=%d\n",
        name_.c_str(),socket_->fd(),(int)state_);   
}

//回调函数
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno=0;
    //从fd读事件读到buffer
    ssize_t n=inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if(n>0)
    {
        //已建立连接的用户，有回调操作了，调用用户传入的回调函数
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);   
    }
    else if(n==0) 
    {
        handleClose();
    }
    else
    {
        errno=saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
    
}

void TcpConnection::handleWrite()
{
    //有写事件
    if(channel_->isWriting())
    {
        int saveErrno=0;
        ssize_t n=outputBuffer_.writeFd(channel_->fd(),&saveErrno);
        if(n>0)
        {
            //复位
            outputBuffer_.retrieve(n);
            //没有数据可写
            if(outputBuffer_.readableBytes()==0)
            {
                //取消读事件
                channel_->disableWriting();
                if(writeCallback_)
                {
                    //将执行操作加入队列
                    loop_->queueInLoop(
                        std::bind(&TcpConnection::writeCallback_
                        ,shared_from_this()));
                }
                if(state_==kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else 
        {
            LOG_ERROR("TcpConnection::handleWrite\n");
        }
    }
    else 
    {
        LOG_ERROR("TcpConnection fd = %d isdown,no more writing\n",channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d, state=%d \n",channel_->fd(),(int)(state_));
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);   //执行连接关闭的回调
    closeCallback_(connPtr);        //执行关闭连接回调，由tcpserver给的
}

void TcpConnection::handleError()
{
    int optval;
    int err=0;
    socklen_t optlen =sizeof optval;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR,&optval, &optlen)<0)
    {
        err=errno;
    }
    else
    {
        err=optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n",name_.c_str(),err);
}

void TcpConnection::send(const std::string& buf)
{
    if(state_==kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,this,
                buf.c_str(),
                buf.size()
            ));
        }
    }
}
//发送数据 应用写的快，而内核发送数据满，需要写入缓冲区，而且设置水位回调
void TcpConnection::sendInLoop(const void* message, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining=len;
    bool faultError=false;

    //之前调用过shutdown，不能再调用send
    if(state_==kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing!");
        return ;
    }
    // 表示channel_第一次开始写数据，而且缓冲区没有带缓冲数据
    if(!channel_->isWriting()&&outputBuffer_.readableBytes()==0)
    {
        nwrote=::write(channel_->fd(),message,len);
        if(nwrote>=0)
        {
            remaining=len-nwrote;
            if(remaining==0&&writeCallback_)
            {
                //一次性发送完成，就不用再给channel设置epollout事件了
                loop_->queueInLoop(
                    std::bind(writeCallback_,shared_from_this())
                );
            }
        }
        else //nwrote<0
        {
            nwrote=0;
            if(errno!=EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET)   //SIGPIPE RESET
                {
                    faultError=true;   
                }
            }
        }
    }
    //还有数据没有发送，且没有发生错误,剩余数据存入缓冲区
    //并且注册epollout事件,poller发现tcp的读事件，会通知sock-channel调用handleWrite回调方法
    //最终调用handlewrite把发送缓冲区发送
    if(!faultError && remaining>0)
    {
        //目前发送缓冲区的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        //判断是否在高水位，且上次并没有调用高水位回调
        if(oldLen + remaining >= highWaterMark_&&
            oldLen<highWaterMark_&&
            highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_,shared_from_this(),
                oldLen+remaining)
            );
            outputBuffer_.append((char*)message+nwrote, remaining);
            if(!channel_->isWriting())
            {
                channel_->enableWriting(); //注册channel的写事件
            }
        }
    }
}

//建立连接，注册channel事件，并且调用回调函数
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    //防止TcpConnection被remove掉了还调用相应的回调函数
    channel_->tie(shared_from_this()); 
    channel_->enableReading();  //向poller注册channel的读事件

    //新连接建立，执行回调
    connectionCallback_(shared_from_this());
}

//连接销毁
void TcpConnection::connectDestoryed()
{
    if(state_==kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); //把channel的感兴趣事件从poller中给删掉
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}


void TcpConnection::shutdown()
{
    if(state_==kConnected)
    {
        setState(kDisconnecting);
        loop_->queueInLoop(
            std::bind(&TcpConnection::shutdownInLoop,this)
        );
    }
}

void TcpConnection::shutdownInLoop()
{
    //说明发送缓冲区发送完成了
    if(!channel_->isWriting())
    {
        socket_->shutdownWrite();   //poller会监听到epollhub,然后调用handleclose
    }
}