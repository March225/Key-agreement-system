#include "ClientOperation.h"
#include "../include/RequestCodec.h"
#include <string.h>
#include <time.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include "./include/CodecFactory.h"
#include "./include/RequestFactory.h"
#include "./include/RespondFactory.h"
#include <iomanip>

using namespace std;

ClientOperation::ClientOperation(ClientInfo* info)
{
	memcpy(&m_info, info, sizeof(ClientInfo));

	//创建共享内存
	m_shm = new SecKeyShm(m_info.shmKey, m_info.maxNode);
}

ClientOperation::~ClientOperation()
{

}

// 密钥协商
int ClientOperation::secKeyAgree()  
{
	//准备请求数据 
	RequestMsg req;
	memset(&req, 0x00, sizeof(RequestMsg));
	req.cmdType = RequestCodec::NewOrUpdate;
	strcpy(req.clientID, m_info.clientID);
	strcpy(req.serverId, m_info.serverID);
	getRandString(sizeof(req.r1), req.r1);
	//使用hmac函数将随机数r1生成哈希值----消息认证码
	char key[64];
	unsigned int len;
	unsigned char md[SHA256_DIGEST_LENGTH];
	memset(key, 0x00, sizeof(key));
	sprintf(key, "@%s+%s@", req.serverId, req.clientID);
	HMAC(EVP_sha256(), key, strlen(key), (unsigned char*)req.r1, strlen(req.r1), md, &len);
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(&req.authCode[2 * i], "%02x", md[i]);
	}

	//cout << "--------------------------------------" << endl;
	//cout << "key:" << key << endl;
	//cout << "r1:" << req.r1 << endl;
	//cout << "authCode:" << req.authCode << endl;
	//cout << "--------------------------------------" << endl;

	//将要发送的数据进行编码
	int dataLen;
	char* outData = NULL;
	CodecFactory* factory = new RequestFactory(&req);
	Codec* pCodec = factory->createCodec();
	pCodec->msgEncode(&outData, dataLen);

	delete factory;
	factory = nullptr;

	//连接服务端
	m_socket.connectToHost(m_info.serverIP, m_info.serverPort);

	//发送请求数据给服务端
	m_socket.sendMsg(outData, dataLen);

	//等待接收服务端的应答
	char* inData;
	m_socket.recvMsg(&inData, dataLen);

	//解码
	factory = new RespondFactory();
	pCodec = factory->createCodec();
	RespondMsg* pMsg = (RespondMsg*)pCodec->msgDecode(inData, dataLen);

	cout << "\n  clientId: " << pMsg->clientID << "  serverId: " << pMsg->serverId << endl;
	
	//判断服务端是否成功
	if (pMsg->rv == 1)
	{
		cout << "  消息认证失败..." << endl;
		return -1;
	}
	else if (pMsg->rv == 2) { // 服务端查询不到客户端ID
		cout << "  ID校验失败,拒绝服务..." << endl;
		return -1;
	}
	else
		cout << "  密钥协商成功..." << endl;

	//将服务端的r2和客户端的r1拼接生成密钥
	char buf[1024];
	unsigned char md1[SHA_DIGEST_LENGTH];
	memset(md1, 0x00, sizeof(md1));
	char seckey[SHA_DIGEST_LENGTH * 2 + 1];
	memset(buf, 0x00, sizeof(buf));
	memset(seckey, 0x00, sizeof(seckey));
	sprintf(buf, "%s%s", req.r1, pMsg->r2);
	SHA1((unsigned char*)buf, strlen((char*)buf), md1);
	for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		sprintf(&seckey[i * 2], "%02x", md1[i]);
	}
	cout << "  密钥: " << seckey << endl;

	//给密钥结构体赋值
	NodeSHMInfo node;
	memset(&node, 0x00, sizeof(NodeSHMInfo));
	node.status = 1;
	strcpy(node.seckey, seckey);
	strcpy(node.clientID, m_info.clientID);
	strcpy(node.serverID, m_info.serverID);
	node.seckeyID = pMsg->seckeyid;
	cout << "  seckeyID: " << pMsg->seckeyid << endl;

	//将密钥信息写入共享内存
	m_shm->shmWrite(&node);

	//关闭网络连接
	m_socket.disConnect();

	//释放资源
	delete factory;
	factory = nullptr;
	return 0;
}

// 密钥校验
int ClientOperation::secKeyCheck()
{
	//从共享内存读取密钥信息
	NodeSHMInfo node;
	memset(&node, 0x00, sizeof(NodeSHMInfo));
	m_shm->shmRead(m_info.clientID, m_info.serverID, &node);

	//检查密钥是否已经注销
	if (node.status != 1) {
		cout << "  无可用密钥..." << endl;
		return -1;
	}

	//准备请求数据 
	RequestMsg req;
	memset(&req, 0x00, sizeof(RequestMsg));
	req.cmdType = RequestCodec::Check;
	memcpy(req.r1, node.seckey, strlen(node.seckey)); //密钥拷贝到r1
	strcpy(req.clientID, m_info.clientID);
	strcpy(req.serverId, m_info.serverID);

	//使用hmac函数将密钥生成哈希值----消息认证码
	char key[64];
	unsigned int len;
	unsigned char md[SHA256_DIGEST_LENGTH];
	memset(key, 0x00, sizeof(key));
	sprintf(key, "@%s+%s@", req.serverId, req.clientID);
	HMAC(EVP_sha256(), key, strlen(key), (unsigned char*)req.r1, strlen(req.r1), md, &len);
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(&req.authCode[2 * i], "%02x", md[i]);
	}

	//cout << "req.clientID: " << req.clientID << endl;
	//cout << "req.serverId: " << req.serverId << endl;
	//cout << "req.r1: " << req.r1 << endl;
	//cout << "req.authCode: " << req.authCode << endl;

	//将要发送的数据进行编码
	int dataLen;
	char* outData = NULL;
	CodecFactory* factory = new RequestFactory(&req);
	Codec* pCodec = factory->createCodec();
	pCodec->msgEncode(&outData, dataLen);

	delete factory;
	factory = nullptr;

	//连接服务端
	m_socket.connectToHost(m_info.serverIP, m_info.serverPort);

	//发送请求数据给服务端
	m_socket.sendMsg(outData, dataLen);

	//等待接收服务端的应答
	char* inData;
	m_socket.recvMsg(&inData, dataLen);

	//解码
	factory = new RespondFactory();
	pCodec = factory->createCodec();
	RespondMsg* pMsg = (RespondMsg*)pCodec->msgDecode(inData, dataLen);

	cout << "\n  clientId: " << pMsg->clientID << "  serverId: " << pMsg->serverId << endl;
	
	//判断服务端是否成功
	if (pMsg->rv == 1)
	{
		cout << "  密钥校验失败..." << endl;
		cout << "  密钥: " << req.r1 << endl;
		return -1;
	}

	cout << "  密钥校验成功..." << endl;
	cout << "  密钥: " << req.r1 << endl;
	return 0;
}

// 密钥注销
int ClientOperation::secKeyRevoke()
{
	//从共享内存读取密钥信息
	NodeSHMInfo node;
	memset(&node, 0x00, sizeof(NodeSHMInfo));
	m_shm->shmRead(m_info.clientID, m_info.serverID, &node);

	//检查密钥是否已经注销
	if (node.status != 1) {
		cout << "  无可用密钥..." << endl;
		return -1;
	}

	//准备请求数据 
	RequestMsg req;
	memset(&req, 0x00, sizeof(RequestMsg));
	req.cmdType = RequestCodec::Revoke;
	memcpy(req.r1, node.seckey, strlen(node.seckey)); //密钥拷贝到r1
	strcpy(req.clientID, m_info.clientID);
	strcpy(req.serverId, m_info.serverID);

	//使用hmac函数将密钥生成哈希值----消息认证码
	char key[64];
	unsigned int len;
	unsigned char md[SHA256_DIGEST_LENGTH];
	memset(key, 0x00, sizeof(key));
	sprintf(key, "@%s+%s@", req.serverId, req.clientID);
	HMAC(EVP_sha256(), key, strlen(key), (unsigned char*)req.r1, strlen(req.r1), md, &len);
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(&req.authCode[2 * i], "%02x", md[i]);
	}

	//cout << "req.clientID: " << req.clientID << endl;
	//cout << "req.serverId: " << req.serverId << endl;
	//cout << "req.r1: " << req.r1 << endl;
	//cout << "req.authCode: " << req.authCode << endl;

	//将要发送的数据进行编码
	int dataLen;
	char* outData = NULL;
	CodecFactory* factory = new RequestFactory(&req);
	Codec* pCodec = factory->createCodec();
	pCodec->msgEncode(&outData, dataLen);

	delete factory;
	factory = nullptr;

	//连接服务端
	m_socket.connectToHost(m_info.serverIP, m_info.serverPort);

	//发送请求数据给服务端
	m_socket.sendMsg(outData, dataLen);

	//等待接收服务端的应答
	char* inData;
	m_socket.recvMsg(&inData, dataLen);

	//解码
	factory = new RespondFactory();
	pCodec = factory->createCodec();
	RespondMsg* pMsg = (RespondMsg*)pCodec->msgDecode(inData, dataLen);

	cout << "\n  clientId: " << pMsg->clientID << "  serverId: " << pMsg->serverId << endl;

	//判断服务端是否成功
	if (pMsg->rv == 1)
	{
		cout << "  信息核对失败，注销申请被拒绝..." << endl;
		return -1;
	}

	//共享内存中密钥的状态修改为不可用
	node.status = 0;
	m_shm->shmWrite(&node); 

	cout << "  信息核对成功，密钥已注销..." << endl;
	cout << "  密钥: " << req.r1 << endl;
	return 0;
}

// 密钥查看
int ClientOperation::secKeyView()
{
	//从共享内存读取密钥信息
	NodeSHMInfo node;
	memset(&node, 0x00, sizeof(NodeSHMInfo));
	m_shm->shmRead(m_info.clientID, m_info.serverID, &node);

	if (node.status==1) {
		cout << "\n  密钥信息如下：" << endl;
		cout << "----------------------------------------------------------------------------------" << endl;
		cout << setw(10) << "clientid " << setw(10) << "serverid " 
			<< setw(10) << "keyid " << setw(10) << "state " << setw(25) << "seckey" << endl;
		cout << setw(5) << node.clientID << setw(10) << node.serverID
			<< setw(12) << node.seckeyID << setw(10) << node.status << setw(45) << node.seckey << endl;
	}
	else {
		cout << "  无可用密钥..." << endl;
	}

}

// char randBuf[64]; , 参数 64, randBuf
void ClientOperation::getRandString(int len, char* randBuf)
{
	int flag = -1;
	//设置随机种子
	srand(time(NULL));
	//随机字符串: A-Z, a-z, 0-9, 特殊字符(!@#$%^&*()_+=)
	char chars[] = "!@#$%^&*()_+=";
	for (int i = 0; i < len - 1; ++i)
	{
		flag = rand() % 4;
		switch (flag)
		{
		case 0:
			randBuf[i] = rand() % 26 + 'A';
			break;
		case 1:
			randBuf[i] = rand() % 26 + 'a';
			break;
		case 2:
			randBuf[i] = rand() % 10 + '0';
			break;
		case 3:
			randBuf[i] = chars[rand() % strlen(chars)];
			break;
		default:
			break;
		}
	}
	randBuf[len - 1] = '\0';
}
