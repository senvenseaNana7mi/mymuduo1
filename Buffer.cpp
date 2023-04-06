#include "Buffer.h"
#include <cerrno>
#include <cstdio>
#include <sys/uio.h>
#include <unistd.h>

//从fd上读取数据，Poller工作在lt模式
//buffer缓冲区是有大小的！但是fd上读取数据的时候却不知道多大
ssize_t Buffer::readFd(int fd,int* saveErrno)
{
    char extrabuf[65536]={0};//栈上的内存空间 64k
    struct iovec vec[2];
    const size_t writeable=writeableBytes();
    //第一块缓冲区
    vec[0].iov_base = begin()+writeIndex_;
    vec[0].iov_len=writeIndex_;
    //第二块缓冲区
    vec[1].iov_base= extrabuf;
    vec[1].iov_len=sizeof extrabuf;

    const int iovcnt = (writeable<sizeof extrabuf)?2 : 1;
    //在非连续缓冲区当中写入同一个fd的内存
    const ssize_t n=::readv(fd,vec,iovcnt); 
    if(n<0)
    {
        *saveErrno=errno;
    }
    //buffer的可写缓冲区已够读取
    else if(n<=writeable)
    {
        writeIndex_+=n;
    }
    else    //extrabuf里面也写入了数据
    {
        writeIndex_=buffer_.size();
        append(extrabuf,n-writeable);   //writeIndex_开始写
    }
    return n;
}

ssize_t Buffer::writeFd(int fd,int* saveErrno)
{
    ssize_t n=write(fd, peek(), readableBytes());
    if(n<0)
    {
        *saveErrno=errno;
    }
    return n;
}