#include <windows.h>
#include <iostream>
#include <process.h>
#include <string>
using namespace std;
#pragma warning( disable : 4996 )
#pragma comment(lib, "ws2_32.lib")
const unsigned short chatport = 60000;//�̶�����˿ں�

//�����Ϣ��ʽ:ע����������Ϣ��ʽ�Ϳͻ�����Ϣ��ʽ��ͬ��Ҫ����
//����ͬ�Ŀͻ��˽�����ǩ
struct message {
	char id = '0';//����Ŀͻ��˷���˵�id��
	bool online;//��Կͻ��˱�ʾ�Ƿ�����, ��Է���˱�ʾ�������Ƿ���������
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
		//�ͻ��˽������Է�����������
		//	1. ���������͵���������
		//	2. ���������ݵ������ͻ��˵�����
		SOCKET clientSocket = *(SOCKET*)(param);
		char  recvbuf[2048] = {};		//���ջ�����
		//���ܵ�������char���͵�,��Ҫת��Ϊmsg���͵�����
		//char(+time +online) -> string -> message =>���
		if (recv(clientSocket, recvbuf, 2048, 0) == SOCKET_ERROR)
		{
			cout << "��ϵͳ��Ϣ������˵�����{{{(>_<)}}}" << endl;
			cout << "��ϵͳ��Ϣ��������3����˳�" << endl;
			Sleep(3000);
			exit(100);
		}
		else {
			if (strlen(recvbuf)!=0) {
				message m = receivechar2msg(recvbuf);
				if (m.id == '0') {
					//1. ���������͵�����
					cout << m.Mtime << " ��������(*^_^*)��:" << m.msg << endl;
					if (m.online == false) {
						cout << "��ϵͳ��Ϣ�������ҹر�, ��ӭ�´�����byebye~\(�R���Q)/~" << endl;
						cout << "��ϵͳ��Ϣ���ͻ����˳�, ������3����˳���" << endl;
						Sleep(3000);
						exit(100);
					}
				}
				if (m.id != '0') {
					//2. ������ת���������ͻ��˵�����
					cout << m.Mtime << " ���ͻ��� "<<m.id<<" (*^_^*)��:" << m.msg << endl;
					if (m.online == false) {
						cout << "��ϵͳ��Ϣ���ͻ��� " << m.id << " ������, ��ӭ�´�����byebye~\(�R���Q)/~" << endl;
					}
				}
			}
		}
	}
}

void SendMsg(void* param)
{
	//�ͻ��˷������ݸ�������
	SOCKET clientSocket = *(SOCKET*)(param);
	char initbuf[2048] = "���������ҡ���Һ�ѽ~ �ܸ����ܺʹ������ =)";
	char* sendmsg;		//���ͻ�����
	message m = sendchar2msg(initbuf);
	sendmsg = msg2char(m);
	if (send(clientSocket, sendmsg, strlen(sendmsg), 0) == SOCKET_ERROR)
	{
		cout << "������Ϣʧ�ܣ�";
	}
	else {
		cout << m.Mtime << " ���ͻ�����\\ ^ o ^ / ��:" << m.msg << endl;//��ʶ�Լ���������Ϣ
	}
	while (1)
	{
		char sendbuf[2048] = {};		//���ͻ�����
		cin.getline(sendbuf, 2048);
		//���͵�������char���͵�
		//char* +time +online -> char*
		char* sendmsg;		//���ͻ�����
		message m = sendchar2msg(sendbuf);
		sendmsg = msg2char(m);
		if (send(clientSocket, sendmsg, strlen(sendmsg), 0) == SOCKET_ERROR)
		{
			cout << "������Ϣʧ�ܣ�";
		}
		else{	
			cout << m.Mtime << " ���ͻ�����\\ ^ o ^ / ��:" << m.msg << endl;//��ʶ�Լ���������Ϣ
			if (m.online == false) {
				cout << "��ϵͳ��Ϣ����ӭ�´�����byebye~\(�R���Q)/~" << endl;
				cout << "��ϵͳ��Ϣ���ͻ����˳�, ������3����˳���" << endl;
				Sleep(3000);
				exit(100);
			}
		}
	}
}

int main()
{
	cout << "-------------------�ͻ���������---------------------------" << endl;
	cout << "-------------��ϲ��ɹ����������ҩd(�R���Q*)o-------------" << endl;
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
		cout << "�׽��ֳ�ʼ��ʧ��!" << endl;
	}
	SOCKET clientSocket;
	if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
	{
		cout << "�׽��ִ���ʧ��!" << endl;
	}
	struct sockaddr_in SeverAddress;		//��������ַ Ҳ���Ǽ���Ҫ���ӵ�Ŀ���ַ
	memset(&SeverAddress, 0, sizeof(sockaddr_in));
	SeverAddress.sin_family = AF_INET;
	SeverAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  //127.0.0.1��ʾ����ip��ַ
	SeverAddress.sin_port = htons(60000);//�趨�˿ں�

	//����
	if (connect(clientSocket, (sockaddr*)&SeverAddress, sizeof(SeverAddress)) == SOCKET_ERROR)
	{
		cout << "��ŷ�ͷ���������ʧ���ˣ����������һ��" << endl;
		return 0;
	}
	else
		cout << "�ɹ�������������, ����Ҵ���к���o(*�R���Q)����" << endl;

	//�����������߳�
	_beginthread(ReceiveMsg, 0, &clientSocket);
	_beginthread(SendMsg, 0, &clientSocket);

	while (1) {}	//�����������һ�ּ����������߳�ִ�����˳�����ʹ������������
	//	�ر�socket
	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
	}

	//	��ֹ
	WSACleanup();
	cout << "�ͻ����˳���" << endl;
	return 0;
}
