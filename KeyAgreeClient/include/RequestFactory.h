#pragma once
#include "../include/CodecFactory.h"
#include "../include/RequestCodec.h"

class RequestFactory :
	public CodecFactory
{
public:
	RequestFactory();
	RequestFactory(RequestMsg* msg);
	~RequestFactory();

	Codec* createCodec();

private:
	bool m_flag = false;
	RequestMsg * m_request;
};

