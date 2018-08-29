#include "ServerAuth.h"
#include "getbuffer.h"
#include "putbuffer.h"
#include "Protocol.h"
#include "ServerApp.h"
#include "Log.h"

#define AUTH_DEBUG

const DWORD TIMER_INTERVAL = 60;

BYTE CServerAuth::m_szBuffer[MAX_PACKET_LEN] = {0};
CServerAuth::CServerAuth()
{
	m_bServerType = 0;
	m_pCon = NULL;
	m_pDataBase = NULL;
	memset(m_szSerialNO, 0, LENGTH_SERIALNO+1);
	memset(m_szUserName, 0, LENGTH_NAME+1);
	memset(m_szChallenge, 0, LENGTH_CHALLENGE+1);
	memset(m_szDigist, 0, LENGTH_CHALLENGE+1);
	memset(&m_tHeader, 0, sizeof(PacketHeader_t));
	AddTimer(TIMER_NORMAL, TIMER_INTERVAL, this);
}

CServerAuth::~CServerAuth()
{
	DelTimer(TIMER_NORMAL, this);
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	if (m_pDataBase) { UnRegisterDataBase(m_pDataBase); m_pDataBase = NULL; }
}

void CServerAuth::SetNetConnection( INetConnection *pCon )
{
	if (NULL == pCon) return;
	m_pCon = pCon;
	pCon->SetSink(this);
}

void CServerAuth::SetPacketHeader( PacketHeader_t& tHeader )
{
	memcpy(&m_tHeader, &tHeader, sizeof(PacketHeader_t));
	SendChallengeRep();
}

int CServerAuth::SendChallengeRep()
{
	CPutBuffer buffer( m_szBuffer, MAX_PACKET_LEN );
	// Header
	buffer << (BYTE)m_tHeader.groupcode << (WORD)CMD_GET_CHALLENGE_REP/*Command ID*/ << (BYTE)m_tHeader.reserved0/*Reserved0*/
		<< (WORD)m_tHeader.version/*Version*/ << (WORD)m_tHeader.reserved1/*Reserved1*/
		<< (DWORD)m_tHeader.destinationid/*Source ID*/
		<< (DWORD)m_tHeader.sourceid/*Destination ID*/
		<< (DWORD)m_tHeader.commandflag/*Command Flag*/
		<< (WORD)0x0001/*Total Segment*/ << (WORD)0x0001/*Sub Segment*/
		<< (WORD)0/*Segment Flag*/ << (WORD)m_tHeader.reserved2/*Reserved2*/
		<< (DWORD)m_tHeader.reserved3/*Reserved3*/;
	// Payload
	buffer << (WORD)0/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;
	GenerateChallenge((PUCHAR)m_szChallenge);
	buffer << CByteArrayBuffer((PUCHAR)m_szChallenge, LENGTH_CHALLENGE);
	if (m_pCon) m_pCon->SendCommand((PUCHAR)buffer, buffer.GetFilledSize());
	return 0;
}

void CServerAuth::SetAuthInfo( BYTE bServerType, PUCHAR pSN, PUCHAR pUserName, PUCHAR pDigist )
{
	m_bServerType = bServerType;
	memcpy(m_szSerialNO, pSN, LENGTH_SERIALNO);
	memcpy(m_szUserName, pUserName, LENGTH_NAME);
	memcpy(m_szDigist, pDigist, LENGTH_CHALLENGE);
	//LOG_DEBUG(LOG_MAIN, "CServerAuth::SetAuthInfo: m_bServerType 0x%02x m_szSerialNO %s m_szUserName %s\n", bServerType, m_szSerialNO, m_szUserName);
	if (NULL == m_pDataBase) m_pDataBase = RegisterDataBase(this);
	LOG_ASSERT_RETVOID(LOG_MAIN, m_pDataBase);
	BYTE szServerType[2] = {0};
	szServerType[0] = bServerType;
	m_pDataBase->GetServerInfo((PUCHAR)szServerType);
}

int CServerAuth::OnDisconnect( int nReason, INetConnection *pCon )
{
	CServerAuthMgr::Instance()->DelElem(this);
	return 0;
}

int CServerAuth::OnCommand( unsigned char *pData, int nLen, INetConnection *pCon )
{
	if ( false == ParsePacketHeader(pData, nLen, m_tHeader) ) return -1;
	if (m_tHeader.commandid != CMD_SERVER_AUTH)
	{
		LOG_DEBUG(LOG_MAIN, "CServerAuth::OnCommand: Wrong CommmandID 0x%04x\n", m_tHeader.commandid);
		return -1;
	}

	int nNeedLen = PACKET_HEADER_SIZE + LENGTH_SERIALNO;
	if (nNeedLen > nLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 CServerAuth::OnCommand: Wrong PacketLen %d NeedLen %d\n", nLen, nNeedLen);
		return -1;
	}

	CGetBuffer bufferGet(pData, nLen);
	bufferGet.Skip(PACKET_HEADER_SIZE);
	BYTE szSerialNO[LENGTH_SERIALNO+1] = {0};
	bufferGet >> CByteArrayBuffer(szSerialNO, LENGTH_SERIALNO);
	BYTE szUserName[LENGTH_NAME+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	nNeedLen += LENGTH_CHALLENGE;
	if (nNeedLen > nLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "2 CServerAuth::OnCommand: Wrong PacketLen %d NeedLen %d\n", nLen, nNeedLen);
		return -1;
	}
	BYTE szDigist[LENGTH_CHALLENGE+1] = {0};
	bufferGet >> CByteArrayBuffer(szDigist, LENGTH_CHALLENGE);
	BYTE cSvrType = 0;
#if defined(SERVERAPP_LOGIN)
	if		(m_tHeader.groupcode == GROUPCODE_D_LOGIN)			   cSvrType = SERVER_TYPE_D;
	else if (m_tHeader.groupcode == GROUPCODE_NOTIFICATION_LOGIN)  cSvrType = SERVER_TYPE_NOTIFICATION;
#elif defined(SERVERAPP_D)
	if      (m_tHeader.groupcode == GROUPCODE_D_LOGIN)			   cSvrType = SERVER_TYPE_LOGIN;
	else if (m_tHeader.groupcode == GROUPCODE_D_NB)				   cSvrType = SERVER_TYPE_NB;
	else if (m_tHeader.groupcode == GROUPCODE_D_STATUS)			   cSvrType = SERVER_TYPE_STATUS;
	else if (m_tHeader.groupcode == GROUPCODE_NOTIFICATION_D)	   cSvrType = SERVER_TYPE_NOTIFICATION;
	else if (m_tHeader.groupcode == GROUPCODE_D_SB)				   cSvrType = SERVER_TYPE_STORAGE;
#elif defined(SERVERAPP_NOTIFY)
	if		(m_tHeader.groupcode == GROUPCODE_NOTIFICATION_STATUS) cSvrType = SERVER_TYPE_STATUS;
	else if (m_tHeader.groupcode == GROUPCODE_NOTIFICATION_LOGIN)  cSvrType = SERVER_TYPE_LOGIN;
	else if (m_tHeader.groupcode == GROUPCODE_NOTIFICATION_D)	   cSvrType = SERVER_TYPE_D;
#elif defined(SERVERAPP_STATUS)
	if		(m_tHeader.groupcode == GROUPCODE_D_STATUS)			   cSvrType = SERVER_TYPE_D;
	else if (m_tHeader.groupcode == GROUPCODE_NOTIFICATION_STATUS) cSvrType = SERVER_TYPE_NOTIFICATION;
#elif defined(SERVERAPP_NB)
	if		(m_tHeader.groupcode == GROUPCODE_D_NB)				   cSvrType = SERVER_TYPE_D;
#elif defined(SERVERAPP_STORAGE_BUSINESS)
	if		(m_tHeader.groupcode == GROUPCODE_D_SB)				   cSvrType = SERVER_TYPE_D;
#endif

	SetAuthInfo(cSvrType, (PUCHAR)szSerialNO, (PUCHAR)szUserName, (PUCHAR)szDigist);
	return 0;
}

void CServerAuth::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	CServerAuthMgr::Instance()->DelElem(this);
}

int CServerAuth::OnGetServerInfo( LIST_SERVERINFO& listInfo )
{
	LOG_ASSERT_RET(LOG_MAIN, m_pCon, -1);
	WORD wError = ERROR_INVALID_SERIALNO;
	bool bExist = false;
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	for (; iter != listInfo.end(); iter++)
	{
		if (m_bServerType != iter->bServerType)
		{
#ifdef AUTH_DEBUG
			LOG_DEBUG(LOG_MAIN, "m_bServerType %d iter->bServerType %d\n", m_bServerType, iter->bServerType);
#endif
			continue;
		}
		if ( memcmp(iter->szSerialNO, m_szSerialNO, LENGTH_SERIALNO) )
		{
#ifdef AUTH_DEBUG
			LOG_DEBUG(LOG_MAIN, "m_szSerialNO %s iter->szSerialNO %s\n", m_szSerialNO, iter->szSerialNO);
			continue;
#endif
		}
		if ( memcmp(iter->szUserName, m_szUserName, LENGTH_NAME) )
		{
#ifdef AUTH_DEBUG
			LOG_DEBUG(LOG_MAIN, "m_szUserName %s iter->szUserName %s\n", m_szUserName, iter->szUserName);
#endif
			continue;
		}
		LOG_DEBUG(LOG_MAIN, "Server is Exist\n");
		bExist = true; break;
	}

#if defined(SERVERAPP_D)
	bool bNBServer = false;
#endif
	if (bExist && (iter != listInfo.end()))
	{
		if ( memcmp(iter->szUserName, m_szUserName, LENGTH_NAME) )
		{
			wError = ERROR_INVALID_USERNAME;
		}
		else
		{
			// Digist = md5(UserName +md5(Password,16) + Challenge, 16)
			BYTE szDigist[LENGTH_CHALLENGE+1] = {0};
			CalcAuthDigistA(szDigist, iter->szUserName, iter->szPassword, m_szChallenge);
			if (0 == memcmp(szDigist, m_szDigist, LENGTH_CHALLENGE)) wError = ERROR_NO;
			else wError = ERROR_INVALID_PASSWORD;
		}

//#if defined(SERVERAPP_D)
//		if (iter->bServerType == SERVER_TYPE_NB) bNBServer = true;
//#endif
	}

	CPutBuffer buffer( m_szBuffer, MAX_PACKET_LEN );
	// Header
	buffer << (BYTE)m_tHeader.groupcode << (WORD)CMD_AUTH_REP/*Command ID*/ << (BYTE)m_tHeader.reserved0/*Reserved0*/
			<< (WORD)m_tHeader.version/*Version*/ << (WORD)m_tHeader.reserved1/*Reserved1*/
			<< (DWORD)m_tHeader.destinationid/*Source ID*/
			<< (DWORD)m_tHeader.sourceid/*Destination ID*/
			<< (DWORD)m_tHeader.commandflag/*Command Flag*/
			<< (WORD)0x0001/*Total Segment*/ << (WORD)0x0001/*Sub Segment*/
			<< (WORD)0/*Segment Flag*/ << (WORD)m_tHeader.reserved2/*Reserved2*/
			<< (DWORD)m_tHeader.reserved3/*Reserved3*/;
	// Payload
	buffer << (WORD)wError/*Error Flag*/ << (WORD)0/*Reserved0*/
			<< (DWORD)0/*Checksum Type && Checksum Value*/
			<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;

//#if defined(SERVERAPP_D)
//	if (bNBServer)
//	{
//		struct sockaddr_in* psinPeer = NULL;
//		m_pCon->GetOpt( NETWORK_TRANSPORT_OPT_GET_PEER_ADDR, &psinPeer );
//		LOG_ASSERT_RET(LOG_MAIN, psinPeer, -1);
//		DWORD dwPublicIP = ntohl( psinPeer->sin_addr.s_addr );
//		WORD wPublicPort = ntohs( psinPeer->sin_port );
//		buffer << dwPublicIP << wPublicPort;
//	}
//#endif
	if (m_pCon) m_pCon->SendCommand((PUCHAR)buffer, buffer.GetFilledSize());

	if (wError == ERROR_NO)
	{
		CAppServerMgr::Instance()->SA_AddServer(*iter, m_pCon);
		m_pCon = NULL;
	}
	return 0;
}

int CServerAuth::OnDataBaseError( int nResult )
{
	return 0;
}

IMPLEMENT_SINGLETON(CServerAuthMgr)
CServerAuthMgr::CServerAuthMgr()
{
	memset(m_szSerialNO, 0, LENGTH_SERIALNO+1);
	memset(m_szUserName, 0, LENGTH_NAME+1);
	memset(m_szPassword, 0, LENGTH_PASSWORD+1);
}

CServerAuthMgr::~CServerAuthMgr()
{
#if defined(SERVERAPP_LOGIN)
	UnRegisterNetListen(GROUPCODE_D_LOGIN, this);
	UnRegisterNetListen(GROUPCODE_NOTIFICATION_LOGIN, this);
#elif defined(SERVERAPP_D)
	UnRegisterNetListen(GROUPCODE_D_LOGIN, this);
	UnRegisterNetListen(GROUPCODE_NOTIFICATION_D, this);
	UnRegisterNetListen(GROUPCODE_D_STATUS, this);
	UnRegisterNetListen(GROUPCODE_D_NB, this);
	UnRegisterNetListen(GROUPCODE_D_SB, this);
#elif defined(SERVERAPP_STATUS)
	UnRegisterNetListen(GROUPCODE_D_STATUS, this);
	UnRegisterNetListen(GROUPCODE_NOTIFICATION_STATUS, this);
#elif defined(SERVERAPP_NOTIFY)
	UnRegisterNetListen(GROUPCODE_NOTIFICATION_D, this);
	UnRegisterNetListen(GROUPCODE_NOTIFICATION_LOGIN, this);
	UnRegisterNetListen(GROUPCODE_NOTIFICATION_STATUS, this);
#elif defined(SERVERAPP_NB)
	UnRegisterNetListen(GROUPCODE_D_NB, this);
#elif defined(SERVERAPP_STORAGE_BUSINESS)
	UnRegisterNetListen(GROUPCODE_D_SB, this);
#endif
}

bool CServerAuthMgr::Start(PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword)
{
	if ((pSN == NULL) || (pUserName == NULL) || (pPassword == NULL)) return false;
	//LOG_DEBUG(LOG_MAIN, "CServerAuthMgr::Start SN:%s UserName:%s Password:%s\n", pSN, pUserName, pPassword);
	memcpy(m_szSerialNO, pSN, LENGTH_SERIALNO);
	memcpy(m_szUserName, pUserName, LENGTH_NAME);
	memcpy(m_szPassword, pPassword, LENGTH_PASSWORD);
	
#if defined(SERVERAPP_LOGIN)
	RegisterNetListen(GROUPCODE_D_LOGIN, this);
	RegisterNetListen(GROUPCODE_NOTIFICATION_LOGIN, this);
#elif defined(SERVERAPP_D)
	RegisterNetListen(GROUPCODE_D_LOGIN, this);
	RegisterNetListen(GROUPCODE_NOTIFICATION_D, this);
	RegisterNetListen(GROUPCODE_D_STATUS, this);
	RegisterNetListen(GROUPCODE_D_NB, this);
	RegisterNetListen(GROUPCODE_D_SB, this);
#elif defined(SERVERAPP_STATUS)
	RegisterNetListen(GROUPCODE_D_STATUS, this);
	RegisterNetListen(GROUPCODE_NOTIFICATION_STATUS, this);
#elif defined(SERVERAPP_NOTIFY)
	RegisterNetListen(GROUPCODE_NOTIFICATION_D, this);
	RegisterNetListen(GROUPCODE_NOTIFICATION_LOGIN, this);
	RegisterNetListen(GROUPCODE_NOTIFICATION_STATUS, this);
#elif defined(SERVERAPP_NB)
	RegisterNetListen(GROUPCODE_D_NB, this);
#elif defined(SERVERAPP_STORAGE_BUSINESS)
	RegisterNetListen(GROUPCODE_D_SB, this);
#endif
	return true;
}

int CServerAuthMgr::OnDispatchConnection( INetConnection* pCon, int nNetType, PUCHAR pData, int nLen )
{
	if (NULL == pCon) return -1;

	PacketHeader_t tHeader;
	if ( false == ParsePacketHeader(pData, nLen, tHeader) ) return DestroyConnection(pCon);
	if (tHeader.commandid != CMD_GET_CHALLENGE)
	{
		LOG_DEBUG(LOG_MAIN, "OnDispatchConnection: Wrong CommmandID 0x%04x\n", tHeader.commandid);
		return DestroyConnection(pCon);
	}

	int nNeedLen = PACKET_HEADER_SIZE;
	if (nNeedLen > nLen)
	{
		LOG_DEBUG(LOG_MAIN, "OnDispatchConnection: Wrong PacketLen %d NeedLen %d\n", nLen, nNeedLen);
		return DestroyConnection(pCon);
	}

	CServerAuth* p = NULL;
	try
	{
		p = new CServerAuth();
	}
	catch(std::bad_alloc &memExp)
	{
		LOG_DEBUG(LOG_MAIN, "new CServerAuth failed\n"); return DestroyConnection(pCon);
	}
	p->SetNetConnection(pCon);
	p->SetPacketHeader(tHeader);
	AddElem(p);
	return 0;
}

int CServerAuthMgr::DestroyConnection(INetConnection* pCon)
{
	if (pCon) { pCon->Disconnect(); NetworkDestroyConnection(pCon); }
	return 0;
}
