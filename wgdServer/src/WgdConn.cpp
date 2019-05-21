#include "WgdConn.h"
#include <stdio.h>
#include "BaseSocket.h"
#include "WgdServer.h"
#include "CommonDef.h"
#include "CommonNet.h"

namespace three_year
{
	CWGDConn::CWGDConn(CWgdServer* pNotify, socketPtr socket) :m_pServer(pNotify), base_socket_(socket)
	{}

	CWGDConn::~CWGDConn()
	{
		int a = 1;
	}

	void CWGDConn::OnClose()
	{
		net_handle_t fd = base_socket_->GetSocket();
		base_socket_->Close();
		m_pServer->RemoveWgdConn(fd);
	}

	int CWGDConn::OnRead()
	{
		u_long avail = 0;
		int nRet = ioctlsocket(base_socket_->GetSocket(), FIONREAD, &avail);
		if ((nRet == SOCKET_ERROR) || (avail == 0))
		{
			OnClose();
			return -1;
		}

		//printf("OnRead : \n");

		char szRcvBuf[8192] = { 0 };

		int READ_BUF_SIZE = 8192;

		for (;;)
		{
			uint32_t free_buf_len = m_in_buf.GetAllocSize() - m_in_buf.GetWriteOffset();
			if (free_buf_len < READ_BUF_SIZE)
				m_in_buf.Extend(READ_BUF_SIZE);

			int nRet = base_socket_->Recv(m_in_buf.GetBuffer() + m_in_buf.GetWriteOffset(), READ_BUF_SIZE);
			//printf("receice : %d\n", nRet);
			if (SOCKET_ERROR == nRet)
			{
				if ((WSAGetLastError() == WSAEINTR))
					continue;
				else
					break;
			}

			m_in_buf.IncWriteOffset(nRet);
		}

		//这里一定要循环把所有的消息都处理掉 因为一次接受的数据可能包含了多个消息结构体
		while (ParseBuffer(m_in_buf) != -1) {}
		return 0;
	}

	int32_t CWGDConn::ParseBuffer(CSimpleBuffer& simpleBuffer)
	{
		uint32_t uintHave = simpleBuffer.GetWriteOffset();
		int32_t nHeadSize = sizeof(WGDHEAD);

		if (uintHave < nHeadSize)
		{
			return -1;
		}

		uchar_t* pBuffer = simpleBuffer.GetBuffer();
		WGDHEAD* pHead = (WGDHEAD*)pBuffer;
		int32_t nTotalSize = nHeadSize + pHead->nDataLen;
		if (uintHave < nTotalSize)
		{
			return -1;
		}

		return DealBuffer(simpleBuffer);
	}

	int32_t CWGDConn::DealBuffer(CSimpleBuffer & simpleBuffer)
	{
		uchar_t* pBuffer = simpleBuffer.GetBuffer();
		WGDHEAD* pHead = (WGDHEAD*)pBuffer;
		int32_t nTotalSize = sizeof(WGDHEAD) + pHead->nDataLen;

		m_pServer->OnReceivedNotify(base_socket_->GetSocket(), pBuffer, nTotalSize);

		simpleBuffer.Read(NULL, nTotalSize);

		return 0;
	}

	void CWGDConn::OnWrite()
	{
		int error = 0;
		socklen_t len = sizeof(error);
		getsockopt(base_socket_->GetSocket(), SOL_SOCKET, SO_ERROR, (char*)&error, &len);
		if (error)
		{
			OnClose();
			return;
		}

		while (m_out_buf.GetWriteOffset() > 0)
		{
			int send_size = m_out_buf.GetWriteOffset();
			if (send_size > NETLIB_MAX_SOCKET_BUF_SIZE)
			{
				send_size = NETLIB_MAX_SOCKET_BUF_SIZE;
			}

			int ret = base_socket_->Send(m_out_buf.GetBuffer(), send_size);
			if (ret <= 0)
			{
				ret = 0;
				break;
			}

			m_out_buf.Read(NULL, ret);
		}
	}

	int CWGDConn::Send(void* data, int len)
	{
		int offset = 0;
		int remain = len;

		while (remain > 0)
		{
			int send_size = remain;
			if (send_size > 8192)
				send_size = 8192;

			int nSend = base_socket_->Send((char*)data + offset, send_size);
			if (nSend <= 0)
			{
				if ((WSAGetLastError() == WSAEINPROGRESS) || (WSAGetLastError() == WSAEWOULDBLOCK))
				{
					continue;
				}

				break;
			}

			offset += nSend;
			remain -= nSend;
		}

		return 0;
	}
}

