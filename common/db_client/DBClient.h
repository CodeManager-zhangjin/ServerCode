#ifndef DB_CLIENT_H
#define DB_CLIENT_H
#include "DBClientHandle.h"
#include "UtilityInterface.h"

class CDBClient : public CDBClientHandle, public INetConnectionSink, public ITimerSink
{
public:
	CDBClient();
	virtual ~CDBClient();

	bool ClientInit(DWORD dwIP, WORD wPort, BYTE bGroupCode, PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword);
	void ClientFini();

	//////////////////////////////////////////////////////////////////////////
	int  OnConnect		(int nReason, INetConnection* pCon);
	int  OnDisconnect	(int nReason, INetConnection* pCon);
	int  OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon);
	int  OnSend			(INetConnection* pCon) { return 0; }
	int  OnCommand		(PUCHAR pData, int nLen, INetConnection* pCon);
	int  OnPeerIPChange	(DWORD dwPeerAddr, WORD wPort, INetConnection* pCon) { return 0; }

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

private:
	bool Connect		();
	DWORD			m_dwIP;
	WORD			m_wPort;
};



#endif
