#include <WinSock2.h>
#include <Windows.h>
#include <pcap.h>

#pragma comment(lib, "packet.lib")
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "ws2_32.lib")

/* ��̫��ͷ����֡�ṹ���װ */
struct ether_header {
    u_char ether_dhost[6];    // Ŀ���ַ
    u_char ether_shost[6];    // Դ��ַ
    u_short ether_type;       // ��̫������
};

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
        while (pcap_next_ex(handle, &Packet_Header, &Packet_Data) >= 0){
            ether_header* eth_protocol;
            eth_protocol = (ether_header*)Packet_Data;
            u_short ether_type = ntohs(eth_protocol->ether_type);  // ��̫������
            u_char* ether_src = eth_protocol->ether_shost;         // ��̫��ԭʼMAC��ַ
            u_char* ether_dst = eth_protocol->ether_dhost;         // ��̫��Ŀ��MAC��ַ

            printf("��������: %d \t", Packet_Header->len);
            printf("����: %04x \t", ether_type);
            printf("ԴMAC��ַ: %02X:%02X:%02X:%02X:%02X:%02X \t",
                ether_src[0], ether_src[1], ether_src[2], ether_src[3], ether_src[4], ether_src[5]);
            printf("Ŀ��MAC��ַ: %02X:%02X:%02X:%02X:%02X:%02X \n",
                ether_dst[0], ether_dst[1], ether_dst[2], ether_dst[3], ether_dst[4], ether_dst[5]);
        }
    }
}

int main(int argc, char* argv[])
{
    MonitorAdapter(5);
    return 0;
}
