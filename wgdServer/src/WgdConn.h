#pragma once

#include "SimpleBuffer.h"
#include "WgdServer.h"

namespace three_year
{
	class CWGDConn
	{
	public:
		CWGDConn(CWgdServer* pNotify, socketPtr socket);

		~CWGDConn();

	public:
		virtual int  OnRead();

		virtual void OnWrite();

		virtual void OnClose();

		int Send(void* data, int len);

		int32_t ParseBuffer(CSimpleBuffer& simpleBuffer);

		int32_t DealBuffer(CSimpleBuffer& simpleBuffer);

	private:
		socketPtr				base_socket_;

		std::string				m_peer_ip;

		uint16_t				m_peer_port;

		CSimpleBuffer			m_in_buf;
		CSimpleBuffer			m_out_buf;

		CWgdServer*				m_pServer;
	};
}


