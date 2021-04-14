#include <cstdio>
#include "ServerOperation.h"
#include <sys/ipc.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <signal.h>
using namespace std;

// 创建守护进程
void createDaemon()
{
	pid_t pid = fork();
	if(pid<0 || pid>0)
	{
		exit(0);
	}
	setsid();
	chdir("/home/haitao/projects/Key-agreement-system/KeyAgreeServer");
	umask(0000);
	int fd = open("/dev/null", O_RDWR);
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
}

int main(int argc, char* argv[])
{
	//设置为守护进程
	//createDaemon();

	//注册SIGUSR1信号处理函数
	struct sigaction act;
	act.sa_flags = 0;
	act.sa_handler = ServerOperation::catchSignal;
	sigemptyset(&act.sa_mask);
	sigaction(SIGUSR1, &act, NULL);

	ServerInfo info;
	strcpy(info.serverID, "1");
	info.maxnode = 10;
	info.shmkey = 0x12340000;

	//数据库信息
	strcpy(info.dbHost, "127.0.0.1");
	strcpy(info.dbUser, "root");
	strcpy(info.dbPasswd, "1");
	strcpy(info.dbName, "key_agreement_system");
	//info.debug_rv = atoi(argv[1]);

	info.sPort = 9898;
	ServerOperation server(&info);
	server.startWork();
    return 0;
}
