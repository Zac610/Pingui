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
//#include <thread>

#  include "threads.h"

using namespace std;

Fl_Thread prime_thread;


#define ALTEZZA_CARATTERI 16
//#define ALTEZZA_PER_LINEA 12
#define LARGHEZZA_NOME_NODO 120
//#define LARGHEZZA_STATUS ALTEZZA_CARATTERI

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
	unsigned cyclesNotReplying;
	//Fl_Box * boxStatus;

	NodeStatus(const string& _ip) : ip(_ip), nodeName(""), status(DOWN), boxTxt(NULL), cyclesNotReplying(0) {}
};

vector<NodeStatus> nodeList;

Fl_Box* debugBox;

class MovingWindows : public Fl_Window
{
public:
	MovingWindows(int w, int h) : Fl_Window(w, h)
	{
		border(0);
	}

	int MovingWindows::handle(int e)
	{
		int ret = 0;
		static int xoff = 0, yoff = 0;	// (or put these in the class)
		ret = Fl_Window::handle(e);
		switch (e)
		{
			// DOWNCLICK IN WINDOW CREATES CURSOR OFFSETS
		case FL_PUSH:

			if (Fl::event_button() == FL_RIGHT_MOUSE)
				if (fl_ask("Are you sure you want to quit?"))
					exit(0);

			xoff = x() - Fl::event_x_root();
			yoff = y() - Fl::event_y_root();
			ret = 1;

		case FL_DRAG:
			// DRAG THE WINDOW AROUND THE SCREEN
			position(xoff + Fl::event_x_root(), yoff + Fl::event_y_root());
			redraw();
			ret = 1;

		case FL_RELEASE:
			show();             // raise
			ret = 1;
		}
		return(ret);
	}
};

MovingWindows* window;

extern "C" void* thPingNode(void * nodeId);


int _wopen(
	_In_z_ wchar_t const* _FileName,
	_In_   int            _OFlag,
	_In_   int            _PMode = 0
)
{
	int _FileHandle;
	// Last parameter passed as 0 because we don't want to validate pmode from _open
	errno_t const _Result = _wsopen_dispatch(_FileName, _OFlag, _SH_DENYNO, _PMode, &_FileHandle, 0);
	return _Result ? -1 : _FileHandle;
}

class NodeBox : public Fl_Box
{
private:
	unsigned m_index;
public:
	NodeBox(unsigned i) :Fl_Box(0, (i + 1)* ALTEZZA_CARATTERI, LARGHEZZA_NOME_NODO, ALTEZZA_CARATTERI, nodeList[i].ip.c_str()), m_index(i) {}

	int NodeBox::handle(int e)
	{
		int ret = 0;
		static int xoff = 0, yoff = 0;	// (or put these in the class)
		ret = Fl_Box::handle(e);
		switch (e)
		{
		case FL_PUSH:
			int doubleClick = Fl::event_clicks();
			if (doubleClick)
			{
				color(FL_YELLOW);
				fl_create_thread(prime_thread, thPingNode, (void *)m_index);

				//thread* myThPingNode = new thread(thPingNode, m_index);
			}
			//ret = 1;
		}
		return(ret);
	}

};

void updateGui(void* userdata)
{
	NodeStatus *nodeStatus = (NodeStatus*)userdata;

	Fl_Color color = FL_RED;
	if (nodeStatus->status == Status::UP)
		color = FL_GREEN;
	nodeStatus->boxTxt->color(color);

	nodeStatus->boxTxt->copy_label(nodeStatus->nodeName.c_str());

	nodeStatus->boxTxt->redraw();

	//Fl::flush();
}


Status pingNode(unsigned nodeId)
{
	//string pingCmd = string("ping -n 1 ") + nodeList[nodeId].ip + string(">NUL");
	//int x = system(pingCmd.c_str());

	HANDLE hIcmpFile;
	unsigned long ipaddr = INADDR_NONE;
	DWORD dwRetVal = 0;
	DWORD dwError = 0;
	char SendData[] = "Data Buffer";
	LPVOID ReplyBuffer = NULL;
	DWORD ReplySize = 0;

	ipaddr = inet_addr(nodeList[nodeId].ip.c_str());
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
	if (dwRetVal != 0) {
		PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
		struct in_addr ReplyAddr;
		ReplyAddr.S_un.S_addr = pEchoReply->Address;

		Status retVal = Status::DOWN;
		switch (pEchoReply->Status) {
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

bool getNameFromIp(string ip, string& name) {
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
	unsigned nodeId = (unsigned)p;
	nodeList[nodeId].nodeName = nodeList[nodeId].ip;

	Status nodeStatus = pingNode(nodeId);
	nodeList[nodeId].status = nodeStatus;
	if (nodeStatus == Status::UP)
	{
		string nodeName;
		if (getNameFromIp(nodeList[nodeId].ip, nodeName))
			nodeList[nodeId].nodeName = nodeName;
	}

	Fl::awake(updateGui, &nodeList[nodeId]);

	return 0L;
}

static int my_handler(int event) {
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
	Sleep(10 * 1000);
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
			nodeList[i].boxTxt->box(FL_FLAT_BOX);
		}

		nodeList[i].boxTxt->color(FL_YELLOW);
		fl_create_thread(prime_thread, thPingNode, (void *)i);

	}
	// lancia un thread che ogni 60 secondi richiama il refreshAll

	fl_create_thread(prime_thread, thSleep60, NULL);
}

bool isNotAlnum(unsigned char c) {
	return (c<' ' || c>'~');
}


//int main(int v, char* a)
//int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
int main(int argc, TCHAR* argv[])
{
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
					str.erase(remove_if(str.begin(), str.end(), isNotAlnum), str.end());
					NodeStatus node(str);
					nodeList.push_back(node);
				}
			}
		}
	}

	window = new MovingWindows(LARGHEZZA_NOME_NODO/*+LARGHEZZA_STATUS*/, nodeList.size() * ALTEZZA_CARATTERI + ALTEZZA_CARATTERI); // la size dipende dal numero di elementi da monitorare recuperati dal file di configurazione

	Fl::add_handler(my_handler);

	debugBox = new Fl_Box(0, 0, LARGHEZZA_NOME_NODO, ALTEZZA_CARATTERI, "Pingui");
	debugBox->box(FL_FLAT_BOX);

	refreshAll(true);

	window->end();
	window->show();

	HWND hWnd = fl_xid(window);
	ShowWindow(hWnd, SW_SHOWNORMAL);
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	//SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, FLAGS);

	Fl::lock();
	return Fl::run();
}