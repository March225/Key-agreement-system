#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "ShareMemory.h"
using namespace std;

ShareMemory::ShareMemory(int key)
{
	m_shmID = shmget(key, 0, 0);
}

ShareMemory::ShareMemory(int key, int size)
{
	m_shmID = shmget(key, size, IPC_CREAT | 0766);	
}

ShareMemory::ShareMemory(const char* name)
{
	key_t key = ftok(name, RandX);
	m_shmID = shmget(key, 0, 0);
}

ShareMemory::ShareMemory(const char* name, int size)
{
	key_t key = ftok(name, RandX);
	m_shmID = shmget(key, size, IPC_CREAT | 0766);
}

ShareMemory::~ShareMemory()
{
	
}

int ShareMemory::getShmID(key_t key, int shmSize, int flag)
{
	m_shmID = shmget(key, shmSize, flag);
	if (m_shmID == -1)
	{
		cout << "  共享内存创建失败..." << endl;
		return -1;
		// 写log日志
	}

	// 共享内存地址初始化
	if (shmSize > 0)
	{
		void* addr = shmat(m_shmID, NULL, 0);
		memset(addr, 0x00, shmSize);
		shmdt(addr);
	}
	cout << "  共享内存创建成功..." << endl;
	cout << "  share memory size: " << shmSize << endl;
	return m_shmID;
}


void *ShareMemory::mapShm()
{
	m_shmAddr = shmat(m_shmID, NULL, 0);
	if(m_shmAddr==(void *)-1)
	{
		return NULL;
	}
	
	return m_shmAddr;
}

int ShareMemory::unmapShm()
{
	int ret = shmdt(m_shmAddr);
	
	return ret;
}

int ShareMemory::delShm()
{
	int ret = shmctl(m_shmID, IPC_RMID, NULL);
	
	return ret;
}




