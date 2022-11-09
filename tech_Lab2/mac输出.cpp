#include <WinSock2.h>
#include <Windows.h>
#include <pcap.h>

#pragma comment(lib, "packet.lib")
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "ws2_32.lib")

/* 以太网头数据帧结构体封装 */
struct ether_header {
    u_char ether_dhost[6];    // 目标地址
    u_char ether_shost[6];    // 源地址
    u_short ether_type;       // 以太网类型
};

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
        while (pcap_next_ex(handle, &Packet_Header, &Packet_Data) >= 0){
            ether_header* eth_protocol;
            eth_protocol = (ether_header*)Packet_Data;
            u_short ether_type = ntohs(eth_protocol->ether_type);  // 以太网类型
            u_char* ether_src = eth_protocol->ether_shost;         // 以太网原始MAC地址
            u_char* ether_dst = eth_protocol->ether_dhost;         // 以太网目标MAC地址

            printf("侦听长度: %d \t", Packet_Header->len);
            printf("类型: %04x \t", ether_type);
            printf("源MAC地址: %02X:%02X:%02X:%02X:%02X:%02X \t",
                ether_src[0], ether_src[1], ether_src[2], ether_src[3], ether_src[4], ether_src[5]);
            printf("目标MAC地址: %02X:%02X:%02X:%02X:%02X:%02X \n",
                ether_dst[0], ether_dst[1], ether_dst[2], ether_dst[3], ether_dst[4], ether_dst[5]);
        }
    }
}

int main(int argc, char* argv[])
{
    MonitorAdapter(5);
    return 0;
}
