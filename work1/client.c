#include <WinSock2.h>
#include <stdio.h>
#include <Windows.h>
#pragma comment(lib,"ws2_32.lib")

int Finished = 0;
char username[20] = {0};

typedef struct sockaddr_in sockaddr_in;
typedef struct WSAData WSAData;
typedef struct sockaddr sockaddr;

unsigned ThreadSend(void* param);
unsigned ThreadRecv(void *param);

void input(char *str, int len);

int main()
{
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(sockVersion, &data) != 0)
    {
        printf("WSAStartup error\n");
        return 1;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)   
    {
        printf("invalid socket\n");
        return 1;
    }

    //目标IP和端口
    sockaddr_in t_addr;
    memset(&t_addr, 0, sizeof(t_addr));
    t_addr.sin_family = AF_INET;
    t_addr.sin_port = htons(1234);
    t_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("connecting......\n");

    if (connect(s, (SOCKADDR*)&t_addr, sizeof(t_addr)))
    {
        printf("connect error\n");
        return 1;
    }

    printf("连接成功\n");
    printf("Please input your username : ");
    
    input(username, sizeof(username));
    // printf("我的用户名为%s\n", username);
    if (send(s, username, sizeof(username), 0) == SOCKET_ERROR)
    {
        printf("提交用户名错误\n");
        return 1;
    }

    //开启接收和发送线程
    HANDLE hThread_send, hThread_recv;
    DWORD dwThread_send, dwThread_recv;

    hThread_send = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadSend, &s, 0, &dwThread_send);
    hThread_recv = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadRecv, &s, 0, &dwThread_recv);

    while (!Finished)
    {}

    CloseHandle(hThread_send);
    CloseHandle(hThread_recv);
    
    closesocket(s);
    WSACleanup();
    return 0;
}

unsigned ThreadSend(void *param)
{
    char buf[128] = {0};
    while (1)
    {
        input(buf, sizeof(buf));

        int sendlen = send(*(SOCKET*)param, buf, sizeof(buf), 0);
        if (sendlen == SOCKET_ERROR) printf("send message failed\n");
        if (strcmp(buf, "quit") == 0)
        {
            Finished = 1;
            return 1;
        }
    }


    return 0;
}

unsigned ThreadRecv(void *param)
{
    char buf[128] = {0};
    while (1)
    {
        int recvlen = recv(*(SOCKET*)param, buf, sizeof(buf), 0);
        if (recvlen == SOCKET_ERROR)
        {
            // printf("something missing\n");
            return 1;
        }
        else if (strlen(buf) > 0)
        {
            printf("%s\n", buf);
        }
    }

    return 0;
}


void input(char *str, int len)
{
    fgets(str, len, stdin);
    int plen = strlen(str);
    if (plen == 0)
    {
        printf("输入失败\n");
    }
    else
    {
        str[plen - 1] = '\0';
    }
}