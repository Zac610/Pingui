#ifndef _NETPLATFORM_H_
#define _NETPLATFORM_H_

#ifdef msw
#include <winsock2.H>
#include <iphlpapi.H>
#include <icmpapi.H>
#include <WS2tcpip.H>
#include <wspiapi.H>
#endif // WINDOWS
enum NStatus
{
	DOWN,
	UP
};


NStatus pingNode(const std::string &_ip)
{
#ifdef msw
	HANDLE hIcmpFile;
	unsigned long ipaddr = INADDR_NONE;
	DWORD dwRetVal = 0;
	DWORD dwError = 0;
	char SendData[] = "Data Buffer";
	LPVOID ReplyBuffer = NULL;
	DWORD ReplySize = 0;

	ipaddr = inet_addr(_ip.c_str());
	if (ipaddr == INADDR_NONE)
		return NStatus::DOWN;

	hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE)
		return NStatus::DOWN;

	// Allocate space for at a single reply
	ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData) + 8;
	ReplyBuffer = (VOID*)malloc(ReplySize);
	if (ReplyBuffer == NULL)
		return NStatus::DOWN;

	dwRetVal = IcmpSendEcho2(hIcmpFile, NULL, NULL, NULL,
													 ipaddr, SendData, sizeof(SendData), NULL,
													 ReplyBuffer, ReplySize, 1000);
	if (dwRetVal != 0)
	{
		PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
		struct in_addr ReplyAddr;
		ReplyAddr.S_un.S_addr = pEchoReply->Address;

		NStatus retVal = NStatus::DOWN;
		switch (pEchoReply->Status)
		{
			case IP_SUCCESS:
				retVal = NStatus::UP;
				break;
			default:
				retVal = NStatus::DOWN;
				break;
		}

		return retVal;
	}
	else
#endif
		return NStatus::DOWN;
}


bool getNameFromIp(std::string ip, std::string& name)
{
#ifdef msw
	struct addrinfo    hints;
	struct addrinfo* res = 0;
	int       status;

	WSADATA   wsadata;
	int statuswsadata;
	if ((statuswsadata = WSAStartup(MAKEWORD(2, 2), &wsadata)) != 0)
		return false;

	hints.ai_family = AF_INET;

	status = getaddrinfo(ip.c_str(), 0, 0, &res);

	char host[512]/*, port[128]*/;

	status = getnameinfo(res->ai_addr, res->ai_addrlen, host, 512, 0, 0, 0);

	name = host;

	freeaddrinfo(res);

	if (name == ip)
		return false;

	name = name.substr(0, name.find('.'));
#endif
	return true;
}



#endif
