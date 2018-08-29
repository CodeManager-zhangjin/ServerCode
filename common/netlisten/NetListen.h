#pragma once

#include "NetListenInterface.h"
#include "UtilityInterface.h"
#include "singleton.h"
#include <set>
#include <map>

class CNetListen : public INetConnectionSink, public INetAcceptorSink, public ITimerSink
{
	DECLARE_SINGLETON(CNetListen)
public:
	CNetListen();
	~CNetListen();

	bool Init(DWORD dwWorkIP, WORD wWorkPort, bool bRelay = false);
	bool InitRawTcp(DWORD dwWorkIP, WORD wWorkPort);

	void RegisterNetListen(BYTE bGroupCode, IConDispatcherSink* pSink);
	void UnRegisterNetListen(BYTE bGroupCode, IConDispatcherSink* pSink);

	// INetConnectionSink interface
	int OnConnect(int nReason, INetConnection *pCon) { return 0; }
	int OnDisconnect(int nReason, INetConnection *pCon);
	int OnCommand(unsigned char *pData, int nLen, INetConnection *pCon);
	int OnReceive(unsigned char *pData, int nLen, INetConnection *pCon);
	int OnSend(INetConnection *pCon) { return 0; }
	int OnPeerIPChange( unsigned long dwPeerAddr, unsigned short wPort, INetConnection *pCon ) { return 0; }
	
	// INetAcceptorSink interface
	int OnConnectIndication(INetConnection* pCon, INetAcceptor* pApt);

	// ITimerSink interface
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

private:
	void Stop();
	void DestroyConnection(INetConnection* pCon, bool bDisConnect = true);

	INetAcceptor*	m_pAcceptorUDP;
	INetAcceptor*	m_pAcceptorTCP;
	INetAcceptor*	m_pAcceptorRawTCP;

	UInt64			m_u64TickCount;

	typedef std::set<IConDispatcherSink*>			IConDispatcherSink_SET;
	typedef std::map<BYTE, IConDispatcherSink_SET>	MAP_DISPATCHER;
	MAP_DISPATCHER m_mapDispatcher;

	struct TmpConInfo_t
	{
		DWORD dwTick;
		int nNetType;
	};
	typedef std::map<INetConnection*, TmpConInfo_t>	MAP_TMPCON;
	MAP_TMPCON m_mapTmpCon;
};
