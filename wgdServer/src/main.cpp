#include "WgdServer.h"
#include <stdio.h>

using namespace three_year;

int main()
{
	auto ws = std::make_shared<CWgdServer>("0.0.0.0", 50052);

	printf("Server begin to work....\n");

	ws->StartWork();

	printf("Server end to work....\n");

	return 0;
}
