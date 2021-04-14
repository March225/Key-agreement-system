#include "RequestCodec.h"
#include <iostream>
using namespace std;

//œâÂëµÄÊ±ºòÊ¹ÓÃ
RequestCodec::RequestCodec()
{
}

//±àÂëµÄÊ±ºòµ÷ÓÃ
RequestCodec::RequestCodec(RequestMsg * msg)
{
	// ž³Öµ²Ù×÷
	memcpy(&m_msg, msg, sizeof(RequestMsg));
}

RequestCodec::~RequestCodec()
{
}

int RequestCodec::msgEncode(char ** outData, int & len)
{
	writeHeadNode(m_msg.cmdType);
	writeNextNode(m_msg.clientId, strlen(m_msg.clientId)+1);
	writeNextNode(m_msg.authCode, strlen(m_msg.authCode) + 1);
	writeNextNode(m_msg.serverId, strlen(m_msg.serverId) + 1);
	writeNextNode(m_msg.r1, strlen(m_msg.r1) + 1);
	packSequence(outData, len);

	return 0;
}

void * RequestCodec::msgDecode(char * inData, int inLen)
{
	//·ŽÐòÁÐ»¯
	unpackSequence(inData, inLen);
	readHeadNode(m_msg.cmdType);
	readNextNode(m_msg.clientId);
	readNextNode(m_msg.authCode);
	readNextNode(m_msg.serverId);
	readNextNode(m_msg.r1);

	return &m_msg;
}
