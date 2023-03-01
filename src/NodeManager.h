#ifndef _NODEMANAGER_H_
#define _NODEMANAGER_H_

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

#include "threads.h"
#include "NetPlatform.h"

#define SECONDS_PER_CYCLE 10

extern "C" void* thPingNode(void * nodeId);
void updateGui(void* userdata);

Fl_Thread prime_thread;

struct NodeStatus
{
	int id;
	std::string ip;
	std::string nodeName;
	NStatus status;
	unsigned cyclesNotReplying;
	bool replied;

	NodeStatus(const int _id, const std::string& _ip, const std::string& _nodeName = "") : id(_id), ip(_ip), nodeName(_nodeName), status(DOWN), cyclesNotReplying(0), replied(false)
	{
		if (nodeName.empty())
			nodeName = ip;
	}
};

std::vector<NodeStatus> nodeList;


extern "C" void* thPingNode(void *p)
{
	NodeStatus *pNode = (NodeStatus*)p;
	//pNode->nodeName = pNode->ip;

	NStatus nodeStatus = pingNode(pNode->ip);
	pNode->status = nodeStatus;
	if (nodeStatus == NStatus::UP)
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
	//Sleep(SECONDS_PER_CYCLE * 1000);
	std::this_thread::sleep_for(std::chrono::seconds(SECONDS_PER_CYCLE));
	refreshAll();
	return 0L;
}


void refreshSingle(const unsigned _index)
{
	fl_create_thread(prime_thread, thPingNode, (void *)&nodeList[_index]);
}


void refreshAll()
{
	for (unsigned i = 0; i < nodeList.size(); i++)
		refreshSingle(i);

	// launch a thread that after SECONDS_PER_CYCLE seconds recalls the refreshAll function
	fl_create_thread(prime_thread, thSleep60, NULL);
}

bool isNotAlnum(unsigned char c)
{
	return (c<' ' || c>'~');
}

//void writeLog(const std::string &_msg);


// Returns the maximum name length of nodes read from conf (or -1 if error)
int LoadNodesFromConf()
{
	int retVal = -1;
	// legge dal file di configurazione gli elementi da monitorare
	std::ifstream objFile;
	try
	{
		objFile.open("Pingui.conf", std::ifstream::in);
	}
	catch (std::ifstream::failure e)
	{
		std::string error = "Pingui.conf" + std::string(" exception: ") + e.what();
		//OutputManager__WriteLog(E_CONF_FILE_NOT_OK, error);
		return -1;
	}

	std::string line;
	if (objFile)
	{
		while (getline(objFile, line))
		{
			std::istringstream iss(line);
			std::string str;
			if (iss >> str)
			{
				if (str[0] != '#') // comment line, ignored
				{
					unsigned pos = 0;
					while (isNotAlnum(str[pos]))
					{
						pos++;
						if (pos >= str.length())
							break;
					}
					str.erase(0, pos); // remove BOM chars, if any

					std::string name = "";
					iss >> name;

					int size = name.size();
					if (size > retVal)
						retVal = name.size();

					NodeStatus node(nodeList.size(), str, name);
					nodeList.push_back(node);
				}
			}
		}
	}
	return retVal;
}

bool SaveNodesToConf()
{
	return true;
}

#endif
