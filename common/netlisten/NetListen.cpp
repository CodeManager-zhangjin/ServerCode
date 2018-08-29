#include "NetListen.h"
#include "Log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


const int INTERVAL_CHECK_TMPCON_TTL	= 60;
const DWORD TIMEOUT_TMPCON = 5;

IMPLEMENT_SINGLETON(CNetListen)
CNetListen::CNetListen()
{
	m_pAcceptorUDP = NULL;
	m_pAcceptorTCP = NULL;
	m_pAcceptorRawTCP = NULL;

	m_u64TickCount = 0;
}

CNetListen::~CNetListen()
{
	Stop();
}

bool CNetListen::Init(DWORD dwWorkIP, WORD wWorkPort, bool bRelay /* = false */)
{
	if (bRelay) m_pAcceptorTCP = CreateTcpMediaAcceptor( this );
	else m_pAcceptorTCP = CreateTcpAcceptor( this );
	if (m_pAcceptorTCP)
	{
		WORD wTempPort = m_pAcceptorTCP->StartListen(wWorkPort, dwWorkIP);
		if (wTempPort != wWorkPort) { Stop(); return false; }
	}

	m_pAcceptorUDP = CreateUdpMediaAcceptor( this );
	if (m_pAcceptorUDP)
	{
		WORD wTempPort = m_pAcceptorUDP->StartListen(wWorkPort, dwWorkIP);
		if (wTempPort != wWorkPort) { Stop(); return false; }
	}

	AddTimer(TIMER_NORMAL, INTERVAL_CHECK_TMPCON_TTL, this);
	return true;
}

bool CNetListen::InitRawTcp( DWORD dwWorkIP, WORD wWorkPort )
{
	m_pAcceptorRawTCP = CreateRawTcpAcceptor( this );
	if (m_pAcceptorRawTCP)
	{
		WORD wTempPort = m_pAcceptorRawTCP->StartListen(wWorkPort, dwWorkIP);
		if (wTempPort != wWorkPort) { Stop(); return false; }
	}
	return true;
}

void CNetListen::Stop()
{
	MAP_TMPCON::iterator iter = m_mapTmpCon.begin();
	for (; m_mapTmpCon.end() != iter; ++iter) DestroyConnection(iter->first);

	if ( m_pAcceptorTCP ) 
	{
		m_pAcceptorTCP->StopListen();
		NetworkDestroyAcceptor( m_pAcceptorTCP );
		m_pAcceptorTCP = NULL;
	}
	if ( m_pAcceptorUDP ) 
	{
		m_pAcceptorUDP->StopListen();
		NetworkDestroyAcceptor( m_pAcceptorUDP );
		m_pAcceptorUDP = NULL;
	}
	if ( m_pAcceptorRawTCP ) 
	{
		m_pAcceptorRawTCP->StopListen();
		NetworkDestroyAcceptor( m_pAcceptorRawTCP );
		m_pAcceptorRawTCP = NULL;
	}

	DelTimer(TIMER_NORMAL, this);
}

void CNetListen::RegisterNetListen( BYTE bGroupCode, IConDispatcherSink* pSink )
{
	LOG_DEBUG(LOG_NETLISTEN, "RegisterNetListen bGroupCode:0x%02x pSink:%p\n", bGroupCode, pSink);
	MAP_DISPATCHER::iterator iter = m_mapDispatcher.find(bGroupCode);
	if (iter != m_mapDispatcher.end())
	{
		iter->second.insert(pSink);
	}
	else
	{
		IConDispatcherSink_SET tSet; tSet.insert(pSink);
		m_mapDispatcher.insert(std::make_pair(bGroupCode, tSet));
	}
}

void CNetListen::UnRegisterNetListen( BYTE bGroupCode, IConDispatcherSink* pSink )
{
	LOG_DEBUG(LOG_NETLISTEN, "UnRegisterNetListen bGroupCode:0x%02x pSink:%p\n", bGroupCode, pSink);
	MAP_DISPATCHER::iterator iter = m_mapDispatcher.find(bGroupCode);
	if (iter != m_mapDispatcher.end())
	{
		iter->second.erase(pSink);
	}
}

int CNetListen::OnDisconnect( int nReason, INetConnection *pCon )
{
	MAP_TMPCON::iterator iter = m_mapTmpCon.find(pCon);	
	if (m_mapTmpCon.end() != iter)
	{
		DestroyConnection(pCon, false); m_mapTmpCon.erase(iter);
	}
	return 0;
}

int CNetListen::OnCommand( unsigned char *pData, int nLen, INetConnection *pCon )
{
 	LOG_DEBUG(LOG_NETLISTEN, "OnCommand pCon:%p nLen:%d\n", pCon, nLen);
	if (NULL == pData) return -1;
	if (nLen <= 0) return -1;
	if (NULL == pCon) return -1;

	BYTE ucGroupCode = pData[0];
	MAP_DISPATCHER::iterator iter = m_mapDispatcher.find(ucGroupCode);	
	if (m_mapDispatcher.end() == iter) {
		LOG_DEBUG(LOG_NETLISTEN, "not find1\n");
		return 0;
	}

	MAP_TMPCON::iterator pos = m_mapTmpCon.find(pCon);
	if (pos == m_mapTmpCon.end()) {
		LOG_DEBUG(LOG_NETLISTEN, "not find2\n");
		return -1;
	}
	m_mapTmpCon.erase(pos);

	IConDispatcherSink_SET::iterator iterSink = iter->second.begin();
	for (; iterSink != iter->second.end(); iterSink++)
	{
		IConDispatcherSink* pSink = *iterSink;
		if (pSink) pSink->OnDispatchConnection(pCon, pos->second.nNetType, pData, nLen);
	}
	return 0;
}

int CNetListen::OnReceive( unsigned char *pData, int nLen, INetConnection *pCon )
{
	return OnCommand(pData, nLen, pCon);
}

int CNetListen::OnConnectIndication( INetConnection* pCon, INetAcceptor* pApt )
{
	if (NULL == pCon) return -1;
	if (NULL == pApt) return -1;
	
	MAP_TMPCON::iterator iter = m_mapTmpCon.find(pCon);
	if (m_mapTmpCon.end() != iter) { DestroyConnection(pCon); return 0; }

	struct sockaddr_in* psinPeer = NULL;
	pCon->GetOpt( NETWORK_TRANSPORT_OPT_GET_PEER_ADDR, &psinPeer );
	if (NULL == psinPeer) return -1;
	WORD wPublicPort = ntohs( psinPeer->sin_port );

	const char* pConType = NULL;
	TmpConInfo_t tInfo; memset(&tInfo, 0, sizeof(TmpConInfo_t));
	if (pApt == m_pAcceptorTCP)
	{
		pConType = "TCP";
		tInfo.nNetType = NETWORK_CONNECT_TYPE_TCP;
	}
	else if (pApt == m_pAcceptorUDP)
	{
		pConType = "UDP";
		tInfo.nNetType = NETWORK_CONNECT_TYPE_UDP;
	}

	LOG_DETAIL(LOG_NETLISTEN, "New connection(0x%08x, type: %s) coming, addr: %s, port: %d\n",
		pCon, pConType, inet_ntoa(psinPeer->sin_addr), wPublicPort);

	m_mapTmpCon.insert(std::make_pair(pCon, tInfo));
	pCon->SetSink(this);
	return 0;
}

void CNetListen::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	MAP_TMPCON::iterator iter = m_mapTmpCon.begin(), iterTemp;
	while (m_mapTmpCon.end() != iter)
	{
		iterTemp = iter; ++iterTemp;
		if (++(iter->second.dwTick) > TIMEOUT_TMPCON)
		{
			INetConnection* pCon = iter->first;
			DestroyConnection(pCon);
			m_mapTmpCon.erase(iter);
		}
		iter = iterTemp;
	}
}

void CNetListen::DestroyConnection( INetConnection* pCon, bool bDisConnect /*= true*/ )
{
	if (NULL == pCon) return;
	try
	{
		if (bDisConnect) pCon->Disconnect();
		NetworkDestroyConnection(pCon);
	}
	catch (...)
	{
		LOG_ERR(LOG_NETLISTEN, "NetworkDestroyConnection(0x%08x) exception\n", pCon);
	}
}
