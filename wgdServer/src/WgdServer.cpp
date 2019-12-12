#include "WgdServer.h"
#include "WgdConn.h"
#include "BaseSocket.h"

#include "SimpleBuffer.h"

namespace three_year
{
	CWgdServer::CWgdServer(const std::string& ip, uint16_t port)
		: ip_(ip), port_(port), b_run_(true)
	{
		FD_ZERO(&read_set_);
		FD_ZERO(&write_set_);
		FD_ZERO(&excep_set_);

		pair_fd_[0] = INVALID_SOCKET;
		pair_fd_[1] = INVALID_SOCKET;

		WSADATA wsaData;
		WORD wReqest = MAKEWORD(1, 1);
		WSAStartup(wReqest, &wsaData);
	}

	CWgdServer::~CWgdServer()
	{
		b_run_ = false;

		if (pair_fd_[0] != INVALID_SOCKET)
			closesocket(pair_fd_[0]);
		if (pair_fd_[1] != INVALID_SOCKET)
			closesocket(pair_fd_[1]);

		WSACleanup();
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
			printf("accept, socket=%d from %s:%d\n", fd, ip_str, port);

			new_socket->set_socket(fd);
			new_socket->set_state(SOCKET_STATE_CONNECTED);
			new_socket->set_remote_ip(ip_str);
			new_socket->set_remote_port(port);
			new_socket->_SetNoDelay();
			new_socket->_SetNonblock();

			auto conn = std::make_shared<CWGDConn>(shared_from_this(), new_socket);
			AddWgdConn(conn, fd);
			AddReadEvCallback(fd, [conn](SOCKET s, int event) {conn->OnRead(); });
			AddEvent(fd, SOCKET_READ | SOCKET_EXCEP);
		}
	}

	void CWgdServer::AddReadEvCallback(SOCKET s, ev_cb cb)
	{
		ev_read_callback_[s] = cb;
	}
	void CWgdServer::AddWriteEvCallback(SOCKET s, ev_cb cb)
	{
		ev_write_callback_[s] = cb;
	}

	void CWgdServer::StartWork()
	{
		auto socketPtr = std::make_shared<CBaseSocket>();
		int ret = socketPtr->Listen(ip_.c_str(), port_);
		if (ret == NETLIB_ERROR)
			return;

		ret = SocketPair();
		if (ret != NETLIB_OK)
			return ;

		//for test notify/cancel waiting in select() from other thread
		std::thread t([this] {this->run(); });

		AddEvent(socketPtr->get_socket(), SOCKET_READ | SOCKET_EXCEP);
		AddReadEvCallback(socketPtr->get_socket(), [this](SOCKET s, int event) {this->OnAccept(s, event); });
		loop();
	}

	void CWgdServer::loop()
	{
		while (b_run_)
		{
			fd_set				read_set;
			fd_set				write_set;
			fd_set				excep_set;

			memcpy(&read_set, &read_set_, sizeof(fd_set));
			memcpy(&write_set, &write_set_, sizeof(fd_set));
			memcpy(&excep_set, &excep_set_, sizeof(fd_set));

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
				if (fd == pair_fd_[1])
				{
					int32_t one;
					recv(pair_fd_[1], (char*)&one, sizeof(one), 0);
					async_cb_();
				}
				else
				{
					auto itor = ev_read_callback_.find(fd);
					if (itor != ev_read_callback_.end())
						itor->second(fd, SOCKET_READ);
				}
			}

			for (u_int i = 0; i < write_set.fd_count; i++)
			{
				SOCKET fd = write_set.fd_array[i];
				auto itor = ev_write_callback_.find(fd);
				if (itor != ev_write_callback_.end())
				{
					itor->second(fd, SOCKET_WRITE);
					RemoveEvent(fd, SOCKET_WRITE);
				}
			}

			for (u_int i = 0; i < excep_set.fd_count; i++)
			{
				SOCKET fd = excep_set.fd_array[i];
				auto itor = ev_read_callback_.find(fd);
				if (itor != ev_read_callback_.end())
					itor->second(fd, SOCKET_ERROR);
			}
		}
	}

	void CWgdServer::async()
	{
		printf("async\n");

#if 0
		// this is in main thread can call send directly, so do this only for testing
		auto itor = conn_map_.begin();
		if (itor != conn_map_.end())
		{
			std::vector<char> buf(100, 'a');
			std::vector<char> send_buf;
			WGDHEAD wh;
			wh.nParentCmd = 200;
			wh.nSubCmd = 300;
			wh.nDataLen = buf.size();
			send_buf.insert(send_buf.end(), (char*)&wh, (char*)&wh + sizeof(WGDHEAD));
			send_buf.insert(send_buf.end(), buf.begin(), buf.end());

			AddEvent(itor->first, SOCKET_WRITE);
			itor->second->get_out_buff()->Write(&send_buf[0], send_buf.size());
			AddWriteEvCallback(itor->first, [itor](SOCKET s, int event) {itor->second->OnWrite(); });
		}
#endif
	}

	//和libuv比有局限 libuv可以多次uv_init_async()/uv_async_send() 这样每个线程都可以有自己的uv_async_t
	// libuv利用了windows的IOCP机制 通过不同的overlap触发 然后根据overlap地址找到自己设定的回调函数
	// linux下是通过eventfd下实现的
	// 这里是事先在主线程里创建好了socket_pair 只支持非主线程的某一个线程触发
	void CWgdServer::uv_init_async(async_cb cb)
	{
		async_cb_ = cb;
	}

	void CWgdServer::uv_async_send()
	{
		uint32_t one = 1;
		send(pair_fd_[0], (char*)&one, sizeof(one), 0);
	}

	void CWgdServer::run()
	{
		Sleep(20*1000);
		uv_init_async([this]() {this->async(); });
		uv_async_send();
	}

	int CWgdServer::SocketPair()
	{
		SOCKET tmp_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (tmp_socket == INVALID_SOCKET)
		{
			printf("create socket failed, err_code=%d\n", WSAGetLastError());
			return NETLIB_ERROR;
		}

		struct sockaddr_in inaddr = { 0 };
		struct sockaddr addr = { 0 };
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
		pair_fd_[0] = ::socket(AF_INET, SOCK_STREAM, 0);
		if (pair_fd_[0] == INVALID_SOCKET)
		{
			printf("SocketPair create fds[0] failed, err_code=%d\n", WSAGetLastError());
			closesocket(tmp_socket);
			return NETLIB_ERROR;
		}
		ret = connect(pair_fd_[0], &addr, len);
		if (ret == SOCKET_ERROR)
		{
			printf("SocketPair connect failed, err_code=%d\n", WSAGetLastError());
			closesocket(pair_fd_[0]);
			closesocket(tmp_socket);
			return NETLIB_ERROR;
		}
		pair_fd_[1] = accept(tmp_socket, 0, 0);
		if (pair_fd_[1] == INVALID_SOCKET)
		{
			printf("SocketPair create fds[1] failed, err_code=%d\n", WSAGetLastError());
			closesocket(pair_fd_[0]);
			closesocket(tmp_socket);
			return NETLIB_ERROR;
		}
		closesocket(tmp_socket);
		AddEvent(pair_fd_[1], SOCKET_READ);
		return NETLIB_OK;
	}

	void CWgdServer::AddEvent(SOCKET fd, uint8_t socket_event)
	{
		if ((socket_event & SOCKET_READ) != 0)
			FD_SET(fd, &read_set_);

		if ((socket_event & SOCKET_WRITE) != 0)
			FD_SET(fd, &write_set_);

		if ((socket_event & SOCKET_EXCEP) != 0)
			FD_SET(fd, &excep_set_);
	}

	void CWgdServer::RemoveEvent(SOCKET fd, uint8_t socket_event)
	{
		if ((socket_event & SOCKET_READ) != 0)
		{
			FD_CLR(fd, &read_set_);

			auto itor = ev_read_callback_.find(fd);
			if (itor != ev_read_callback_.end())
				ev_read_callback_.erase(itor);
		}
			
		if ((socket_event & SOCKET_WRITE) != 0)
		{
			FD_CLR(fd, &write_set_);
			auto itor = ev_write_callback_.find(fd);
			if (itor != ev_write_callback_.end())
				ev_write_callback_.erase(itor);
		}
			
		if ((socket_event & SOCKET_EXCEP) != 0)
		{
			FD_CLR(fd, &excep_set_);
			auto itor = ev_read_callback_.find(fd);
			if (itor != ev_read_callback_.end())
				ev_read_callback_.erase(itor);
		}		
	}

	void CWgdServer::AddWgdConn(connPtr conn, net_handle_t fd)
	{
		printf("Add Conn %d \n", fd);
		conn_map_.insert(make_pair(fd, conn));
	}

	void CWgdServer::RemoveWgdConn(net_handle_t handle)
	{
		printf("Remove Conn %d \n", handle);

		RemoveEvent(handle, SOCKET_ALL);
		auto itor = conn_map_.find(handle);
		if (itor != conn_map_.end())
			conn_map_.erase(itor);
	}

	connPtr CWgdServer::FindWgdConn(net_handle_t handle)
	{
		auto itor = conn_map_.find(handle);
		if (itor != conn_map_.end())
			return itor->second;

		return connPtr();
	}

	int	CWgdServer::OnReceivedNotify(net_handle_t fd, void* pData, int len)
	{
		if (NULL == pData || len <= 0)
			return -1;

		OnTest(fd, pData, len);
		return 0;
	}

	void CWgdServer::OnTest(net_handle_t fd, void* pData, int len)
	{
		WGDHEAD* pHead = (WGDHEAD*)pData;

		printf("Have Receive Data Cmd: %d\n", pHead->nSubCmd);

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

