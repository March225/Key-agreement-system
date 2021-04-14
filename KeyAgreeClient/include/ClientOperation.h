#pragma once
#include "TcpSocket.h"
#include "SecKeyShm.h"
#include "MySQL.h"

class ClientInfo
{
public:
	char			dbHost[24]; 	// 数据库地址
	char			dbUser[24]; 	// 数据库用户名
	char			dbPasswd[24]; 	// 数据库密码
	char			dbName[24]; 	// 数据库名称

	char clientID[12];			// 客户端ID
	char serverID[12];			// 服务器ID
	//char authCode[65];		// 网点授权码
	char serverIP[32];			// 服务器IP
	unsigned short serverPort;	// 服务器端口
	int maxNode;				// 共享内存节点个数
	int shmKey;					// 共享内存的Key
};

class ClientOperation
{
public:
	ClientOperation(ClientInfo *info);
	~ClientOperation();

	// 密钥协商
	int secKeyAgree();
	// 密钥校验
	int secKeyCheck();
	// 密钥注销
	int secKeyRevoke();
	// 密钥查看
	int secKeyView();

private:
	void getRandString(int len, char* randBuf);

private:
	MySQL* m_mysql;
	ClientInfo m_info;
	TcpSocket m_socket;
	SecKeyShm* m_shm;
};

