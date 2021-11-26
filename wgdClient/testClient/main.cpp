// NetClient.cpp : Defines the entry point for the console application.
//

#include "JSClient.h"
#include "CommonNet.h"
#include <stdio.h>
#include <memory>

#define TEST_SIZE  (200)

using namespace three_year;

class CMyNotify:public CNotify
{
public:
	CMyNotify() {};
	~CMyNotify() {};

public:
	 virtual int onData(void* data, int len);
};

int CMyNotify::onData(void* data, int len)
{
	WGDHEAD* pHead = (WGDHEAD*)data;
	
	printf("Have receive Server nParentCmd:[%d] sub: [%d] totolSize:[%d]\n", pHead->nParentCmd, pHead->nSubCmd, len);

	if (pHead->nParentCmd == 0 && pHead->nSubCmd == 100)
	{
		//exit(0);
	}
		
	return 0;
}

int main()
{
 	std::shared_ptr<CMyNotify> pNofity = std::make_shared<CMyNotify>();
	std::shared_ptr<CJSClient> cjsClient = std::make_shared<CJSClient>(pNofity.get());

	int nRet = cjsClient->Connect("172.16.11.81", 12000);
	if (nRet == -1)
	{
		printf("Connect Main Server fail\n");
		return -1;
	}

	std::vector<char> buf(TEST_SIZE, 'a');
	WGDHEAD head;
	head.nParentCmd = 12;
	head.nSubCmd = 34;
	head.nDataLen = TEST_SIZE;
	std::vector<char> send_buf;
	send_buf.insert(send_buf.end(), (char*)&head, (char*)&head + sizeof(WGDHEAD));
	send_buf.insert(send_buf.end(), buf.begin(), buf.end());
	cjsClient->Send(&send_buf[0], send_buf.size());

	while (true)
	{
		auto c = getchar();
		if (c == 'q')
		{
			printf("q pressed, quiting...\n");
			break;
		}
		else if (c == 's') {
			printf("begin send data\n");
			cjsClient->Send(&send_buf[0], send_buf.size());
			continue;
		}
	}

	return 0;
}

