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

#include "threads.h"
#include "MovingWindow.h"
#include "NodeManager.h"

using namespace std;

#define ALTEZZA_CARATTERI 16
#define LARGHEZZA_NOME_NODO 120
#define LARGHEZZA_LAST_SEEN 120

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


Fl_Box* debugBox;


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
						refreshSingle(m_index);
					}
					else
					{
						copy_label(nodeList[m_index].ip.c_str());
						redraw();
					}
				}
				ret = 1;
				break;
			}
			return(ret);
		}
};

struct NodeStatusGui
{
	NodeBox* boxTxt;
	Fl_Box* lastSeenBox;

//	NodeStatusGui() : boxTxt(NULL), lastSeenBox(NULL) {}
};
std::vector<NodeStatusGui> nodeListGui;



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

	NodeStatusGui *nsg = &nodeListGui[nodeStatus->id];

	if (nodeStatus->replied)
		nsg->lastSeenBox->copy_label(stringPassed.c_str());

	if (strcmp(nsg->lastSeenBox->label(), "never") == 0)
		color = FL_GRAY;

	nsg->boxTxt->color(color);
	nsg->lastSeenBox->color(color);

	nsg->boxTxt->copy_label(nodeStatus->nodeName.c_str());

	nsg->boxTxt->redraw();
	nsg->lastSeenBox->redraw();
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

//int main(int v, char* a)
//int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
int main(int argc, TCHAR* argv[])
{
	initLog();

	//writeLog("bbb"+to_string(23));
	InitNodesFromConf();

	MovingWindow* mainWindow = new MovingWindow(LARGHEZZA_NOME_NODO+LARGHEZZA_LAST_SEEN, nodeList.size() * ALTEZZA_CARATTERI + ALTEZZA_CARATTERI); // la size dipende dal numero di elementi da monitorare recuperati dal file di configurazione

	for (unsigned i = 0; i < nodeList.size(); i++)
	{
		NodeStatusGui nsg;
		nsg.boxTxt = new NodeBox(i);
		nsg.boxTxt->box(FL_THIN_DOWN_BOX);
		nsg.boxTxt->color(FL_YELLOW);
		nsg.lastSeenBox = new Fl_Box(LARGHEZZA_NOME_NODO, (i + 1)* ALTEZZA_CARATTERI, LARGHEZZA_LAST_SEEN, ALTEZZA_CARATTERI, "never");
		nsg.lastSeenBox->box(FL_THIN_DOWN_BOX);
		nodeListGui.push_back(nsg);
	}

	Fl::add_handler(my_handler);

	debugBox = new Fl_Box(0, 0, LARGHEZZA_NOME_NODO, ALTEZZA_CARATTERI, "Node");
	debugBox->box(FL_FLAT_BOX);

	Fl_Box *lsbox = new Fl_Box(LARGHEZZA_NOME_NODO, 0, LARGHEZZA_LAST_SEEN, ALTEZZA_CARATTERI, "Last seen");
	lsbox->box(FL_FLAT_BOX);

	refreshAll();

	mainWindow->end();
	mainWindow->show();

	HWND hWnd = fl_xid(mainWindow);
	ShowWindow(hWnd, SW_SHOWNORMAL);
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	Fl::lock();
	return Fl::run();
}
