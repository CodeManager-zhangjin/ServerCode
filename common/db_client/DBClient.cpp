#include "DBClient.h"
#include "putbuffer.h"
#include "Log.h"

const int DATABASE_STATUS_CONNECTED = 0;
const int DATABASE_STATUS_DISCONNECT = 2;

CDBClient::CDBClient()
{
	m_dwIP = 0;
	m_wPort = 0;
}

CDBClient::~CDBClient()
{
	ClientFini();
}

bool CDBClient::ClientInit(DWORD dwIP, WORD wPort, BYTE bGroupCode, PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword)
{
	m_dwIP = dwIP; m_wPort = wPort;

	m_bGroupCode = bGroupCode;

	memcpy(m_szSerialNO, pSN, LENGTH_SERIALNO);
	memcpy(m_szUserName, pUserName, LENGTH_NAME);
	memcpy(m_szPassword, pPassword, LENGTH_PASSWORD);

	AddTimer(TIMER_NORMAL, 30, this);

	return Connect();
}

void CDBClient::ClientFini()
{
	if (m_pCon) { m_pCon->Disconnect(); NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	DelTimer(TIMER_NORMAL, this);
}

int CDBClient::OnConnect( int nReason, INetConnection* pCon )
{
	if (m_pCon != pCon) return -1;
	LOG_DEBUG(LOG_DB_CLIENT, "OnConnect nReason %d\n", nReason);
	if (nReason && m_pCon)
	{
		NetworkDestroyConnection(m_pCon); m_pCon = NULL; return -1;
	}
	GetChallenge();

	if(m_pSdbSink)   m_pSdbSink->OnStorageDBStatus(DATABASE_STATUS_CONNECTED);
	else if(m_pSink) m_pSink->OnDataBaseStatus(DATABASE_STATUS_CONNECTED);
	else 	if(m_pSbSink)	m_pSbSink->OnStorageBusinessStatus(DATABASE_STATUS_CONNECTED);
}

int CDBClient::OnDisconnect( int nReason, INetConnection* pCon )
{
	if (m_pCon != pCon) return -1;
	LOG_DEBUG(LOG_DB_CLIENT, "OnDisconnect nReason %d\n", nReason);
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }

	if(m_pSdbSink)   m_pSdbSink->OnStorageDBStatus(DATABASE_STATUS_DISCONNECT);
	else if(m_pSink) m_pSink->OnDataBaseStatus(DATABASE_STATUS_DISCONNECT);
	else if(m_pSbSink) m_pSbSink->OnStorageBusinessStatus(DATABASE_STATUS_DISCONNECT);
}

int CDBClient::OnCommand( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_DB_CLIENT, "CDBClient::OnCommand pCon %p nLen %d\n", pCon, nLen);
	if (m_pCon != pCon) return -1;
	if (NULL == pData) return -1;
	if (nLen <= 0) return -1;
	try {
		ProcessCommand(pData, nLen);
	}
	catch (CParserException& e) {
		e.Descript();
	}
	catch (...) {
		LOG_ERR(LOG_DB_CLIENT, "other exception occured\n");
	}
	return 0;
}

int CDBClient::OnReceive(PUCHAR pData, int nLen, INetConnection* pCon)
{
	return OnCommand(pData, nLen, pCon);
}

bool CDBClient::Connect()
{
	if (m_pCon) { m_pCon->Disconnect(); NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	m_pCon = CreateTcpCon(this);
	LOG_ASSERT_RET(LOG_MAIN, m_pCon, false);

	m_pCon->Connect(m_dwIP, m_wPort, NETWORK_CONNECT_TYPE_TCP);
	return true;
}

void CDBClient::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	//LOG_DEBUG(LOG_DB_CLIENT, "CDBClient::OnTimer m_pCon %p\n", m_pCon);
	if (m_pCon) return;
	Connect();
}
