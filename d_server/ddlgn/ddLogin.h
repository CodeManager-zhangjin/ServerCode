#pragma once

#include "ddlgnInterface.h"
#include "ElemSetDDLgn.h"
#include "singleton.h"
#include "putbuffer.h"
#include "UtilityInterface.h"
#include "NetListenInterface.h"
#include "Thread.h"

class CDDLogin : public INetConnectionSink, public ITimerSink
{
public:
	CDDLogin();
	~CDDLogin();

	int ConnectLgn		(DWORD dwIP, WORD wPort);

	// INetConnectionSink
	int OnConnect		(int nReason, INetConnection* pCon);
	int OnDisconnect	(int nReason, INetConnection* pCon)					;
	int OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon)		;
	int OnSend			(INetConnection* pCon)								{ return 0; }
	int OnCommand		(PUCHAR pData, int nLen, INetConnection* pCon)		;
	int OnPeerIPChange	(DWORD dwPeerAddr, WORD wPort, INetConnection* pCon){ return 0; }

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

private:
	int GetDServerInfo(PUCHAR pSN);

	int OnGetDServerInfo(PUCHAR pData, int nLen, INetConnection* pCon);

	int SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError = 0, WORD wTotalSeg = 1, WORD wSubSeg = 1);

	int UdpConnect();
	int TcpConnect();
	
	INetConnection*	m_pCon;
	PacketHeader_t	m_tHeader;
	int				m_nStep;
	DWORD m_dwIP;
	WORD m_wPort;

	typedef int (CDDLogin::*LgnServerHandler)(PUCHAR, int, INetConnection*);
	struct HandleEntry_t
	{
		WORD wCommand;
		LgnServerHandler pHandler;
	} ;
	static const HandleEntry_t m_Handles[];
	static BYTE m_szTempData[MAX_PACKET_LEN];
};

#define DEFINE_PUTBUFFER(bufPut) \
	CPutBuffer bufPut(m_szTempData, MAX_PACKET_LEN);\
	bufPut.Skip(PACKET_HEADER_SIZE);

// 1 定时连接咚咚登录服务器进行检查
// 2 接受注册服务器信息相关指令
class CDDLoginMgr : public CElemSetDDLgn<CDDLogin>, public ITimerSink, public IConDispatcherSink, public CThread
{
	DECLARE_SINGLETON(CDDLoginMgr)
public:
	CDDLoginMgr();
	~CDDLoginMgr(){}

	bool Start(PUCHAR pSN);
	void Stop();

	void SetSerialNo(PUCHAR pSN);
	PUCHAR GetSerialNo() { return (PUCHAR)m_szDServerSN; }

	void SetSink(IDDLoginSink* pSink) { m_pLoginSink = pSink; }
	IDDLoginSink* GetSink() { return m_pLoginSink; }

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

	// IConDispatcherSink
	int OnDispatchConnection(INetConnection* pCon, int nNetType, PUCHAR pData, int nLen);

	// CThread
	void ThreadLoop();

private:
	int CalcRandomInterval();

private:
	IDDLoginSink* m_pLoginSink;
	BYTE m_szDServerSN[LENGTH_SERIALNO+1];

	DWORD m_dwDD121IP;
};
