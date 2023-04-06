#include <arpa/inet.h>
#include <cstdio>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include<strings.h>
#include <cstdlib>
int main()
{
    int fd=socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in inet;
    bzero(&inet, sizeof inet);
    inet.sin_family=AF_INET;    
    inet.sin_port=htons(1999);
    inet.sin_addr.s_addr=inet_addr("127.0.0.1");
    socklen_t len=sizeof(sockaddr_in);
    int ret=bind(fd,(sockaddr*)&inet,len);
    if(ret!=0)
    {
        perror("bind errno");
        exit(-1);
    }
    listen(fd, 3);
    close(fd);
    return 0;
}