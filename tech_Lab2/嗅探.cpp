#include <WinSock2.h>
#include <Windows.h>
#include <pcap.h>

#pragma comment(lib, "packet.lib")
#pragma comment(lib, "wpcap.lib")

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
            printf("侦听长度: %d \n", Packet_Header->len);
        }
    }
}

int main(int argc, char* argv[])
{
    MonitorAdapter(5);
}
