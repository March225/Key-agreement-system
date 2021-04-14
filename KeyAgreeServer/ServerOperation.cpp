#include "ServerOperation.h"
#include <iostream>
#include <pthread.h>
#include <string.h>
#include "./include/RequestFactory.h"
#include "./include/RespondFactory.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <signal.h>
#include <iomanip>
using namespace std;

bool ServerOperation::m_stop = false; //静态变量初始化

void ServerOperation::catchSignal(int signo)
{
	m_stop = true;
}


ServerOperation::ServerOperation(ServerInfo* info)
{
	memcpy(&m_info, info, sizeof(ServerInfo));

	//创建共享内存
	m_shm = new SecKeyShm(m_info.shmkey, m_info.maxnode);

	//连接MYSQL数据库
	m_mysql = new MySQL();
	m_mysql->InitSQL(m_info.dbHost, m_info.dbUser, m_info.dbPasswd, m_info.dbName);
	m_mysql->DoSql("set names utf8"); //设置字符集为utf8,否则会乱码

	//将数据库中的密钥信息拷贝到共享内存
	string rSql = "select clientid, serverid, keyid, state, seckey from seckeyinfo";
	MYSQL_RES* res = m_mysql->GetSqlRes(rSql);
	if (res->row_count > 0) {
		MYSQL_ROW row = nullptr;
		NodeSHMInfo node; //密钥信息结构体
		memset(&node, 0x00, sizeof(NodeSHMInfo));
		cout << "  密钥信息如下：" << endl;
		cout << "----------------------------------------------------------------------------------" << endl;
		cout << setw(10) << "clientid " << setw(10) << "serverid " << setw(10) << "keyid " << setw(10) << "state " << setw(25) << "seckey" << endl;
		while (row = mysql_fetch_row(res)) //获取行
		{
			strcpy(node.clientID, row[0]);
			strcpy(node.serverID, row[1]);
			node.seckeyID = atoi(row[2]);
			node.status = atoi(row[3]);
			strcpy(node.seckey, row[4]);
			//将秘钥信息写入共享内存
			m_shm->shmWrite(&node);

			cout << setw(5) << row[0] << setw(10) << row[1] << setw(12) << row[2] << setw(10) << row[3] << setw(45) << row[4] << endl;
		}
		cout << "----------------------------------------------------------------------------------" << endl;
	}

	//NodeSHMInfo node1; 
	//const char* clientID = "2";
	//const char* serverID = "1";
	//m_shm->shmRead(clientID, serverID, &node1);
	//cout << node1.clientID << ", " << node1.serverID << ", " << node1.seckeyID << ", " << node1.status << ", " << node1.seckey << endl;

}

ServerOperation::~ServerOperation()
{
	if (m_shm != nullptr) delete m_shm;
	if (m_mysql != nullptr) delete m_mysql;
}

void ServerOperation::startWork()
{
	//socket-setsockopt-bind-listen
	m_server.setListen(m_info.sPort);
	pthread_t threadID;
	while (1)
	{
		if (m_stop)
			break;

		//accept新的客户端连接
		m_client = m_server.acceptConn();

		//创建一个子线程
		pthread_create(&threadID, NULL, working, this);

		//设置子线程为分离属性
		pthread_detach(threadID);
		m_listSocket.insert(make_pair(threadID, m_client));
	}
}

// 密钥协商
int ServerOperation::secKeyAgree(RequestMsg* reqMsg, RespondMsg* rspMsg)
{
	//验证消息认证码
	char key[64];
	unsigned int len;
	unsigned char md0[SHA256_DIGEST_LENGTH];
	char authCode[SHA256_DIGEST_LENGTH * 2 + 1] = { 0 };

	memset(key, 0x00, sizeof(key));
	sprintf(key, "@%s+%s@", reqMsg->serverId, reqMsg->clientId);
	HMAC(EVP_sha256(), key, strlen(key), (unsigned char*)reqMsg->r1, strlen(reqMsg->r1), md0, &len);
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(&authCode[2 * i], "%02x", md0[i]);
	}

	//cout << "--------------------------------------" << endl;
	//cout << "key:" << key << endl;
	//cout << "r1:" << reqMsg->r1 << endl;
	//cout << "authCode:" << authCode << endl;
	//cout << "--------------------------------------" << endl;

	//将生成的消息认证码和客户端的reqMsg->r1消息认证码做比对
	if (strcmp(authCode, reqMsg->authCode) != 0)
	{
		cout << "  消息认证失败..." << endl;
		rspMsg->rv = 1;
		return -1;
	}
	else {
		//cout << "  消息认证成功..." << endl;
	}

	//生成随机字符串r2
	getRandString(sizeof(rspMsg->r2), rspMsg->r2);

	//将随机字符串r2和reqMsg->r1进行拼接, 然后生成秘钥
	char buf[64];
	unsigned char md[SHA_DIGEST_LENGTH];
	char seckey[SHA_DIGEST_LENGTH * 2 + 1];
	memset(buf, 0x00, sizeof(buf));
	memset(seckey, 0x00, sizeof(seckey));
	sprintf(buf, "%s%s", reqMsg->r1, rspMsg->r2);
	SHA1((unsigned char*)buf, strlen((char*)buf), md);
	for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		sprintf(&seckey[i * 2], "%02x", md[i]);
	}
	cout << "  clientId: " << reqMsg->clientId << "  serverId: " << reqMsg->serverId << endl;
	cout << "  密钥协商成功..." << endl;
	cout << "  密钥: " << seckey << endl;
	rspMsg->rv = 0; //rv设为0，表示协商成功

	//获取当前时间
	time_t t = time(NULL);
	char time[64] = { 0 };
	char ch[20] = { 0 };
	strftime(ch, sizeof(ch) - 1, "%Y-%m-%d-%H-%M-%S", localtime(&t));
	sprintf(time, "%s", ch);

	NodeSHMInfo node;
	memset(&node, 0x00, sizeof(NodeSHMInfo));

	//更新共享内存中的密钥信息
	node.status = 1;
	strcpy(node.seckey, seckey);
	strcpy(node.clientID, rspMsg->clientId);
	strcpy(node.serverID, m_info.serverID);
	node.seckeyID = rspMsg->seckeyid;
	m_shm->shmWrite(&node);

	//判断该客户端对应的密钥或密钥残像是否已经存在
	char sql[256] = { 0 };
	MYSQL_RES* res = nullptr;
	sprintf(sql, "select * from seckeyinfo where clientid = %s && serverid = %s", reqMsg->clientId, reqMsg->serverId);
	res = m_mysql->GetSqlRes(sql);
	MYSQL_ROW row = nullptr;
	if (res->row_count > 0) { //该客户端对应的密钥或密钥残像已经存在
		//更新数据库中的密钥信息
		memset(sql,0x00,sizeof(sql));
		sprintf(sql, "update seckeyinfo set seckey='%s', state=%d where clientid=%s", seckey, node.status, reqMsg->clientId);

		row = mysql_fetch_row(res);
		rspMsg->seckeyid = atoi(row[2]); //旧的密钥ID赋值给回应结构体
		cout << "  密钥已被覆盖..." << endl;
	}
	else {
		//从数据库获取新的密钥ID
		memset(sql,0x00,sizeof(sql));
		sprintf(sql, "SELECT IFNULL((SELECT MAX(keyid) FROM seckeyinfo), 0)");
		res = m_mysql->GetSqlRes(sql);

		row = mysql_fetch_row(res);
		rspMsg->seckeyid = atoi(row[0]) + 1; //新密钥ID赋值给回应结构体

		memset(sql,0x00,sizeof(sql));
		sprintf(sql, "insert into seckeyinfo(clientid,serverid,createtime,state,seckey) values(%s,%s,'%s',%d,'%s')",
			reqMsg->clientId, reqMsg->serverId, time, node.status, seckey);

	}

	//将密钥信息写入数据库
	m_mysql->DoSql(sql);

	return 0;
}

// 密钥校验
int ServerOperation::secKeyCheck(RequestMsg* reqMsg, RespondMsg* rspMsg)
{
	//从共享内存读取密钥信息
	NodeSHMInfo node;
	memset(&node, 0x00, sizeof(NodeSHMInfo));
	m_shm->shmRead(reqMsg->clientId, m_info.serverID, &node);

	//验证消息认证码(也就是进行密钥校验)
	char key[64];
	unsigned int len;
	unsigned char md0[SHA256_DIGEST_LENGTH];
	char authCode[SHA256_DIGEST_LENGTH * 2 + 1] = { 0 };

	memset(key, 0x00, sizeof(key));
	sprintf(key, "@%s+%s@", reqMsg->serverId, reqMsg->clientId);
	HMAC(EVP_sha256(), key, strlen(key), (unsigned char*)node.seckey, strlen(node.seckey), md0, &len);
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(&authCode[2 * i], "%02x", md0[i]);
	}

	//cout << "--------------------------------------" << endl;
	//cout << "key:" << key << endl;
	//cout << "r1:" << reqMsg->r1 << endl;
	//cout << "authCode:" << authCode << endl;
	//cout << "--------------------------------------" << endl;

	cout << "  clientId: " << reqMsg->clientId << "  serverId: " << reqMsg->serverId << endl;

	//将生成的消息认证码和客户端的reqMsg->r1(密钥)的消息认证码做比对
	if (strcmp(authCode, reqMsg->authCode) != 0)
	{
		cout << "  密钥校验失败..." << endl;
		cout << "  密钥: " << node.seckey << endl;
		rspMsg->rv = 1; // 1 表示失败
		return -1;
	}
	cout << "  密钥校验成功..." << endl;
	cout << "  密钥: " << reqMsg->r1 << endl;
	rspMsg->rv = 0;
	return 0;
}

// 密钥注销
int ServerOperation::secKeyRevoke(RequestMsg* reqMsg, RespondMsg* rspMsg)
{
	//从共享内存读取密钥信息
	NodeSHMInfo node;
	memset(&node, 0x00, sizeof(NodeSHMInfo));
	m_shm->shmRead(reqMsg->clientId, m_info.serverID, &node);

	//验证消息认证码(也就是进行密钥校验)
	char key[64];
	unsigned int len;
	unsigned char md0[SHA256_DIGEST_LENGTH];
	char authCode[SHA256_DIGEST_LENGTH * 2 + 1] = { 0 };

	memset(key, 0x00, sizeof(key));
	sprintf(key, "@%s+%s@", reqMsg->serverId, reqMsg->clientId);
	HMAC(EVP_sha256(), key, strlen(key), (unsigned char*)node.seckey, strlen(node.seckey), md0, &len);
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(&authCode[2 * i], "%02x", md0[i]);
	}

	//cout << "--------------------------------------" << endl;
	//cout << "key:" << key << endl;
	//cout << "r1:" << reqMsg->r1 << endl;
	//cout << "authCode:" << authCode << endl;
	//cout << "--------------------------------------" << endl;

	cout << "  clientId: " << reqMsg->clientId << "  serverId: " << reqMsg->serverId << endl;

	//将生成的消息认证码和客户端的reqMsg->r1(密钥)的消息认证码做比对
	if (strcmp(authCode, reqMsg->authCode) != 0)
	{
		cout << "  信息核对失败，拒绝注销申请..." << endl;
		cout << "  密钥: " << node.seckey << endl;
		rspMsg->rv = 1;
		return -1;
	}

	//注销密钥
	node.status = 0;
	m_shm->shmWrite(&node); //共享内存中密钥的状态修改为不可用

	char Sql[128] = { 0 };
	sprintf(Sql, "update seckeyinfo set state = %d where clientID = %s", 0, reqMsg->clientId);
	m_mysql->DoSql(Sql); //数据库中密钥的状态修改为不可用
	
	cout << "  信息核对成功，密钥已注销..." << endl;
	cout << "  密钥: " << reqMsg->r1 << endl;
	rspMsg->rv = 0;
	return 0;
}

void ServerOperation::getRandString(int len, char* randBuf)
{
	int flag = -1;
	// 设置随机种子
	srand(time(NULL));
	// 随机字符串: A-Z, a-z, 0-9, 特殊字符(!@#$%^&*()_+=)
	char chars[] = "!@#$%^&*()_+=";
	for (int i = 0; i < len - 1; ++i)
	{
		flag = rand() % 4;
		switch (flag)
		{
		case 0:
			randBuf[i] = 'Z' - rand() % 26;
			break;
		case 1:
			randBuf[i] = 'z' - rand() % 26;
			break;
		case 3:
			randBuf[i] = rand() % 10 + '0';
			break;
		case 2:
			randBuf[i] = chars[rand() % strlen(chars)];
			break;
		default:
			break;
		}
	}
	randBuf[len - 1] = '\0';
}

// 友元函数, 可以在该友元函数中通过对应的类对象调用期私有成员函数或者私有变量
// 子线程 - 进行业务流程处理
void* working(void* arg)
{
	//接收数据
	pthread_t thread = pthread_self();
	ServerOperation* op = (ServerOperation*)arg;
	TcpSocket* socket = op->m_listSocket[thread];

	char* inData;
	int dataLen;
	socket->recvMsg(&inData, dataLen);

	//解码
	CodecFactory* factory = new RequestFactory();
	Codec* pCodec = factory->createCodec();
	RequestMsg* pMsg = (RequestMsg*)pCodec->msgDecode(inData, dataLen);
	//	delete factory;
	//	delete pCodec;

	//应答结构体
	RespondMsg rspMsg;
	strcpy(rspMsg.serverId, op->m_info.serverID);
	strcpy(rspMsg.clientId, pMsg->clientId);

	//判断clientID和serverID是否合法
	string rSql = "select name from secnode where id=";
	rSql += pMsg->clientId;
	MYSQL_RES* res = op->m_mysql->GetSqlRes(rSql);

	if (res->row_count <= 0 || strcmp(pMsg->serverId, op->m_info.serverID) != 0) {
		cout << "\n  ID校验失败..." << endl;
		rspMsg.rv = 2;
	}
	else {
		cout << "\n  ID校验成功..." << endl;
		//判断客户端要请求什么服务
		switch (pMsg->cmdType)
		{
		case RequestCodec::NewOrUpdate:
			op->secKeyAgree(pMsg, &rspMsg);
			break;
		case RequestCodec::Check:
			op->secKeyCheck(pMsg, &rspMsg);
			break;
		case RequestCodec::Revoke:
			op->secKeyRevoke(pMsg, &rspMsg);
			break;
		case RequestCodec::View:
			//op->secKeyView();
			break;
		default:
			break;
		}
	}

	//将要发送给客户端的应答结构体进行编码
	char* sendData = nullptr;
	factory = new RespondFactory(&rspMsg);
	pCodec = factory->createCodec();
	pCodec->msgEncode(&sendData, dataLen);

	//发送数据给客户端
	socket->sendMsg(sendData, dataLen);
	free(sendData);

	//关闭连接
	socket->disConnect();

	auto it = op->m_listSocket.find(thread);
	op->m_listSocket.erase(it);

	delete socket;
	delete factory;
	cout << "----------------------------------------------------------------------------------" << endl;
}
