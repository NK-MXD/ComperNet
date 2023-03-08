#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <queue>
#include<mutex>
using namespace  std;

#pragma comment(lib,"ws2_32.lib")//����ws2_32.lib���ļ�
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

const int TIMEOUT = 500;//��ʱʱ������2��
const int BUFFER_SIZE = 0x3A90;
const int MAX_UDP_LEN = 5000;
const int WINDOW_SIZE = 7;
char buffer[100000000];
int len = 0;

SOCKET m_ClientSocket;
SOCKADDR_IN m_ServerAddress; //Զ�̵�ַ
SOCKADDR_IN m_ClientAddress; //�ͻ��˵�ַ

queue<pair<int, int>> list;//���к�
int has_send = 0;//�ѷ���δȷ��
int next_send = 0;
int send_ok = 0;//�յ�ȷ��
int resend = 0;//�ط�����
int base = 0;
int last_pack = 0;
//�ļ�����
int num ;//��Ƭ����
int time_begin = clock();
int CWND = 1;
int SSTH = 64;
int cwnd_temp = 65;
int dupACK = 0;//����ACK
int startflag = 0;//�Ƿ���ٻָ�
mutex mtx;

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
void send_single_package(char* message, int length, int order, int isLast = 0) {
	char* sendBuf;
	int lenofsend;

	char* tmp;
	int tmp_len;
	if (!isLast)//�������һ����
	{
		tmp = new char[length + 3];
		tmp[1] = NOT_END;//flag
		tmp[2] = order; //���к�
		for (int i = 3; i < length + 3; i++)
		{
			tmp[i] = message[i - 3];//����
		}
		tmp[0] = check_sum(tmp + 1, length + 2);//��һλ����У���
		tmp_len = length + 3;//���ݰ�����
	}
	else
	{
		tmp = new char[length + 4];
		tmp[1] = END;
		tmp[2] = order;
		tmp[3] = length;//���һ�����ĳ���
		for (int i = 4; i < length + 4; i++)
		{
			tmp[i] = message[i - 4];
		}
		tmp[0] = check_sum(tmp + 1, length + 3);
		tmp_len = length + 4;
	}
	sendto(m_ClientSocket, tmp, tmp_len, 0, (sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
}

//���������߳�
DWORD WINAPI recv_msg(LPVOID lparam) {
	char recv[3];
	int lentmp = sizeof(m_ServerAddress);
	while (1) {
		//ȷ������������ݰ�
		if (send_ok == num)
			break;
		if (recvfrom(m_ClientSocket, recv, 3, 0, (sockaddr*)&m_ServerAddress, &lentmp) != SOCKET_ERROR && check_sum(recv, 3) == 0 &&
			recv[1] == ACK) {
			if (startflag)dupACK++;
			if ((unsigned char)recv[2] == unsigned char((send_ok) % ((int)UCHAR_MAX + 1))) {
				if (CWND <= SSTH)
				{//�������׶�
					CWND++;
					mtx.lock();
					printf("%s%d\n", "��ǰ���ڴ�С:", CWND);
					mtx.unlock();
				}
				else//ӵ������
				{
					//��������
					cwnd_temp++;
					if (cwnd_temp % CWND == 0 && CWND < (int)UCHAR_MAX) {
						CWND++;
						mtx.lock();
						printf("%s%d\n", "��ǰ���ڴ�С:", CWND);
						mtx.unlock();
					}
				}
				send_ok++;
				mtx.lock();
				printf("%s%d\n\n", "�Ѿ�ȷ��", send_ok);
				list.pop();
				mtx.unlock();
				base++;
				dupACK = 0;
				startflag = 1;
				resend = 0;
				continue;
			}
			if (dupACK == 3)//��ָ��׶�
			{
				startflag = 0;
				dupACK = 0;
				SSTH = CWND / 2;
				CWND = CWND / 2 + 3;//����ӵ������׶�
				cwnd_temp = SSTH + 1;
				next_send = base;
				resend++;
				mtx.lock();
				has_send -= list.size();
				printf("%s%d\n", "��ǰ���ڴ�С:", CWND);
				while (!list.empty())
				{
					list.pop();
				}
				cout << "�ش���" << has_send << "����" << endl;
				mtx.unlock();
				continue;
			}
			//��ʱ
			if (clock() - list.front().first > TIMEOUT) {
				startflag = 0;
				dupACK = 0;
				SSTH = CWND / 2;
				CWND = 1;
				cwnd_temp = SSTH + 1;//�����������׶�
				next_send = base;
				resend++;
				mtx.lock();
				has_send -= list.size();
				printf("%s%d\n", "��ǰ���ڴ�С:", CWND);
				while (!list.empty())
				{
					list.pop();
				}
				cout << "�ش���" << has_send << "����" << endl;
				mtx.unlock();
			}
		}
		//����
		else {
			startflag = 0;
			dupACK = 0;
			SSTH = CWND / 2;
			CWND = 1;//������
			cwnd_temp = SSTH + 1;
			next_send = base;
			resend++;
			mtx.lock();
			has_send -= list.size();
			printf("%s%d\n", "��ǰ���ڴ�С:", CWND);
			while (!list.empty()) {
				list.pop();
			}
			cout << "�ش���" << has_send << "����" << endl;
			mtx.unlock();
		}
	}
	return 1;
}
//�������ݺ���
//�漰���������ں��ۼ�ȷ�ϻ���
/*              ���ͷ��Ŀ��ô���
				|
+ + + + + + + + [+ + + + + + + + ] + + + + + + + +
				|          | |
			   base has_send next_send
*/
void send_msg(char* message, int length) {
	num = length / MAX_UDP_LEN + (length % MAX_UDP_LEN != 0);
	//���߳�
	DWORD  threadId;
	CreateThread(NULL, 0, recv_msg, 0, 0, &threadId);
	while (1) {
		//ȷ������������ݰ�
		if (send_ok == num)
			break;
		//���ʹ���
		if (list.size() < CWND && has_send < num) {
			send_single_package(message + has_send * MAX_UDP_LEN,
				has_send == num - 1 ? length - (num - 1) * MAX_UDP_LEN : MAX_UDP_LEN,
				next_send % ((int)UCHAR_MAX + 1),
				has_send == num - 1);
			mtx.lock();
			list.push(make_pair(clock(), next_send % ((int)UCHAR_MAX + 1)));
			//����ǰ���ݰ��ķ��봰����
			printf("%s%d\n", "sending seq:", next_send % ((int)UCHAR_MAX + 1));
			has_send++;
			printf("%s%d\n", "�Ѿ�����: ", has_send);
			next_send++;
			mtx.unlock();
		}
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
	setsockopt(m_ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&TIMEOUT, sizeof(int));
	// Զ�˵�ַ
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

	printf("�ɹ������ͻ���, ��ʼ����������...\n");
	cout << "-----------------------------------------------------------------" << endl;

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
		setsockopt(m_ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&TIMEOUT, sizeof(int));
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
