#pragma once
#include "ShareMemory.h"

class NodeSHMInfo
{
public:
	int status; // 0表示正在使用 1表示弃用
	int seckeyID;
	char clientID[12];
	char serverID[12];
	char seckey[128];
};

class SecKeyShm : public ShareMemory
{
public:
	SecKeyShm(int key);
	SecKeyShm(int key, int maxNode);
	SecKeyShm(const char* pathName);
	SecKeyShm(const char* pathName, int maxNode);
	~SecKeyShm();

	bool isExist(const char* clientID, const char* serverID);
	int shmWrite(NodeSHMInfo* pNodeInfo);
	int shmRead(const char* clientID, const char* serverID, NodeSHMInfo* pNodeInfo);

private:
	int m_maxNode;
};

