/*
Author: xiaoduo
Date: 2022/12/1
Description: 计网实验3-2基于滑动窗口机制的可靠UDP传输协议客户端Client实现
*/
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")//链接ws2_32.lib库文件
#pragma warning(disable:4996)

#include <WS2tcpip.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <queue>
using namespace  std;

const char* CLIENT_IP = "127.0.0.1";
int CLIENT_PORT = 9001;
const unsigned char FIRST_SHAKE = 0x01;
const unsigned char SECOND_SHAKE = 0x02;
const unsigned char THIRD_SHAKE = 0x03;
const unsigned char FIRST_WAVE = 0x04;
const unsigned char SECOND_WAVE = 0x05;
const unsigned char ACK = 0x06;
const unsigned char NAK = 0x07;
const unsigned char END = 0x08;
const unsigned char NOT_END = 0x09;

const int TIMEOUT = 200;//超时时间设置2秒
const int BUFFER_SIZE = 0x3A90;
const int MAX_UDP_LEN = 5000;
const int WINDOW_SIZE = 7;
char buffer[100000000];
int len = 0;

SOCKET m_ClientSocket;
SOCKADDR_IN m_ServerAddress; //远程地址
SOCKADDR_IN m_ClientAddress; //客户端地址

//检验校验和
unsigned char check_sum(char* package, int len)
{
	if (len == 0) {
		return ~(0);
	}
	unsigned long sum = 0;
	int i = 0;
	while (len--) {
		sum += (unsigned char)package[i++];
		if (sum & 0xFF00) {
			sum &= 0x00FF;
			sum++;
		}
	}
	return ~(sum & 0x00FF);
}

// 发送单个数据包
void send_single_package(char* message, int length, int order, int isLast = 0) {
	char* sendBuf;
	int lenofsend;

	if (isLast) {
		//协议设计格式
		/*
		struct msgHead {
			unsigned char checknum;
			unsigned char flag;//标志位
			unsigned char seq; //表示数据包的序列号, 一个字节为周期
			int length; //表示数据包的长度（包含头）
		};
		*/
		sendBuf = new char[length + 4];
		sendBuf[1] = END;
		sendBuf[2] = order;
		sendBuf[3] = length;

		for (int i = 4; i < length + 4; i++)
			sendBuf[i] = message[i - 4];
		sendBuf[0] = check_sum(sendBuf + 1, length + 3);
		lenofsend = length + 4;
	}
	else {
		sendBuf = new char[length + 3];
		sendBuf[1] = NOT_END;
		sendBuf[2] = order;
		for (int i = 3; i < length + 3; i++)
			sendBuf[i] = message[i - 3];
		sendBuf[0] = check_sum(sendBuf + 1, length + 2);
		lenofsend = length + 3;
	}
	sendto(m_ClientSocket, sendBuf, lenofsend, 0, (sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
}
//发送数据函数
//涉及到滑动窗口和累计确认机制
/*              发送方的可用窗口
			    |
+ + + + + + + + [+ + + + + + + + ] + + + + + + + +
                |          | |     
               base has_send next   
*/
void send_msg(char* message, int length) {
	int timeout_num = 0;
	//1. 确定窗口的base值
	static int base = 0;
	//2. 确定当前传输到的seq
	int has_send = 0;
	int next_package = base;
	//3. 确定已经确认的seq
	int ack_send = 0;
	int package_num = length / MAX_UDP_LEN + (length % MAX_UDP_LEN != 0);
	//4. 将包保存到窗口当中
	queue<int> window;
	while (1) {
		//确认完毕所有数据包
		if (ack_send == package_num)
			break;
		//发送窗口
		while (window.size() < WINDOW_SIZE && has_send != package_num) {
			send_single_package(message + has_send * MAX_UDP_LEN,
				has_send == package_num - 1 ? length - (package_num - 1) * MAX_UDP_LEN : MAX_UDP_LEN,
				next_package % ((int)UCHAR_MAX + 1),
				has_send == package_num - 1);
			window.push(next_package % ((int)UCHAR_MAX + 1));
			//将当前数据包的放入窗口中
			printf("%s%d\n", "sending seq:", next_package % ((int)UCHAR_MAX + 1));
			printf("%s%d\n", "发送窗口大小:", WINDOW_SIZE - window.size());
			next_package++;
			has_send++;
		}
		char recv[3];
		int lentmp = sizeof(m_ServerAddress);
		//cout << "正在监听" << endl;
		setsockopt(m_ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&TIMEOUT, sizeof(int));
		if (recvfrom(m_ClientSocket, recv, 3, 0, (sockaddr*)&m_ServerAddress, &lentmp) != SOCKET_ERROR && check_sum(recv, 3) == 0 &&
			recv[1] == ACK ) {
			//接收端返回最后一个确认的数据包, 在此之前的都已经确认
			while (window.front() < (unsigned char)recv[2]) {
				printf("%s%d\n", "已经确认: ", (unsigned char)recv[2]);
				//确认包数+1
				//窗口base+1
				ack_send++;
				base++;
				window.pop();
			}
			printf("%s%d\n", "已经确认: ", (unsigned char)recv[2]);
			ack_send++;
			base++;
			timeout_num = 0;
			window.pop();
		}
		else {
			next_package = base;
			timeout_num++;
			has_send -= window.size();//将发送数据包回退
			while (!window.empty()) window.pop();
		}
		//同一个包丢5次的处理模式
		if (timeout_num >= 5) {
			return;
		}
	}
}

int main() {
	int m_RemoteAddressLen;

	// socket环境
	WSADATA  wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup error:" << GetLastError() << endl;
		return false;
	}

	// socket对象
	m_ClientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_ClientSocket == INVALID_SOCKET)
	{
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
		return false;
	}

	// 远端地址
	const char* ip = "127.0.0.1";
	// const char* ip = "192.168.0.1";
	int	port = 10000;
	m_ClientAddress.sin_family = AF_INET;
	m_ClientAddress.sin_addr.S_un.S_addr = inet_addr(CLIENT_IP);
	m_ClientAddress.sin_port = htons(CLIENT_PORT);
	auto ret = bind(m_ClientSocket, (sockaddr*)&m_ClientAddress, sizeof(SOCKADDR));
	if (ret == SOCKET_ERROR)
	{
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
		return false;
	}

	m_ServerAddress.sin_family = AF_INET;
	m_ServerAddress.sin_port = htons(port);
	m_RemoteAddressLen = sizeof(m_ServerAddress);
	inet_pton(AF_INET, ip, &m_ServerAddress.sin_addr);

	printf("成功启动客户端, 开始与服务端握手...\n");
	cout << "-----------------------------------------------------------------" << endl;
	// 接收和发送
	/*char recvBuf[1024] = { 0 };
	char sendBuf[1024] = "Nice to meet you!";*/
	//非阻塞模式设置
	/*unsigned long ul = 1;
	int reddt = ioctlsocket(m_ClientSocket, FIONBIO, (unsigned long*)&ul);
	if (reddt == SOCKET_ERROR)
	{
		closesocket(m_ClientSocket);
		return 1;
	}*/
	//三次握手建立连接
	while (1)
	{
		char sendBuf[2];
		sendBuf[1] = FIRST_SHAKE;
		sendBuf[0] = check_sum(sendBuf + 1, 1);
		int ret = sendto(m_ClientSocket, sendBuf, 2, 0, (sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
		if (ret != SOCKET_ERROR)
		{
			printf("%s\n", "成功发送第一次握手SYN请求!");
		}
		else
			return 0;
		//开始计时
		int begin = clock();
		char recv[2];
		int len = sizeof(m_ServerAddress);
		int fail = 0;
		//超时重发
		setsockopt(m_ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&TIMEOUT, sizeof(int));
		while (recvfrom(m_ClientSocket, recv, 2, 0, (sockaddr*)&m_ServerAddress, &len) == SOCKET_ERROR)
		{
			if (clock() - begin > TIMEOUT)
			{
				fail = 1;
				break;
			}
		}
		//接收成功第一次握手ACK, 向服务端第二次握手SYN发送ACK
		if (fail == 0 && check_sum(recv, 2) == 0 && recv[1] == SECOND_SHAKE)
		{
			printf("%s\n", "成功接收服务端SYN_ACK!");
			sendBuf[1] = THIRD_SHAKE;
			sendBuf[0] = check_sum(sendBuf + 1, 1);
			sendto(m_ClientSocket, sendBuf, 2, 0, (sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
			printf("%s\n", "发送第三次握手ACK!");
			break;
		}
	}
	printf("三次握手成功连接服务端\n");
	cout << "-----------------------------------------------------------------" << endl;

	// 下面开始发送文件
	printf("开始发送文件\n");
	string filename;
	while (1) {
		printf("输入要发送文件名：");
		cin >> filename;
		ifstream fin(filename.c_str(), ifstream::binary);
		if (!fin) {
			printf("文件找不到\n");
			continue;
		}
		unsigned char t = fin.get();
		while (fin) {
			buffer[len++] = t;
			t = fin.get();
		}
		fin.close();
		break;
	}
	//先发送文件名
	send_msg((char*)(filename.c_str()), filename.length());
	//然后发送具体的文件内容
	int time_begin = clock();
	send_msg(buffer, len);
	int time_end = clock();
	printf("文件发送成功\n");
	int runtime = (time_end - time_begin) / CLOCKS_PER_SEC;
	printf("文件传输时间 %d s\n", runtime);
	double kbps = ((len * 8.0) / 1000) / runtime;
	printf("吞吐量: %f kbps\n", kbps);

	cout << "-----------------------------------------------------------------" << endl;
	printf("开始断开连接\n");
	while (1)
	{
		char sendBuf[2];
		sendBuf[1] = FIRST_WAVE;
		sendBuf[0] = check_sum(sendBuf + 1, 1);
		sendto(m_ClientSocket, sendBuf, 2, 0, (sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
		printf("%s\n", "客户端发送挥手消息");
		int begin = clock();
		char recv[2];
		int len = sizeof(m_ServerAddress);
		int fail = 0;
		//超时重传
		while (recvfrom(m_ClientSocket, recv, 2, 0, (sockaddr*)&m_ServerAddress, &len) == SOCKET_ERROR)
		{
			if (clock() - begin > TIMEOUT)
			{
				fail = 1;
				break;
			}
		};
		if (fail == 0 && check_sum(recv, 2) == 0 && recv[1] == SECOND_WAVE)
		{
			printf("%s\n", "接收第二次挥手");
			break;
		}
	}
	printf("挥手成功, 断开连接\n");
	closesocket(m_ClientSocket);
	WSACleanup();
	system("pause");
	return 0;
}
