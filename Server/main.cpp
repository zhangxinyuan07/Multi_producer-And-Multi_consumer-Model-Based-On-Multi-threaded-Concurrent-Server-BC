// 任务管理中心服务器
#include "server.h"
#include "worker.h"
#include <unistd.h>
#include <pthread.h>
#include <string.h>

using namespace std;

CTcpServer TcpServer;

int maxsize = 8; // 任务队列最大容量

CTaskQueue<int> TaskManager(maxsize); // 创建任务管理中心
CProducerWaiter<int> Producer;        // 创建生产者
CConsumerWaiter<int> Consumer;        // 创建消费者

// 这是每个连接进来的客户端的信息
struct sockInfo
{
    int fd; // 通信的文件描述符
    struct sockaddr_in addr;
    pthread_t tid; // 线程号
};

struct sockInfo sockinfos[128]; // 最多同时创建128个子线程连接客户端

// 用于与客户端通信的子线程的工作函数
void *working(void *arg);

// 与生产者通信的内容
bool comWithProducer(int cfd, char *comBuffer);

// 与消费者通信的内容
bool comWithConsumer(int cfd, char *comBuffer);

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        printf("按照如下格式运行 : %s prot_number\n", basename(argv[0]));
        exit(-1);
    }

    //获取端口号
    int port = atoi(argv[1]);

    // 1. 初始化服务器(socket创建, 绑定, 监听)
    if (TcpServer.InitServer(port) == false)
    {
        printf("服务端初始化失败，程序退出。\n");
        return -1;
    }

    // 初始化数据
    int max = sizeof(sockinfos) / sizeof(sockinfos[0]);
    for (int i = 0; i < max; i++)
    {
        bzero(&sockinfos[i], sizeof(sockinfos[i]));
        sockinfos[i].fd = -1;
        sockinfos[i].tid = -1;
    }

    // 不断循环等待客户端连接，一旦一个客户端连接进来，就创建一个子线程进行通信
    while (1)
    {
        struct sockaddr_in cliaddr;
        int len = sizeof(cliaddr);
        // 接受连接
        int cfd = accept(TcpServer.m_listenfd, (struct sockaddr *)&cliaddr, (socklen_t *)&len);

        struct sockInfo *pinfo;
        for (int i = 0; i < max; i++)
        {
            // 从这个数组中找到一个可以用的sockInfo元素
            if (sockinfos[i].fd == -1)
            {
                pinfo = &sockinfos[i];
                break;
            }
            if (i == max - 1)
            {
                sleep(1);
                i--;
            }
        }

        // 把客户端信息赋值给结构体
        pinfo->fd = cfd;
        memcpy(&pinfo->addr, &cliaddr, len);

        // 创建子线程
        pthread_create(&pinfo->tid, NULL, working, pinfo);

        pthread_detach(pinfo->tid);
    }

    TcpServer.CloseListen();

    return 0;
}

void *working(void *arg)
{
    struct sockInfo *pinfo = (struct sockInfo *)arg;

    char cliIp[16];
    inet_ntop(AF_INET, &pinfo->addr.sin_addr.s_addr, cliIp, sizeof(cliIp));
    unsigned short cliPort = ntohs(pinfo->addr.sin_port);
    printf("客户端IP : %s, 端口 : %d\n", cliIp, cliPort);

    // 与客户端通信
    char comBuffer[1024];

    while (1)
    {
        // sleep(1);
        memset(comBuffer, 0, sizeof(comBuffer) / sizeof(char));

        // 接收数据
        if (TcpServer.Recv(pinfo->fd, comBuffer, sizeof(comBuffer)) <= 0)
            break;
        // printf("接收：%s\n", comBuffer);

        // 把接收到的数据转交给生产者或消费者去处理
        // comBuffer的第0位用来判断是生产者('1')还是消费者('0')
        if (comBuffer[0] == '0')
        {
            // 转交给生产者服务员
            if (comWithProducer(pinfo->fd, comBuffer) == false)
                break;
        }
        else if (comBuffer[0] == '1')
        {
            // 转交给消费者服务员
            if (comWithConsumer(pinfo->fd, comBuffer) == false)
                break;
        }
        else
        {
            // 格式有误, 无法识别该发送给消费者还是生产者
            sprintf(comBuffer, "客户端数据格式错误, 请重新发送...\n");
            if (TcpServer.Send(pinfo->fd, comBuffer, strlen(comBuffer) + 1) <= 0)
                break;
        }
    }

    printf("客户端已断开连接...\n");
    close(pinfo->fd);
    return NULL;
}

bool comWithProducer(int cfd, char *comBuffer)
{
    // 生产者接收到一个任务消息, 从中提取出任务ID, 把任务加到任务队列里去
    // 第七位开始到换行符前为任务ID
    int len = 0; // 任务ID的位数
    for (int i = 7; i < strlen(comBuffer); i++)
    {
        if (comBuffer[i] == '\n')
        {
            break;
        }
        else
        {
            len++;
        }
    }
    char tmp[len];
    strncpy(tmp, comBuffer + 7, len);
    int taskItem = atoi(tmp);                 // 任务ID提取完毕
    Producer.add_task(TaskManager, taskItem); // 生产任务
    cout << "已收到任务ID : " << taskItem << endl;

    // 处理完以后, 给生产者客户端回复一条信息
    sprintf(comBuffer, "已接收生产者任务\n");
    if (TcpServer.Send(cfd, comBuffer, strlen(comBuffer) + 1) <= 0)
        return false;
    // printf("发送：%s\n", comBuffer);
    return true;
}

bool comWithConsumer(int cfd, char *comBuffer)
{
    // 消费者接收到请求任务的消息, 从任务队列里取出任务发送给客户端
    int taskItem = Consumer.consumeProduct(TaskManager);
    sprintf(comBuffer, "任务ID : %d\n", taskItem);
    if (TcpServer.Send(cfd, comBuffer, strlen(comBuffer) + 1) <= 0)
        return false;
    cout << "已发送任务ID : " << taskItem << endl;
    return true;
}