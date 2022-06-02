// 客户端

#include "client.h"

CTcpClient TcpClient; // 创建一个TCP客户端

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("按该格式运行 : %s prot_number role_number(0 : producer, 1 : consumer)\n", basename(argv[0]));
        exit(-1);
    }

    //获取端口号
    int port = atoi(argv[1]);
    // 获取角色信息, 0代表生产者, 1代表消费者
    int role = atoi(argv[2]);

    if (!(role == 0 || role == 1))
    {
        printf("程序启动失败: 角色信息错误. 0为生产者, 1为消费者.\n");
        exit(-1);
    }

    // 1. 创建客户端socket
    if (TcpClient.InitClient() == false)
    {
        printf("客户端初始化失败，程序退出...\n");
        return -1;
    }

    // 2. 连接服务器
    if (TcpClient.Connect(port) == false)
    {
        printf("连接服务器失败，程序退出...\n");
        return -1;
    }

    // 3. 与服务器通信
    char strbuffer[1024];
    int i = 0;
    while (true)
    {

        memset(strbuffer, 0, sizeof(strbuffer));

        if (role == 0)
        {
            // 第1位代表是生产者, 第七位到换行符之前为任务ID
            sprintf(strbuffer, "0 ID : %d\n", i++);
        }
        else if (role == 1)
        {
            sprintf(strbuffer, "1 请求任务\n");
        }

        if (TcpClient.Send(strbuffer, strlen(strbuffer)) <= 0)
            break;
        // printf("发送：%s\n", strbuffer);

        if (TcpClient.Recv(strbuffer, sizeof(strbuffer)) <= 0)
            break;
        printf("接收：%s\n", strbuffer);

        sleep(1);
    }
    printf("服务器已断开连接...\n");
    return 0;
}