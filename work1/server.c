#include<WinSock2.h>//winsock2的头文件
#include<stdio.h>


#pragma comment(lib, "ws2_32.lib")

#define MAX_LINK_NUMBER 10	

typedef struct Client
{
	SOCKET s;
	char username[20];	//客户的用户名
	int idx;
}Client;

void initClient(Client *client);
typedef struct sockaddr_in sockaddr_in;
typedef struct WSAData WSAData;
typedef struct sockaddr sockaddr;

unsigned linkCount = 0;	//当前的连接数

Client clients[MAX_LINK_NUMBER]; //管理的客户


DWORD WINAPI AnswerThread(LPVOID lparam);


int main()
{

	//加载winsock2的环境
	WSADATA wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0)
	{
		printf("WSAStartup error\n");
		return 1;
	}

	//创建流式套接字
	SOCKET welcome = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (welcome == INVALID_SOCKET)
	{
		printf("create socket error\n");
		return 1;
	}

	//bind
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1234);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(welcome, (SOCKADDR*)&addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		printf("bind error\n");
		return 1;
	}

	//listen
	if (listen(welcome, 5) == SOCKET_ERROR)
	{
		printf("listen error\n");
		return 1;
	}


	//初始化结构体数组
	for (int i = 0; i < MAX_LINK_NUMBER; ++i)
		initClient(clients + i);

	//等待连接
	sockaddr_in remoteAddr;
	memset(&remoteAddr, 0, sizeof(remoteAddr));
	int len = sizeof(remoteAddr);
	printf("初始化完成，等待连接\n");
	while (1)
	{
		if (linkCount >= MAX_LINK_NUMBER) continue;
		Client *client = &clients[linkCount];
		client->s = accept(welcome, (SOCKADDR*)&remoteAddr, &len);
		if (client->s == INVALID_SOCKET)
		{
			printf("accept error\n");
			continue;
		}
		//接收到连接
		client->idx = linkCount;
		recv(client->s, client->username, sizeof(client->username), 0);	//接收用户名
		printf("接收到一个连接,IP:%s port:%d username:%s\n", inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port), client->username);



		//单独开启线程处理与客户端的连接
		DWORD dwThreadId;
		HANDLE hThread;
		hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)AnswerThread, client, 0, &dwThreadId);
		if (hThread == NULL){	//线程创建失败，连接取消
			printf("线程创建出错\n");
			continue;
		}
		else{
			printf("线程创建成功,服务器继续等待新的连接......\n");
			CloseHandle(hThread);	//关闭句柄，并不是结束线程
		}
		
		++linkCount;
	}

	//关闭监听套接字
	closesocket(welcome);

	//清理winsock2的环境
	WSACleanup();



	return 0;
}

//接收和转发
DWORD WINAPI AnswerThread(LPVOID lparam)
{
	printf("线程%d开始处理\n", GetCurrentThreadId());

	Client *client = (Client*)lparam;
	char recvData[128] = {0};
	char buffer[256] = {0};
	char colon[] = ":";

	while (1)	
	{
		int recvlen = recv(client->s, recvData, sizeof(recvData), 0);
		if (recvlen > 0)
		{
			if (strcmp(recvData, "quit") == 0)
			{
				break;
			}
			else{
				strcpy(buffer, client->username);
				strcat(buffer, colon);
				strcat(buffer, recvData);
				//给发送者之外的所有客户发送
				for (int i = 0; i < linkCount; ++i)
				{
					if (i != client->idx)
						send(clients[i].s, buffer, sizeof(buffer), 0);
				}
			}
		}
		else
		{
			printf("接收信息失败\n");
			printf("%d\n", GetLastError());
		}
	}

	closesocket(client->s);
	printf("线程号%d结束\n", GetCurrentThreadId());
	return 0;
}

void initClient(Client *client)
{
	client->idx = -1;
	client->s = INVALID_SOCKET;
	memset(&client->username, 0, sizeof(client->username));
}