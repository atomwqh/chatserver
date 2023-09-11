/*
muduo网络库给用户提供了两个主要的类
TcpServer ： 用于编写服务器程序的
TcpClient ： 用于编写客户端程序的

epoll + 线程池
好处：能够把网络I/O的代码和业务代码区分开
                        用户的连接和断开       用户的可读写事件
*/
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/*基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针，epoll事件循环
3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写时间的回调函数（参数占位）
5.设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程

主要精力在于业务代码的处理onConnection和onMessage
*/
class ChatServer
{
public:
    ChatServer(EventLoop *loop,               // 事件循环
               const InetAddress &listenAddr, // IP+Port端口号
               const string &nameArg)         // 服务器的名字
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 给服务器注册用户连接的创建和断开回调，底层发现事件后会调用onConnection函数
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务器端的线程数量 1个I/O线程   3个worker线程
        _server.setThreadNum(4);
    }
    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // 专门处理用户的连接创建和断开  epoll listenfd accept
    // 这里就是提供了一个调用的接口
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:online" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:offline" << endl;
            conn->shutdown(); // close(fd) 资源回收
            // _loop->quit(); 退出expoll
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn, // 连接，连接读数据写数据
                   Buffer *buffer,               // 缓冲区，提高数据的性能
                   Timestamp time)               // 接收到数据的时间信息
    {
        string buf = buffer->retrieveAllAsString(); // 可以将数据全部放到字符串当中
        cout << "recv data:" << buf << " time:" << time.toFormattedString() << endl;
        conn->send(buf);
    }

    TcpServer _server; // #1 第一步
    EventLoop *_loop;  // #2 第二步 epoll
};

int main()
{
    EventLoop loop;                               // 类似于创建一个epoll
    InetAddress addr("127.0.0.1", 6000);          // 做其他项目可能要改变
    ChatServer server(&loop, addr, "ChatServer"); // 创建服务器对象

    server.start(); // listenfd epoll_ctl => epoll
    loop.loop();    // epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等，事件发生就回调
}