
// myRouterDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "myRouter.h"
#include "myRouterDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

string info;
DeviceInfo nowAdapter;
vector<ARPTable> IPMACTable;
vector<MyRouteTable> RouteTable;
list<PacketCache> cache;


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CmyRouterDlg dialog



CmyRouterDlg::CmyRouterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MYROUTER_DIALOG, pParent)
	, mask(0)
	, des(0)
	, next(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CmyRouterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, LOG, log);
	DDX_Control(pDX, TABLE, Ctable);
	DDX_IPAddress(pDX, MASK, mask);
	DDX_IPAddress(pDX, DES, des);
	DDX_IPAddress(pDX, NEXT, next);
	DDX_Control(pDX, MASK, cmask);
	DDX_Control(pDX, DES, cdes);
	DDX_Control(pDX, NEXT, cnext);
}

BEGIN_MESSAGE_MAP(CmyRouterDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CmyRouterDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CmyRouterDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CmyRouterDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CmyRouterDlg message handlers

BOOL CmyRouterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//初始化的时候就进行设备选择等
	pcap_if_t* adapters;
	char errbuf[PCAP_ERRBUF_SIZE];
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &adapters, errbuf) == -1) {
		info.append("Error in pcap_findalldevs_ex :").append(errbuf);
		log.SetWindowTextW(CA2T(info.c_str()));
		return -1;
	}

	int flag = 0, ip_num = 0;
	for (pcap_addr_t* a = adapters->addresses; a != NULL; a = a->next) {
		if (a->addr->sa_family == AF_INET) {
			flag = 1;
			strcpy(nowAdapter.DeviceName, adapters->name);
			if (adapters->description) {
				strcpy(nowAdapter.Description, adapters->description);
			}
			else {
				strcpy(nowAdapter.Description, "null");
			}
			//考虑到一块网卡可能绑定多个IP地址
			nowAdapter.ip[ip_num] = ((struct sockaddr_in*)a->addr)->sin_addr.S_un.S_addr;
			nowAdapter.netmask[ip_num] = ((struct sockaddr_in*)a->netmask)->sin_addr.S_un.S_addr;
			ip_num++;
		}
	}
	if (flag == 0) {
		MessageBox(_T("error!"));
		return 0;
	}
	if ((nowAdapter.adhandle =
		pcap_open(nowAdapter.DeviceName,
			65535,
			PCAP_OPENFLAG_PROMISCUOUS,
			1000,
			NULL,
			errbuf)) == NULL) {
		MessageBox(_T("网卡设备打不开!"));
		return -1;
	}

	// 获取网卡设备MAC地址
	GetDeviceMAC(&nowAdapter);

	// 输出网卡MAC地址:
	info.append("打印网卡参数：\r\n设备名：").append(nowAdapter.DeviceName).append("\r\nMAC地址：").append(BYTEToMac(nowAdapter.MACAddr)).append("\r\nIP地址：\r\n");
	for (int j = 0; nowAdapter.ip[j] != '\0'; j++) {
		info.append(DWORDToIP(nowAdapter.ip[j])).append("\t\t");
	}
	info.append("\r\n");
	log.SetWindowTextW(CA2T(info.c_str()));

	// 初始化路由表
	MyRouteTable rt;
	// 直接投递的路由表项
	for (int i = 0; i < ip_num; i++) {
		rt.Mask = nowAdapter.netmask[i];
		rt.DstIP = nowAdapter.ip[i] & nowAdapter.netmask[i];
		rt.NextHop = 0;
		RouteTable.push_back(rt);
	}

	pcap_freealldevs(adapters);

	// 动态路由开启数据包捕获线程
	CreateThread(NULL, 0, &CapturePacket, &nowAdapter, 0, NULL);

	string table = ShowRouteTable();
	Ctable.SetWindowTextW(CA2T(table.c_str()));
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CmyRouterDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CmyRouterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CmyRouterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CmyRouterDlg::OnBnClickedOk()
{
	
	// TODO: Add your control notification handler code here
	CDialogEx::OnOK();
}


void CmyRouterDlg::OnBnClickedButton1()
{
	MyRouteTable rt;
	cmask.GetAddress(mask);
	//如果不逆序的话，最终输出的IP是反着的
	WORD hiWord = HIWORD(mask);
	WORD loWord = LOWORD(mask);
	BYTE nf1 = HIBYTE(hiWord);
	BYTE nf2 = LOBYTE(hiWord);
	BYTE nf3 = HIBYTE(loWord);
	BYTE nf4 = LOBYTE(loWord);
	DWORD rmask = nf1 | nf2 << 8 | nf3 << 16 | nf4 << 24;
	rt.Mask = rmask;

	cdes.GetAddress(des);
	hiWord = HIWORD(des);
	loWord = LOWORD(des);
	nf1 = HIBYTE(hiWord);
	nf2 = LOBYTE(hiWord);
	nf3 = HIBYTE(loWord);
	nf4 = LOBYTE(loWord);
	DWORD rdes = nf1 | nf2 << 8 | nf3 << 16 | nf4 << 24;
	rt.DstIP = rdes;

	cnext.GetAddress(next);
	hiWord = HIWORD(next);
	loWord = LOWORD(next);
	nf1 = HIBYTE(hiWord);
	nf2 = LOBYTE(hiWord);
	nf3 = HIBYTE(loWord);
	nf4 = LOBYTE(loWord);
	DWORD rnext = nf1 | nf2 << 8 | nf3 << 16 | nf4 << 24;
	rt.NextHop = rnext;

	RouteTable.push_back(rt);
	info.append("已增加该路由表项\r\n");
	log.SetWindowTextW(CA2T(info.c_str()));
	string table = ShowRouteTable();
	Ctable.SetWindowTextW(CA2T(table.c_str()));
}


void CmyRouterDlg::OnBnClickedButton2()
{
	cmask.GetAddress(mask);
	//如果不逆序的话，最终输出的IP是反着的
	WORD hiWord = HIWORD(mask);
	WORD loWord = LOWORD(mask);
	BYTE nf1 = HIBYTE(hiWord);
	BYTE nf2 = LOBYTE(hiWord);
	BYTE nf3 = HIBYTE(loWord);
	BYTE nf4 = LOBYTE(loWord);
	DWORD rmask = nf1 | nf2 << 8 | nf3 << 16 | nf4 << 24;

	cdes.GetAddress(des);
	hiWord = HIWORD(des);
	loWord = LOWORD(des);
	nf1 = HIBYTE(hiWord);
	nf2 = LOBYTE(hiWord);
	nf3 = HIBYTE(loWord);
	nf4 = LOBYTE(loWord);
	DWORD rdes = nf1 | nf2 << 8 | nf3 << 16 | nf4 << 24;

	cnext.GetAddress(next);
	hiWord = HIWORD(next);
	loWord = LOWORD(next);
	nf1 = HIBYTE(hiWord);
	nf2 = LOBYTE(hiWord);
	nf3 = HIBYTE(loWord);
	nf4 = LOBYTE(loWord);
	DWORD rnext = nf1 | nf2 << 8 | nf3 << 16 | nf4 << 24;
	// TODO: Add your control notification handler code here
	if (rnext == 0) {
		info.append("直接投递路由，不能删除\r\n");
		log.SetWindowTextW(CA2T(info.c_str()));
		return;
	}
	if (RouteTable.empty()) {
		return;
	}
	// 遍历路由表项
	vector<MyRouteTable>::iterator i;
	for (i = RouteTable.begin(); i != RouteTable.end(); i++) {
		if ((i->DstIP == rdes)
			&& (i->Mask == rmask)
			&& (i->NextHop == rnext)) {
			RouteTable.erase(i);
			info.append("已删除该路由表项\r\n");
			log.SetWindowTextW(CA2T(info.c_str()));
			string table = ShowRouteTable();
			Ctable.SetWindowTextW(CA2T(table.c_str()));
			return;
		}
	}
	info.append("不存在该路由表项\r\n");
	log.SetWindowTextW(CA2T(info.c_str()));
	string table = ShowRouteTable();
	Ctable.SetWindowTextW(CA2T(table.c_str()));
}


void CmyRouterDlg::showLog()
{
	// TODO: Add your implementation code here.
	log.SetWindowTextW(CA2T(info.c_str()));
}

char* BYTEToMac(BYTE* mac) {
	char* buffer = new char[32];
	sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return buffer;
}

char* DWORDToIP(DWORD ip) {
	u_char* p;
	p = (u_char*)&ip;
	char* buffer = new char[32];
	sprintf(buffer, "%03d.%03d.%03d.%03d", p[0], p[1], p[2], p[3]);
	return buffer;
}

//显示路由表
string ShowRouteTable() {
	string table;
	table.append("MASK----------DES----------NextHop\r\n");
	vector<MyRouteTable>::iterator i;
	for (i = RouteTable.begin(); i != RouteTable.end(); i++) {
		table.append(DWORDToIP(i->Mask)).append("---").append(DWORDToIP(i->DstIP)).append("---").append(DWORDToIP(i->NextHop));
		if (i->NextHop == 0) {
			table.append("(直接投递)");
		}
		table.append("\r\n");
	}
	return table;
}

//发送ARP请求
void ARPRequest(pcap_t* adhandle, BYTE* SrcMAC, DWORD SendIp, DWORD RecvIp) {
	ARPStruct  ARPFrame;
	ARPFrame.FrameHeader.FrameType = htons(0x0806);
	ARPFrame.HardwareType = htons(0x0001);
	ARPFrame.ProtocolType = htons(0x0800);
	ARPFrame.HLen = 6;
	ARPFrame.PLen = 4;
	ARPFrame.Operation = htons(0x0001);
	ARPFrame.SendIP = SendIp;
	ARPFrame.RecvIP = RecvIp;

	for (int i = 0; i < 6; i++)
	{
		ARPFrame.FrameHeader.DesMAC[i] = 0xff;
		ARPFrame.FrameHeader.SrcMAC[i] = SrcMAC[i];
		ARPFrame.SendHa[i] = SrcMAC[i];
		ARPFrame.RecvHa[i] = 0x00;
	}
	if (pcap_sendpacket(adhandle, (u_char*)&ARPFrame, sizeof(ARPStruct)) == 0) {
		info.append("发送ARP请求成功!\r\n\r\n");
		WriteLog();
		return;
	}
}

// 查询路由表: 策略是从头查到尾, 选择掩码长度最长的
// 下一跳为0的直接投递
DWORD SearchRouteTable(DWORD DstIP) {
	DWORD temp;
	int flag = 0;
	DWORD maxmask = 0;
	vector<MyRouteTable>::iterator i;
	for (i = RouteTable.begin(); i != RouteTable.end(); i++) {
		if ((DstIP & i->Mask) == i->DstIP) {
			if (i->Mask >= maxmask) {
				flag = 1;
				if (i->NextHop == 0) {// 直接投递
					temp = DstIP;
				}
				else {
					temp = i->NextHop;
				}
				maxmask = i->Mask;
			}
		}
	}
	if (flag) {
		return temp;
	}
	return -1;
}

// 计算校验和
u_short checksum(u_short* buffer, int size) {
	u_long cksum = 0;
	while (size > 1)
	{
		cksum += *buffer++;
		size -= sizeof(u_short);
	}
	if (size)
	{
		cksum += *(u_char*)buffer;
	}
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (u_short)(~cksum);
}
// 检验校验和
bool checkIPHeader(char* buff) {
	IPStruct* ip_header = (IPStruct*)buff;
	u_short checksumBuf = ip_header->Checksum;
	u_short check_buff[sizeof(IPStruct)];
	ip_header->Checksum = 0;
	memset(check_buff, 0, sizeof(IPStruct));
	memcpy(check_buff, ip_header, sizeof(IPStruct));
	ip_header->Checksum = checksum(check_buff, sizeof(IPStruct));
	if (ip_header->Checksum == checksumBuf)
	{
		return 1;
	}
	return 0;
}


//获取本地网卡MAC地址
void GetDeviceMAC(LPVOID lpParameter) {
	DeviceInfo* device = (DeviceInfo*)lpParameter;
	ARPStruct* ARPFrame;
	pcap_pkthdr* header;
	const u_char* pkt_data;
	UCHAR SrcMAC[6];
	DWORD SrcIP;
	// 无所谓
	for (int i = 0; i < 6; i++) {
		SrcMAC[i] = 0x55;
	}
	SrcIP = inet_addr("5.5.5.5");
	// 发送ARP报文
	ARPRequest(device->adhandle, SrcMAC, SrcIP, device->ip[0]);
	int res;
	while (true) {
		res = pcap_next_ex(device->adhandle, &header, &pkt_data);
		if (res == 0) {
			// 超时时间到
			continue;
		}
		else if (res == 1) {
			ARPFrame = (ARPStruct*)(pkt_data);
			if (ARPFrame->FrameHeader.FrameType == htons(0x0806)
				&& ARPFrame->Operation == htons(0x0002)
				&& ARPFrame->SendIP == device->ip[0]) {
				ARPTable ipmac;
				for (int i = 0; i < 6; i++) {
					device->MACAddr[i] = ARPFrame->SendHa[i];
					ipmac.MACAddr[i] = ARPFrame->SendHa[i];
				}
				for (int j = 0; device->ip[j] != '\0'; j++) {
					ipmac.IPAddr = device->ip[j];
					IPMACTable.push_back(ipmac);
				}
				return;
			}
		}
	}
	return;
}

bool MACTableSearch(DWORD ip, BYTE* MAC) {
	if (IPMACTable.empty()) {
		return false;
	}
	vector<ARPTable>::iterator i;
	for (i = IPMACTable.begin(); i != IPMACTable.end(); i++) {
		if (ip == i->IPAddr) {
			for (int j = 0; j < 6; j++) {
				MAC[j] = i->MACAddr[j];
			}
			return true;
		}
	}
	return false;
}

void MACTableUpdate(DWORD ip, BYTE* MAC) {
	if (IPMACTable.empty()) {
		return;
	}
	vector<ARPTable>::iterator i;
	for (i = IPMACTable.begin(); i != IPMACTable.end(); i++) {
		if (ip == i->IPAddr) {
			for (int j = 0; j < 6; j++) {
				i->MACAddr[j] = MAC[j];
			}
			return;
		}
	}
	return;
}

// 处理ARP数据报
void DealARP(struct pcap_pkthdr* header, const u_char* pkt_data) {
	// 更新IP地址和MAC地址的对应关系
	ARPStruct* ARP;
	ARP = (ARPStruct*)pkt_data;
	BYTE mac[6];
	if (ARP->Operation == htons(0x0002)) {
		// 收到响应数据报, 搜索ARP缓存表
		info.append("收到ARP响应数据报：");
		info.append(DWORDToIP(ARP->SendIP));
		info.append("--");
		info.append(BYTEToMac(ARP->SendHa));
		info.append("\r\n");
		WriteLog();
		if (MACTableSearch(ARP->SendIP, mac)) {
			// 判断MAC地址映射表中的MAC地址是否为最新
			if (memcmp(mac, ARP->SendHa, 6) == 0) {
				info.append("该映射关系已存在。\r\n\r\n");
				WriteLog();
			}
			else {
				MACTableUpdate(ARP->SendIP, ARP->SendHa);
				info.append("该映射关系已更新。\r\n\r\n");
				WriteLog();
			}
		}
		else {
			ARPTable ipmac;
			ipmac.IPAddr = ARP->SendIP;
			for (int i = 0; i < 6; i++) {
				ipmac.MACAddr[i] = ARP->SendHa[i];
			}
			IPMACTable.push_back(ipmac);
			info.append("该映射关系已保存。\r\n\r\n");
			WriteLog();
			if (cache.empty()) {
				return;
			}
			list<PacketCache>::iterator i;// 遍历数据包缓存队列查看是否有可以转发的IP数据包
			int j = 0;
			for (i = cache.begin(); i != cache.end() && j < cache.size(); i++, j++) {
				if (i->ip == ARP->SendIP) {// 如果有数据包的目标IP地址为ARP包IP地址
					PacketStruct* IPFrame;
					IPFrame = (PacketStruct*)i->data;
					for (int j = 0; j < 6; j++) {
						IPFrame->FrameHeader.DesMAC[j] = ARP->SendHa[j];
					}
					if (pcap_sendpacket(nowAdapter.adhandle, (u_char*)i->data, i->len) == -1) {
						info.append("发送失败。\r\n\r\n");
						WriteLog();
						return;
					}
					else {
						cache.erase(i);
						info.append("转发缓存区中目的地址是该MAC地址的IP数据包,转发IP数据包：\n");
						info.append(DWORDToIP(IPFrame->IPHeader.SrcIP));
						info.append("->");
						info.append(DWORDToIP(i->ip));
						info.append(" ");
						info.append(BYTEToMac(IPFrame->FrameHeader.SrcMAC));
						info.append("->");
						info.append(BYTEToMac(IPFrame->FrameHeader.DesMAC));
						info.append("\r\n\r\n");
						WriteLog();
					}
				}
			}
		}
	}
}

// 处理IP数据包
void DealIP(struct pcap_pkthdr* header, const u_char* pkt_data) {
	PacketStruct* IPFrame;
	IPFrame = (PacketStruct*)pkt_data;
	info.append("收到IP数据报：");
	info.append(DWORDToIP(IPFrame->IPHeader.SrcIP));
	info.append("->");
	info.append(DWORDToIP(IPFrame->IPHeader.DstIP));
	info.append("\r\n");
	// 本地发出的报文不进行处理
	if (IPFrame->FrameHeader.SrcMAC[0] == nowAdapter.MACAddr[0]
		&& IPFrame->FrameHeader.SrcMAC[1] == nowAdapter.MACAddr[1]
		&& IPFrame->FrameHeader.SrcMAC[2] == nowAdapter.MACAddr[2]
		&& IPFrame->FrameHeader.SrcMAC[3] == nowAdapter.MACAddr[3]
		&& IPFrame->FrameHeader.SrcMAC[4] == nowAdapter.MACAddr[4]
		&& IPFrame->FrameHeader.SrcMAC[5] == nowAdapter.MACAddr[5]
		)
		return;
	// 超时的进行丢弃
	if (IPFrame->IPHeader.TTL <= 0) {// 超时
		return;
	}
	// 数据报校验
	IPStruct* IpHeader = &(IPFrame->IPHeader);
	if (checkIPHeader((char*)IpHeader) == 0) {
		// 校验出错
		info.append("数据报头部校验和出错，丢弃数据报。\r\n");
		WriteLog();
		return;
	}
	// 如果当前路由表中存在该网络路由路径, 直接投递
	DWORD NextHop;		// 经过路由选择算法得到的下一站目的IP地址
	if ((NextHop = SearchRouteTable((DWORD)IPFrame->IPHeader.DstIP)) == -1) {
		// 找不到对应路由表项，抛弃数据报
		info.append("路由选择失败，抛弃报文!\r\n\r\n");
		WriteLog();
		return;
	}
	// 转发数据报
	else {
		// 转发数据帧的源MAC地址变成路由器端口MAC地址，目的地址将在MAC地址映射表中查询
		for (int i = 0; i < 6; i++) {
			IPFrame->FrameHeader.SrcMAC[i] = nowAdapter.MACAddr[i];
		}
		IPFrame->IPHeader.TTL -= 1;// TTL减一
		u_short check_buff[sizeof(IPStruct)];
		IPFrame->IPHeader.Checksum = 0;
		memset(check_buff, 0, sizeof(IPStruct));
		IPStruct* ip_header = &(IPFrame->IPHeader);
		memcpy(check_buff, ip_header, sizeof(IPStruct));
		// 计算IP头部校验和
		IPFrame->IPHeader.Checksum = checksum(check_buff, sizeof(IPStruct));
		// 查询MAC地址映射表，更新数据帧头目的MAC地址
		if (MACTableSearch(NextHop, IPFrame->FrameHeader.DesMAC)) {
			int res = pcap_sendpacket(nowAdapter.adhandle, (u_char*)pkt_data, header->len);
			if (res == 0) {
				// 成功转发
				info.append("IP数据报转发：");
				info.append(DWORDToIP(IPFrame->IPHeader.SrcIP));
				info.append("->");
				info.append(DWORDToIP(NextHop));
				info.append("\n");
				info.append(BYTEToMac(IPFrame->FrameHeader.SrcMAC));
				info.append("->");
				info.append(BYTEToMac(IPFrame->FrameHeader.DesMAC));
				info.append("\r\n\r\n");
			}
			else {
				info.append("转发IP数据报时出错!\r\n\r\n");
			}
		}
		else {
			// MAC地址映射表不存在映射关系
			PacketCache* Packet = new PacketCache();
			Packet->ip = NextHop;
			Packet->len = header->len;
			memset(Packet->data, 0, Packet->len);
			memcpy(Packet->data, pkt_data, Packet->len);
			if (cache.size() < 65535) {// 存入缓存队列
				cache.push_back(*Packet);
				info.append("缺少目的MAC地址，已存入缓冲区：\r\n");
				info.append(DWORDToIP(IPFrame->IPHeader.SrcIP));
				info.append("->");
				info.append(DWORDToIP(IPFrame->IPHeader.DstIP));
				info.append(" ");
				info.append(BYTEToMac(IPFrame->FrameHeader.SrcMAC));
				info.append("->");
				info.append("xx:xx:xx:xx:xx:xx");
				info.append("已发送ARP请求:");
				info.append(DWORDToIP(Packet->ip));
				info.append("\r\n\r\n");
				ARPRequest(nowAdapter.adhandle, nowAdapter.MACAddr, nowAdapter.ip[0], Packet->ip);
			}
			else {
				// 存满了丢弃数据包
				info.append("转发缓冲区溢出，丢弃IP数据包：\r\n");
				info.append(DWORDToIP(IPFrame->IPHeader.SrcIP));
				info.append("->");
				info.append(DWORDToIP(IPFrame->IPHeader.DstIP));
				info.append(" ");
				info.append(BYTEToMac(IPFrame->FrameHeader.SrcMAC));
				info.append("->");
				info.append("xx:xx:xx:xx:xx:xx\r\n\r\n");
			}
		}
		WriteLog();
	}
}

//数据包捕获
DWORD WINAPI CapturePacket(LPVOID lpParameter) {
	DeviceInfo* device = (DeviceInfo*)lpParameter;
	pcap_pkthdr* header;
	const u_char* pkt_data;
	int res;
	while (true) {
		// 捕获接口的数据包
		res = pcap_next_ex(device->adhandle, &header, &pkt_data);
		/*CWnd* pWnd = AfxGetMainWnd();
		HWND hHwnd = pWnd->m_hWnd;
		MessageBoxW(hHwnd, _T("hhh"), _T("hhhh"), MB_OK);*/
		//HWND hwnd;
		//hwnd = ::GetDlgItem(AfxGetMainWnd()->m_hWnd, LOG); //获取句柄
		//SetWindowText(hwnd, _T("hhhhhhhhhhhhhhh")); //设置文本
		if (res == 0) {
			// 超时时间到
			continue;
		}
		else if (res == 1) {
			FrameStruct* FrameHeader;
			FrameHeader = (FrameStruct*)pkt_data;
			// 判断数据报类型
		/*	CWnd* pWnd = AfxGetMainWnd();
			HWND hHwnd = pWnd->m_hWnd;
			MessageBoxW(hHwnd, _T("收到数据报"), _T("数据包"), MB_OK);*/
			if (memcmp(FrameHeader->DesMAC, device->MACAddr, 6) == 0) {
				switch (ntohs(FrameHeader->FrameType))
				{
					// 如果是ARP数据报: 回应本地MAC地址
				case 0x0806:
					DealARP(header, pkt_data);
					//MessageBoxW(hHwnd, _T("收到ARP数据报"), _T("ARP"), MB_OK);
					break;
					// 如果是IP数据报: 能处理处理, 不能处理暂时放入缓存区当中
				case 0x0800:
					DealIP(header, pkt_data);
					//MessageBoxW(hHwnd, _T("收到IP数据报"), _T("IP"), MB_OK);
					break;
				default:
					break;
				}
			}
		}
	}
	return 0;
}

void WriteLog() {
	FILE* fptr;
	fptr = fopen("log.txt", "w");
	if (fptr == NULL)
	{
		printf("Error!");
		exit(1);
	}
	fprintf(fptr, "%s", info.c_str());
	fclose(fptr);
}