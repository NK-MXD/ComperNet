#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <queue>
#include<mutex>
using namespace  std;

#pragma comment(lib,"ws2_32.lib")//链接ws2_32.lib库文件
#pragma warning(disable:4996)

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

const int TIMEOUT = 500;//超时时间设置2秒
const int BUFFER_SIZE = 0x3A90;
const int MAX_UDP_LEN = 5000;
const int WINDOW_SIZE = 7;
char buffer[100000000];
int len = 0;

SOCKET m_ClientSocket;
SOCKADDR_IN m_ServerAddress; //远程地址
SOCKADDR_IN m_ClientAddress; //客户端地址

queue<pair<int, int>> list;//序列号
int has_send = 0;//已发送未确认
int next_send = 0;
int send_ok = 0;//收到确认
int resend = 0;//重发次数
int base = 0;
int last_pack = 0;
//文件内容
int num ;//分片个数
int time_begin = clock();
int CWND = 1;
int SSTH = 64;
int cwnd_temp = 65;
int dupACK = 0;//冗余ACK
int startflag = 0;//是否快速恢复
mutex mtx;

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

	char* tmp;
	int tmp_len;
	if (!isLast)//不是最后一个包
	{
		tmp = new char[length + 3];
		tmp[1] = NOT_END;//flag
		tmp[2] = order; //序列号
		for (int i = 3; i < length + 3; i++)
		{
			tmp[i] = message[i - 3];//存入
		}
		tmp[0] = check_sum(tmp + 1, length + 2);//第一位存入校验和
		tmp_len = length + 3;//数据包长度
	}
	else
	{
		tmp = new char[length + 4];
		tmp[1] = END;
		tmp[2] = order;
		tmp[3] = length;//最后一个包的长度
		for (int i = 4; i < length + 4; i++)
		{
			tmp[i] = message[i - 4];
		}
		tmp[0] = check_sum(tmp + 1, length + 3);
		tmp_len = length + 4;
	}
	sendto(m_ClientSocket, tmp, tmp_len, 0, (sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
}

//接收数据线程
DWORD WINAPI recv_msg(LPVOID lparam) {
	char recv[3];
	int lentmp = sizeof(m_ServerAddress);
	while (1) {
		//确认完毕所有数据包
		if (send_ok == num)
			break;
		if (recvfrom(m_ClientSocket, recv, 3, 0, (sockaddr*)&m_ServerAddress, &lentmp) != SOCKET_ERROR && check_sum(recv, 3) == 0 &&
			recv[1] == ACK) {
			if (startflag)dupACK++;
			if ((unsigned char)recv[2] == unsigned char((send_ok) % ((int)UCHAR_MAX + 1))) {
				if (CWND <= SSTH)
				{//慢启动阶段
					CWND++;
					mtx.lock();
					printf("%s%d\n", "当前窗口大小:", CWND);
					mtx.unlock();
				}
				else//拥塞避免
				{
					//线性增长
					cwnd_temp++;
					if (cwnd_temp % CWND == 0 && CWND < (int)UCHAR_MAX) {
						CWND++;
						mtx.lock();
						printf("%s%d\n", "当前窗口大小:", CWND);
						mtx.unlock();
					}
				}
				send_ok++;
				mtx.lock();
				printf("%s%d\n\n", "已经确认", send_ok);
				list.pop();
				mtx.unlock();
				base++;
				dupACK = 0;
				startflag = 1;
				resend = 0;
				continue;
			}
			if (dupACK == 3)//快恢复阶段
			{
				startflag = 0;
				dupACK = 0;
				SSTH = CWND / 2;
				CWND = CWND / 2 + 3;//进入拥塞避免阶段
				cwnd_temp = SSTH + 1;
				next_send = base;
				resend++;
				mtx.lock();
				has_send -= list.size();
				printf("%s%d\n", "当前窗口大小:", CWND);
				while (!list.empty())
				{
					list.pop();
				}
				cout << "重传第" << has_send << "个包" << endl;
				mtx.unlock();
				continue;
			}
			//超时
			if (clock() - list.front().first > TIMEOUT) {
				startflag = 0;
				dupACK = 0;
				SSTH = CWND / 2;
				CWND = 1;
				cwnd_temp = SSTH + 1;//进入慢启动阶段
				next_send = base;
				resend++;
				mtx.lock();
				has_send -= list.size();
				printf("%s%d\n", "当前窗口大小:", CWND);
				while (!list.empty())
				{
					list.pop();
				}
				cout << "重传第" << has_send << "个包" << endl;
				mtx.unlock();
			}
		}
		//出错
		else {
			startflag = 0;
			dupACK = 0;
			SSTH = CWND / 2;
			CWND = 1;//慢启动
			cwnd_temp = SSTH + 1;
			next_send = base;
			resend++;
			mtx.lock();
			has_send -= list.size();
			printf("%s%d\n", "当前窗口大小:", CWND);
			while (!list.empty()) {
				list.pop();
			}
			cout << "重传第" << has_send << "个包" << endl;
			mtx.unlock();
		}
	}
	return 1;
}
//发送数据函数
//涉及到滑动窗口和累计确认机制
/*              发送方的可用窗口
				|
+ + + + + + + + [+ + + + + + + + ] + + + + + + + +
				|          | |
			   base has_send next_send
*/
void send_msg(char* message, int length) {
	num = length / MAX_UDP_LEN + (length % MAX_UDP_LEN != 0);
	//开线程
	DWORD  threadId;
	CreateThread(NULL, 0, recv_msg, 0, 0, &threadId);
	while (1) {
		//确认完毕所有数据包
		if (send_ok == num)
			break;
		//发送窗口
		if (list.size() < CWND && has_send < num) {
			send_single_package(message + has_send * MAX_UDP_LEN,
				has_send == num - 1 ? length - (num - 1) * MAX_UDP_LEN : MAX_UDP_LEN,
				next_send % ((int)UCHAR_MAX + 1),
				has_send == num - 1);
			mtx.lock();
			list.push(make_pair(clock(), next_send % ((int)UCHAR_MAX + 1)));
			//将当前数据包的放入窗口中
			printf("%s%d\n", "sending seq:", next_send % ((int)UCHAR_MAX + 1));
			has_send++;
			printf("%s%d\n", "已经发送: ", has_send);
			next_send++;
			mtx.unlock();
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
	setsockopt(m_ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&TIMEOUT, sizeof(int));
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
