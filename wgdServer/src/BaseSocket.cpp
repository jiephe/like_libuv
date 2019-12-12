#include "BaseSocket.h"
#include "CommonDef.h"

namespace three_year
{
	CBaseSocket::CBaseSocket()
	{
		socket_ = INVALID_SOCKET;
		state_ = SOCKET_STATE_IDLE;
	}

	CBaseSocket::~CBaseSocket()
	{}

	int CBaseSocket::Listen(const char* server_ip, uint16_t port)
	{
		set_local_ip(server_ip);
		set_local_port(port);

		socket_ = socket(AF_INET, SOCK_STREAM, 0);
		if (socket_ == INVALID_SOCKET)
		{
			printf("socket failed, err_code=%d\n", _GetErrorCode());
			return NETLIB_ERROR;
		}

		printf("Listen Socket:[%d]...\n", socket_);

		_SetReuseAddr();
		_SetNonblock();

		sockaddr_in serv_addr;
		_SetAddr(server_ip, port, &serv_addr);
		int ret = ::bind(socket_, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (ret == SOCKET_ERROR)
		{
			printf("bind failed, err_code=%d\n", _GetErrorCode());
			closesocket(socket_);
			return NETLIB_ERROR;
		}

		ret = listen(socket_, 64);
		if (ret == SOCKET_ERROR)
		{
			printf("listen failed, err_code=%d\n", _GetErrorCode());
			closesocket(socket_);
			return NETLIB_ERROR;
		}

		set_state(SOCKET_STATE_LISTENING);
		printf("Listen on %s:%d\n", server_ip, port);
		return NETLIB_OK;
	}

	net_handle_t CBaseSocket::Connect(const char* server_ip, uint16_t port)
	{
		set_remote_ip(server_ip);
		set_remote_port(port);

		socket_ = socket(AF_INET, SOCK_STREAM, 0);
		if (socket_ == INVALID_SOCKET)
		{
			printf("socket failed, err_code=%d\n", _GetErrorCode());
			return NETLIB_INVALID_HANDLE;
		}

		_SetNonblock();
		_SetNoDelay();
		sockaddr_in serv_addr;
		_SetAddr(server_ip, port, &serv_addr);
		int ret = connect(socket_, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if ((ret == SOCKET_ERROR) && (!_IsBlock(_GetErrorCode())))
		{
			printf("connect failed, err_code=%d\n", _GetErrorCode());
			closesocket(socket_);
			return NETLIB_INVALID_HANDLE;
		}

		set_state(SOCKET_STATE_CONNECTING);
		return (net_handle_t)socket_;
	}

	int CBaseSocket::Send(void* buf, int len)
	{
		if (get_state() != SOCKET_STATE_CONNECTED)
			return NETLIB_ERROR;

		int ret = send(socket_, (char*)buf, len, 0);
		return ret;
	}

	int CBaseSocket::Recv(void* buf, int len)
	{
		return recv(socket_, (char*)buf, len, 0);
	}

	int CBaseSocket::Close()
	{
		set_state(SOCKET_STATE_CLOSING);
		closesocket(socket_);
		return 0;
	}

	void CBaseSocket::SetSendBufSize(uint32_t send_size)
	{
		int ret = setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, (const char*)&send_size, 4);
		if (ret == SOCKET_ERROR)
			printf("set SO_SNDBUF error: %d\n", WSAGetLastError());
	}

	void CBaseSocket::SetRecvBufSize(uint32_t recv_size)
	{
		int ret = setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, (const char*)&recv_size, 4);
		if (ret == SOCKET_ERROR)
			printf("set SO_RCVBUF error: %d\n", WSAGetLastError());
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
		int ret = ioctlsocket(socket_, FIONBIO, &nonblock);
		if (ret == SOCKET_ERROR)
			printf("set nonblock error: %d\n", WSAGetLastError());
	}

	void CBaseSocket::_SetReuseAddr()
	{
		int reuse = 1;
		int ret = setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
		if (ret == SOCKET_ERROR)
			printf("set SO_REUSEADDR error: %d\n", WSAGetLastError());
	}

	void CBaseSocket::_SetNoDelay()
	{
		int nodelay = 1;
		int ret = setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
		if (ret == SOCKET_ERROR)
			printf("set TCP_NODELAY error: %d\n", WSAGetLastError());
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
				return;
			pAddr->sin_addr.s_addr = *(uint32_t*)host->h_addr;
		}
	}

	META_FUNCTION_CPP(std::string, CBaseSocket, remote_ip)
	META_FUNCTION_CPP(uint16_t, CBaseSocket, remote_port)
	META_FUNCTION_CPP(std::string, CBaseSocket, local_ip)
	META_FUNCTION_CPP(uint16_t, CBaseSocket, local_port)
	META_FUNCTION_CPP(uint8_t, CBaseSocket, state)
	META_FUNCTION_CPP(SOCKET, CBaseSocket, socket)
}



