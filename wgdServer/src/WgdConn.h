#pragma once

#include "internal.h"

namespace three_year
{
	class CWGDConn
	{
	public:
		CWGDConn(wgdSvrPtr wgd_svr, socketPtr socket);

		~CWGDConn();

	public:
		virtual int  OnRead();

		virtual void OnWrite();

		virtual void OnClose();

		int Send(void* data, int len);

		int32_t ParseBuffer(bufferPtr buff);

		int32_t DealBuffer(bufferPtr buff);

		bufferPtr get_out_buff() { return out_buf_; }

	private:
		socketPtr				base_socket_;

		std::string				peer_ip_;

		uint16_t				peer_port_;

		bufferPtr				in_buf_;

		bufferPtr				out_buf_;

		wgdSvrPtr				wgd_svr_;
	};
}


