#include <string.h>
#include <stdlib.h>
#include <iostream>
#include "./include/RequestCodec.h"
#include "./include/RespondCodec.h"
#include "./include/RequestFactory.h"
#include "./include/RespondFactory.h"
#include "ClientOperation.h"
#include <sys/ipc.h>
#include <sys/shm.h>

using namespace std;
int usage();

int main(int argc, char* argv[])
{
	ClientInfo info;
	memset(&info, 0x00, sizeof(ClientInfo));
	strcpy(info.clientID, argv[1]);
	strcpy(info.serverID, "1");
	strcpy(info.serverIP, "192.168.1.110");
	info.serverPort = 9898;
	info.maxNode = 1;
	info.shmKey = atoi(argv[1]);

	////数据库信息
	//strcpy(info.dbHost, "127.0.0.1");
	//strcpy(info.dbUser, "root");
	//strcpy(info.dbPasswd, "1");
	//strcpy(info.dbName, "STP_db");

	ClientOperation client(&info);

	//enum CmdType{NewOrUpdate=1, Check, Revoke, View};
	int nSel;
	while (1)
	{
		nSel = usage();
		switch (nSel)
		{
		case RequestCodec::NewOrUpdate:
			client.secKeyAgree();
			break;
		case RequestCodec::Check:
			client.secKeyCheck();
			break;
		case RequestCodec::Revoke:
			client.secKeyRevoke();
			break;
		case RequestCodec::View:
			client.secKeyView();
			break;
		case 0:
			exit(0);

		default:
			break;
		}
		cout << "----------------------------------------------------------------------------------" << endl;
	}
}

int usage()
{
	int nSel = -1;
	printf("\n  /******************** 欢迎使用密钥协商系统！*******************/");
	printf("\n  /**************************************************************/");
	printf("\n  /*                        1.密钥协商                          */");
	printf("\n  /*                        2.密钥校验                          */");
	printf("\n  /*                        3.密钥注销                          */");
	printf("\n  /*                        4.密钥查看                          */");
	printf("\n  /*                        0.退出系统                          */");
	printf("\n  /**************************************************************/");
	printf("\n  /**************************************************************/");
	printf("\n\n  选择:");

	scanf("%d", &nSel);
	while (getchar() != '\n');

	return nSel;
}
