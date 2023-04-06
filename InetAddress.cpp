#include "InetAddress.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdio>
#include <netinet/in.h>
#include<string>
#include<string.h>
#include <sys/socket.h>


InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_,sizeof addr_);
    addr_.sin_family=AF_INET;
    addr_.sin_addr.s_addr=inet_addr(ip.c_str());
    addr_.sin_port=htons(port);
}

std::string InetAddress::toIp() const
{
    char buf[64]={0};
    ::inet_ntop(AF_INET, &addr_, buf, 64);
    return  buf;
}
std::string InetAddress::toIpPort() const
{
    char buf[64]={0};
    inet_ntop(AF_INET, &addr_, buf, 64);
    size_t end=strlen(buf);
    uint16_t port=ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u",port);
    return buf;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}
