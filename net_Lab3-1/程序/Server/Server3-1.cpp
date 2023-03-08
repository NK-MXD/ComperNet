/*
Author: xiaoduo
Date: 2022/11/18
Description: ����ʵ��3-1����ͣ�Ȼ��ƵĿɿ�UDP����Э������Serverʵ��
*/
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")//����ws2_32.lib���ļ�
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
SOCKADDR_IN m_ClientAdress; //Զ�̵�ַ
SOCKET m_ServerSocket;

//��Ʊ��ĸ�ʽ
struct msgHead {
	unsigned char checknum;
	unsigned char flag;//��־λ 
	unsigned char seq; //��ʾ���ݰ������к�, һ���ֽ�Ϊ����
	int length; //��ʾ���ݰ��ĳ��ȣ�����ͷ��
};

//����У���
//UDP����͵ļ��㷽���ǣ�
//1. ��ÿ16λ��͵ó�һ��32λ������
//2. ������32λ��������16λ��Ϊ0�����16λ�ӵ�16λ�ٵõ�һ��32λ������
//3. �ظ���2��ֱ����16λΪ0������16λȡ�����õ�У��͡�

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

void recv_message(char* message, int& len_recv) {
	char recv[MAX_UDP_LEN + 4];
	int lentmp = sizeof(m_ClientAdress);
	//���ն˵�˳��
	static unsigned char receive_order = 0;
	len_recv = 0;
	while (1) {
		while (1) {
			memset(recv, 0, sizeof(recv));
			while (recvfrom(m_ServerSocket, recv, MAX_UDP_LEN + 4, 0, (sockaddr*)&m_ClientAdress, &lentmp) == SOCKET_ERROR);
			char send[3];
			int flag = 0;
			//����ACKȷ�ϵ����к�
			if (check_sum(recv, MAX_UDP_LEN + 4) == 0 && (unsigned char)recv[2] == receive_order) {
				receive_order++;
				flag = 1;
			}
			send[1] = ACK;
			send[2] = receive_order - 1;
			send[0] = check_sum(send + 1, 2);
			cout << "���ն�ȷ�����к�: " << receive_order - 1 << endl;
			sendto(m_ServerSocket, send, 3, 0, (sockaddr*)&m_ClientAdress, sizeof(m_ClientAdress));
			if (flag)
				break;
		}
		// ���ն˽���Ϣ���뻺������
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
	SOCKADDR_IN m_BindAddress;   //�󶨵�ַ
	int m_RemoteAddressLen;


	// socket����
	WSADATA  wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup error:" << GetLastError() << endl;
		return false;
	}

	// socket����
	m_ServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_ServerSocket == INVALID_SOCKET)
	{
		closesocket(m_ServerSocket);
		m_ServerSocket = INVALID_SOCKET;
		return false;
	}

	// ��ռ��<ip, port>
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

	// ���պͷ���
	/*char recvBuf[1024] = { 0 };
	char sendBuf[1024] = "Nice to meet you, too!";*/
	m_RemoteAddressLen = sizeof(m_ClientAdress);

	printf("����������ð�ռ�õ�����, ��ip: %s, port: %d\n", inet_ntoa(m_BindAddress.sin_addr), ntohs(m_BindAddress.sin_port));
	printf("����˵ȴ�����\n");
	cout << "-----------------------------------------------------------------" << endl;

	//�������ͻ��˽�������
	while (1)
	{
		//��������ʵ��
		char recv[2];
		int len_tmp = sizeof(m_ClientAdress);
		while (recvfrom(m_ServerSocket, recv, 2, 0, (sockaddr*)&m_ClientAdress, &len_tmp) == SOCKET_ERROR);
		//һ������
		if (check_sum(recv, 2) != 0 || recv[1] != FIRST_SHAKE)
			continue;
		printf("%s\n", "�ɹ����տͻ���SYN����!");
		while (1)
		{
			recv[1] = SECOND_SHAKE;
			recv[0] = check_sum(recv + 1, 1);
			//��������
			sendto(m_ServerSocket, recv, 2, 0, (sockaddr*)&m_ClientAdress, sizeof(m_ClientAdress));
			printf("%s\n", "�ɹ����ͷ����SYN_ACK!");
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
			//��������
			if (fail == 0 && check_sum(recv, 2) == 0 && recv[1] == THIRD_SHAKE) {
				printf("%s\n", "�ɹ����տͻ���ACK!");
				break;
			}
		}
		break;
	}
	printf("�����������ֳɹ�!\n");
	cout << "-----------------------------------------------------------------" << endl;

	//���濪ʼ�����ļ�
	printf("��ʼ�����ļ�\n");
	//�Ƚ����ļ�����
	recv_message(buffer, len);
	buffer[len] = 0;
	cout << buffer << endl;
	//Ȼ������ļ�����
	string file_name(buffer);
	recv_message(buffer, len);
	printf("��Ϣ���ճɹ�\n");
	ofstream fout(file_name.c_str(), ofstream::binary);
	for (int i = 0; i < len; i++)
		fout << buffer[i];
	fout.close();

	printf("�ļ����ճɹ�\n");
	cout << "-----------------------------------------------------------------" << endl;
	printf("��ʼ�Ͽ�����\n");
	while (1)
	{
		char recv[2];
		int len_tmp = sizeof(m_ClientAdress);
		while (recvfrom(m_ServerSocket, recv, 2, 0, (sockaddr*)&m_ClientAdress, &len_tmp) == SOCKET_ERROR);
		if (check_sum(recv, 2) != 0 || recv[1] != (char)FIRST_WAVE)
			continue;
		printf("%s\n", "�ɹ����յ�һ�λ�����Ϣ");
		recv[1] = SECOND_WAVE;
		recv[0] = check_sum(recv + 1, 1);
		sendto(m_ServerSocket, recv, 2, 0, (sockaddr*)&m_ClientAdress, sizeof(m_ClientAdress));
		printf("%s\n", "�ɹ����͵ڶ��λ���");
		break;
	}
	printf("���ֳɹ�, �Ͽ�����\n");
	closesocket(m_ServerSocket);
	WSACleanup();
	system("pause");
	return 0;
}
