#include <stdio.h>

#include "WgdConn.h"
#include "BaseSocket.h"
#include "WgdServer.h"
#include "CommonDef.h"
#include "CommonNet.h"
#include "SimpleBuffer.h"

namespace three_year
{
	CWGDConn::CWGDConn(wgdSvrPtr wgd_svr, socketPtr socket) :wgd_svr_(wgd_svr), base_socket_(socket)
	{
		in_buf_ = std::make_shared<CSimpleBuffer>();
		out_buf_ = std::make_shared<CSimpleBuffer>();
	}

	CWGDConn::~CWGDConn()
	{}

	void CWGDConn::OnClose()
	{
		net_handle_t fd = base_socket_->get_socket();
		base_socket_->Close();
		wgd_svr_->RemoveWgdConn(fd);
	}

	int CWGDConn::OnRead()
	{
		u_long avail = 0;
		int nRet = ioctlsocket(base_socket_->get_socket(), FIONREAD, &avail);
		if ((nRet == SOCKET_ERROR) || (avail == 0))
		{
			OnClose();
			return -1;
		}

		//printf("OnRead : \n");
		char szRcvBuf[MAX_SEND_RCV_LEN] = { 0 };
		while (true)
		{
			uint32_t free_buf_len = in_buf_->GetAllocSize() - in_buf_->GetWriteOffset();
			if (free_buf_len < MAX_SEND_RCV_LEN)
				in_buf_->Extend(MAX_SEND_RCV_LEN);

			int nRet = base_socket_->Recv(in_buf_->GetBuffer() + in_buf_->GetWriteOffset(), MAX_SEND_RCV_LEN);
			if (SOCKET_ERROR == nRet)
			{
				if ((WSAGetLastError() == WSAEINTR))
					continue;
				else
					break;
			}

			in_buf_->IncWriteOffset(nRet);
		}

		while (ParseBuffer(in_buf_) != -1) 
		{}
		
		return 0;
	}

	int32_t CWGDConn::ParseBuffer(bufferPtr buff)
	{
		uint32_t uintHave = buff->GetWriteOffset();
		int32_t nHeadSize = sizeof(WGDHEAD);
		if (uintHave < nHeadSize)
			return -1;

		uchar_t* pBuffer = buff->GetBuffer();
		WGDHEAD* pHead = (WGDHEAD*)pBuffer;
		int32_t nTotalSize = nHeadSize + pHead->nDataLen;
		if (uintHave < nTotalSize)
			return -1;

		return DealBuffer(buff);
	}

	int32_t CWGDConn::DealBuffer(bufferPtr buff)
	{
		uchar_t* pBuffer = buff->GetBuffer();
		WGDHEAD* pHead = (WGDHEAD*)pBuffer;
		int32_t nTotalSize = sizeof(WGDHEAD) + pHead->nDataLen;

		wgd_svr_->OnReceivedNotify(base_socket_->get_socket(), pBuffer, nTotalSize);

		buff->Read(NULL, nTotalSize);
		return 0;
	}

	void CWGDConn::OnWrite()
	{
		int error = 0;
		socklen_t len = sizeof(error);
		getsockopt(base_socket_->get_socket(), SOL_SOCKET, SO_ERROR, (char*)&error, &len);
		if (error)
		{
			OnClose();
			return;
		}

		while (out_buf_->GetWriteOffset() > 0)
		{
			int send_size = out_buf_->GetWriteOffset();
			if (send_size > NETLIB_MAX_SOCKET_BUF_SIZE)
				send_size = NETLIB_MAX_SOCKET_BUF_SIZE;

			int ret = base_socket_->Send(out_buf_->GetBuffer(), send_size);
			if (ret <= 0)
			{
				ret = 0;
				break;
			}

			out_buf_->Read(NULL, ret);
		}
	}

	int CWGDConn::Send(void* data, int len)
	{
		int offset = 0;
		int remain = len;

		while (remain > 0)
		{
			int send_size = remain;
			if (send_size > MAX_SEND_RCV_LEN)
				send_size = MAX_SEND_RCV_LEN;

			int nSend = base_socket_->Send((char*)data + offset, send_size);
			if (nSend <= 0)
			{
				if ((WSAGetLastError() == WSAEINPROGRESS) || (WSAGetLastError() == WSAEWOULDBLOCK))
					continue;
				break;
			}

			offset += nSend;
			remain -= nSend;
		}

		return 0;
	}
}

