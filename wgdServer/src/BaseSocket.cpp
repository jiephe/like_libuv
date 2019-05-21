#include "BaseSocket.h"
#include "WgdServer.h"
#include "WgdConn.h"

namespace three_year
{
	CBaseSocket::CBaseSocket()
	{
		m_socket = INVALID_SOCKET;
		m_state = SOCKET_STATE_IDLE;
	}

	CBaseSocket::~CBaseSocket()
	{
		//printf("CBaseSocket::~CBaseSocket, socket=%d\n", m_socket);
	}

	int CBaseSocket::Listen(const char* server_ip, uint16_t port)
	{
		m_local_ip = server_ip;
		m_local_port = port;

		m_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (m_socket == INVALID_SOCKET)
		{
			printf("socket failed, err_code=%d\n", _GetErrorCode());
			return NETLIB_ERROR;
		}

		printf("Main Socket[%d]...\n", m_socket);

		_SetReuseAddr();
		_SetNonblock();

		sockaddr_in serv_addr;
		_SetAddr(server_ip, port, &serv_addr);
		int ret = ::bind(m_socket, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (ret == SOCKET_ERROR)
		{
			printf("bind failed, err_code=%d\n", _GetErrorCode());
			closesocket(m_socket);
			return NETLIB_ERROR;
		}

		ret = listen(m_socket, 64);
		if (ret == SOCKET_ERROR)
		{
			printf("listen failed, err_code=%d\n", _GetErrorCode());
			closesocket(m_socket);
			return NETLIB_ERROR;
		}

		m_state = SOCKET_STATE_LISTENING;

		printf("CBaseSocket::Listen on %s:%d\n", server_ip, port);

		return NETLIB_OK;
	}

	net_handle_t CBaseSocket::Connect(const char* server_ip, uint16_t port)
	{
		printf("CBaseSocket::Connect, server_ip=%s, port=%d\n", server_ip, port);

		m_remote_ip = server_ip;

		m_remote_port = port;

		m_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (m_socket == INVALID_SOCKET)
		{
			printf("socket failed, err_code=%d\n", _GetErrorCode());
			return NETLIB_INVALID_HANDLE;
		}

		_SetNonblock();
		_SetNoDelay();
		sockaddr_in serv_addr;
		_SetAddr(server_ip, port, &serv_addr);
		int ret = connect(m_socket, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if ((ret == SOCKET_ERROR) && (!_IsBlock(_GetErrorCode())))
		{
			printf("connect failed, err_code=%d\n", _GetErrorCode());
			closesocket(m_socket);
			return NETLIB_INVALID_HANDLE;
		}
		m_state = SOCKET_STATE_CONNECTING;

		return (net_handle_t)m_socket;
	}

	int CBaseSocket::Send(void* buf, int len)
	{
		if (m_state != SOCKET_STATE_CONNECTED)
			return NETLIB_ERROR;

		int ret = send(m_socket, (char*)buf, len, 0);
		return ret;
	}

	int CBaseSocket::Recv(void* buf, int len)
	{
		return recv(m_socket, (char*)buf, len, 0);
	}

	int CBaseSocket::Close()
	{
		m_state = SOCKET_STATE_CLOSING;
		closesocket(m_socket);
		return 0;
	}

	void CBaseSocket::SetSendBufSize(uint32_t send_size)
	{
		int ret = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (const char*)&send_size, 4);
		if (ret == SOCKET_ERROR)
		{
			//log("set SO_SNDBUF failed for fd=%d", m_socket);
		}

		socklen_t len = 4;
		char size = 0;
		getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &size, &len);
		//log("socket=%d send_buf_size=%d", m_socket, size);
	}

	void CBaseSocket::SetRecvBufSize(uint32_t recv_size)
	{
		int ret = setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&recv_size, 4);
		if (ret == SOCKET_ERROR)
		{
			//log("set SO_RCVBUF failed for fd=%d", m_socket);
		}

		socklen_t len = 4;
		char size = 0;
		getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &size, &len);
		//log("socket=%d recv_buf_size=%d", m_socket, size);
	}

	int CBaseSocket::_GetErrorCode()
	{
		return WSAGetLastError();
	}

	bool CBaseSocket::_IsBlock(int error_code)
	{
		return ((error_code == WSAEINPROGRESS) || (error_code == WSAEWOULDBLOCK));
	}

	void CBaseSocket::_SetNonblock()
	{
		u_long nonblock = 1;
		int ret = ioctlsocket(m_socket, FIONBIO, &nonblock);
		if (ret == SOCKET_ERROR)
		{
			//log("_SetNonblock failed, err_code=%d", _GetErrorCode());
		}
	}

	void CBaseSocket::_SetReuseAddr()
	{
		int reuse = 1;
		int ret = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
		if (ret == SOCKET_ERROR)
		{
			//log("_SetReuseAddr failed, err_code=%d", _GetErrorCode());
		}
	}

	void CBaseSocket::_SetNoDelay()
	{
		int nodelay = 1;
		int ret = setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
		if (ret == SOCKET_ERROR)
		{
			//log("_SetNoDelay failed, err_code=%d", _GetErrorCode());
		}
	}

	void CBaseSocket::_SetAddr(const char* ip, const uint16_t port, sockaddr_in* pAddr)
	{
		memset(pAddr, 0, sizeof(sockaddr_in));
		pAddr->sin_family = AF_INET;
		pAddr->sin_port = htons(port);
		pAddr->sin_addr.s_addr = inet_addr(ip);
		if (pAddr->sin_addr.s_addr == INADDR_NONE)
		{
			hostent* host = gethostbyname(ip);
			if (host == NULL)
			{
				//log("gethostbyname failed, ip=%s", ip);
				return;
			}

			pAddr->sin_addr.s_addr = *(uint32_t*)host->h_addr;
		}
	}
}



