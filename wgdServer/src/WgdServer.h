#pragma once

#include "ostype.h"
#include "IServerNotify.h"
#include "CommonNet.h"
#include "CommonDef.h"
#include "internal.h"

namespace three_year
{
	typedef std::map<net_handle_t, connPtr>			ConnMap_t;
	using ev_cb = std::function<void(SOCKET, int)>;
	using async_cb = std::function<void()>;

	class CWgdServer : public IServerNotify, public std::enable_shared_from_this<CWgdServer>
	{
	public:
		CWgdServer(const std::string& ip, uint16_t port);

		virtual ~CWgdServer();

	public:
		void AddReadEvCallback(SOCKET s, ev_cb cb);

		void AddWriteEvCallback(SOCKET s, ev_cb cb);

		void OnAccept(SOCKET s, int event);

	public:
		virtual int			OnConnect(net_handle_t fd) { return 0; }

		virtual int			OnReceivedNotify(net_handle_t fd, void* pData, int len);

		void	OnTest(net_handle_t fd, void* pData, int len);

	public:
		void uv_init_async(async_cb cb);

		void uv_async_send();

		int SocketPair();

		void AddEvent(SOCKET fd, uint8_t socket_event);

		void RemoveEvent(SOCKET fd, uint8_t socket_event);

		connPtr FindWgdConn(net_handle_t handle);

		void AddWgdConn(connPtr conn, net_handle_t fd);

		void RemoveWgdConn(net_handle_t handle);

	public:
		void	StartWork();

		void	loop();

	private:
		void run();

		void async();

	private:
		std::string						ip_;

		uint16_t						port_;

		bool							b_run_;

		fd_set							read_set_;
		fd_set							write_set_;
		fd_set							excep_set_;

		ConnMap_t						conn_map_;

		std::map<SOCKET, ev_cb>			ev_read_callback_;

		std::map<SOCKET, ev_cb>			ev_write_callback_;

		SOCKET							pair_fd_[2];		//for async from other thread --invoke select

		async_cb						async_cb_;
	};
}
