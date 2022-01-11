#ifndef _NODEMANAGER_H_
#define _NODEMANAGER_H_

#include <string>
#include <vector>

#define SECONDS_PER_CYCLE 10

extern "C" void* thPingNode(void * nodeId);
void updateGui(void* userdata);

Fl_Thread prime_thread;


enum Status
{
	DOWN,
	UP
};

struct NodeStatus
{
	int id;
	std::string ip;
	std::string nodeName;
	Status status;
	unsigned cyclesNotReplying;
	bool replied;

	NodeStatus(const int _id, const std::string& _ip) : id(_id), ip(_ip), nodeName(""), status(DOWN), cyclesNotReplying(0), replied(false) {}
};

std::vector<NodeStatus> nodeList;

Status pingNode(const std::string &_ip)
{
	HANDLE hIcmpFile;
	unsigned long ipaddr = INADDR_NONE;
	DWORD dwRetVal = 0;
	DWORD dwError = 0;
	char SendData[] = "Data Buffer";
	LPVOID ReplyBuffer = NULL;
	DWORD ReplySize = 0;

	ipaddr = inet_addr(_ip.c_str());
	if (ipaddr == INADDR_NONE)
		return Status::DOWN;

	hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE)
		return Status::DOWN;

	// Allocate space for at a single reply
	ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData) + 8;
	ReplyBuffer = (VOID*)malloc(ReplySize);
	if (ReplyBuffer == NULL)
		return Status::DOWN;

	dwRetVal = IcmpSendEcho2(hIcmpFile, NULL, NULL, NULL,
													 ipaddr, SendData, sizeof(SendData), NULL,
													 ReplyBuffer, ReplySize, 1000);
	if (dwRetVal != 0)
	{
		PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
		struct in_addr ReplyAddr;
		ReplyAddr.S_un.S_addr = pEchoReply->Address;

		Status retVal = Status::DOWN;
		switch (pEchoReply->Status)
		{
			case IP_SUCCESS:
				retVal = Status::UP;
				break;
			default:
				retVal = Status::DOWN;
				break;
		}

		return retVal;
	}
	else
		return Status::DOWN;
}

bool getNameFromIp(std::string ip, std::string& name)
{
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

	return true;
}

extern "C" void* thPingNode(void *p)
{
	NodeStatus *pNode = (NodeStatus*)p;
	pNode->nodeName = pNode->ip;

	Status nodeStatus = pingNode(pNode->ip);
	pNode->status = nodeStatus;
	if (nodeStatus == Status::UP)
	{
		std::string nodeName;
		if (getNameFromIp(pNode->ip, nodeName))
			pNode->nodeName = nodeName;
	}

	Fl::awake(updateGui, pNode);

	return 0L;
}

void refreshAll();

extern "C" void* thSleep60(void* p)
{
	Sleep(SECONDS_PER_CYCLE * 1000);
	refreshAll();
	return 0L;
}

void refreshAll()
{
	for (unsigned i = 0; i < nodeList.size(); i++)
	{
		fl_create_thread(prime_thread, thPingNode, (void *)&nodeList[i]);
	}
	// lancia un thread che ogni 60 secondi richiama il refreshAll

	fl_create_thread(prime_thread, thSleep60, NULL);
}



#endif
