/*
Author: xiaoduo
Date: 2022/11/18
Description: ����ʵ��3-1����ͣ�Ȼ��ƵĿɿ�UDP����Э��ͻ���Clientʵ��
*/
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")//����ws2_32.lib���ļ�
#pragma warning(disable:4996)

#include <WS2tcpip.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
using namespace  std;

const unsigned char FIRST_SHAKE = 0x01;
const unsigned char SECOND_SHAKE = 0x02;
const unsigned char THIRD_SHAKE = 0x03;
const unsigned char FIRST_WAVE = 0x04;
const unsigned char SECOND_WAVE = 0x05;
const unsigned char ACK = 0x06;
const unsigned char NAK = 0x07;
const unsigned char END = 0x08;
const unsigned char NOT_END = 0x09;

const int TIMEOUT = 1000;
const int BUFFER_SIZE = 0x3A90;
const int MAX_UDP_LEN = 5000;
char buffer[100000000];
int len = 0;

SOCKET m_ClientSocket;
SOCKADDR_IN m_ServerAddress; //Զ�̵�ַ

//����У���
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

// ���͵������ݰ�
// 1. ȷ���Ƿ�Ϊ���һ�����ݰ�
// 2. ע�ⰴ˳����з���
// 3. ��ʱ�ش�
bool send_single_package(char* message, int length, int order, int isLast = 0) {
	char* sendBuf;
	int lenofsend;

	if (isLast) {
		//Э����Ƹ�ʽ
		/*
		struct msgHead {
			unsigned char checknum;
			unsigned char flag;//��־λ 
			unsigned char seq; //��ʾ���ݰ������к�, һ���ֽ�Ϊ����
			int length; //��ʾ���ݰ��ĳ��ȣ�����ͷ��
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
	while (1) {
		sendto(m_ClientSocket, sendBuf, lenofsend, 0, (sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
		int begintime = clock();
		char recv[3];
		int lentmp = sizeof(m_ServerAddress);
		int fail_send = 0;
		//��ʱ�ش�
		while (recvfrom(m_ClientSocket, recv, 3, 0, (sockaddr*)&m_ServerAddress, &lentmp) == SOCKET_ERROR)
			if (clock() - begintime > TIMEOUT) {
				fail_send = 1;
				break;
			}
		if (fail_send == 0 && check_sum(recv, 3) == 0 && recv[1] == ACK && recv[2] == (char)order)
		{
			//cout << "���Ͷ��յ����к�: " << recv[2] << endl;
			return true;
		}
	}
}
//�������ݺ���
void send_msg(char* message, int length) {
	//1. ���㷢�����ݰ�����Ŀ
	int package_num = length / MAX_UDP_LEN + (length % MAX_UDP_LEN != 0);
	static int order = 0;
	//2. һ��һ������
	int flag_last = 0;
	for (int i = 0; i < package_num; i++) {
		if (i == package_num - 1) {
			flag_last = 1;
		}
		send_single_package(message + i * MAX_UDP_LEN, i == package_num - 1 ? length - (package_num - 1) * MAX_UDP_LEN : MAX_UDP_LEN, order++,
			flag_last);
		cout << "���Ͱ������к�: " << order << endl;
	}
}


int main() {
	int m_RemoteAddressLen;

	// socket����
	WSADATA  wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup error:" << GetLastError() << endl;
		return false;
	}

	// socket����
	m_ClientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_ClientSocket == INVALID_SOCKET)
	{
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
		return false;
	}

	// Զ�˵�ַ
	const char* ip = "127.0.0.1";
	int	port = 9000;
	m_ServerAddress.sin_family = AF_INET;
	m_ServerAddress.sin_port = htons(port);
	m_RemoteAddressLen = sizeof(m_ServerAddress);
	inet_pton(AF_INET, ip, &m_ServerAddress.sin_addr);

	printf("�ɹ������ͻ���, ��ʼ����������...\n");
	cout << "-----------------------------------------------------------------" << endl;
	// ���պͷ���
	/*char recvBuf[1024] = { 0 };
	char sendBuf[1024] = "Nice to meet you!";*/

	//�������ֽ�������
	while (1)
	{
		char sendBuf[2];
		sendBuf[1] = FIRST_SHAKE;
		sendBuf[0] = check_sum(sendBuf + 1, 1);
		int ret = sendto(m_ClientSocket, sendBuf, 2, 0, (sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
		if (ret != SOCKET_ERROR)
		{
			printf("%s\n", "�ɹ����͵�һ������SYN����!");
		}
		else
			return 0;
		//��ʼ��ʱ
		int begin = clock();
		char recv[2];
		int len = sizeof(m_ServerAddress);
		int fail = 0;
		//��ʱ�ط�
		while (recvfrom(m_ClientSocket, recv, 2, 0, (sockaddr*)&m_ServerAddress, &len) == SOCKET_ERROR)
		{
			if (clock() - begin > TIMEOUT)
			{
				fail = 1;
				break;
			}
		}
		//���ճɹ���һ������ACK, �����˵ڶ�������SYN����ACK
		if (fail == 0 && check_sum(recv, 2) == 0 && recv[1] == SECOND_SHAKE)
		{
			printf("%s\n", "�ɹ����շ����SYN_ACK!");
			sendBuf[1] = THIRD_SHAKE;
			sendBuf[0] = check_sum(sendBuf + 1, 1);
			sendto(m_ClientSocket, sendBuf, 2, 0, (sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
			printf("%s\n", "���͵���������ACK!");
			break;
		}
	}
	printf("�������ֳɹ����ӷ����\n");
	cout << "-----------------------------------------------------------------" << endl;

	// ���濪ʼ�����ļ�
	printf("��ʼ�����ļ�\n");
	string filename;
	while (1) {
		printf("����Ҫ�����ļ�����");
		cin >> filename;
		ifstream fin(filename.c_str(), ifstream::binary);
		if (!fin) {
			printf("�ļ��Ҳ���\n");
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
	//�ȷ����ļ���
	send_msg((char*)(filename.c_str()), filename.length());
	//Ȼ���;�����ļ�����
	int time_begin = clock();
	send_msg(buffer, len);
	int time_end = clock();
	printf("�ļ����ͳɹ�\n");
	int runtime = (time_end - time_begin) / CLOCKS_PER_SEC;
	printf("�ļ�����ʱ�� %d s\n", runtime);
	double kbps = ((len * 8.0) / 1000) / runtime;
	printf("������: %f kbps\n", kbps);

	cout << "-----------------------------------------------------------------" << endl;
	printf("��ʼ�Ͽ�����\n");
	while (1)
	{
		char sendBuf[2];
		sendBuf[1] = FIRST_WAVE;
		sendBuf[0] = check_sum(sendBuf + 1, 1);
		sendto(m_ClientSocket, sendBuf, 2, 0, (sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
		printf("%s\n", "�ͻ��˷��ͻ�����Ϣ");
		int begin = clock();
		char recv[2];
		int len = sizeof(m_ServerAddress);
		int fail = 0;
		//��ʱ�ش�
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
			printf("%s\n", "���յڶ��λ���");
			break;
		}
	}
	printf("���ֳɹ�, �Ͽ�����\n");
	closesocket(m_ClientSocket);
	WSACleanup();
	system("pause");
	return 0;
}
