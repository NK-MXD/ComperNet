#include <WinSock2.h>
#include <Windows.h>
#include <pcap.h>

#pragma comment(lib, "packet.lib")
#pragma comment(lib, "wpcap.lib")

int enumAdapters()
{
	pcap_if_t* allAdapters;    // 所有网卡设备保存
	pcap_if_t* ptr;            // 用于遍历的指针
	int index = 0;
	char errbuf[PCAP_ERRBUF_SIZE];

	/* 获取本地机器设备列表 */
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &allAdapters, errbuf) != -1)
	{
		/* 打印网卡信息列表 */
		for (ptr = allAdapters; ptr != NULL; ptr = ptr->next)
		{
			/* 获取设备数目 */
			++index;
			/* Name */
			printf("网卡ID: %s\n", ptr->name);
			/* Description */
			if (ptr->description)
				printf("设备描述: %s\n", ptr->description);
			/* Loopback Address*/
			printf("回环地址: %s\n", (ptr->flags & PCAP_IF_LOOPBACK) ? "yes" : "no");
			/* IP addresses */
			printf("网卡地址: %x \n", ptr->addresses);
		}
	}

	/* 不再需要设备列表了，释放它 */
	pcap_freealldevs(allAdapters);
	return index;
}

int main(int argc, char* argv[])
{
	int network = enumAdapters();
	printf("网卡数量: %d \n", network);
	system("Pause");
}
