#include "WgdServer.h"
#include <stdio.h>
#include <string>

using namespace three_year;

int main()
{
	CWgdServer ws(std::string("0.0.0.0"), 32005);

	bool bRet = ws.init();
	if (bRet)
		printf("Server init Ok!\n");
	else
	{
		printf("Server init Fail!\n");
		return -1;
	}

	printf("Server begin to work....\n");

	ws.StartWork();

	printf("Server end to work....\n");

	return 0;
}
