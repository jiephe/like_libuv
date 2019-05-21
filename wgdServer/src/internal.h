#pragma once

#include <memory>
#include <map>
#include <functional>
#include <string>
#include <vector>
#include <thread>

namespace three_year
{
	class CWGDConn;
	class CBaseSocket;

	using connPtr = std::shared_ptr<CWGDConn>;
	using socketPtr = std::shared_ptr<CBaseSocket>;
}


