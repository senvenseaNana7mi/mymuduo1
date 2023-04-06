#include <mymuduo/TcpServer.h>
#include<mymuduo/Logger.h>
#include <string>
#include<functional>

class EchoServer
{
public:
    EchoServer(EventLoop *loop,const InetAddress& addr,
                const std::string &name):
                server_(loop, addr,name),
                loop_(loop)
    {
        //注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection,this,std::placeholders::_1)
        );
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage,this,std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3)
        );
        //设置适合的loop线程数量
        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }
private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("conn up: %s",conn->peerAddress().toIpPort().c_str());
        }
        else 
        {
            LOG_INFO("Connection DOWN: %s",
            conn->peerAddress().toIpPort().c_str());
        }
    }

    //可读写事件回调
    void onMessage(const TcpConnectionPtr& conn,
        Buffer* buf,Timestamp Time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown(); //写端 EPOLLHUB =》 closecallback
    }
    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(1997);
    EchoServer server(&loop,addr,"EchoServer-01");
    server.start();
    loop.loop();
    return 0;
}