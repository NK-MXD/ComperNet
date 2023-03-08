/*
Author: xiaoduo
Date: 2022/11/18
Description: 计网实验3-1基于停等机制的可靠UDP传输协议服务端Server实现
*/
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")//链接ws2_32.lib库文件
#pragma warning(disable:4996)

#include <WS2tcpip.h>
#include <iostream>
#include <queue>
#include <string>
#include <fstream>
#include <time.h>
using namespace std;

const unsigned char FIRST_SHAKE = 0x01;
const unsigned char SECOND_SHAKE = 0x02;
const unsigned char THIRD_SHAKE = 0x03;
const unsigned char FIRST_WAVE = 0x04;
const unsigned char SECOND_WAVE = 0x05;
const unsigned char ACK = 0x06;
const unsigned char NAK = 0x07;
const unsigned char END = 0x08;
const unsigned char NOT_END = 0x09;

const int TIMEOUT = 500;
const int BUFFER_SIZE = 0x3A90;
const int MAX_UDP_LEN = 5000;
char buffer[100000000];
int len;
SOCKADDR_IN m_ClientAdress; //远程地址
SOCKET m_ServerSocket;

//设计报文格式
struct msgHead {
	unsigned char checknum;
	unsigned char flag;//标志位 
	unsigned char seq; //表示数据包的序列号, 一个字节为周期
	int length; //表示数据包的长度（包含头）
};

//计算校验和
//UDP检验和的计算方法是：
//1. 按每16位求和得出一个32位的数；
//2. 如果这个32位的数，高16位不为0，则高16位加低16位再得到一个32位的数；
//3. 重复第2步直到高16位为0，将低16位取反，得到校验和。

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

void recv_message(char* message, int& len_recv) {
	char recv[MAX_UDP_LEN + 4];
	int lentmp = sizeof(m_ClientAdress);
	//接收端的顺序
	static unsigned char receive_order = 0;
	len_recv = 0;
	while (1) {
		while (1) {
			memset(recv, 0, sizeof(recv));
			while (recvfrom(m_ServerSocket, recv, MAX_UDP_LEN + 4, 0, (sockaddr*)&m_ClientAdress, &lentmp) == SOCKET_ERROR);
			char send[3];
			int flag = 0;
			//发送ACK确认的序列号
			if (check_sum(recv, MAX_UDP_LEN + 4) == 0 && (unsigned char)recv[2] == receive_order) {
				receive_order++;
				flag = 1;
			}
			send[1] = ACK;
			send[2] = receive_order - 1;
			send[0] = check_sum(send + 1, 2);
			cout << "接收端确认序列号: " << receive_order - 1 << endl;
			sendto(m_ServerSocket, send, 3, 0, (sockaddr*)&m_ClientAdress, sizeof(m_ClientAdress));
			if (flag)
				break;
		}
		// 接收端将消息放入缓存区中
		if (END == recv[1]) {
			for (int i = 4; i < recv[3] + 4; i++)
				message[len_recv++] = recv[i];
			break;
		}
		else {
			for (int i = 3; i < MAX_UDP_LEN + 3; i++)
				message[len_recv++] = recv[i];
		}
	}
}

int main() {
	SOCKADDR_IN m_BindAddress;   //绑定地址
	int m_RemoteAddressLen;


	// socket环境
	WSADATA  wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup error:" << GetLastError() << endl;
		return false;
	}

	// socket对象
	m_ServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_ServerSocket == INVALID_SOCKET)
	{
		closesocket(m_ServerSocket);
		m_ServerSocket = INVALID_SOCKET;
		return false;
	}

	// 绑定占用<ip, port>
	const char* ip = "127.0.0.1";
	int port = 9000;
	m_BindAddress.sin_family = AF_INET;
	m_BindAddress.sin_addr.S_un.S_addr = inet_addr(ip);
	m_BindAddress.sin_port = htons(port);
	auto ret = bind(m_ServerSocket, (sockaddr*)&m_BindAddress, sizeof(SOCKADDR));
	if (ret == SOCKET_ERROR)
	{
		closesocket(m_ServerSocket);
		m_ServerSocket = INVALID_SOCKET;
		return false;
	}

	// 接收和发送
	/*char recvBuf[1024] = { 0 };
	char sendBuf[1024] = "Nice to meet you, too!";*/
	m_RemoteAddressLen = sizeof(m_ClientAdress);

	printf("服务端已设置绑定占用的连接, 其ip: %s, port: %d\n", inet_ntoa(m_BindAddress.sin_addr), ntohs(m_BindAddress.sin_port));
	printf("服务端等待握手\n");
	cout << "-----------------------------------------------------------------" << endl;

	//服务端与客户端建立连接
	while (1)
	{
		//三次握手实现
		char recv[2];
		int len_tmp = sizeof(m_ClientAdress);
		while (recvfrom(m_ServerSocket, recv, 2, 0, (sockaddr*)&m_ClientAdress, &len_tmp) == SOCKET_ERROR);
		//一次握手
		if (check_sum(recv, 2) != 0 || recv[1] != FIRST_SHAKE)
			continue;
		printf("%s\n", "成功接收客户端SYN请求!");
		while (1)
		{
			recv[1] = SECOND_SHAKE;
			recv[0] = check_sum(recv + 1, 1);
			//二次握手
			sendto(m_ServerSocket, recv, 2, 0, (sockaddr*)&m_ClientAdress, sizeof(m_ClientAdress));
			printf("%s\n", "成功发送服务端SYN_ACK!");
			int begin = clock();
			int fail = 0;
			while (recvfrom(m_ServerSocket, recv, 2, 0, (sockaddr*)&m_ClientAdress, &len_tmp) == SOCKET_ERROR)
			{
				if (clock() - begin > TIMEOUT)
				{
					fail = 1;
					break;
				}
			}
			if (fail == 0 && check_sum(recv, 2) == 0 && recv[1] == FIRST_SHAKE)
				continue;
			//三次握手
			if (fail == 0 && check_sum(recv, 2) == 0 && recv[1] == THIRD_SHAKE) {
				printf("%s\n", "成功接收客户端ACK!");
				break;
			}
		}
		break;
	}
	printf("建立三次握手成功!\n");
	cout << "-----------------------------------------------------------------" << endl;

	//下面开始接收文件
	printf("开始接收文件\n");
	//先接收文件名称
	recv_message(buffer, len);
	buffer[len] = 0;
	cout << buffer << endl;
	//然后接收文件内容
	string file_name(buffer);
	recv_message(buffer, len);
	printf("信息接收成功\n");
	ofstream fout(file_name.c_str(), ofstream::binary);
	for (int i = 0; i < len; i++)
		fout << buffer[i];
	fout.close();

	printf("文件接收成功\n");
	cout << "-----------------------------------------------------------------" << endl;
	printf("开始断开连接\n");
	while (1)
	{
		char recv[2];
		int len_tmp = sizeof(m_ClientAdress);
		while (recvfrom(m_ServerSocket, recv, 2, 0, (sockaddr*)&m_ClientAdress, &len_tmp) == SOCKET_ERROR);
		if (check_sum(recv, 2) != 0 || recv[1] != (char)FIRST_WAVE)
			continue;
		printf("%s\n", "成功接收第一次挥手消息");
		recv[1] = SECOND_WAVE;
		recv[0] = check_sum(recv + 1, 1);
		sendto(m_ServerSocket, recv, 2, 0, (sockaddr*)&m_ClientAdress, sizeof(m_ClientAdress));
		printf("%s\n", "成功发送第二次挥手");
		break;
	}
	printf("挥手成功, 断开连接\n");
	closesocket(m_ServerSocket);
	WSACleanup();
	system("pause");
	return 0;
}
