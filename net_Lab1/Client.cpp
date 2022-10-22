#include <windows.h>
#include <iostream>
#include <process.h>
#include <string>
using namespace std;
#pragma warning( disable : 4996 )
#pragma comment(lib, "ws2_32.lib")
const unsigned short chatport = 60000;//固定聊天端口号

//设计消息格式:注服务器端消息格式和客户端消息格式不同需要区分
//给不同的客户端建立标签
struct message {
	char id = '0';//代表的客户端服务端的id号
	bool online;//针对客户端表示是否在线, 针对服务端表示聊天室是否正常运行
	char Mtime[32] = { NULL };
	char msg[1024] = { 0 };
};

char* msg2char(message m) {
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
	m.id = '0';
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


void ReceiveMsg(void* param)
{
	while (1)
	{
		//客户端接受来自服务器的数据
		//	1. 服务器发送的聊天内容
		//	2. 服务器传递的其它客户端的内容
		SOCKET clientSocket = *(SOCKET*)(param);
		char  recvbuf[2048] = {};		//接收缓冲区
		//接受的数据是char类型的,需要转化为msg类型的数据
		//char(+time +online) -> string -> message =>输出
		if (recv(clientSocket, recvbuf, 2048, 0) == SOCKET_ERROR)
		{
			cout << "【系统消息】服务端掉线了{{{(>_<)}}}" << endl;
			cout << "【系统消息】程序将在3秒后退出" << endl;
			Sleep(3000);
			exit(100);
		}
		else {
			if (strlen(recvbuf)!=0) {
				message m = receivechar2msg(recvbuf);
				if (m.id == '0') {
					//1. 服务器发送的聊天
					cout << m.Mtime << " 【服务器(*^_^*)】:" << m.msg << endl;
					if (m.online == false) {
						cout << "【系统消息】聊天室关闭, 欢迎下次闲聊byebye~\(≧▽≦)/~" << endl;
						cout << "【系统消息】客户端退出, 程序将在3秒后退出！" << endl;
						Sleep(3000);
						exit(100);
					}
				}
				if (m.id != '0') {
					//2. 服务器转发的其它客户端的内容
					cout << m.Mtime << " 【客户端 "<<m.id<<" (*^_^*)】:" << m.msg << endl;
					if (m.online == false) {
						cout << "【系统消息】客户端 " << m.id << " 下线了, 欢迎下次闲聊byebye~\(≧▽≦)/~" << endl;
					}
				}
			}
		}
	}
}

void SendMsg(void* param)
{
	//客户端发送数据给服务器
	SOCKET clientSocket = *(SOCKET*)(param);
	char initbuf[2048] = "【新人入室】大家好呀~ 很高兴能和大家闲聊 =)";
	char* sendmsg;		//发送缓冲区
	message m = sendchar2msg(initbuf);
	sendmsg = msg2char(m);
	if (send(clientSocket, sendmsg, strlen(sendmsg), 0) == SOCKET_ERROR)
	{
		cout << "发送消息失败！";
	}
	else {
		cout << m.Mtime << " 【客户端我\\ ^ o ^ / 】:" << m.msg << endl;//标识自己发出的消息
	}
	while (1)
	{
		char sendbuf[2048] = {};		//发送缓冲区
		cin.getline(sendbuf, 2048);
		//发送的数据是char类型的
		//char* +time +online -> char*
		char* sendmsg;		//发送缓冲区
		message m = sendchar2msg(sendbuf);
		sendmsg = msg2char(m);
		if (send(clientSocket, sendmsg, strlen(sendmsg), 0) == SOCKET_ERROR)
		{
			cout << "发送消息失败！";
		}
		else{	
			cout << m.Mtime << " 【客户端我\\ ^ o ^ / 】:" << m.msg << endl;//标识自己发出的消息
			if (m.online == false) {
				cout << "【系统消息】欢迎下次闲聊byebye~\(≧▽≦)/~" << endl;
				cout << "【系统消息】客户端退出, 程序将在3秒后退出！" << endl;
				Sleep(3000);
				exit(100);
			}
		}
	}
}

int main()
{
	cout << "-------------------客户端上线啦---------------------------" << endl;
	cout << "-------------恭喜你成功进入闲聊室ヾ(≧▽≦*)o-------------" << endl;
	cout << " _          _   _                           _       " << endl
		<< "| |        | | ( )                         (_)      " << endl
		<< "| |     ___| |_|/ ___    __ _  ___  ___ ___ _ _ __  " << endl
		<< "| |    / _ \\ __| / __|  / _` |/ _ \\/ __/ __| | '_ \\ " << endl
		<< "| |___|  __/ |_  \\__ \\ | (_| | (_) \\__ \\__ \\ | |_) |" << endl
		<< "|______\\___|\\__| |___/  \\__, |\\___/|___/___/_| .__/ " << endl
		<< "                         __/ |               | |    " << endl
		<< "                        |___/                |_|    " << endl;
	cout << "-----------------------------------------------------------" << endl;
	WSADATA  wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		cout << "套接字初始化失败!" << endl;
	}
	SOCKET clientSocket;
	if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
	{
		cout << "套接字创建失败!" << endl;
	}
	struct sockaddr_in SeverAddress;		//服务器地址 也就是即将要连接的目标地址
	memset(&SeverAddress, 0, sizeof(sockaddr_in));
	SeverAddress.sin_family = AF_INET;
	SeverAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  //127.0.0.1表示本机ip地址
	SeverAddress.sin_port = htons(60000);//设定端口号

	//连接
	if (connect(clientSocket, (sockaddr*)&SeverAddress, sizeof(SeverAddress)) == SOCKET_ERROR)
	{
		cout << "啊欧和服务器连接失败了！别灰心再试一次" << endl;
		return 0;
	}
	else
		cout << "成功进入闲聊室内, 跟大家打个招呼吧o(*≧▽≦)ツ嗨" << endl;

	//创建两个子线程
	_beginthread(ReceiveMsg, 0, &clientSocket);
	_beginthread(SendMsg, 0, &clientSocket);

	while (1) {}	//这里采用另外一种技术避免主线程执行完退出——使其无限期休眠
	//	关闭socket
	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
	}

	//	终止
	WSACleanup();
	cout << "客户端退出！" << endl;
	return 0;
}
