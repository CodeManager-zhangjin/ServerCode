#pragma once

#include "UtilityInterface.h"
#include "ElemSet_Http.h"
#include "singleton.h"

class CHttp : public IHttp, public INetConnectionSink, public ITimerSink
{
public:
	CHttp(IHttpSink* pSink);
	~CHttp();
	
	// IHttp
	int SetHttp(const char* pHost);
	int SetHttp(const char* pHost, WORD wPort);
	int SetHttp(DWORD dwHost, WORD wPort);
	int HttpRequest(CSTRING& request);
	
	// INetConnectionSink
	int OnConnect		(int nReason, INetConnection* pCon);
	int OnDisconnect	(int nReason, INetConnection* pCon);
	int OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSend			(INetConnection* pCon) {return 0;}
	int OnCommand		(PUCHAR pData, int nLen, INetConnection*pCon) {return 0;}
	int OnPeerIPChange	(DWORD dwPeerAddr, WORD wPort, INetConnection* pCon) {return 0;}

	// ITimerSink
	void OnTimer		(TimerReason_e eReason, ITimerSink* pSink);

private:
	bool ConnectPeer();
	DWORD m_dwHost;
	WORD m_wPort;
	INetConnection* m_pCon;
	IHttpSink*		m_pHttpSink;
	CSTRING			m_httpStr;
	int m_nStep;
	int m_nTTL;
};

class CHttpMgr : public CElemSetHttp<CHttp>
{
	DECLARE_SINGLETON(CHttpMgr)
public:
	CHttpMgr(){}
	~CHttpMgr(){}
};
