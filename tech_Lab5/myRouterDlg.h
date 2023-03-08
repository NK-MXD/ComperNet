
// myRouterDlg.h : header file
//
#include <iostream>
#include<fstream>
#include "pcap.h"
#include <string>
#include <vector>
#include <list>
#include <iterator>
#include <WinSock2.h>
using namespace std;

#pragma warning(disable:4996)
#pragma comment(lib, "packet.lib")
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "ws2_32.lib")

#pragma pack(1)		            // �����ֽڶ��뷽ʽ

typedef struct FrameStruct {	// ֡�ײ�
    BYTE	DesMAC[6];	        // Ŀ�ĵ�ַ
    BYTE 	SrcMAC[6];	        // Դ��ַ
    WORD	FrameType;	        // ֡����
} FrameStruct;

typedef struct ARPStruct {     // ARP֡
    FrameStruct FrameHeader;  // ֡ͷ���ṹ��
    WORD HardwareType;          // Ӳ������
    WORD ProtocolType;          // Э������
    BYTE HLen;                  // Ӳ����ַ����
    BYTE PLen;                  // Э���ַ����
    WORD Operation;             // �����ֶ�
    BYTE SendHa[6];             // ԴMAC��ַ
    DWORD SendIP;               // ԴIP��ַ
    BYTE RecvHa[6];             // Ŀ��MAC��ַ
    DWORD RecvIP;               // Ŀ��IP��ַ
} ARPStruct;

typedef struct IPStruct {		// IP�ײ�
    BYTE Ver_HLen;              // �汾+ͷ������
    BYTE TOS;                   // ��������
    WORD TotalLen;              // �ܳ���
    WORD ID;                    // ��ʶ
    WORD Flag_Segment;          // ��־+Ƭƫ��
    BYTE TTL;                   // ����ʱ��
    BYTE Protocol;              // Э��
    WORD Checksum;              // ͷ��У���
    ULONG SrcIP;                // ԴIP��ַ
    ULONG DstIP;                // Ŀ��IP��ַ
} IPStruct;

typedef struct PacketStruct {	        // ����֡�ײ���IP�ײ������ݰ�
    FrameStruct FrameHeader;  // ֡�ײ�
    IPStruct IPHeader;        // IP�ײ�
} PacketStruct;

#pragma pack()	                // �ָ�Ĭ�϶��뷽ʽ

typedef struct PacketCache {       // ������е����ݰ�
    DWORD ip;                   // Ŀ��IP��ַ
    int IfNo;                   // �ӿ����
    int len;                    // ���ݰ�����
    char data[65535];           // ���ݰ�
} PacketCache;

typedef struct DeviceInfo {	    // �ӿ���Ϣ
    char DeviceName[64];        // �豸��
    char Description[128];      // �豸����
    BYTE MACAddr[6];            // MAC��ַ
    DWORD ip[5];                // IP��ַ�б�
    DWORD netmask[5];           // �����ַ�б�
    pcap_t* adhandle;           // pcap���
} IfInfo_t;

typedef struct MyRouteTable {	// ·�ɱ����
    DWORD Mask;                 // ��������
    DWORD DstIP;                // Ŀ�ĵ�ַ
    DWORD NextHop;              // ��һ����              
} MyRouteTable;

typedef struct ARPTable {       // IP-MAC��ַӳ������
    DWORD IPAddr;               // IP��ַ
    BYTE MACAddr[6];            // MAC��ַ
} ARPTable;

char* BYTEToMac(BYTE* mac);
char* DWORDToIP(DWORD ip);
string ShowRouteTable();
void ARPRequest(pcap_t* adhandle, BYTE* SrcMAC, DWORD SendIp, DWORD RecvIp);
DWORD SearchRouteTable(int& IfNo, DWORD DstIP);
u_short checksum(u_short* buffer, int size);
bool checkIPHeader(char* buff);
void GetDeviceMAC(LPVOID lpParameter);
bool MACTableSearch(DWORD ip, BYTE* MAC);
void MACTableUpdate(DWORD ip, BYTE* MAC);
void DealIP(struct pcap_pkthdr* header, const u_char* pkt_data);
DWORD WINAPI CapturePacket(LPVOID lpParameter);
void DealARP(struct pcap_pkthdr* header, const u_char* pkt_data);
void WriteLog();

// CmyRouterDlg dialog
class CmyRouterDlg : public CDialogEx
{
// Construction
public:
	CmyRouterDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MYROUTER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CEdit log;
	CEdit Ctable;
	DWORD mask;
	DWORD des;
	DWORD next;
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedButton1();
    afx_msg void OnBnClickedButton2();
    CIPAddressCtrl cmask;
    CIPAddressCtrl cdes;
    CIPAddressCtrl cnext;
    void showLog();
};

