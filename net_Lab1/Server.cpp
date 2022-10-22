#include <iostream>  
#include <windows.h> 
#include <process.h> //�̴߳���ͷ�ļ�
#include <string.h>
#include <time.h>
using namespace std;
#pragma warning( disable : 4996 )
#pragma comment(lib, "WS2_32.lib")  
#define MAX_CLIENT_NUM 9
//��ʾ���� ws2_32.dll	ws2_32.dll��������socket�汾
const unsigned short chatport = 60000;//�̶�����˿ں�
int clientnum = 0;//�������Ŀͻ���id
SOCKET client[MAX_CLIENT_NUM];

//�����Ϣ��ʽ:ע����������Ϣ��ʽ�Ϳͻ�����Ϣ��ʽ��ͬ��Ҫ����
//����ͬ�Ŀͻ��˽�����ǩ
struct message {
	char id = '0';//����Ŀͻ��˷���˵�id��
	bool online;//��Կͻ��˱�ʾ�Ƿ�����, ��Է���˱�ʾ�������Ƿ���������
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

//������Ϣ���̺߳���
void ReceiveMsg(LPVOID lpParameter)
{
	int receiveid = clientnum;//��ʶ��ǰ�ͻ���
	while (1)
	{
		//��������������
		SOCKET revClientSocket = (SOCKET)lpParameter;
		char recvbuf[2048] = {};		//���ջ�����
		//���ܵ�������char���͵�,��Ҫת��Ϊmsg���͵�����
		//char(+time +online) -> string -> message =>���
		if (recv(revClientSocket, recvbuf, 2048, 0) == SOCKET_ERROR)
		{
			cout << WSAGetLastError() << endl;
			cout << "��ϵͳ��Ϣ�����ݽ���ʧ��, �ͻ��� "<< receiveid<<" ������{{{(>_<)}}}" << endl;
			/*cout << "������3����˳�" << endl;
			Sleep(3000);
			exit(100);*/
		}
		else {
			if (strlen(recvbuf) != 0) {
				//��������ʾ�����ͻ��˵���Ϣ:
				message m = receivechar2msg(recvbuf);
				cout << m.Mtime << " ���ͻ��� " << receiveid << " (*^_^*)��:" << m.msg << endl;
				//�������������ͻ���ת������
				m.id = receiveid + 48;
				char* sendmsg1 = msg2char(m);
				for (int i = 0; i < MAX_CLIENT_NUM; i++) {
					if (client[i] != NULL && i != receiveid - 1) {
						SOCKET revClient = client[i];
						if (send(revClient, sendmsg1, strlen(sendmsg1), 0) == SOCKET_ERROR)
						{
							cout << "��ϵͳ��Ϣ��������Ϣʧ�ܣ�" << endl;
						}
					}
				}
				if (m.online == false) {
					cout << "��ϵͳ��Ϣ���ͻ��� " << receiveid << " ������, ��ӭ�´�����byebye~\(�R���Q)/~" << endl;
					client[receiveid - 1] = NULL;
					break;
				}
			}
		}
	}
}

//�������ݵ��̺߳���
void SendMsg(LPVOID lpParameter)
{	
	//���������͵���������:
	while (1)
	{
		//��������������
		SOCKET revClientSocket = (SOCKET)lpParameter;
		char sendbuf[2048] = {};		//���뻺����
		cin.getline(sendbuf, 2048);
		//���͵�������char���͵�
		//char* +time +online -> char*
		char *sendmsg;		//���ͻ�����
		message m = sendchar2msg(sendbuf);
		m.id = '0';
		sendmsg = msg2char(m);
		cout << m.Mtime << " ��������\\^o^/��:" << m.msg << endl;
		for (int i = 0; i < MAX_CLIENT_NUM; i++) {
			if (client[i] != NULL) {
				SOCKET revClient = client[i];
				if (send(revClient, sendmsg, strlen(sendmsg), 0) == SOCKET_ERROR)
				{
					cout << "������Ϣʧ�ܣ�" << endl;
				}
			}
		}
		if (m.online == false) {
			cout << "�����ҹر�, ��ӭ�´�����byebye~\(�R���Q)/~" << endl;
			cout << "������˳�, ������3����˳���" << endl;
			Sleep(3000);
			exit(100);
		}
	}
}

//ͳһ�շ����ݵ��̺߳���
void chat(void* param) {
	_beginthread(ReceiveMsg, 0, param);
	_beginthread(SendMsg, 0, param);
}

int main()
{
	cout << "-------------------�����������---------------------------" << endl;
	cout << "-------------��ϲ�㴴���ɹ������ҩd(�R���Q*)o-------------" << endl;
	cout << " _          _   _                           _       " << endl
		<< "| |        | | ( )                         (_)      " << endl
		<< "| |     ___| |_|/ ___    __ _  ___  ___ ___ _ _ __  " << endl
		<< "| |    / _ \\ __| / __|  / _` |/ _ \\/ __/ __| | '_ \\ " << endl
		<< "| |___|  __/ |_  \\__ \\ | (_| | (_) \\__ \\__ \\ | |_) |" << endl
		<< "|______\\___|\\__| |___/  \\__, |\\___/|___/___/_| .__/ " << endl
		<< "                         __/ |               | |    " << endl
		<< "                        |___/                |_|    " << endl;
	cout << "-----------------------------------------------------------" << endl;

	/*1. �׽��ֳ�ʼ��*/
	WSADATA wsaData;
	//����ṹ�������洢��WSAStartup�������ú󷵻ص� Windows Sockets ���ݡ�
	WORD sockVersion = MAKEWORD(2, 2);
	//windows�����̿�İ汾����Ϣ, ��������2.2�汾
	if (WSAStartup(sockVersion, &wsaData) != 0) //��ʼ�����ֽ�
	{
		cout << "�׽��ֳ�ʼ��ʧ��!" << endl;
		return 0;
	}

	/*2. �����������׽���*/
	SOCKET SeverSocket;
	if ((SeverSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
	{
		cout << "�׽��ִ���ʧ�ܣ�" << endl;
		return 0;
	}

	/*3. ���׽���*/
	struct sockaddr_in SeverAddress;		//һ���󶨵�ַ:��IP��ַ,�ж˿ں�,��Э����
	memset(&SeverAddress, 0, sizeof(sockaddr_in)); //��ʼ���ṹ��
	SeverAddress.sin_family = AF_INET; //ָ��ip�ṹ
	SeverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//����IP��ַ 
	SeverAddress.sin_port = htons(60000);//�趨�˿ں�

		//ָ���󶨵�IP��ַ�Ͷ˿ں�
	if (bind(SeverSocket, (sockaddr*)&SeverAddress, sizeof(SeverAddress)) == SOCKET_ERROR)
	{
		cout << "�׽��ְ�ʧ�ܣ�" << endl;
		return 0;
	}


	/*4. ����������*/
	if (listen(SeverSocket, SOMAXCONN) == SOCKET_ERROR) //����SOMAXCONN������ͬʱ�������ٸ�����
	{
		cout << "����ʧ�ܣ�" << endl;
		return 0;
	}
	else
		cout << "��Ҫ�ż�~ �����������ٻ������С���(/����)" << endl;

	/*5. �����������ɹ�,��������*/
	while (1) {
		sockaddr_in revClientAddress;//�׽��ֵĵ�ַ���˿�
		SOCKET revClientSocket = INVALID_SOCKET;//�ͻ������ӵ����ֽ�
		int addlen = sizeof(revClientAddress);
		if ((revClientSocket = accept(SeverSocket, (sockaddr*)&revClientAddress, &addlen)) == INVALID_SOCKET)
		{
			cout << "��ŷ���ܿͻ�������ʧ���ˣ����������һ��" << endl;
			return 0;
		}
		else
			cout << "��ϵͳ��Ϣ���ҵ�һλһ�������С���q(�R���Qq)����������к���!" << endl;
		client[clientnum++] = revClientSocket;
		cout << "��ϵͳ��Ϣ���ͻ��� " << clientnum << " ������!" << endl;
		/*6. �����߳�,�����շ�*/
		/*_beginthread(ReceiveMsg, 0, &revClientSocket);
		_beginthread(SendMsg, 0, &revClientSocket);*/
		chat((LPVOID)revClientSocket);

		//while (1) {}
		//Ϊ�˱������߳��˳�ʹ���̱߳���������ʹ���߳̽���ѭ��

	}
	/*7. �ر�����*/
	for (int i = 0; i < MAX_CLIENT_NUM; i++) {
		SOCKET ClientSocket = client[i];
		closesocket(ClientSocket);
	}

	closesocket(SeverSocket);
	/*8. ������ֹͣ*/
	WSACleanup();
	cout << "������ֹͣ��" << endl;
	return 0;
}
