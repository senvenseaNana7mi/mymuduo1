#pragma once 
#include"nocopyable.h"
#include"InetAddress.h"


//封装sockfd
class Socket:nocopyable
{
public:
    explicit Socket(int sockfd):sockfd_(sockfd)
    {}

    ~Socket();

    int fd() const {return sockfd_;}
    void bindAddress(const InetAddress&);
    void listen();
    int accept(InetAddress* peeraddr);

    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:

    const int sockfd_;    
};