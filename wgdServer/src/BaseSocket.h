#pragma once

#include "ostype.h"
#include "internal.h"

namespace three_year
{
	class CBaseSocket
	{
	public:
		CBaseSocket();

		virtual ~CBaseSocket();

	public:
		void SetSendBufSize(uint32_t send_size);
		void SetRecvBufSize(uint32_t recv_size);

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

	public:
		META_FUNCTION_H(std::string, remote_ip)
		META_FUNCTION_H(uint16_t, remote_port)
		META_FUNCTION_H(std::string, local_ip)
		META_FUNCTION_H(uint16_t, local_port)
		META_FUNCTION_H(uint8_t, state)
		META_FUNCTION_H(SOCKET, socket)
	};
}

