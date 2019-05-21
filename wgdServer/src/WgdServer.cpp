#include "WgdServer.h"
#include "WgdConn.h"
#include "BaseSocket.h"

namespace three_year
{
	CWgdServer::CWgdServer(const std::string& ip, uint16_t port)
		: ip_(ip), port_(port)
	{
		m_bRunning = true;

		FD_ZERO(&m_read_set);
		FD_ZERO(&m_write_set);
		FD_ZERO(&m_excep_set);

		fds[0] = INVALID_SOCKET;
		fds[1] = INVALID_SOCKET;
	}

	CWgdServer::~CWgdServer()
	{
		m_bRunning = false;

		if (fds[0] != INVALID_SOCKET)
			closesocket(fds[0]);
		if (fds[1] != INVALID_SOCKET)
			closesocket(fds[1]);

		int ret = NETLIB_OK;
		if (WSACleanup() != 0)
		{
			ret = NETLIB_ERROR;
		}
	}

	bool CWgdServer::init()
	{
		int ret = NETLIB_OK;

		WSADATA wsaData;
		WORD wReqest = MAKEWORD(1, 1);
		if (WSAStartup(wReqest, &wsaData) != 0)
		{
			ret = NETLIB_ERROR;
		}

		if (ret == NETLIB_ERROR)
			return false;

		return true;
	}

	void CWgdServer::OnAccept(SOCKET s, int event)
	{
		SOCKET fd = 0;
		sockaddr_in peer_addr;
		socklen_t addr_len = sizeof(sockaddr_in);
		char ip_str[64];
		while ((fd = accept(s, (sockaddr*)&peer_addr, &addr_len)) != INVALID_SOCKET)
		{
			auto new_socket = std::make_shared<CBaseSocket>();
			uint32_t ip = ntohl(peer_addr.sin_addr.s_addr);
			uint16_t port = ntohs(peer_addr.sin_port);

			sprintf_s(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip >> 24, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

			//printf("AcceptNewSocket, socket=%d from %s:%d\n", fd, ip_str, port);

			new_socket->SetSocket(fd);
			new_socket->SetState(SOCKET_STATE_CONNECTED);
			new_socket->SetRemoteIP(ip_str);
			new_socket->SetRemotePort(port);
			new_socket->_SetNoDelay();
			new_socket->_SetNonblock();

			auto conn = std::make_shared<CWGDConn>(this, new_socket);
			AddWgdConn(conn, fd);
			AddEvCb(fd, [conn](SOCKET s, int event) {conn->OnRead(); });
			AddEvent(fd, SOCKET_READ | SOCKET_EXCEP);
		}
	}

	void CWgdServer::AddEvCb(SOCKET s, ev_cb cb)
	{
		m_ev_map[s] = cb;
	}

	void CWgdServer::async_cb()
	{
		printf("async_cb call!\n");
	}

	//和libuv有局限 libuv可以多次uv_init_async()/uv_async_send() 这样每个线程都可以有自己的uv_async_t
	// libuv利用了windows的IOCP机制 通过不同的overlap触发 然后根据overlap地址找到自己设定的回调函数
	// linux下是通过eventfd下实现的
	// 这里是事先在主线程里创建好了socket_pair 只支非主线程的某一个线程触发
	void CWgdServer::uv_init_async(void_cb cb)
	{
		callback_ = cb;
	}

	void CWgdServer::uv_async_send()
	{
		uint32_t one = 1;
		send(fds[0], (char*)&one, sizeof(one), 0);
	}

	void CWgdServer::run()
	{
		Sleep(2000);

#if 0
		uint32_t one = 1;
		send(fds[0], (char*)&one, sizeof(one), 0);
		one = 100;
		send(fds[0], (char*)&one, sizeof(one), 0);
#else
		uv_init_async([this]() {this->async_cb(); });
		uv_async_send();
#endif
	}

	int CWgdServer::SocketPair()
	{
		SOCKET tmp_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (tmp_socket == INVALID_SOCKET)
		{
			printf("create socket failed, err_code=%d\n", WSAGetLastError());
			return NETLIB_ERROR;
		}
	
		struct sockaddr_in inaddr = {0};
		struct sockaddr addr = {0};
		inaddr.sin_family = AF_INET;
		inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		inaddr.sin_port = 0;
		int yes = 1;
		setsockopt(tmp_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
		int ret = bind(tmp_socket, (struct sockaddr *)&inaddr, sizeof(inaddr));
		if (ret == SOCKET_ERROR)
		{
			printf("SocketPair bind failed, err_code=%d\n", WSAGetLastError());
			closesocket(tmp_socket);
			return NETLIB_ERROR;
		}
		ret = listen(tmp_socket, 1);
		if (ret == SOCKET_ERROR)
		{
			printf("SocketPair listen failed, err_code=%d\n", WSAGetLastError());
			closesocket(tmp_socket);
			return NETLIB_ERROR;
		}
		int len = sizeof(inaddr);
		getsockname(tmp_socket, &addr, &len);
		fds[0] = ::socket(AF_INET, SOCK_STREAM, 0);
		if (fds[0] == INVALID_SOCKET)
		{
			printf("SocketPair create fds[0] failed, err_code=%d\n", WSAGetLastError());
			closesocket(tmp_socket);
			return NETLIB_ERROR;
		}
		ret = connect(fds[0], &addr, len);
		if (ret == SOCKET_ERROR)
		{
			printf("SocketPair connect failed, err_code=%d\n", WSAGetLastError());
			closesocket(fds[0]);
			closesocket(tmp_socket);
			return NETLIB_ERROR;
		}
		fds[1] = accept(tmp_socket, 0, 0);
		if (fds[1] == INVALID_SOCKET)
		{
			printf("SocketPair create fds[1] failed, err_code=%d\n", WSAGetLastError());
			closesocket(fds[0]);
			closesocket(tmp_socket);
			return NETLIB_ERROR;
		}
		closesocket(tmp_socket);
		AddEvent(fds[1], SOCKET_READ);
		return NETLIB_OK;
	}

	int CWgdServer::StartWork()
	{
		auto socketPtr = std::make_shared<CBaseSocket>();
		int ret = socketPtr->Listen(ip_.c_str(), port_);
		if (ret == NETLIB_ERROR)
			return ret;

		ret = SocketPair();
		if (ret != NETLIB_OK)
		{
			printf("SocketPair err");
			return NETLIB_ERROR;
		}

		//for test notify/cancel waiting in select() from other thread
		std::thread t1([this] {this->run(); });

		AddEvent(socketPtr->GetSocket(), SOCKET_READ | SOCKET_EXCEP);
		AddEvCb(socketPtr->GetSocket(), [this](SOCKET s, int event) {this->OnAccept(s, event); });

		loop();

		return 0;
	}


	void CWgdServer::loop()
	{
		while (m_bRunning)
		{
			fd_set				read_set;
			fd_set				write_set;
			fd_set				excep_set;

			memcpy(&read_set, &m_read_set, sizeof(fd_set));
			memcpy(&write_set, &m_write_set, sizeof(fd_set));
			memcpy(&excep_set, &m_excep_set, sizeof(fd_set));

			int nfds = select(0, &read_set, &write_set, &excep_set, NULL);
			if (nfds == SOCKET_ERROR)
			{
				printf("select failed, error code: %d\n", GetLastError());
				Sleep(MIN_TIMER_DURATION);
				continue;
			}

			if (nfds == 0)
				continue;

			for (u_int i = 0; i < read_set.fd_count; i++)
			{
				SOCKET fd = read_set.fd_array[i];
				if (fd == fds[1])
				{
					int32_t one;
					recv(fds[1], (char*)&one, sizeof(one), 0);
					callback_();
				}
				else
				{
					auto itor = m_ev_map.find(fd);
					if (itor != m_ev_map.end())
						itor->second(fd, SOCKET_READ);
				}
			}

			for (u_int i = 0; i < excep_set.fd_count; i++)
			{
				SOCKET fd = excep_set.fd_array[i];
				auto itor = m_ev_map.find(fd);
				if (itor != m_ev_map.end())
					itor->second(fd, SOCKET_ERROR);
			}
		}
	}

	void CWgdServer::AddEvent(SOCKET fd, uint8_t socket_event)
	{
		if ((socket_event & SOCKET_READ) != 0)
			FD_SET(fd, &m_read_set);

		if ((socket_event & SOCKET_WRITE) != 0)
			FD_SET(fd, &m_write_set);

		if ((socket_event & SOCKET_EXCEP) != 0)
			FD_SET(fd, &m_excep_set);
	}

	void CWgdServer::RemoveEvent(SOCKET fd, uint8_t socket_event)
	{
		if ((socket_event & SOCKET_READ) != 0)
			FD_CLR(fd, &m_read_set);

		if ((socket_event & SOCKET_WRITE) != 0)
			FD_CLR(fd, &m_write_set);

		if ((socket_event & SOCKET_EXCEP) != 0)
			FD_CLR(fd, &m_excep_set);
	}

	void CWgdServer::AddWgdConn(connPtr conn, net_handle_t fd)
	{
		printf("Add Conn %d \n", fd);
		m_conn_map.insert(make_pair(fd, conn));
	}

	void CWgdServer::RemoveWgdConn(net_handle_t handle)
	{
		printf("Remove Conn %d \n", handle);

		RemoveEvent(handle, SOCKET_ALL);

		auto itor = m_conn_map.find(handle);
		if (itor != m_conn_map.end())
			m_conn_map.erase(itor);

		auto iter = m_ev_map.find(handle);
		if (iter != m_ev_map.end())
			m_ev_map.erase(iter);
	}

	connPtr CWgdServer::FindWgdConn(net_handle_t handle)
	{
		auto itor = m_conn_map.find(handle);
		if (itor != m_conn_map.end())
			return itor->second;

		return connPtr();
	}

	int	CWgdServer::ReConnect(net_handle_t fd)
	{
		return 0;
	}

	int	CWgdServer::OnExcept(net_handle_t fd)
	{
		return 0;
	}

	int	CWgdServer::OnReceivedNotify(net_handle_t fd, void* pData, int len)
	{
		if (NULL == pData || len <= 0)
		{
			return -1;
		}

		OnTest(fd, pData, len);

		return 0;
	}

	void CWgdServer::OnTest(net_handle_t fd, void* pData, int len)
	{
		WGDHEAD* pHead = (WGDHEAD*)pData;

		//printf("Have Receive Data Cmd: %d\n", pHead->nSubCmd);

		connPtr pConn = FindWgdConn(fd);
		if (pConn.get())
		{
			std::vector<char> buf(1024, 'a');
			std::vector<char> send_buf;
			WGDHEAD wh;
			wh.nParentCmd = pHead->nSubCmd;
			wh.nSubCmd = 100;
			wh.nDataLen = buf.size();
			send_buf.insert(send_buf.end(), (char*)&wh, (char*)&wh + sizeof(WGDHEAD));
			send_buf.insert(send_buf.end(), buf.begin(), buf.end());
			pConn->Send(&send_buf[0], send_buf.size());
		}
	}
}

