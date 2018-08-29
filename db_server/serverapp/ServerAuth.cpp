#include "ServerAuth.h"
#include "getbuffer.h"
#include "putbuffer.h"
#include "Protocol.h"
#include "ServerApp.h"
#include "Log.h"

const DWORD TIMER_INTERVAL = 60;

BYTE CServerAuth::m_szBuffer[MAX_PACKET_LEN] = {0};
CServerAuth::CServerAuth()
{
	m_bServerType = 0;
	m_pCon = NULL;
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
}

void CServerAuth::SetNetConnection( INetConnection *pCon )
{
	LOG_DEBUG(LOG_DB_SERVER, "CServerAuth::%s\n", __FUNCTION__);
	if (NULL == pCon) return;
	m_pCon = pCon;
	pCon->SetSink(this);
}

void CServerAuth::SetPacketHeader( PacketHeader_t& tHeader )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s 1\n", __FUNCTION__);
	memcpy(&m_tHeader, &tHeader, sizeof(PacketHeader_t));
	SendChallengeRep();
	LOG_DEBUG(LOG_DB_SERVER, "%s 2\n", __FUNCTION__);
}

int CServerAuth::SendChallengeRep()
{
	LOG_DEBUG(LOG_DB_SERVER, "%s \n", __FUNCTION__);

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
	LOG_DEBUG(LOG_DB_SERVER, "CServerAuth::%s\n", __FUNCTION__);
	m_bServerType = bServerType;
	memcpy(m_szSerialNO, pSN, LENGTH_SERIALNO);
	memcpy(m_szUserName, pUserName, LENGTH_NAME);
	memcpy(m_szDigist, pDigist, LENGTH_CHALLENGE);
	LOG_DEBUG(LOG_DB_SERVER, "%s %d %s %s\n",__FUNCTION__, bServerType, m_szSerialNO, m_szUserName);

	CAppServerMgr::Instance()->ServerAuth((DWORD)this, pSN);
	// 接下来数据库查询完成从OnQuery_ServerInfo回调回来
}

int CServerAuth::OnDisconnect( int nReason, INetConnection *pCon )
{
	CServerAuthMgr::Instance()->DelElem(this);
	return 0;
}

int CServerAuth::OnCommand( unsigned char *pData, int nLen, INetConnection *pCon )
{
//	LOG_DEBUG(LOG_DB_SERVER, "CServerAuth::%s\n", __FUNCTION__);
	if ( false == ParsePacketHeader(pData, nLen, m_tHeader) ) return -1;
	if (m_tHeader.commandid != CMD_SERVER_AUTH)
	{
		LOG_DEBUG(LOG_DB_SERVER, "CServerAuth::OnCommand: Wrong CommmandID 0x%04x\n", m_tHeader.commandid);
		return -1;
	}

	int nNeedLen = PACKET_HEADER_SIZE + LENGTH_SERIALNO;
	if (nNeedLen > nLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "1 CServerAuth::OnCommand: Wrong PacketLen %d NeedLen %d\n", nLen, nNeedLen);
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

	BYTE bServerType = SERVER_TYPE_D;
//	LOG_DEBUG(LOG_DB_SERVER, "%s  SERVER_TYPE_D = %d\n",__FUNCTION__, SERVER_TYPE_D);
//	LOG_DEBUG(LOG_DB_SERVER, "%s szSerialNO = %s, bServerType = %d\n",__FUNCTION__, szSerialNO, bServerType);

	     if (m_tHeader.groupcode == GROUPCODE_DB_LOGIN)		   bServerType = SERVER_TYPE_LOGIN;
	else if (m_tHeader.groupcode == GROUPCODE_DB_D)            bServerType = SERVER_TYPE_D;
	else if (m_tHeader.groupcode == GROUPCODE_DB_STATUS)	   bServerType = SERVER_TYPE_STATUS;
	else if (m_tHeader.groupcode == GROUPCODE_DB_NOTIFICATION) bServerType = SERVER_TYPE_NOTIFICATION;
	else if (m_tHeader.groupcode == GROUPCODE_DB_NB)		   bServerType = SERVER_TYPE_NB;
	else if (m_tHeader.groupcode == GROUPCODE_SDB_SB)		   bServerType = SERVER_TYPE_STORAGE;
	else {
		LOG_DEBUG(LOG_DB_SERVER, "Not Find\n");
		return 0;
	}

	LOG_DEBUG(LOG_DB_SERVER, "%s szSerialNO = %s, bServerType = %d\n",__FUNCTION__, szSerialNO, bServerType);
	SetAuthInfo(bServerType, (PUCHAR)szSerialNO, (PUCHAR)szUserName, (PUCHAR)szDigist);
	return 0;
}

void CServerAuth::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	CServerAuthMgr::Instance()->DelElem(this);
}

int CServerAuth::OnQuery_ServerInfo( LIST_SERVERINFO& listInfo )
{
	LOG_ASSERT_RET(LOG_DB_SERVER, m_pCon, -1);
	WORD wError = ERROR_INVALID_SERIALNO;
	bool bExist = false;
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	for (; iter != listInfo.end(); iter++)
	{
//		LOG_DEBUG(LOG_DB_SERVER, "%s SType:%d, SN:%s, UName:%s\n", __FUNCTION__, iter->bServerType, iter->szSerialNO, iter->szUserName);
//		LOG_DEBUG(LOG_DB_SERVER, "%s mType:%d, mSN:%s, mName:%s\n", __FUNCTION__, m_bServerType, m_szSerialNO, m_szUserName);
		if (m_bServerType != iter->bServerType) continue;
		if ( memcmp(iter->szSerialNO, m_szSerialNO, LENGTH_SERIALNO) ) continue;
		bExist = true; break;
	}

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
		
// 			BYTE szHexDigist[2*LENGTH_CHALLENGE+1] = {0};
// 			BYTE szHexDigist2[2*LENGTH_CHALLENGE+1] = {0};
// 			Ascii2HexStr((char*)szHexDigist, (char*)m_szDigist, LENGTH_CHALLENGE);
// 			Ascii2HexStr((char*)szHexDigist2, (char*)szDigist, LENGTH_CHALLENGE);
// 			LOG_DEBUG(LOG_DB_SERVER, "SN:%s(%s)\n", m_szSerialNO, iter->szSerialNO);
// 			LOG_DEBUG(LOG_DB_SERVER, "UserName:%s(%s)\n", m_szUserName, iter->szUserName);
// 			LOG_DEBUG(LOG_DB_SERVER, "Challenge:%s\n", m_szChallenge);
// 			LOG_DEBUG(LOG_DB_SERVER, "Digist:%s(%s)\n", szHexDigist, szHexDigist2);
		}
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
	if (m_pCon) m_pCon->SendCommand((PUCHAR)buffer, buffer.GetFilledSize());
	LOG_DEBUG(LOG_DB_SERVER, "%s Error = %d\n", __FUNCTION__, wError);
	if (wError == ERROR_NO)
	{
		CAppServerMgr::Instance()->AddServer(*iter, m_pCon);
		m_pCon = NULL;
	}
	return 0;
}

IMPLEMENT_SINGLETON(CServerAuthMgr)
CServerAuthMgr::CServerAuthMgr()
{
}

CServerAuthMgr::~CServerAuthMgr()
{
#if defined(DBSERVER_DB)
	UnRegisterNetListen(GROUPCODE_DB_LOGIN, this);
	UnRegisterNetListen(GROUPCODE_DB_D, this);
	UnRegisterNetListen(GROUPCODE_DB_STATUS, this);
	UnRegisterNetListen(GROUPCODE_DB_NOTIFICATION, this);
	UnRegisterNetListen(GROUPCODE_DB_NB, this);
#endif
#if defined(DBSERVER_SB)
//	UnRegisterNetListen(GROUPCODE_D_SB, this);
	UnRegisterNetListen(GROUPCODE_DB_D, this);
#endif
#if defined(DBSERVER_SDB)
	UnRegisterNetListen(GROUPCODE_SDB_SB, this);
	UnRegisterNetListen(GROUPCODE_DB_D, this);
#endif
}

bool CServerAuthMgr::Start()
{
#if defined(DBSERVER_DB)
	RegisterNetListen(GROUPCODE_DB_LOGIN, this);
	RegisterNetListen(GROUPCODE_DB_D, this);
	RegisterNetListen(GROUPCODE_DB_STATUS, this);
	RegisterNetListen(GROUPCODE_DB_NOTIFICATION, this);
	RegisterNetListen(GROUPCODE_DB_NB, this);
#endif
#if defined(DBSERVER_SB)
//	RegisterNetListen(GROUPCODE_D_SB, this);
	RegisterNetListen(GROUPCODE_DB_D, this);
#endif
#if defined(DBSERVER_SDB)
	RegisterNetListen(GROUPCODE_SDB_SB, this);
	RegisterNetListen(GROUPCODE_DB_D, this);
#endif
	return true;
}

int CServerAuthMgr::OnDispatchConnection( INetConnection* pCon, int nNetType, PUCHAR pData, int nLen )
{
	if (NULL == pCon) return -1;
	LOG_DEBUG(LOG_DB_SERVER, "OnDispatchConnection pCon:%p pData:%p nLen:%d\n", pCon, pData, nLen);
	PacketHeader_t tHeader;
	if ( false == ParsePacketHeader(pData, nLen, tHeader) ) return DestroyConnection(pCon);

	if (tHeader.commandid != CMD_GET_CHALLENGE)
	{
		LOG_DEBUG(LOG_DB_SERVER, "OnDispatchConnection: Wrong CommmandID 0x%04x\n", tHeader.commandid);
		return DestroyConnection(pCon);
	}
	int nNeedLen = PACKET_HEADER_SIZE;
	if (nNeedLen > nLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "OnDispatchConnection: Wrong PacketLen %d NeedLen %d\n", nLen, nNeedLen);
		return DestroyConnection(pCon);
	}

	CServerAuth* p = NULL;
	try
	{
		p = new CServerAuth();
	}
	catch(std::bad_alloc &memExp)
	{
		LOG_DEBUG(LOG_DB_SERVER, "new CServerAuth failed\n"); return DestroyConnection(pCon);
	}

	p->SetNetConnection(pCon);

	LOG_DEBUG(LOG_DB_SERVER, "%s SourceID = %d\n", __FUNCTION__, tHeader.sourceid);
	p->SetPacketHeader(tHeader);

	AddElem(p);
	return 0;
}

int CServerAuthMgr::DestroyConnection(INetConnection* pCon)
{
	LOG_DEBUG(LOG_DB_SERVER, "DestroyConnection %p\n", pCon);
	if (pCon) { pCon->Disconnect(); NetworkDestroyConnection(pCon); }
	return 0;
}

bool CServerAuthMgr::IsAlive(CServerAuth* pElem)
{
	ELEM_SET::iterator iter = m_setElement.begin();
	for (; iter != m_setElement.end(); iter++) {
		if (pElem == *iter) {
			return true;
		}
	}
	return false;
}
