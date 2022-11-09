#include <WinSock2.h>
#include <Windows.h>
#include <pcap.h>

#pragma warning(disable:4996)
#pragma comment(lib, "packet.lib")
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "ws2_32.lib")

/* ��̫��ͷ����֡�ṹ���װ */
struct ether_header {
    u_char ether_dhost[6];    // Ŀ���ַ
    u_char ether_shost[6];    // Դ��ַ
    u_short ether_type;       // ��̫������
};

/* �����IPЭ���װ�ṹ���װ */
struct ip_hander {
    u_char      _version_handerLen;     // �汾 4  �ײ����� 4
    u_char      _diffserv;              // ��������
    u_short     _totalLen;              // �ܳ���
    u_short     _identification;        // ��ʶ
    u_short     _flag_offset;           // ��־ 3 + Ƭƫ�� 13
    u_char      _timeLive;              // ����ʱ��
    u_char      _protocol;              // Э��
    u_short     _checkSum;              // �ײ�У���
    long        _src;                   // Դ��ַ
    long        _desc;                  // Ŀ�ĵ�ַ
};

/* �����TCPЭ�� */
struct TCP_hander {
    u_short      _sport;         // Դ�˿�16bits
    u_short      _dport;         // Ŀ�Ķ˿�16bits
    u_int        _seqNum;        // ���к�32bits
    u_int        _ackNum;        // ȷ�Ϻ�32bits
    u_short      _off_res_flag;  // ����ƫ�� 4  ����λ 6  ��־λ 6
    u_short      _winSize;       // ���ڴ�С16bits
    u_short      _checkSum;      // У���16bits
    u_short      _urgentPoint;   // ����ָ��16bits
};

/* �����UDPЭ�� */
struct UDP_handler {
    u_short      _sport;     // Դ�˿�
    u_short      _dport;     // Ŀ�Ķ˿�
    u_short      _len;       // ���ݳ���
    u_short      _checksum;  // У���
};

// ���MAC��ַ��
void PrintEtherHeader(const u_char* packetData)
{
    ether_header* eth_protocol;
    eth_protocol = (ether_header*)packetData;
    u_char* ether_src = eth_protocol->ether_shost;         // ��̫��ԭʼMAC��ַ
    u_char* ether_dst = eth_protocol->ether_dhost;         // ��̫��Ŀ��MAC��ַ
    u_short ether_type = ntohs(eth_protocol->ether_type);  // ��̫������

    //printf("���� %d \t", sizeof(eth_protocol));
    printf("����: %04x \t", ether_type);
    printf("ԴMAC��ַ: %02X:%02X:%02X:%02X:%02X:%02X \t",
        ether_src[0], ether_src[1], ether_src[2], ether_src[3], ether_src[4], ether_src[5]);
    printf("Ŀ��MAC��ַ: %02X:%02X:%02X:%02X:%02X:%02X \n",
        ether_dst[0], ether_dst[1], ether_dst[2], ether_dst[3], ether_dst[4], ether_dst[5]);
}

// IP��ַ�Ĳ���
void PrintIPHeader(const u_char* packetData)
{
    ip_hander* ip_protocol;

    // Ϊ������������·��
    ip_protocol = (ip_hander*)(packetData + 14);
    SOCKADDR_IN Src_Addr, Dst_Addr = { 0 };

    u_short check_sum = ntohs(ip_protocol->_checkSum);
    int ttl = ip_protocol->_timeLive;
    int proto = ip_protocol->_protocol;

    Src_Addr.sin_addr.s_addr = ip_protocol->_src;
    Dst_Addr.sin_addr.s_addr = ip_protocol->_desc;

    printf("Դ��ַ: %15s --> ", inet_ntoa(Src_Addr.sin_addr));
    printf("Ŀ���ַ: %15s --> ", inet_ntoa(Dst_Addr.sin_addr));

    printf("У���: %5X --> TTL: %4d --> Э������: ", check_sum, ttl);
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

// ��ӡTCP��Ϣ
void PrintTCPHeader(const unsigned char* packetData)
{
    TCP_hander* tcp_protocol;
    // +14 ����������·�� +20 ����IP��
    tcp_protocol = (TCP_hander*)(packetData + 14 + 20);

    u_short sport = ntohs(tcp_protocol->_sport);
    u_short dport = ntohs(tcp_protocol->_dport);
    int window = tcp_protocol->_winSize;
    int flags = tcp_protocol->_off_res_flag;

    printf("Դ�˿�: %6d --> Ŀ��˿�: %6d --> ���ڴ�С: %7d --> ��־: (%d)",
        sport, dport, window, flags);

    if (flags & 0x08) printf("PSH ���ݴ���\n");
    else if (flags & 0x10) printf("ACK ��Ӧ\n");
    else if (flags & 0x02) printf("SYN ��������\n");
    else if (flags & 0x20) printf("URG \n");
    else if (flags & 0x01) printf("FIN �ر�����\n");
    else if (flags & 0x04) printf("RST ��������\n");
    else printf("None δ֪\n");
}

// ��ӡUDP��Ϣ
void PrintUDPHeader(const unsigned char* packetData)
{
    UDP_handler* udp_protocol;
    // +14 ����������·�� +20 ����IP��
    udp_protocol = (UDP_handler*)(packetData + 14 + 20);

    u_short sport = ntohs(udp_protocol->_sport);
    u_short dport = ntohs(udp_protocol->_dport);
    u_short datalen = ntohs(udp_protocol->_len);

    printf("Դ�˿�: %5d --> Ŀ��˿�: %5d --> ��С: %5d \n", sport, dport, datalen);
}


void MonitorAdapter(int nChoose)
{
    pcap_if_t* adapters;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &adapters, errbuf) != -1)
    {
        // �ҵ�ָ��������
        for (int x = 0; x < nChoose - 1; ++x) {
            // �Ҳ��������
            if (adapters == NULL) {
                printf("error!");
                return;
            }
            adapters = adapters->next;
        }

        char errorBuf[PCAP_ERRBUF_SIZE];

        pcap_t* handle = pcap_open(adapters->name, //�򿨵��豸����
            65536, //���뱣�����ĳ���
            PCAP_OPENFLAG_PROMISCUOUS, // ����Ϊ����ģʽ
            1000, // ��ʱ1s
            NULL, // Զ������ȷ��
            errbuf // ������Ϣ
        );

        printf("��ʼ����: % \n", adapters->description);
        pcap_pkthdr* Packet_Header;    // ���ݰ�ͷ
        const u_char* Packet_Data;    // ���ݱ���
        //�����������ݰ�
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
