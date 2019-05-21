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
	using void_cb = std::function<void()>;

	class CWgdServer : public IServerNotify
	{
	public:
		CWgdServer(const std::string& ip, uint16_t port);

		virtual ~CWgdServer();

	public:
		void AddEvCb(SOCKET s, ev_cb cb);

		void OnAccept(SOCKET s, int event);

	public:
		virtual int			OnExcept(net_handle_t fd);

		virtual int			OnConnect(net_handle_t fd) { return 0; }

		virtual int			ReConnect(net_handle_t fd);

		virtual int			OnReceivedNotify(net_handle_t fd, void* pData, int len);

		void	OnTest(net_handle_t fd, void* pData, int len);

	public:
		//like libuv
		void uv_init_async(void_cb cb);

		void uv_async_send();

		int SocketPair();

		void AddEvent(SOCKET fd, uint8_t socket_event);

		void RemoveEvent(SOCKET fd, uint8_t socket_event);

		connPtr FindWgdConn(net_handle_t handle);

		void AddWgdConn(connPtr conn, net_handle_t fd);

		void RemoveWgdConn(net_handle_t handle);

	public:
		bool	init();

		int		StartWork();

		void	loop();

	private:
		void run();

		void async_cb();
	private:
		std::string						ip_;
		uint16_t						port_;

		bool							m_bRunning;

		fd_set							m_read_set;
		fd_set							m_write_set;
		fd_set							m_excep_set;

		ConnMap_t						m_conn_map;

		std::map<SOCKET, ev_cb>			m_ev_map;

		SOCKET							fds[2];		//for async from other thread --invoke select

		void_cb							callback_;
	};
}
