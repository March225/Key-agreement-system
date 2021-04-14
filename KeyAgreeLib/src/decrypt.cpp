#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/aes.h>
#include "SecKeyShm.h"
#include "decrypt.h"

int decryptFun(const unsigned char* inData, int inLen, unsigned char* &outData, int& outLen, const char* serverID, const char* clientID, int shmKey)
{
	// 根据两个id找到密钥，对输入数据进行解密，然后输出

	//从共享内存读取密钥信息
	SecKeyShm shm(shmKey);
	NodeSHMInfo node;
	memset(&node, 0x00, sizeof(NodeSHMInfo));
	shm.shmRead(clientID, serverID, &node);
	//printf("  密钥信息: %s\n", node.seckey);

	// aes数据解密
	// 1. 秘钥
	AES_KEY aes;
	AES_set_decrypt_key((const unsigned char*)node.seckey, 128, &aes);

	// 2. 解密
	int length;
	// 计算第三个参数length的长度, 包含了字符串末尾的\0
	if ((inLen + 1) % 16 == 0)
	{
		// 长度刚好合适
		length = inLen + 1;
	}
	else
	{
		length = ((inLen + 1) / 16 + 1) * 16;
	}

	outData = (unsigned char*)calloc(length, 1);
	outLen = length;
	unsigned char iv[16];
	memset(iv, 'a', sizeof(iv));

	AES_cbc_encrypt(inData, outData, length, &aes, iv, AES_DECRYPT);
	return 0;	
}
