#include "Protocol.h"
#include "Client.h"
#include "NetListenInterface.h"
#include "ServerAppInterface.h"
#include "UtilityDataStruct.h"
#include "getbuffer.h"
#include "Log.h"
#include <algorithm>

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_SINGLETON(SmsLimit)

SmsLimit::SmsLimit()
{
	LOG_DEBUG(LOG_MAIN,"SmsLimit::%s\n",__FUNCTION__);
	AddTimer(TIMER_SMS_LIMIT, 60, this);
	LOG_DEBUG(LOG_MAIN, "CClient::%s Timer 60s \n", __FUNCTION__);
}
SmsLimit::~SmsLimit()
{
	LOG_DEBUG(LOG_MAIN,"SmsLimit::%s\n",__FUNCTION__);
	DelTimer(TIMER_SMS_LIMIT, this);
	}
/********************************************************************************
 *功能：同一个手机号码一个小时内只能执行10次
 ********************************************************************************/
void  SmsLimit::OnTimer(TimerReason_e eReason, ITimerSink* pSink)
{
	LOG_DEBUG(LOG_MAIN," CClient::%s\n",__FUNCTION__);
	MAP_SMS_LIMIT::iterator iter = m_mapSmsLimit.begin();
	while(iter != m_mapSmsLimit.end())
	{
		LOG_DEBUG(LOG_MAIN,"SMS Phone = %s, Count = %d, TTL = %d\n",iter->first.c_str(), iter->second.dwCount, iter->second.dwTTL);
		if(iter->second.dwTTL >= 60)
//		if(iter->second.dwTTL >= 5)
		{
			m_mapSmsLimit.erase(iter++);
		}
		else
		{
			iter->second.dwTTL = iter->second.dwTTL + 1;
			iter++;
		}
	}
}
SmsLimit* GetSmsLimitHandle()
{
	return SmsLimit::Instance();
}
//////////////////////////////////////////////////////////////////////////
DWORD g_dwCameraID = 0;

BYTE CClient::m_szBuffer[MAX_PACKET_LEN] = {0};
const CClient::HandlerEntry CClient::mHandlers[] =
{
	{ CMD_GET_REGISTER_INFO,	&CClient::OnGetRegisterInfo		},
	{ CMD_QUERY_USER,			&CClient::OnQueryUser			},
	{ CMD_SMS,					&CClient::OnSmsAuth				},
	{ CMD_SET_SECRET,			&CClient::OnSetSecret			},
	{ CMD_GET_DSERVER_INFO,		&CClient::OnGetDServerInfo		},
};

CClient::CClient(DWORD dwClientID)
{
	m_pCon = NULL;
	m_dwClientID = dwClientID;
	m_pDataBase = NULL;
	
	LOG_DEBUG(LOG_MAIN, "this %p dwClientID %d\n", this, dwClientID);
}

CClient::~CClient()
{
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	if (m_pDataBase) { UnRegisterDataBase(m_pDataBase); m_pDataBase = NULL; }
}

void CClient::SetNetConnection( INetConnection* pCon )
{
	if (NULL == pCon) return;
	if (m_pCon == pCon) return;
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	m_pCon = pCon; m_pCon->SetSink(this);
}

int CClient::OnDisconnect( int nReason, INetConnection* pCon )
{
	if (pCon != m_pCon) return -1;
	CClientMgr::Instance()->DelElem(m_dwClientID);
	return 0;
}

int CClient::OnReceive( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return OnCommand(pData, nLen, pCon);
}

int CClient::OnCommand( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return ProcessCommand(pData, nLen);
}

int CClient::OnQueryMobilePhone(PUCHAR pMobilePhone, int nReason)
{
	LOG_DEBUG(LOG_MAIN, "CClient::OnQueryMobilePhone pMobilePhone %s nReason %d\n", pMobilePhone, nReason);
	DECLARE_PUTBUFFER( buffer )
	int nMobilePhoneLen = (BYTE)strlen((const char*)pMobilePhone);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nMobilePhoneLen <= LENGTH_MOBILEPHONE, -1);
	BYTE szBase64[64] = {0};
	int nBase64Len = Base64EncVal((char*)szBase64, (const char*)pMobilePhone, nMobilePhoneLen);
	buffer << (BYTE)nBase64Len;
	buffer << CByteArrayBuffer(szBase64, nBase64Len);
	return SendPacket(buffer, CMD_QUERY_USER_REP, (WORD)nReason);
}

int CClient::OnSetSecret(int nReason)
{
	DECLARE_PUTBUFFER( buffer )
	return SendPacket(buffer, CMD_SET_SECRET_REP, (WORD)nReason);
}

int CClient::OnGetDServerInfo( DServerInfo_t& tInfo )
{
	DECLARE_PUTBUFFER( buffer )
	buffer << CByteArrayBuffer((PUCHAR)tInfo.szSerialNO, LENGTH_SERIALNO);
	buffer << tInfo.nPermission << tInfo.nCapacity;
	return SendPacket(buffer, CMD_GET_DSERVER_INFO_REP, ERROR_NO);
}

int CClient::ProcessCommand( PUCHAR pData, int nLen )
{
	if ( false == ParsePacketHeader(pData, nLen, m_tHeader) ) return -1;
	if (m_tHeader.groupcode == GROUPCODE_LOGIN_CLIENT)
	{
		static int g_clientHandlersCount = sizeof( mHandlers) / sizeof( mHandlers[0] );
		for ( int i = 0; i < g_clientHandlersCount; ++i )
		{
			if ( mHandlers[i].bCommand != m_tHeader.commandid ) continue;
			PMFHANDLER pHandler = mHandlers[i].pmfHandler;
			return (this->*pHandler)(pData+PACKET_HEADER_SIZE, nLen-PACKET_HEADER_SIZE);
		}
	}
	return -1;
}

// 获取重定向地址
int CClient::OnGetRegisterInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = 3*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwCameraID = 0, dwVendorID = 0, dwConfigureIndex = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwCameraID >> dwVendorID >> dwConfigureIndex;

	LOG_DEBUG(LOG_MAIN, "dwCameraID %d dwVendorID %d dwConfigureIndex %d\n", dwCameraID, dwVendorID, dwConfigureIndex);
	// get dservers from serverapp
	if (dwCameraID) dwVendorID = CClientMgr::Instance()->CalcVendorID(dwCameraID);
	else dwVendorID = CalcRealVendorID(dwVendorID);

	LIST_SERVERINFO listInfo;
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->SA_GetDServers(dwVendorID, dwConfigureIndex, listInfo);
	return SendCmd_RegisterInfo(dwVendorID, dwConfigureIndex, listInfo);
}

int CClient::OnGetDServerInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = LENGTH_SERIALNO;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	BYTE szSN[LENGTH_SERIALNO+1] = {0};
	bufferGet >> CByteArrayBuffer(szSN, LENGTH_SERIALNO);

	LOG_DEBUG(LOG_MAIN, "DServer SN %s\n", szSN);

	if (NULL == m_pDataBase) m_pDataBase = RegisterDataBase(this);
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	return m_pDataBase->GetDServerInfo((PUCHAR)szSN);
}

int CClient::OnQueryUser( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	if (NULL == m_pDataBase) m_pDataBase = RegisterDataBase(this);
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);

	CGetBuffer bufferGet(pData, nLen);
	int nNeedLen = 0;
	BYTE szMobilePhone[LENGTH_MOBILEPHONE+1] = {0};
	if (false == GetBase64Str(bufferGet, (PUCHAR)szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;
	if (-1 == m_pDataBase->QueryMobilePhone((PUCHAR)szMobilePhone))
	{
		return OnQueryMobilePhone((PUCHAR)szMobilePhone, (int)ERROR_SYSTEM);
	}
	return 0;
}

int CClient::OnSmsAuth( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	CGetBuffer bufferGet(pData, nLen);
	int nNeedLen = 0;
	BYTE szMessage[LENGTH_MSGCONTENT+1] = {0};
	if (false == GetBase64Str(bufferGet, (PUCHAR)szMessage, LENGTH_MSGCONTENT, nLen, nNeedLen)) return -1;
	nNeedLen += sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	UINT nCount = 0;
	bufferGet >> nCount;

	LIST_SMSINFO listInfo;
	SmsInfo_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += sizeof(DWORD) + sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		memset(&tInfo, 0, sizeof(SmsInfo_t));
		bufferGet >> tInfo.dwVendorID >> tInfo.bLanguage;
		if (false == GetBase64Str(bufferGet, (PUCHAR)tInfo.szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;

		LOG_DEBUG(LOG_MAIN, "dwVendorID:%d bLanguage:%d MobilePhone %s\n", tInfo.dwVendorID, tInfo.bLanguage, tInfo.szMobilePhone);
		
		//////////////////////////////////////////////////////////////////////////
		//新增
		LOG_DEBUG(LOG_MAIN,"%s =====================================================\n",__FUNCTION__);
		MAP_SMS_LIMIT::iterator iter = GetSmsLimitHandle()->m_mapSmsLimit.find((char*)tInfo.szMobilePhone);
		LOG_DEBUG(LOG_MAIN,"%s m_mapSmsLimit.size = %d\n", __FUNCTION__, GetSmsLimitHandle()->m_mapSmsLimit.size());
		if(iter != GetSmsLimitHandle()->m_mapSmsLimit.end())
		{
			DWORD dwPhoneTimes = iter->second.dwCount;
			if(dwPhoneTimes >= 10)//一小时内可以发送短信次数
//			if(dwPhoneTimes >= 1)//一小时内可以发送短信次数
			{
				iter->second.dwCount = iter->second.dwCount + 1;
				LOG_WARN(LOG_MAIN,"SMS too frequent, szMobilePhone = %s, dwPhoneTimes = %d\n", tInfo.szMobilePhone, iter->second.dwCount);
				DECLARE_PUTBUFFER( buffer )
				return SendPacket(buffer, CMD_SMS_REP, (WORD)ERROR_USEUP);
			} else {
//				LOG_DEBUG(LOG_MAIN,"SMS %d\n",abs(time(NULL) - iter->second.dwTimestamp));
				if( abs(time(NULL) - iter->second.dwTimestamp) < 60 )
				{
					LOG_DEBUG(LOG_MAIN,"SMS Repeat the request verification code\n");
					return -1;
				}
				iter->second.dwTimestamp = time(NULL);
				iter->second.dwCount = iter->second.dwCount + 1;
				LOG_DEBUG(LOG_MAIN,"SMS szMobilePhone = %s  Count = %d TTL = %d\n",tInfo.szMobilePhone, iter->second.dwCount, iter->second.dwTTL);
			}
		} else {
			LOG_DEBUG(LOG_MAIN,"SMS new szMobilePhone = %s\n", tInfo.szMobilePhone);
			SmsLimit_t smsLimit;
			smsLimit.dwCount = 1;
			smsLimit.dwTTL = 1;
			smsLimit.dwTimestamp = time(NULL);
			GetSmsLimitHandle()->m_mapSmsLimit.insert(std::make_pair((char*)tInfo.szMobilePhone, smsLimit));
		}
		LOG_DEBUG(LOG_MAIN,"%s =====================================================\n",__FUNCTION__);
		/////////////////////////////////////////////////////////////////////////
		PUCHAR p = (PUCHAR)strstr((const char*)tInfo.szMobilePhone, "@");
		if (p == NULL) // 短信
		{
			if (false == IsValidMobilePhone((const char*)tInfo.szMobilePhone)) continue;
		}
		listInfo.push_back(tInfo);
	}
	//////////////////////////////////////////////////////////////////////////
	LOG_DEBUG(LOG_MAIN,"%s szMessage:%s ClientID:%d\n",__FUNCTION__, szMessage, m_dwClientID);
	// notification
	if (listInfo.size())
	{
		IServerAppHandle* pHandle = ServerApp_GetHandle();
		LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
		if (false == pHandle->SA_Sms_Notification(szMessage, listInfo, m_dwClientID))
		{
			DECLARE_PUTBUFFER( buffer )
			return SendPacket(buffer, CMD_SMS_REP, ERROR_NOTIFYSERVER_OFFLINE);
		}
		return 0;
	}

	DECLARE_PUTBUFFER( buffer )
	return SendPacket(buffer, CMD_SMS_REP, (WORD)ERROR_INVALID_MOBILEPHONE);
}

int CClient::OnSetSecret( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	if (NULL == m_pDataBase) m_pDataBase = RegisterDataBase(this);
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);

	int nNeedLen = sizeof(DWORD) + sizeof(BYTE);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwVendorID = 0;
	BYTE szUserName[LENGTH_NAME+1] = {0};
	BYTE bLanguage = 0;
	bufferGet >> dwVendorID >> bLanguage;
	if (false == GetVariableStr(bufferGet, (PUCHAR)szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	nNeedLen += LENGTH_PASSWORD;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	BYTE szPassword[LENGTH_PASSWORD+1] = {0};
	bufferGet >> CByteArrayBuffer(szPassword, LENGTH_PASSWORD);
	
	BYTE szMobilePhone[LENGTH_MOBILEPHONE+1] = {0};
	if (false == GetBase64Str(bufferGet, (PUCHAR)szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;
	if(-1 == m_pDataBase->SetSecret(dwVendorID, bLanguage, (PUCHAR)szUserName, (PUCHAR)szPassword, (PUCHAR)szMobilePhone))
	{
		return OnSetSecret((int)ERROR_SYSTEM);
	}
	return 0;
}

int CClient::SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg /* = 1 */, WORD wSubSeg /* = 1 */)
{
	LOG_ASSERT_RET(LOG_DB_SERVER, m_pCon, -1);
	int nLen = buffer.GetFilledSize();
	buffer.SetOffset(0);
	// Header
	buffer << (BYTE)m_tHeader.groupcode << (WORD)wCommand/*Command ID*/ << (BYTE)m_tHeader.reserved0/*Reserved0*/
		<< (WORD)m_tHeader.version/*Version*/ << (WORD)m_tHeader.reserved1/*Reserved1*/
		<< (DWORD)m_dwClientID/*Source ID*/
		<< (DWORD)m_tHeader.sourceid/*Destination ID*/
		<< (DWORD)m_tHeader.commandflag/*Command Flag*/
		<< (WORD)wTotalSeg/*Total Segment*/ << (WORD)wSubSeg/*Sub Segment*/
		<< (WORD)0/*Segment Flag*/ << (WORD)m_tHeader.reserved2/*Reserved2*/
		<< (DWORD)m_tHeader.reserved3/*Reserved3*/;
	// Payload
	buffer << (WORD)wError/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;
	buffer.SetOffset(nLen);
	LOG_DEBUG(LOG_MAIN, "pCon %p SendData cmd:0x%04x err:0x%04x len:%d\n", m_pCon, wCommand, wError, nLen);
	return m_pCon->SendCommand((PUCHAR)buffer, nLen);
}

int CClient::SendCmd_RegisterInfo( DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo )
{
	struct sockaddr_in* psinPeer = NULL;
	m_pCon->GetOpt( NETWORK_TRANSPORT_OPT_GET_PEER_ADDR, &psinPeer );
	if (NULL == psinPeer) return -1;
	DWORD dwIP = ntohl(psinPeer->sin_addr.s_addr); // 设备/客户端IP

	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = listInfo.size();
	LOG_DEBUG(LOG_MAIN, "SendCmd_RegisterInfo dwCount %d\n", dwCount);
	if (dwCount == 0)
	{
		buffer << dwVendorID << dwConfigureIndex << dwIP << dwCount;
		return SendPacket(buffer, CMD_GET_REGISTER_INFO_REP, ERROR_NO);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_RegisterInfo(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwVendorID << dwConfigureIndex << dwIP << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << iter->dwServerID << iter->dwIP << iter->nNetID;
			PutVariableStr(buffer, (PUCHAR)iter->szPosition);
			iter++;
		}
		SendPacket(buffer, CMD_GET_REGISTER_INFO_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

void CClient::OnSmsRepsonse( int nReason )
{
	DECLARE_PUTBUFFER( buffer )
	SendPacket(buffer, CMD_SMS_REP, (WORD)nReason);
}

IMPLEMENT_SINGLETON(CClientMgr)
CClientMgr::CClientMgr()
{
	m_dwClientID = 0;
}

CClientMgr::~CClientMgr()
{
	UnRegisterNetListen(GROUPCODE_LOGIN_CLIENT, this);
}

bool CClientMgr::Start()
{
	ServerApp_SetSink(this);

	RegisterNetListen(GROUPCODE_LOGIN_CLIENT, this); return true;
}

int CClientMgr::OnDispatchConnection( INetConnection* pCon, int nNetType, PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CClient* p = NULL;
	try
	{
		p = new CClient(++m_dwClientID);
	}
	catch(std::bad_alloc &memExp)
	{
		LOG_ASSERT_RET(LOG_MAIN, p, -1);
	}
	AddElem(m_dwClientID, p);
	p->SetNetConnection(pCon);
	p->OnCommand(pData, nLen, pCon);
	return 0;
}

int CClientMgr::SetPieceOfSerialNO( LIST_PIECEOFSERIALNO& listInfo )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	LIST_PIECEOFSERIALNO::iterator iter = listInfo.begin();
	for (; iter != listInfo.end(); iter++)
	{
		LOG_DEBUG(LOG_MAIN, "PieceOfSerialNO begin %d, end %d, vendorid %d\n", iter->dwBegin, iter->dwEnd, iter->dwVendorID);
	}
	m_listPieceOfSerialNO.clear();
	m_listPieceOfSerialNO.insert(m_listPieceOfSerialNO.end(), listInfo.begin(), listInfo.end());
	return 0;
}

DWORD CClientMgr::CalcVendorID( DWORD dwCameraID )
{
	DWORD dwVendorID = 0;
	g_dwCameraID = dwCameraID;
	LIST_PIECEOFSERIALNO::iterator iter = std::find_if(m_listPieceOfSerialNO.begin(), m_listPieceOfSerialNO.end(), FindVendorIDByCameraID());
	if (iter != m_listPieceOfSerialNO.end())
	{
		dwVendorID = iter->dwVendorID;
	}
	LOG_DEBUG(LOG_MAIN, "%s dwCameraID %d ==> dwVendorID %d\n", __FUNCTION__, dwCameraID, dwVendorID);
	return dwVendorID;
}

void CClientMgr::SA_OnSmsRepsonse_Notify( DWORD dwClientID, int nReason )
{
	CClient* p = GetElem(dwClientID);
	LOG_DEBUG(LOG_MAIN, "%s dwClientID %d:%p, nReason %d\n", __FUNCTION__, dwClientID, p, nReason);
	if (p) p->OnSmsRepsonse(nReason);
}
