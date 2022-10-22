#include <iostream>  
#include <windows.h> 
#include <process.h> //线程处理头文件
#include <string.h>
#include <time.h>
using namespace std;
#pragma warning( disable : 4996 )
#pragma comment(lib, "WS2_32.lib")  
#define MAX_CLIENT_NUM 9
//显示加载 ws2_32.dll	ws2_32.dll就是最新socket版本
const unsigned short chatport = 60000;//固定聊天端口号
int clientnum = 0;//标记聊天的客户端id
SOCKET client[MAX_CLIENT_NUM];

//设计消息格式:注服务器端消息格式和客户端消息格式不同需要区分
//给不同的客户端建立标签
struct message {
	char id = '0';//代表的客户端服务端的id号
	bool online;//针对客户端表示是否在线, 针对服务端表示聊天室是否正常运行
	char Mtime[32] = { NULL };
	char msg[1024] = { 0 };
};

char* msg2char(message m){
	char charmsg[2048];
	memset(charmsg, 0, sizeof(charmsg));
	charmsg[0] = m.id;
	if (m.online == true) {
		charmsg[1] = '1';
	}
	else {
		charmsg[1] = '0';
	}
	strcat_s(charmsg, m.Mtime);
	strcat_s(charmsg, m.msg);
	return charmsg;
};

message sendchar2msg(char* buf) {
	message m;
	char tmp[32] = { NULL };
	time_t t = time(0);
	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&t));
	strcpy(m.Mtime, tmp);
	if (strcmp(buf, "exit") == 0) {
		m.online = false;
	}
	else {
		m.online = true;
	}
	strcpy(m.msg, buf);
	return m;
};

message receivechar2msg(char* buf) {
	message m;
	m.id = buf[0];
	if (buf[1] == '0') {
		m.online = false;
	}
	else {
		m.online = true;
	}
	strncpy(m.Mtime, buf + 2, 19);
	strcpy(m.msg, buf + 21);
	return m;
};

//接受消息的线程函数
void ReceiveMsg(LPVOID lpParameter)
{
	int receiveid = clientnum;//标识当前客户端
	while (1)
	{
		//服务器接受数据
		SOCKET revClientSocket = (SOCKET)lpParameter;
		char recvbuf[2048] = {};		//接收缓冲区
		//接受的数据是char类型的,需要转化为msg类型的数据
		//char(+time +online) -> string -> message =>输出
		if (recv(revClientSocket, recvbuf, 2048, 0) == SOCKET_ERROR)
		{
			cout << WSAGetLastError() << endl;
			cout << "【系统消息】数据接收失败, 客户端 "<< receiveid<<" 掉线了{{{(>_<)}}}" << endl;
			/*cout << "程序将在3秒后退出" << endl;
			Sleep(3000);
			exit(100);*/
		}
		else {
			if (strlen(recvbuf) != 0) {
				//服务器显示其它客户端的消息:
				message m = receivechar2msg(recvbuf);
				cout << m.Mtime << " 【客户端 " << receiveid << " (*^_^*)】:" << m.msg << endl;
				//服务器向其它客户端转发内容
				m.id = receiveid + 48;
				char* sendmsg1 = msg2char(m);
				for (int i = 0; i < MAX_CLIENT_NUM; i++) {
					if (client[i] != NULL && i != receiveid - 1) {
						SOCKET revClient = client[i];
						if (send(revClient, sendmsg1, strlen(sendmsg1), 0) == SOCKET_ERROR)
						{
							cout << "【系统消息】发送消息失败！" << endl;
						}
					}
				}
				if (m.online == false) {
					cout << "【系统消息】客户端 " << receiveid << " 下线了, 欢迎下次闲聊byebye~\(≧▽≦)/~" << endl;
					client[receiveid - 1] = NULL;
					break;
				}
			}
		}
	}
}

//发送数据的线程函数
void SendMsg(LPVOID lpParameter)
{	
	//服务器发送的聊天内容:
	while (1)
	{
		//服务器发送数据
		SOCKET revClientSocket = (SOCKET)lpParameter;
		char sendbuf[2048] = {};		//输入缓冲区
		cin.getline(sendbuf, 2048);
		//发送的数据是char类型的
		//char* +time +online -> char*
		char *sendmsg;		//发送缓冲区
		message m = sendchar2msg(sendbuf);
		m.id = '0';
		sendmsg = msg2char(m);
		cout << m.Mtime << " 【服务器\\^o^/】:" << m.msg << endl;
		for (int i = 0; i < MAX_CLIENT_NUM; i++) {
			if (client[i] != NULL) {
				SOCKET revClient = client[i];
				if (send(revClient, sendmsg, strlen(sendmsg), 0) == SOCKET_ERROR)
				{
					cout << "发送消息失败！" << endl;
				}
			}
		}
		if (m.online == false) {
			cout << "聊天室关闭, 欢迎下次闲聊byebye~\(≧▽≦)/~" << endl;
			cout << "服务端退出, 程序将在3秒后退出！" << endl;
			Sleep(3000);
			exit(100);
		}
	}
}

//统一收发数据的线程函数
void chat(void* param) {
	_beginthread(ReceiveMsg, 0, param);
	_beginthread(SendMsg, 0, param);
}

int main()
{
	cout << "-------------------服务端上线啦---------------------------" << endl;
	cout << "-------------恭喜你创建成功闲聊室ヾ(≧▽≦*)o-------------" << endl;
	cout << " _          _   _                           _       " << endl
		<< "| |        | | ( )                         (_)      " << endl
		<< "| |     ___| |_|/ ___    __ _  ___  ___ ___ _ _ __  " << endl
		<< "| |    / _ \\ __| / __|  / _` |/ _ \\/ __/ __| | '_ \\ " << endl
		<< "| |___|  __/ |_  \\__ \\ | (_| | (_) \\__ \\__ \\ | |_) |" << endl
		<< "|______\\___|\\__| |___/  \\__, |\\___/|___/___/_| .__/ " << endl
		<< "                         __/ |               | |    " << endl
		<< "                        |___/                |_|    " << endl;
	cout << "-----------------------------------------------------------" << endl;

	/*1. 套接字初始化*/
	WSADATA wsaData;
	//这个结构被用来存储被WSAStartup函数调用后返回的 Windows Sockets 数据。
	WORD sockVersion = MAKEWORD(2, 2);
	//windows网络编程库的版本号信息, 声明采用2.2版本
	if (WSAStartup(sockVersion, &wsaData) != 0) //初始化套字节
	{
		cout << "套接字初始化失败!" << endl;
		return 0;
	}

	/*2. 创建服务器套接字*/
	SOCKET SeverSocket;
	if ((SeverSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
	{
		cout << "套接字创建失败！" << endl;
		return 0;
	}

	/*3. 绑定套接字*/
	struct sockaddr_in SeverAddress;		//一个绑定地址:有IP地址,有端口号,有协议族
	memset(&SeverAddress, 0, sizeof(sockaddr_in)); //初始化结构体
	SeverAddress.sin_family = AF_INET; //指定ip结构
	SeverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//本机IP地址 
	SeverAddress.sin_port = htons(60000);//设定端口号

		//指定绑定的IP地址和端口号
	if (bind(SeverSocket, (sockaddr*)&SeverAddress, sizeof(SeverAddress)) == SOCKET_ERROR)
	{
		cout << "套接字绑定失败！" << endl;
		return 0;
	}


	/*4. 服务器监听*/
	if (listen(SeverSocket, SOMAXCONN) == SOCKET_ERROR) //其中SOMAXCONN用于能同时监听多少个请求
	{
		cout << "监听失败！" << endl;
		return 0;
	}
	else
		cout << "不要着急~ 服务器正在召唤聊天的小伙伴(/▽＼)" << endl;

	/*5. 服务器监听成功,连接请求*/
	while (1) {
		sockaddr_in revClientAddress;//套接字的地址，端口
		SOCKET revClientSocket = INVALID_SOCKET;//客户端连接到套字节
		int addlen = sizeof(revClientAddress);
		if ((revClientSocket = accept(SeverSocket, (sockaddr*)&revClientAddress, &addlen)) == INVALID_SOCKET)
		{
			cout << "啊欧接受客户端连接失败了！别灰心再试一次" << endl;
			return 0;
		}
		else
			cout << "【系统消息】找到一位一起聊天的小伙伴q(≧▽≦q)，跟他打个招呼吧!" << endl;
		client[clientnum++] = revClientSocket;
		cout << "【系统消息】客户端 " << clientnum << " 上线啦!" << endl;
		/*6. 创建线程,建立收发*/
		/*_beginthread(ReceiveMsg, 0, &revClientSocket);
		_beginthread(SendMsg, 0, &revClientSocket);*/
		chat((LPVOID)revClientSocket);

		//while (1) {}
		//为了避免主线程退出使子线程被迫消亡，使主线程进入循环

	}
	/*7. 关闭连接*/
	for (int i = 0; i < MAX_CLIENT_NUM; i++) {
		SOCKET ClientSocket = client[i];
		closesocket(ClientSocket);
	}

	closesocket(SeverSocket);
	/*8. 服务器停止*/
	WSACleanup();
	cout << "服务器停止！" << endl;
	return 0;
}
