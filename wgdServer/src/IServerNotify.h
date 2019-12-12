#pragma once

namespace three_year
{
	class IServerNotify
	{
	public:
		virtual int OnConnect(net_handle_t fd) = 0;

		virtual int OnReceivedNotify(net_handle_t fd, void* pData, int len) = 0;
	};
}
