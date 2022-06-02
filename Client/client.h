// TCP通信客户端类
#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

class CTcpClient
{
public:
    int m_fd; // 客户端socket文件描述符

    CTcpClient();

    bool InitClient(); // 初始化服务器 (创建socket)

    bool Connect(int port); // 连接服务器

    // 向对端发送报文
    int Send(const void *buf, const int buflen);
    // 接收对端的报文
    int Recv(void *buf, const int buflen);

    void CloseClient(); // 关闭用于连接的socket

    ~CTcpClient();
};

CTcpClient::CTcpClient()
{
    m_fd = 0;
}

CTcpClient::~CTcpClient()
{
    if (m_fd != 0)
        close(m_fd);
}

bool CTcpClient::InitClient()
{
    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd == -1)
    {
        perror("socket");
        return false;
    }

    return true;
}

bool CTcpClient::Connect(int port)
{
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr.s_addr);
    servaddr.sin_port = htons(port);

    int ret = connect(m_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    if (ret == -1)
    {
        perror("connect");
        return false;
    }

    return true;
}

int CTcpClient::Send(const void *buf, const int buflen)
{
    return send(m_fd, buf, buflen, 0);
}

int CTcpClient::Recv(void *buf, const int buflen)
{
    return recv(m_fd, buf, buflen, 0);
}

void CTcpClient::CloseClient() // 关闭socket
{
    if (m_fd != 0)
    {
        close(m_fd);
        m_fd = 0;
    }
}

#endif