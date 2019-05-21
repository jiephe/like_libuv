#pragma once

#include "ostype.h"
#include <string>
#include "internal.h"

namespace three_year
{
	class CBaseSocket
	{
	public:
		CBaseSocket();

		virtual ~CBaseSocket();

		SOCKET GetSocket() { return m_socket; }
		void SetSocket(SOCKET fd) { m_socket = fd; }
		void SetState(uint8_t state) { m_state = state; }

		void SetRemoteIP(char* ip) { m_remote_ip = ip; }
		void SetRemotePort(uint16_t port) { m_remote_port = port; }
		void SetSendBufSize(uint32_t send_size);
		void SetRecvBufSize(uint32_t recv_size);

		const char*	GetRemoteIP() { return m_remote_ip.c_str(); }
		uint16_t	GetRemotePort() { return m_remote_port; }
		const char*	GetLocalIP() { return m_local_ip.c_str(); }
		uint16_t	GetLocalPort() { return m_local_port; }

	public:
		void _SetNonblock();
		void _SetReuseAddr();
		void _SetNoDelay();

	public:
		int Listen(
			const char*		server_ip,
			uint16_t		port);

		net_handle_t Connect(
			const char*		server_ip,
			uint16_t		port);

		int Send(void* buf, int len);

		int Recv(void* buf, int len);

		int Close();

	private:
		int _GetErrorCode();
		bool _IsBlock(int error_code);
		void _SetAddr(const char* ip, const uint16_t port, sockaddr_in* pAddr);

	private:
		std::string		m_remote_ip;
		uint16_t		m_remote_port;
		std::string		m_local_ip;
		uint16_t		m_local_port;

		uint8_t			m_state;
		SOCKET			m_socket;
	};
}

