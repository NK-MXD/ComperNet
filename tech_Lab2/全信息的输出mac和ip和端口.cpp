#include <WinSock2.h>
#include <Windows.h>
#include <pcap.h>

#pragma warning(disable:4996)
#pragma comment(lib, "packet.lib")
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "ws2_32.lib")

/* 以太网头数据帧结构体封装 */
struct ether_header {
    u_char ether_dhost[6];    // 目标地址
    u_char ether_shost[6];    // 源地址
    u_short ether_type;       // 以太网类型
};

/* 网络层IP协议封装结构体封装 */
struct ip_hander {
    u_char      _version_handerLen;     // 版本 4  首部长度 4
    u_char      _diffserv;              // 服务类型
    u_short     _totalLen;              // 总长度
    u_short     _identification;        // 标识
    u_short     _flag_offset;           // 标志 3 + 片偏移 13
    u_char      _timeLive;              // 生存时间
    u_char      _protocol;              // 协议
    u_short     _checkSum;              // 首部校验和
    long        _src;                   // 源地址
    long        _desc;                  // 目的地址
};

/* 传输层TCP协议 */
struct TCP_hander {
    u_short      _sport;         // 源端口16bits
    u_short      _dport;         // 目的端口16bits
    u_int        _seqNum;        // 序列号32bits
    u_int        _ackNum;        // 确认号32bits
    u_short      _off_res_flag;  // 数据偏移 4  保留位 6  标志位 6
    u_short      _winSize;       // 窗口大小16bits
    u_short      _checkSum;      // 校验和16bits
    u_short      _urgentPoint;   // 紧急指针16bits
};

/* 传输层UDP协议 */
struct UDP_handler {
    u_short      _sport;     // 源端口
    u_short      _dport;     // 目的端口
    u_short      _len;       // 数据长度
    u_short      _checksum;  // 校验和
};

// 输出MAC地址等
void PrintEtherHeader(const u_char* packetData)
{
    ether_header* eth_protocol;
    eth_protocol = (ether_header*)packetData;
    u_char* ether_src = eth_protocol->ether_shost;         // 以太网原始MAC地址
    u_char* ether_dst = eth_protocol->ether_dhost;         // 以太网目标MAC地址
    u_short ether_type = ntohs(eth_protocol->ether_type);  // 以太网类型

    //printf("长度 %d \t", sizeof(eth_protocol));
    printf("类型: %04x \t", ether_type);
    printf("源MAC地址: %02X:%02X:%02X:%02X:%02X:%02X \t",
        ether_src[0], ether_src[1], ether_src[2], ether_src[3], ether_src[4], ether_src[5]);
    printf("目标MAC地址: %02X:%02X:%02X:%02X:%02X:%02X \n",
        ether_dst[0], ether_dst[1], ether_dst[2], ether_dst[3], ether_dst[4], ether_dst[5]);
}

// IP地址的捕获
void PrintIPHeader(const u_char* packetData)
{
    ip_hander* ip_protocol;

    // 为了跳过数据链路层
    ip_protocol = (ip_hander*)(packetData + 14);
    SOCKADDR_IN Src_Addr, Dst_Addr = { 0 };

    u_short check_sum = ntohs(ip_protocol->_checkSum);
    int ttl = ip_protocol->_timeLive;
    int proto = ip_protocol->_protocol;

    Src_Addr.sin_addr.s_addr = ip_protocol->_src;
    Dst_Addr.sin_addr.s_addr = ip_protocol->_desc;

    printf("源地址: %15s --> ", inet_ntoa(Src_Addr.sin_addr));
    printf("目标地址: %15s --> ", inet_ntoa(Dst_Addr.sin_addr));

    printf("校验和: %5X --> TTL: %4d --> 协议类型: ", check_sum, ttl);
    switch (ip_protocol->_protocol)
    {
        case 1: printf("ICMP \n"); break;
        case 2: printf("IGMP \n"); break;
        case 6: printf("TCP \n");  break;
        case 17: printf("UDP \n"); break;
        case 89: printf("OSPF \n"); break;
        default: printf("None \n"); break;
    }
}

// 打印TCP信息
void PrintTCPHeader(const unsigned char* packetData)
{
    TCP_hander* tcp_protocol;
    // +14 跳过数据链路层 +20 跳过IP层
    tcp_protocol = (TCP_hander*)(packetData + 14 + 20);

    u_short sport = ntohs(tcp_protocol->_sport);
    u_short dport = ntohs(tcp_protocol->_dport);
    int window = tcp_protocol->_winSize;
    int flags = tcp_protocol->_off_res_flag;

    printf("源端口: %6d --> 目标端口: %6d --> 窗口大小: %7d --> 标志: (%d)",
        sport, dport, window, flags);

    if (flags & 0x08) printf("PSH 数据传输\n");
    else if (flags & 0x10) printf("ACK 响应\n");
    else if (flags & 0x02) printf("SYN 建立连接\n");
    else if (flags & 0x20) printf("URG \n");
    else if (flags & 0x01) printf("FIN 关闭连接\n");
    else if (flags & 0x04) printf("RST 连接重置\n");
    else printf("None 未知\n");
}

// 打印UDP信息
void PrintUDPHeader(const unsigned char* packetData)
{
    UDP_handler* udp_protocol;
    // +14 跳过数据链路层 +20 跳过IP层
    udp_protocol = (UDP_handler*)(packetData + 14 + 20);

    u_short sport = ntohs(udp_protocol->_sport);
    u_short dport = ntohs(udp_protocol->_dport);
    u_short datalen = ntohs(udp_protocol->_len);

    printf("源端口: %5d --> 目标端口: %5d --> 大小: %5d \n", sport, dport, datalen);
}


void MonitorAdapter(int nChoose)
{
    pcap_if_t* adapters;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &adapters, errbuf) != -1)
    {
        // 找到指定的网卡
        for (int x = 0; x < nChoose - 1; ++x) {
            // 找不到的情况
            if (adapters == NULL) {
                printf("error!");
                return;
            }
            adapters = adapters->next;
        }

        char errorBuf[PCAP_ERRBUF_SIZE];

        pcap_t* handle = pcap_open(adapters->name, //打卡的设备名称
            65536, //必须保留包的长度
            PCAP_OPENFLAG_PROMISCUOUS, // 网卡为混乱模式
            1000, // 超时1s
            NULL, // 远程连接确认
            errbuf // 错误信息
        );

        printf("开始侦听: % \n", adapters->description);
        pcap_pkthdr* Packet_Header;    // 数据包头
        const u_char* Packet_Data;    // 数据本身
        //持续接收数据包
        while (pcap_next_ex(handle, &Packet_Header, &Packet_Data) >= 0) {
            PrintEtherHeader(Packet_Data);
            PrintIPHeader(Packet_Data);
            PrintTCPHeader(Packet_Data);
            PrintUDPHeader(Packet_Data);
        }
    }
}

int main(int argc, char* argv[])
{
    MonitorAdapter(5);
}
