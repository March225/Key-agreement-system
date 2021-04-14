#include "SecKeyShm.h"
#include <string.h>
#include <iostream>
using namespace std;

SecKeyShm::SecKeyShm(int key) : ShareMemory(key)
{
}

SecKeyShm::SecKeyShm(int key, int maxNode) 
	: ShareMemory(key, maxNode * sizeof(NodeSHMInfo))
	, m_maxNode(maxNode)
{
	//连接共享内存
	void* p = mapShm();

	//将maxNode写入共享内存的前四个字节
	memcpy(p, &m_maxNode, sizeof(int));

	//初始化结构体元素
	NodeSHMInfo* pNode = (NodeSHMInfo*)((char*)p + sizeof(int));
	memset(pNode, 0x00, sizeof(NodeSHMInfo) * m_maxNode);

	//断开与共享内存的关联
	unmapShm();

	cout << "  shared memory created success..." << endl;
	cout << "  share memory size: " << sizeof(int) + maxNode * sizeof(NodeSHMInfo) << endl;
}

SecKeyShm::SecKeyShm(const char * pathName) : ShareMemory(pathName)
{
}

SecKeyShm::SecKeyShm(const char * pathName, int maxNode) 
	: ShareMemory(pathName, maxNode * sizeof(NodeSHMInfo))
	, m_maxNode(maxNode)
{

}

SecKeyShm::~SecKeyShm()
{
}

bool SecKeyShm::isExist(const char* clientID, const char* serverID)
{
	int ret = 0;
	//连接共享内存
	void* p = mapShm();
	//获得共享内存大小--头4个字节表示共享内存大小
	memcpy(&m_maxNode, p, sizeof(int));
	NodeSHMInfo* pNode = (NodeSHMInfo*)((char*)p + sizeof(int));

	// 判断传入的网点密钥是否已经存在
	int i = 0;
	for (i = 0; i < m_maxNode; i++)
	{
		if (strcmp(pNode[i].clientID, clientID) == 0 &&
			strcmp(pNode[i].serverID, serverID) == 0)
		{
			// 如果该网点秘钥已经存在
			return true;
		}
	}

	//断开与共享内存的关联
	unmapShm();

	return false;
}

int SecKeyShm::shmWrite(NodeSHMInfo * pNodeInfo)
{
	int ret = 0;

	//连接共享内存
	void* p = mapShm();

	//获得共享内存大小--头4个字节表示共享内存大小
	memcpy(&m_maxNode, p, sizeof(int));
	//cout << "m_maxNode=="<< m_maxNode << endl;

	NodeSHMInfo* pNode = (NodeSHMInfo*)((char*)p + sizeof(int));

	// 判断传入的网点密钥是否已经存在
	int i = 0;
	for (i = 0; i < m_maxNode; i++)
	{
		if (strcmp(pNode[i].clientID, pNodeInfo->clientID) == 0 &&
			strcmp(pNode[i].serverID, pNodeInfo->serverID) == 0)
		{
			// 如果找到了该网点秘钥已经存在, 使用新密钥的值覆盖旧值
			memcpy(&pNode[i], pNodeInfo, sizeof(NodeSHMInfo));
			break;
		}
	}
	//cout << "i==" << i << endl;

	//若没有找到原来的, 则找一个空闲位置写入
	NodeSHMInfo tmpNode;
	memset(&tmpNode, 0x00, sizeof(tmpNode));
	if (i == m_maxNode)
	{
		for (i = 0; i < m_maxNode; i++)
		{
			if (memcmp(&pNode[i], &tmpNode, sizeof(NodeSHMInfo)) == 0)
			{
				memcpy(&pNode[i], pNodeInfo, sizeof(NodeSHMInfo));
				break;
			}
		}
	}
	//cout << "i==" << i << endl;


	//没有空间可用	
	if (i == m_maxNode)
	{
		cout << "no space to left" << endl;
		ret = -1;
	}
	//断开与共享内存的关联
	unmapShm();

	return ret;
}

int SecKeyShm::shmRead(const char * clientID, const char * serverID, NodeSHMInfo * pNodeInfo)
{
	int ret = 0;

	//连接共享内存
	void* p = mapShm();

	//获得共享内存大小--头4个字节表示共享内存大小
	memcpy(&m_maxNode, p, sizeof(int));
	//cout << "m_maxNode=="<< m_maxNode << endl;

	NodeSHMInfo* pNode = (NodeSHMInfo*)((char*)p + sizeof(int));

	//cout << "clientID:" << clientID << endl;
	//cout << "serverID:" << serverID << endl;

	int i = 0;
	for (i = 0; i < m_maxNode; i++)
	{
		if (strcmp(pNode[i].clientID, clientID) == 0 &&
			strcmp(pNode[i].serverID, serverID) == 0)
		{
			// 找到的节点信息, 拷贝到传出参数
			memcpy(pNodeInfo, &pNode[i], sizeof(NodeSHMInfo));
			break;
		}
	}

	//断开与共享内存的关联
	unmapShm();

	return ret;
}
