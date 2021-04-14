#include <stdio.h>
#include "string.h"
#include "./include/encrypt.h"
#include "./include/decrypt.h"

int main()
{	
	const char* inData = "hello world!";
	int inLen = strlen(inData);
	unsigned char* outData = NULL;
	int outLen = 0;
	const char* serverID = "1";
	const char* clientID = "2";
	int shmKey;

	// 加密
	// int encryptFun(const unsigned char* inData, int inLen, unsigned char* &outData, int& outLen, const char* serverID, const char* clientID, int shmKey);
	shmKey = 2;
	encryptFun((const unsigned char*)inData, inLen, outData, outLen, serverID, clientID, shmKey);
	printf("  原始数据: %s\n\n", inData);

	// 解密
	// int decryptFun(const unsigned char* inData, int inLen, unsigned char* &outData, int& outLen, const char* serverID, const char* clientID, int shmKey);
	unsigned char* decData = NULL;
	int decLen = 0;
	shmKey = 0x12340000;
	decryptFun((const unsigned char*)outData, outLen, decData, decLen, serverID, clientID, shmKey);

	printf("  解密之后的数据: %s\n", decData);

	return 0;
}
