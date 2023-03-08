
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

#pragma pack(1)		            // 进入字节对齐方式

typedef struct FrameStruct {	// 帧首部
    BYTE	DesMAC[6];	        // 目的地址
    BYTE 	SrcMAC[6];	        // 源地址
    WORD	FrameType;	        // 帧类型
} FrameStruct;

typedef struct ARPStruct {     // ARP帧
    FrameStruct FrameHeader;  // 帧头部结构体
    WORD HardwareType;          // 硬件类型
    WORD ProtocolType;          // 协议类型
    BYTE HLen;                  // 硬件地址长度
    BYTE PLen;                  // 协议地址长度
    WORD Operation;             // 操作字段
    BYTE SendHa[6];             // 源MAC地址
    DWORD SendIP;               // 源IP地址
    BYTE RecvHa[6];             // 目的MAC地址
    DWORD RecvIP;               // 目的IP地址
} ARPStruct;

typedef struct IPStruct {		// IP首部
    BYTE Ver_HLen;              // 版本+头部长度
    BYTE TOS;                   // 服务类型
    WORD TotalLen;              // 总长度
    WORD ID;                    // 标识
    WORD Flag_Segment;          // 标志+片偏移
    BYTE TTL;                   // 生存时间
    BYTE Protocol;              // 协议
    WORD Checksum;              // 头部校验和
    ULONG SrcIP;                // 源IP地址
    ULONG DstIP;                // 目的IP地址
} IPStruct;

typedef struct PacketStruct {	        // 包含帧首部和IP首部的数据包
    FrameStruct FrameHeader;  // 帧首部
    IPStruct IPHeader;        // IP首部
} PacketStruct;

#pragma pack()	                // 恢复默认对齐方式

typedef struct PacketCache {       // 缓存队列的数据包
    DWORD ip;                   // 目的IP地址
    int IfNo;                   // 接口序号
    int len;                    // 数据包长度
    char data[65535];           // 数据包
} PacketCache;

typedef struct DeviceInfo {	    // 接口信息
    char DeviceName[64];        // 设备名
    char Description[128];      // 设备描述
    BYTE MACAddr[6];            // MAC地址
    DWORD ip[5];                // IP地址列表
    DWORD netmask[5];           // 掩码地址列表
    pcap_t* adhandle;           // pcap句柄
} IfInfo_t;

typedef struct MyRouteTable {	// 路由表表项
    DWORD Mask;                 // 子网掩码
    DWORD DstIP;                // 目的地址
    DWORD NextHop;              // 下一跳步              
} MyRouteTable;

typedef struct ARPTable {       // IP-MAC地址映射表表项
    DWORD IPAddr;               // IP地址
    BYTE MACAddr[6];            // MAC地址
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

