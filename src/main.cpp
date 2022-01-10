#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <WS2tcpip.h>
#include <wspiapi.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/x.H>
#include <FL/fl_ask.H>

#include <fstream>
#include <sstream>
#include <vector>

#include "threads.h"
#include "MovingWindow.h"

using namespace std;

Fl_Thread prime_thread;

#define ALTEZZA_CARATTERI 16
#define LARGHEZZA_NOME_NODO 120
#define LARGHEZZA_LAST_SEEN 120
#define SECONDS_PER_CYCLE 10

struct TimePassed
{
	unsigned int value;
	string unit;
};


TimePassed getTimePassed(unsigned int seconds)
{
	if (seconds < 60*2)
		return {seconds, "second"};

	unsigned int minutes = seconds / 60;
	if (minutes < 60*2)
		return {minutes, "minute"};

	unsigned int hours = seconds / (60*60) ;
	if (hours < 24*2)
		return {hours, "hour"};

	unsigned int days = seconds / (60*60*24);
	if (days < 7*2)
		return {days, "day"};

	unsigned int weeks = seconds / (60*60*24*7);
	if (weeks < 4*2)
		return {weeks, "week"};

	unsigned int months = seconds / (60*60*24*7*4);
	if (months < 12*2)
		return {months, "month"};

	return {seconds/(60*60*24*7*4*12), "year"};
}

string getStringPassed(unsigned int seconds)
{
	TimePassed tp = getTimePassed(seconds);

	string plural = "";
	if (tp.value != 1)
		plural = "s";

	return to_string(tp.value) + " " + tp.unit + plural;
}


void initLog()
{
	system("echo Start Pingui.exe 1 > pingui.log");
}

void writeLog(const string &_msg)
{
	string fullLine = string("echo ")+ _msg + string(" >> pingui.log");
	system(fullLine.c_str());
}

enum Status
{
	DOWN,
	UP
};

class NodeBox;

struct NodeStatus
{
	string ip;
	string nodeName;
	Status status;
	NodeBox* boxTxt;
	Fl_Box* lastSeenBox;
	unsigned cyclesNotReplying;
	bool replied;

	NodeStatus(const string& _ip) : ip(_ip), nodeName(""), status(DOWN), boxTxt(NULL), cyclesNotReplying(0), replied(false) {}
};

vector<NodeStatus> nodeList;

Fl_Box* debugBox;

extern "C" void* thPingNode(void * nodeId);


class NodeBox : public Fl_Box
{
	private:
		unsigned m_index;
	public:
		NodeBox(unsigned i) :Fl_Box(0, (i + 1)* ALTEZZA_CARATTERI, LARGHEZZA_NOME_NODO, ALTEZZA_CARATTERI, nodeList[i].ip.c_str()), m_index(i) {}

		int handle(int e)
		{
			int ret = 0;
			ret = Fl_Box::handle(e);
			switch (e)
			{
				case FL_PUSH:
					{
						int doubleClick = Fl::event_clicks();
						if (doubleClick)
						{
							color(FL_YELLOW);
							fl_create_thread(prime_thread, thPingNode, (void *)&nodeList[m_index]);
						}
					}
					ret = 1;
					break;
				case FL_ENTER:
					copy_label(nodeList[m_index].ip.c_str());
					redraw();
					ret = 1;
					break;
			}
			return(ret);
		}
};

void updateGui(void* userdata)
{
	NodeStatus *nodeStatus = (NodeStatus*)userdata;

	Fl_Color color = FL_RED;
	string stringPassed = "now";
	if (nodeStatus->status == Status::UP)
	{
		color = FL_GREEN;
		nodeStatus->cyclesNotReplying = 0;
		nodeStatus->replied = true;
	}
	else
	{
		stringPassed = getStringPassed(nodeStatus->cyclesNotReplying * SECONDS_PER_CYCLE);
		nodeStatus->cyclesNotReplying++;
	}
	if (nodeStatus->replied)
		nodeStatus->lastSeenBox->copy_label(stringPassed.c_str());

	nodeStatus->boxTxt->color(color);
	nodeStatus->lastSeenBox->color(color);

	nodeStatus->boxTxt->copy_label(nodeStatus->nodeName.c_str());

	nodeStatus->boxTxt->redraw();
	nodeStatus->lastSeenBox->redraw();
}


Status pingNode(const string &_ip)
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
			case IP_DEST_HOST_UNREACHABLE:
			case IP_DEST_NET_UNREACHABLE:
			case IP_REQ_TIMED_OUT:
				retVal = Status::DOWN;
			default:
				retVal = Status::UP;
				break;
		}

		return retVal;
	}
	else
		return Status::DOWN;
}

bool getNameFromIp(string ip, string& name)
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
		string nodeName;
		if (getNameFromIp(pNode->ip, nodeName))
			pNode->nodeName = nodeName;
	}

	Fl::awake(updateGui, pNode);

	return 0L;
}

static int my_handler(int event)
{
	if (event == FL_SHORTCUT)
	{
		debugBox->label("<esc>");

		return 1; // eat all shortcut keys
	}

	return 0;
}

void refreshAll(bool isFirstTime);

extern "C" void* thSleep60(void* p)
{
	Sleep(SECONDS_PER_CYCLE * 1000);
	refreshAll(false);
	return 0L;
}

void refreshAll(bool isFirstTime)
{
	for (unsigned i = 0; i < nodeList.size(); i++)
	{
		if (isFirstTime)
		{
			nodeList[i].boxTxt = new NodeBox(i);
			nodeList[i].boxTxt->box(FL_THIN_DOWN_BOX);
			nodeList[i].lastSeenBox = new Fl_Box(LARGHEZZA_NOME_NODO, (i + 1)* ALTEZZA_CARATTERI, LARGHEZZA_LAST_SEEN, ALTEZZA_CARATTERI, "never");
			nodeList[i].lastSeenBox->box(FL_THIN_DOWN_BOX);
		}

		nodeList[i].boxTxt->color(FL_YELLOW);
		fl_create_thread(prime_thread, thPingNode, (void *)&nodeList[i]);

	}
	// lancia un thread che ogni 60 secondi richiama il refreshAll

	fl_create_thread(prime_thread, thSleep60, NULL);
}

bool isNotAlnum(unsigned char c)
{
	return (c<' ' || c>'~');
}


//int main(int v, char* a)
//int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
int main(int argc, TCHAR* argv[])
{
	initLog();

	//writeLog("bbb"+to_string(23));

	// legge dal file di configurazione gli elementi da monitorare
	ifstream objFile;
	try
	{
		objFile.open("Pingui.conf", ifstream::in);
	}
	catch (ifstream::failure e)
	{
		string error = "Pingui.conf" + string(" exception: ") + e.what();
		//OutputManager__WriteLog(E_CONF_FILE_NOT_OK, error);
		return -1;
	}

	string line;
	if (objFile)
	{
		while (getline(objFile, line))
		{
			istringstream iss(line);
			string str;
			if (iss >> str)
			{
				if (str[0] != '#')
				{
					unsigned pos = 0;
					while (isNotAlnum(str[pos]))
					{
						pos++;
						if (pos >= str.length())
							break;
					}

					str.erase(0, pos);
					NodeStatus node(str);
					nodeList.push_back(node);
				}
			}
		}
	}

	MovingWindow* mainWindow = new MovingWindow(LARGHEZZA_NOME_NODO+LARGHEZZA_LAST_SEEN, nodeList.size() * ALTEZZA_CARATTERI + ALTEZZA_CARATTERI); // la size dipende dal numero di elementi da monitorare recuperati dal file di configurazione

	Fl::add_handler(my_handler);

	debugBox = new Fl_Box(0, 0, LARGHEZZA_NOME_NODO, ALTEZZA_CARATTERI, "Node");
	debugBox->box(FL_FLAT_BOX);

	Fl_Box *lsbox = new Fl_Box(LARGHEZZA_NOME_NODO, 0, LARGHEZZA_LAST_SEEN, ALTEZZA_CARATTERI, "Last seen");
	lsbox->box(FL_FLAT_BOX);

	refreshAll(true);

	mainWindow->end();
	mainWindow->show();

	HWND hWnd = fl_xid(mainWindow);
	ShowWindow(hWnd, SW_SHOWNORMAL);
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	Fl::lock();
	return Fl::run();
}
