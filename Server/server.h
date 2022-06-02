#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <list>
#include <cstring>
using namespace std;

class CTcpServer
{
public:
    int m_listenfd; // 服务端用于监听的socket

    CTcpServer();

    bool InitServer(int port); // 初始化服务端 (socket创建, 绑定, 监听)

    // 向对端发送报文
    int Send(int clientfd, const void *buf, const int buflen);
    // 接收对端的报文
    int Recv(int clientfd, void *buf, const int buflen);

    void CloseListen(); // 关闭用于监听的socket

    ~CTcpServer();
};

CTcpServer::CTcpServer()
{
    // 构造函数初始化socket
    m_listenfd = 0;
}

CTcpServer::~CTcpServer()
{
    if (m_listenfd != 0)
        close(m_listenfd); // 析构函数关闭socket
}

// 初始化服务端的socket，port为通信端口 (创建,绑定,监听)
bool CTcpServer::InitServer(int port)
{
    if (m_listenfd != 0)
    {
        close(m_listenfd);
        m_listenfd = 0;
    }

    // 1. 创建一个用于监听的socket
    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenfd == -1)
    {
        perror("socket");
        return false;
    }

    struct sockaddr_in servaddr; // 服务端地址信息的数据结构
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;                // 协议族，在socket编程中只能是AF_INET
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 本主机的任意ip地址
    servaddr.sin_port = htons(port);              // 用于绑定的通信端口

    // 2. 将这个用于监听的socket文件描述符和本地的IP和端口绑定
    // 客户端连接服务器的时候使用的就是这个IP和端口
    int ret = bind(m_listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (ret == -1)
    {
        perror("bind");
        close(m_listenfd);
        m_listenfd = 0;
        return false;
    }

    // 3. 监听
    int backlog = 128; // 客户端请求队列大小
    ret = listen(m_listenfd, backlog);
    if (ret == -1)
    {
        perror("listen");
        close(m_listenfd);
        m_listenfd = 0;
        return false;
    }

    return true;
}

int CTcpServer::Send(int clientfd, const void *buf, const int buflen)
{
    return send(clientfd, buf, buflen, 0);
}

int CTcpServer::Recv(int clientfd, void *buf, const int buflen)
{
    return recv(clientfd, buf, buflen, 0);
}

void CTcpServer::CloseListen() // 关闭用于监听的socket
{
    if (m_listenfd != 0)
    {
        close(m_listenfd);
        m_listenfd = 0;
    }
}

#endif