#include "ServerApp.h"
#include "putbuffer.h"
#include "getbuffer.h"
#include "ServerAuth.h"
#include "Log.h"

#if defined(SERVERAPP_STATUS)
#include "CacheInterface.h"
#elif defined(SERVERAPP_STORAGE_BUSINESS)
#include "qiniuInterface.h"
#include "StoreCallbackInterface.h"
#endif

const DWORD TIMER_INTERVAL = 60;
const DWORD TIMER_INTERVAL2 = 600;

const CAppServer::HandlerEntry CAppServer::mHandlers[] =
{
	{ CMD_GET_CHALLENGE_REP,		&CAppServer::OnChallenge			},
	{ CMD_AUTH_REP,					&CAppServer::OnAuth					},
	{ CMD_SMS,						&CAppServer::OnSms					},
	{ CMD_PUSH,						&CAppServer::OnPush					},
	{ CMD_SMS_REP,					&CAppServer::OnSms					},
	{ CMD_PUSH_REP,					&CAppServer::OnPush					},
	{ CMD_REPORT_DEVICE_STATUS,		&CAppServer::OnReportDeviceStatus	},
	{ CMD_REPORT_USER_STATUS,		&CAppServer::OnReportUserStatus		},
	{ CMD_GET_DEVICE_STATUS,		&CAppServer::OnGetDeviceStatus		},
	{ CMD_GET_DEVICE_STATUS_REP,	&CAppServer::OnGetDeviceStatusRep	},
	{ CMD_TRANS_CLIENTINFO,			&CAppServer::OnTransClientInfo		},
	{ CMD_TRANS_SERVERINFO,			&CAppServer::OnTransServerInfo		}, 
	{ CMD_ADD_TMP_USER,				&CAppServer::OnAddTempUser			},
	{ CMD_ADD_TMP_USER_REP,			&CAppServer::OnAddTempUser			},
	{ CMD_REPORT_NETWORK,			&CAppServer::OnReportNetwork		},
	{ CMD_SDK_TUNNEL,				&CAppServer::OnSdkTunnel			},
	{ CMD_SDK_TUNNEL_REP,			&CAppServer::OnSdkTunnelRep			},

	{ CMD_GET_UPLOAD_TOKEN,			&CAppServer::OnQiniu_GetUploadToken		},
	{ CMD_GET_DOWNLOAD_URLS,		&CAppServer::OnQiniu_GetDownloadUrls	},
	{ CMD_GET_DOWNLOAD_URLS2,		&CAppServer::OnQiniu_GetDownloadUrls2	},
	{ CMD_GET_UPLOAD_TOKEN_REP,		&CAppServer::OnQiniu_GetUploadTokenRep	},
	{ CMD_GET_DOWNLOAD_URLS_REP,	&CAppServer::OnQiniu_GetDownloadUrlsRep	},
	{ CMD_REPORT_UPLOAD_RESULT,		&CAppServer::OnQiniu_ReportUploadResult	},
	{ CMD_REPORT_ALARMSTATUS,		&CAppServer::OnRepoartAlarmStatus		},
};

CAppServer::CAppServer()
{
	memset(&m_tBaseInfo, 0, sizeof(ServerInfo_t));
	m_bGroupCode = GROUPCODE;
	m_wWorkPort = 0;
	m_bAuth = false;
	m_nTimeout = 0;
	AddTimer(TIMER_NORMAL, TIMER_INTERVAL, this);
//待删除
//#if defined(SERVERAPP_STORAGE_BUSINESS)
//	m_pDataBase = RegisterDataBase(this);
//#endif
}

CAppServer::~CAppServer()
{
	DelTimer(TIMER_NORMAL, this);
//待删除
//#if defined(SERVERAPP_STORAGE_BUSINESS)
//	if (m_pDataBase) { UnRegisterDataBase(m_pDataBase); m_pDataBase = NULL; }
//#endif
}

int CAppServer::Start(ServerInfo_t& tInfo)
{
	SetServerInfo(tInfo);

	m_pCon = CreateTcpCon(this);
	LOG_ASSERT_RET(LOG_MAIN, m_pCon, -1);
	LOG_DEBUG(LOG_MAIN, "start connecting name:%s %s:%d\n", m_tBaseInfo.szName, IpDword2Str(m_tBaseInfo.dwIP), m_wWorkPort);
	return m_pCon->Connect(m_tBaseInfo.dwIP, m_wWorkPort, NETWORK_CONNECT_TYPE_TCP);
}

void CAppServer::GetServerInfo(ServerInfo_t& tInfo)
{
	memcpy(&tInfo, &m_tBaseInfo, sizeof(ServerInfo_t));
}

void CAppServer::SetServerInfo( ServerInfo_t& tInfo )
{
	memcpy(&m_tBaseInfo, &tInfo, sizeof(ServerInfo_t));

#if defined(SERVERAPP_LOGIN)
	if (tInfo.bServerType == SERVER_TYPE_D)
	{
		m_bGroupCode = GROUPCODE_D_LOGIN; m_wWorkPort = SERVER_PORT_D;
	}
	else if (tInfo.bServerType == SERVER_TYPE_NOTIFICATION)
	{
		m_bGroupCode = GROUPCODE_NOTIFICATION_LOGIN; m_wWorkPort = SERVER_PORT_NOTIFICATION;
	}
#elif defined(SERVERAPP_D)
	if (tInfo.bServerType == SERVER_TYPE_LOGIN)
	{
		m_bGroupCode = GROUPCODE_D_LOGIN; m_wWorkPort = SERVER_PORT_LOGIN;
	}
	else if (tInfo.bServerType == SERVER_TYPE_NOTIFICATION)
	{
		m_bGroupCode = GROUPCODE_NOTIFICATION_D; m_wWorkPort = SERVER_PORT_NOTIFICATION;
	}
	else if (tInfo.bServerType == SERVER_TYPE_STATUS)
	{
		m_bGroupCode = GROUPCODE_D_STATUS; m_wWorkPort = SERVER_PORT_STATUS;
	}
	else if (tInfo.bServerType == SERVER_TYPE_NB)
	{
		m_bGroupCode = GROUPCODE_D_NB; m_wWorkPort = SERVER_PORT_NB;
	}
	else if (tInfo.bServerType == SERVER_TYPE_STORAGE)
	{
		m_bGroupCode = GROUPCODE_D_SB; m_wWorkPort = SERVER_PORT_SB;
	}
#elif defined(SERVERAPP_NOTIFY)
	if (tInfo.bServerType == SERVER_TYPE_D)
	{
		m_bGroupCode = GROUPCODE_NOTIFICATION_D; m_wWorkPort = SERVER_PORT_D;
	}
	else if (tInfo.bServerType == SERVER_TYPE_STATUS)
	{
		m_bGroupCode = GROUPCODE_NOTIFICATION_STATUS; m_wWorkPort = SERVER_PORT_STATUS;
	}
	else if (tInfo.bServerType == SERVER_TYPE_LOGIN)
	{
		m_bGroupCode = GROUPCODE_NOTIFICATION_LOGIN; m_wWorkPort = SERVER_PORT_LOGIN;
	}
#elif defined(SERVERAPP_STATUS)
	if (tInfo.bServerType == SERVER_TYPE_D)
	{
		m_bGroupCode = GROUPCODE_D_STATUS; m_wWorkPort = SERVER_PORT_D;
	}
	else if (tInfo.bServerType == SERVER_TYPE_NOTIFICATION)
	{
		m_bGroupCode = GROUPCODE_NOTIFICATION_STATUS; m_wWorkPort = SERVER_PORT_NOTIFICATION;
	}
#elif defined(SERVERAPP_NB)
	if (tInfo.bServerType == SERVER_TYPE_D)
	{
		m_bGroupCode = GROUPCODE_D_NB; m_wWorkPort = SERVER_PORT_D;
	}
#elif defined(SERVERAPP_STORAGE_BUSINESS)
	if (tInfo.bServerType == SERVER_TYPE_D)
	{
		m_bGroupCode = GROUPCODE_D_SB; m_wWorkPort = SERVER_PORT_D;
	}
	else if (tInfo.bServerType == SERVER_TYPE_NOTIFICATION)
	{
		m_bGroupCode = GROUPCODE_NOTIFICATION_D; m_wWorkPort = SERVER_PORT_NOTIFICATION;
	}
#endif

}

void CAppServer::SetNetConnection( INetConnection* pCon )
{
	if (NULL == pCon) return;
	LOG_DEBUG(LOG_MAIN, "SetNetConnection Con:%p m_pCon %p name:%s %s:%d\n", pCon, m_pCon, m_tBaseInfo.szName, IpDword2Str(m_tBaseInfo.dwIP), m_wWorkPort);
	if (m_pCon) { m_pCon->Disconnect(); NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	m_pCon = pCon; m_pCon->SetSink(this);

	//////////////////////////////////////////////////////////////////////////
	if (m_tBaseInfo.bServerType != SERVER_TYPE_NB)
	{
		struct sockaddr_in* psinPeer = NULL;
		pCon->GetOpt( NETWORK_TRANSPORT_OPT_GET_PEER_ADDR, &psinPeer );
		if (psinPeer) m_tBaseInfo.dwIP = ntohl( psinPeer->sin_addr.s_addr );
	}
	//////////////////////////////////////////////////////////////////////////

	m_bAuth = true; m_nTimeout = 0;
#if defined(SERVERAPP_D)
	if (m_tBaseInfo.bServerType == SERVER_TYPE_STATUS)
	{
		IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
		if (pSink) pSink->SA_Online_Status();
	}
#endif
}

int CAppServer::OnConnect( int nReason, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "ServerID %d OnConnect nReason %d\n", m_tBaseInfo.dwServerID, nReason);
	if (pCon != m_pCon) return -1;
	if (nReason)
	{
		if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
		return -1;
	}

	return SendCmd_GetChallenge();
}

int CAppServer::OnDisconnect( int nReason, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "ServerID %d OnDisconnect nReason %d\n", m_tBaseInfo.dwServerID, nReason);
	if (pCon != m_pCon) return -1;
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	m_bAuth = false;
#if defined(SERVERAPP_STATUS)
	// 清空设备以及客户端状态
	ICacheHandle* pCacheHandle = Cache_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pCacheHandle, -1);
	pCacheHandle->Cache_ClearStatus(m_tBaseInfo.dwServerID);
#endif
	return 0;
}

int CAppServer::OnReceive( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return OnCommand(pData, nLen, pCon);
}

int CAppServer::OnCommand( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return ProcessCommand(pData, nLen);
}

int CAppServer::OnPeerIPChange( DWORD dwPeerAddr, WORD wPort, INetConnection *pCon )
{
	return 0;
}

int CAppServer::ProcessCommand( PUCHAR pData, int nLen )
{
	if ( false == ParsePacketHeader(pData, nLen, m_tHeader) ) return -1;
	if (m_tHeader.groupcode != m_bGroupCode) return -1;
	if ((m_bAuth == false) &&
		(m_tHeader.commandid != CMD_AUTH_REP) &&
		(m_tHeader.commandid != CMD_GET_CHALLENGE_REP)
		) return -1;
	//////////////////////////////////////////////////////////////////////////
	static int g_serverHandlersCount = sizeof( mHandlers) / sizeof( mHandlers[0] );
	for ( int i = 0; i < g_serverHandlersCount; ++i )
	{
		if ( mHandlers[i].bCommand != m_tHeader.commandid ) continue;
		PMFHANDLER pHandler = mHandlers[i].pmfHandler;
		return (this->*pHandler)(pData+PACKET_HEADER_SIZE, nLen-PACKET_HEADER_SIZE);
	}
	return -1;
}

void CAppServer::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
//#if defined(SERVERAPP_NOTIFY)
//	Notify_SmsInfo_t tNotify_SmsInfo; memset(&tNotify_SmsInfo, 0, sizeof(Notify_SmsInfo_t));
//	tNotify_SmsInfo.bLanguage = 0;
//	tNotify_SmsInfo.dwVendorID = 1;
//	memcpy(tNotify_SmsInfo.szMessageContent, "123456", 6);
//	memcpy(tNotify_SmsInfo.szMobilePhone, "13675893336", 11);
//	m_pSms->SendSms(tNotify_SmsInfo, m_tHeader.reserved3);
//#endif

	if (m_pCon) return;
	if ( (m_bAuth == false) && (++m_nTimeout == 10) )
	{
		AddTimer(TIMER_NORMAL, TIMER_INTERVAL2, this);
	}

	m_pCon = CreateTcpCon(this);
	LOG_ASSERT_RETVOID(LOG_MAIN, m_pCon);
	LOG_DEBUG(LOG_MAIN, "restart connecting name:%s %s:%d\n", m_tBaseInfo.szName, IpDword2Str(m_tBaseInfo.dwIP), m_wWorkPort);
	m_pCon->Connect(m_tBaseInfo.dwIP, m_wWorkPort, NETWORK_CONNECT_TYPE_TCP);
}

int CAppServer::OnChallenge( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d ServerType %d ServerID %d Con %p\n", 
		__FUNCTION__, nLen, m_tBaseInfo.bServerType, m_tBaseInfo.dwServerID, m_pCon);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = LENGTH_CHALLENGE;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	BYTE szChallengeStr[LENGTH_CHALLENGE+1] = {0};
	bufferGet >> CByteArrayBuffer((PUCHAR)szChallengeStr, LENGTH_CHALLENGE);

	PUCHAR pSN = CServerAuthMgr::Instance()->GetSN();
	PUCHAR pUserName = CServerAuthMgr::Instance()->GetUserName();
	PUCHAR pPassword = CServerAuthMgr::Instance()->GetPassword();
	//LOG_DEBUG(LOG_MAIN, "SN:%s UserName:%s Password:%s\n", pSN, pUserName, pPassword);
	return SendCmd_Auth(pSN, pUserName, pPassword, (PUCHAR)szChallengeStr);
}

int CAppServer::OnAuth( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d ServerType %d ServerID %d Con %p error %d\n", 
		__FUNCTION__, nLen, m_tBaseInfo.bServerType, m_tBaseInfo.dwServerID, m_pCon, m_tHeader.error);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	if (0 == m_tHeader.error)
	{
		m_bAuth = true; m_nTimeout = 0;
#if defined(SERVERAPP_D)
		if (m_tBaseInfo.bServerType == SERVER_TYPE_STATUS)
		{
			IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
			if (pSink) pSink->SA_Online_Status();
		}
#endif
	}
	else if (m_pCon)
	{
		m_bAuth = false;
		m_pCon->Disconnect(); NetworkDestroyConnection(m_pCon); m_pCon = NULL;
	}
	return 0;
}

int CAppServer::OnSms( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
#if defined(SERVERAPP_NOTIFY)
	Notify_SmsInfo_t tNotify_SmsInfo; memset(&tNotify_SmsInfo, 0, sizeof(Notify_SmsInfo_t));

	CGetBuffer bufferGet(pData, nLen);
	int nNeedLen = 0;
	if (false == GetBase64Str(bufferGet, (PUCHAR)tNotify_SmsInfo.szMessageContent, LENGTH_MSGCONTENT, nLen, nNeedLen)) return -1;

	UINT nCount = 0;
	bufferGet >> nCount;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += sizeof(DWORD) + sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tNotify_SmsInfo.dwVendorID >> tNotify_SmsInfo.bLanguage;
		memset(tNotify_SmsInfo.szMobilePhone, 0, LENGTH_MOBILEPHONE);
		if (false == GetBase64Str(bufferGet, (PUCHAR)tNotify_SmsInfo.szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;

		LOG_DEBUG(LOG_MAIN, "VendorID %d Language %d SMS/Mail %s Msg %s\n",
			tNotify_SmsInfo.dwVendorID, tNotify_SmsInfo.bLanguage, tNotify_SmsInfo.szMobilePhone, tNotify_SmsInfo.szMessageContent);
		PUCHAR p = (PUCHAR)strstr((const char*)tNotify_SmsInfo.szMobilePhone, "@");
		if (p == NULL) // 短信
		{
			if (false == IsValidMobilePhone((const char*)tNotify_SmsInfo.szMobilePhone))
			{
				DECLARE_PUTBUFFER( buffer )
				SendPacket(buffer, CMD_SMS_REP, (WORD)ERROR_INVALID_MOBILEPHONE);
				continue;
			}
			Notify_Sms(tNotify_SmsInfo);
		}
		else // 邮箱
		{
			Notify_MailInfo_t tMailInfo;
			tMailInfo.bLanguage = tNotify_SmsInfo.bLanguage;
			tMailInfo.dwVendorID = tNotify_SmsInfo.dwVendorID;
			memcpy(tMailInfo.szMessageContent, &tNotify_SmsInfo.szMessageContent, LENGTH_MSGCONTENT+1);
			memcpy(tMailInfo.szMail, &tNotify_SmsInfo.szMobilePhone, LENGTH_MOBILEPHONE+1);
			Notify_Mail(tMailInfo);
		}
	}
	DECLARE_PUTBUFFER( buffer )
	return SendPacket(buffer, CMD_SMS_REP, ERROR_NO);
#elif defined(SERVERAPP_LOGIN)
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink) pSink->SA_OnSmsRepsonse_Notify(m_tHeader.reserved3, (int)m_tHeader.error);
#endif
	return 0;
}
#if defined(SERVERAPP_NOTIFY)

//int CAppServer::OnSmsResponse( ISms* pSms, int nResult, DWORD dwUserData )
//{
//	LOG_DEBUG(LOG_MAIN, "%s nResult %d dwUserData %d\n", __FUNCTION__, nResult, dwUserData);
//	DECLARE_PUTBUFFER( buffer )
//	// dwUserData为LgnServer中的ClientID
//	m_tHeader.reserved3 = dwUserData;
//	return SendPacket(buffer, CMD_SMS_REP, (WORD)nResult);
//}
//
//int CAppServer::OnPushMessageResponse( IPush* pPush, int nResult, DWORD dwUserData )
//{
//	LOG_DEBUG(LOG_MAIN, "%s nResult %d dwUserData %d\n", __FUNCTION__, nResult, dwUserData);
//	return 0;
//}

#endif
//推送4
int CAppServer::OnPush( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
#if defined(SERVERAPP_NOTIFY)
	Notify_PushInfo_t tNotify_PushInfo; memset(&tNotify_PushInfo, 0, sizeof(Notify_PushInfo_t));

	CGetBuffer bufferGet(pData, nLen);
	int nNeedLen = 0;
	if (false == GetBase64Str(bufferGet, (PUCHAR)tNotify_PushInfo.szMessageContent, LENGTH_MSGCONTENT, nLen, nNeedLen)) return -1;
	LOG_DEBUG(LOG_MAIN, "Message %s\n", tNotify_PushInfo.szMessageContent);

	UINT nCount = 0;
	DWORD dwDeviceID = 0;
	DWORD dwStatus = 0;

	bufferGet >> dwDeviceID >> dwStatus >> nCount;

	LIST_NOTIFY_PUSHINFO lstInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += sizeof(DWORD) + 2*sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_MAIN, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tNotify_PushInfo.dwVendorID >> tNotify_PushInfo.bPushType >> tNotify_PushInfo.bLanguage;
	
		memset(tNotify_PushInfo.szToken, 0, LENGTH_TOKEN);
		if (false == GetVariableStr(bufferGet, (PUCHAR)tNotify_PushInfo.szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;

		tNotify_PushInfo.dwDeviceID = dwDeviceID;
		tNotify_PushInfo.dwStatus = dwStatus;
		LOG_DEBUG(LOG_MAIN, "dwVendorID %d bPushType %d bLanguage %d szToken %s\n",
			tNotify_PushInfo.dwVendorID, tNotify_PushInfo.bPushType, tNotify_PushInfo.bLanguage, tNotify_PushInfo.szToken);
		lstInfo.push_back(tNotify_PushInfo);
	}
	Notify_Push(lstInfo);
#endif
	return 0;
}

int CAppServer::OnReportDeviceStatus( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	DWORD dwDeviceID = 0, dwStatus = 0;
	int nNeedLen = 2*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwDeviceID >> dwStatus;

	BYTE szMessage[LENGTH_MSGCONTENT+1] = {0};
	if (false == GetBase64Str(bufferGet, (PUCHAR)szMessage, LENGTH_MSGCONTENT, nLen, nNeedLen)) return -1;

	nNeedLen += 2*sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	UINT nSmsCount = 0, nPushCount = 0;
	bufferGet >> nSmsCount >> nPushCount;

	LIST_SMSINFO listSmsInfo;
	SmsInfo_t tInfo1;
	for (UINT i = 0; i < nSmsCount; i++)
	{
		nNeedLen += 2*sizeof(DWORD) + sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_MAIN, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		memset(&tInfo1, 0, sizeof(SmsInfo_t));
		bufferGet >> tInfo1.dwUserID >> tInfo1.dwVendorID >> tInfo1.bLanguage;
		if (false == GetBase64Str(bufferGet, (PUCHAR)tInfo1.szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo1.szDeviceName, LENGTH_NAME, nLen, nNeedLen)) return -1;

		listSmsInfo.push_back(tInfo1);
	}
	
	//////////////////////////////////////////////////////////////////////////
	// 推送
	LIST_PUSHINFO listPushInfo;
	PushInfo_t tInfo2;
	for (UINT j = 0; j < nPushCount; j++)
	{
		nNeedLen += 2*sizeof(DWORD) + 2*sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_MAIN, "4 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		memset(&tInfo2, 0, sizeof(PushInfo_t));
		bufferGet >> tInfo2.dwUserID >> tInfo2.dwVendorID >> tInfo2.bPushType >> tInfo2.bLanguage;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo2.szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo2.szDeviceName, LENGTH_NAME, nLen, nNeedLen)) return -1;

		// 判断该Token值的用户是否在线，如果在线，则不发送推送
		//DWORD dwServerID = pCacheHandle->Cache_GetUserServer(tInfo2.dwUserID, (PUCHAR)tInfo2.szToken);
		//if (0 == dwServerID)
		//{
			listPushInfo.push_back(tInfo2);
		//}
	}

//	CAppServerMgr::Instance()->SA_Push_Notification((PUCHAR)szMessage, listSmsInfo, listPushInfo);
	BYTE szTimeStamp[LENGTH_TIMESTAMP+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szTimeStamp, LENGTH_TIMESTAMP, nLen, nNeedLen)) return -1;

#if defined(SERVERAPP_STATUS)
	ICacheHandle* pCacheHandle = Cache_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pCacheHandle, -1);
	//////////////////////////////////////////////////////////////////////////
	// 状态改变本地保存
	LIST_DWORD listTempUserID;
	pCacheHandle->Cache_Device(m_tBaseInfo.dwServerID, dwDeviceID, dwStatus);
	if ( (AT_OFFLINE != dwStatus) && (AT_ONLINE != dwStatus) )
	{
		//////////////////////////////////////////////////////////////////////////
		// 其他状态改变通知在线用户
		CAppServerMgr::Instance()->ReportDeviceStatus_D(m_tBaseInfo.dwServerID, dwDeviceID, dwStatus, (PUCHAR)szMessage, (PUCHAR)szTimeStamp, listSmsInfo, listPushInfo);
	}
#elif defined(SERVERAPP_D)
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink) pSink->SA_OnReportDeviceStatus_Status(dwDeviceID, dwStatus, (PUCHAR)szMessage, (PUCHAR)szTimeStamp, listSmsInfo, listPushInfo);
#endif
	return 0;
}

// 上报客户端状态
int CAppServer::OnReportUserStatus( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

#if defined(SERVERAPP_STATUS)
	int nNeedLen = 3*sizeof(DWORD) + sizeof(BYTE);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwUserID = 0, dwSessionID = 0, dwStatus = 0;		// 0-offline; 1-online;
	BYTE bPushType = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwUserID >> dwSessionID >> dwStatus >> bPushType;
	BYTE szToken[LENGTH_TOKEN+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;

	ICacheHandle* pCacheHandle = Cache_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pCacheHandle, -1);
	pCacheHandle->Cache_User(m_tBaseInfo.dwServerID, dwUserID, dwSessionID, dwStatus);
#endif
	return 0;
}

int CAppServer::OnGetDeviceStatus(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	GetDeviceStatus_t tInfo;
	UINT nCount = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tInfo.dwUserID >> tInfo.dwSessionID >> nCount;
	
	nNeedLen += nCount * sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	for (UINT i = 0; i < nCount; i++)
	{
		DWORD dwDeviceID;
		bufferGet >> dwDeviceID;
		tInfo.listDeviceID.push_back(dwDeviceID);
	}

#if defined(SERVERAPP_D)
	// Dserver收到来自状态服务器的设备状态请求
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink) pSink->SA_OnGetDeviceStatus_Status(m_tHeader.reserved3, tInfo);
#elif defined(SERVERAPP_STATUS)
	// 状态服务器收到来自Dserver的设备状态请求
	ICacheHandle* pCacheHandle = Cache_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pCacheHandle, -1);
	MAP_DWORD_LSTDWORD mapInfo;
	pCacheHandle->Cache_GetServerIDByDeviceID(tInfo.listDeviceID, mapInfo);

	MAP_DWORD_LSTDWORD::iterator iter = mapInfo.begin();
	for (; iter != mapInfo.end(); iter++)
	{
		CAppServerMgr::Instance()->GetDeviceStatus_D(m_tBaseInfo.dwServerID, iter->first, tInfo.dwUserID, tInfo.dwSessionID, iter->second);
	}
#endif
	return 0;
}

int CAppServer::OnGetDeviceStatusRep(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	GetDeviceStatusRep_t tInfo;
	UINT nCount = 0;
	CGetBuffer buffer(pData, nLen);
	buffer >> tInfo.dwUserID >> tInfo.dwSessionID >> nCount;
	
	nNeedLen += nCount * 2 * sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	for (DWORD i = 0; i < nCount; ++i)
	{
		DeviceStatus_t info; memset(&info, 0, sizeof(DeviceStatus_t));
		buffer >> info.dwDeviceID >> info.dwStatus;
		tInfo.listInfo.push_back(info);
	}

	LIST_DEVICESTATUS::iterator iter = tInfo.listInfo.begin();
	for (; iter != tInfo.listInfo.end(); iter++)
	{
		if (false == GetBase64Str(buffer, (PUCHAR)iter->szStatusMsg, LENGTH_MSGCONTENT, nLen, nNeedLen)) return -1;
	}

#if defined(SERVERAPP_D)
	// Dserver收到来自状态服务器的设备状态回应
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink) pSink->SA_OnGetDeviceStatusRep_Status(tInfo);
#elif defined(SERVERAPP_STATUS)
	// 状态服务器收到来自Dserver的设备状态回应
	CAppServerMgr::Instance()->GetDeviceStatusRep_D(m_tHeader.reserved3, tInfo.dwUserID, tInfo.dwSessionID, tInfo.listInfo);
#endif
	return 0;
}

int CAppServer::OnTransClientInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	
	int nNeedLen = 5*sizeof(DWORD) + 2*sizeof(BYTE) + 4*sizeof(WORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	WORD wCount = 0;
	TransClientInfo_t tInfo;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tInfo.bSrcType >> tInfo.dwServerID >> tInfo.dwUserID >> tInfo.dwSessionID >> tInfo.dwDeviceID >> tInfo.bType;
	bufferGet >> tInfo.tNetInfo.dwPublicIP >> tInfo.tNetInfo.wPublicPortTCP >> tInfo.tNetInfo.wPublicPortUDP >> tInfo.tNetInfo.wLocalPortUDP >> wCount;

	nNeedLen += wCount*sizeof(DWORD) + sizeof(WORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	for (WORD i = 0; i < wCount; i++)
	{
		DWORD dwIP = 0;
		bufferGet >> dwIP;
		tInfo.tNetInfo.listLocalIPs.push_back(dwIP);
	}
	bufferGet >> tInfo.tNetInfo.wNetworkType >> tInfo.nTransmitSessionMode;

#if defined(SERVERAPP_D)
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink)
	{
		if (m_tBaseInfo.bServerType == SERVER_TYPE_STATUS)
		{
			tInfo.bSrcType = SRC_TYPE_STATUS;
			pSink->SA_OnTransClientInfo_Status(tInfo, (bool)m_tHeader.reserved0);
		}
		if (m_tBaseInfo.bServerType == SERVER_TYPE_NB)
		{
			tInfo.bSrcType = SRC_TYPE_RELAY;
			tInfo.dwServerID = m_tBaseInfo.dwServerID;
			tInfo.tNetInfo.dwPublicIP = m_tBaseInfo.dwIP;
			tInfo.tNetInfo.wPublicPortTCP = SERVER_PORT_NB;
			tInfo.tNetInfo.wPublicPortUDP = SERVER_PORT_NB;
			pSink->SA_OnTransClientInfo_Relay(tInfo, m_tBaseInfo.dwServerID);
		}
	}
#elif defined(SERVERAPP_STATUS)
	ICacheHandle* pCacheHandle = Cache_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pCacheHandle, -1);
	DWORD dwServerID = pCacheHandle->Cache_GetServerIDByDeviceID(tInfo.dwDeviceID);
	CAppServer* p = CAppServerMgr::Instance()->GetElem(dwServerID);
	if (p) p->SendCmd_TransClientInfo(tInfo, m_tHeader.reserved0);
#endif
	return 0;
}

int CAppServer::OnTransServerInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = 4*sizeof(DWORD) + 2*sizeof(BYTE);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	WORD wCount = 0;
	TransServerInfo_t tInfo;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tInfo.bSrcType >> tInfo.dwServerID >> tInfo.dwUserID >> tInfo.dwSessionID >> tInfo.dwDeviceID >> tInfo.bType;

	for (int k = 0; k < 2; k++)
	{
		nNeedLen += 2*sizeof(DWORD) + 4*sizeof(WORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}

		bufferGet >> tInfo.tConnectInfo[k].dwID;
		bufferGet >> tInfo.tConnectInfo[k].tNetInfo.dwPublicIP >> tInfo.tConnectInfo[k].tNetInfo.wPublicPortTCP 
			>> tInfo.tConnectInfo[k].tNetInfo.wPublicPortUDP >> tInfo.tConnectInfo[k].tNetInfo.wLocalPortUDP >> wCount;

		nNeedLen += wCount*sizeof(DWORD) + sizeof(WORD) + sizeof(UINT);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_MAIN, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		for (WORD i = 0; i < wCount; i++)
		{
			DWORD dwIP = 0;
			bufferGet >> dwIP;
			tInfo.tConnectInfo[k].tNetInfo.listLocalIPs.push_back(dwIP);
		}
		bufferGet >> tInfo.tConnectInfo[k].tNetInfo.wNetworkType;
		bufferGet >> CByteArrayBuffer((PUCHAR)tInfo.tConnectInfo[k].szUserName, LENGTH_CHALLENGE);
		bufferGet >> CByteArrayBuffer((PUCHAR)tInfo.tConnectInfo[k].szPassword, LENGTH_CHALLENGE);
	}

#if defined(SERVERAPP_D)
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink) pSink->SA_OnTransServerInfo_Status(tInfo);
#elif defined(SERVERAPP_NB)
	// 不处理，等待设备注册
#elif defined(SERVERAPP_STATUS)
	CAppServer* p = CAppServerMgr::Instance()->GetElem(tInfo.dwServerID);
	if (p) p->SendCmd_TransServerInfo(tInfo);
#endif
	return 0;
}

int CAppServer::OnSdkTunnel( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = sizeof(BYTE) + 4*sizeof(DWORD) + sizeof(WORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	WORD wTunnelDataLen = 0;
	SdkTunnel_t tTunnel; memset(&tTunnel, 0, sizeof(SdkTunnel_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTunnel.bSrcType >> tTunnel.dwServerID >> tTunnel.dwUserID >> tTunnel.dwSessionID >> tTunnel.dwDeviceID >> wTunnelDataLen;
	tTunnel.nTunnelDataLen = (int)wTunnelDataLen;
	if (tTunnel.nTunnelDataLen > LENGTH_TUNNELDATA)
	{
		LOG_DEBUG(LOG_MAIN, "Wrong TunnelDataLen %d\n", tTunnel.nTunnelDataLen); return -1;
	}
	nNeedLen += tTunnel.nTunnelDataLen;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	BYTE szTunnelData[LENGTH_TUNNELDATA+1] = {0};
	bufferGet >> CByteArrayBuffer((PUCHAR)szTunnelData, tTunnel.nTunnelDataLen);
	tTunnel.pTunnelData = (PUCHAR)szTunnelData;

#if defined(SERVERAPP_D)
	tTunnel.bSrcType = SRC_TYPE_STATUS;
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink) pSink->SA_OnSdkTunnel_Status(tTunnel);
#elif defined(SERVERAPP_STATUS)
	ICacheHandle* pCacheHandle = Cache_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pCacheHandle, -1);
	DWORD dwServerID = pCacheHandle->Cache_GetServerIDByDeviceID(tTunnel.dwDeviceID);
	CAppServer* p = CAppServerMgr::Instance()->GetElem(dwServerID);
	if (p) p->SendCmd_SdkTunnel(tTunnel);
#endif
	return 0;
}

int CAppServer::OnSdkTunnelRep( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = sizeof(BYTE) + 4*sizeof(DWORD) + sizeof(WORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	WORD wTunnelDataLen = 0;
	SdkTunnel_t tTunnel; memset(&tTunnel, 0, sizeof(SdkTunnel_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTunnel.bSrcType >> tTunnel.dwServerID >> tTunnel.dwUserID >> tTunnel.dwSessionID >> tTunnel.dwDeviceID >> wTunnelDataLen;
	tTunnel.nTunnelDataLen = (int)wTunnelDataLen;
	if (tTunnel.nTunnelDataLen > LENGTH_TUNNELDATA)
	{
		LOG_DEBUG(LOG_MAIN, "Wrong TunnelDataLen %d\n", tTunnel.nTunnelDataLen); return -1;
	}
	nNeedLen += tTunnel.nTunnelDataLen;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	BYTE szTunnelData[LENGTH_TUNNELDATA+1] = {0};
	bufferGet >> CByteArrayBuffer((PUCHAR)szTunnelData, tTunnel.nTunnelDataLen);
	tTunnel.pTunnelData = (PUCHAR)szTunnelData;

#if defined(SERVERAPP_D)
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink) pSink->SA_OnSdkTunnelRep_Status(tTunnel);
#elif defined(SERVERAPP_STATUS)
	CAppServer* p = CAppServerMgr::Instance()->GetElem(tTunnel.dwServerID);
	if (p) p->SendCmd_SdkTunnelRep(tTunnel);
#endif
	return 0;
}

int CAppServer::OnAddTempUser( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = 4*sizeof(DWORD) + 2*sizeof(BYTE);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	WORD wCount = 0;
	TransClientInfo_t tClientInfo;
	TransServerInfo_t tServerInfo;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tClientInfo.bSrcType >> tClientInfo.dwServerID >> tClientInfo.dwUserID >> tClientInfo.dwSessionID >> tClientInfo.dwDeviceID >> tClientInfo.bType;

	nNeedLen += 2*sizeof(DWORD) + 4*sizeof(WORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	bufferGet >> tServerInfo.tConnectInfo[1].dwID;
	bufferGet >> tClientInfo.tNetInfo.dwPublicIP >> tClientInfo.tNetInfo.wPublicPortTCP 
		>> tClientInfo.tNetInfo.wPublicPortUDP >> tClientInfo.tNetInfo.wLocalPortUDP >> wCount;

	nNeedLen += wCount*sizeof(DWORD) + sizeof(WORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	for (WORD i = 0; i < wCount; i++)
	{
		DWORD dwIP = 0;
		bufferGet >> dwIP;
		tClientInfo.tNetInfo.listLocalIPs.push_back(dwIP);
	}
	bufferGet >> tClientInfo.tNetInfo.wNetworkType;
	bufferGet >> CByteArrayBuffer((PUCHAR)tServerInfo.tConnectInfo[1].szUserName, LENGTH_CHALLENGE);
	bufferGet >> CByteArrayBuffer((PUCHAR)tServerInfo.tConnectInfo[1].szPassword, LENGTH_CHALLENGE);

	//////////////////////////////////////////////////////////////////////////
	tServerInfo.bSrcType = tClientInfo.bSrcType;
	tServerInfo.dwServerID = tClientInfo.dwServerID;
	tServerInfo.dwUserID = tClientInfo.dwUserID;
	tServerInfo.dwSessionID = tClientInfo.dwSessionID;
	tServerInfo.dwDeviceID = tClientInfo.dwDeviceID;
	tServerInfo.bType = tClientInfo.bType;
	tServerInfo.tConnectInfo[0].dwID = 0;
	memset(tServerInfo.tConnectInfo[0].szUserName, 0, LENGTH_CHALLENGE);
	memset(tServerInfo.tConnectInfo[0].szPassword, 0, LENGTH_CHALLENGE);
	tServerInfo.tConnectInfo[0].tNetInfo.dwPublicIP = 0;
	tServerInfo.tConnectInfo[0].tNetInfo.wPublicPortTCP = 0;
	tServerInfo.tConnectInfo[0].tNetInfo.wPublicPortUDP = 0;
	tServerInfo.tConnectInfo[0].tNetInfo.wLocalPortUDP = 0;
	tServerInfo.tConnectInfo[0].tNetInfo.wNetworkType = 0;

	// 赋值：转发服务器连接信息
	tServerInfo.tConnectInfo[1].dwID = m_tBaseInfo.dwServerID;
	tServerInfo.tConnectInfo[1].tNetInfo.dwPublicIP = m_tNetInfo.dwPublicIP;
	tServerInfo.tConnectInfo[1].tNetInfo.wPublicPortTCP = m_tNetInfo.wPublicPortTCP;
	tServerInfo.tConnectInfo[1].tNetInfo.wPublicPortUDP = m_tNetInfo.wPublicPortUDP;
	tServerInfo.tConnectInfo[1].tNetInfo.wLocalPortUDP = m_tNetInfo.wLocalPortUDP;
	tServerInfo.tConnectInfo[1].tNetInfo.wNetworkType = m_tNetInfo.wNetworkType;
	tServerInfo.tConnectInfo[1].tNetInfo.listLocalIPs = m_tNetInfo.listLocalIPs;
	//////////////////////////////////////////////////////////////////////////

#if defined(SERVERAPP_D)
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink)
	{
		tServerInfo.tConnectInfo[1].dwID = m_tBaseInfo.dwServerID;
		tServerInfo.tConnectInfo[1].tNetInfo.dwPublicIP = m_tBaseInfo.dwIP;
		tServerInfo.tConnectInfo[1].tNetInfo.wPublicPortTCP = SERVER_PORT_NB;
		tServerInfo.tConnectInfo[1].tNetInfo.wPublicPortUDP = SERVER_PORT_NB;
		tServerInfo.tConnectInfo[1].tNetInfo.wLocalPortUDP = 0;
		tServerInfo.tConnectInfo[1].tNetInfo.wNetworkType = 3;
		tServerInfo.tConnectInfo[1].tNetInfo.listLocalIPs.clear();
		pSink->SA_OnAddTempUser_Relay(tClientInfo, tServerInfo);
	}
#elif defined(SERVERAPP_NB)
	TempUser_t tInfo; memset(&tInfo, 0, sizeof(TempUser_t));
	memcpy(tInfo.szUserName, tServerInfo.tConnectInfo[1].szUserName, LENGTH_CHALLENGE);
	memcpy(tInfo.szPassword, tServerInfo.tConnectInfo[1].szPassword, LENGTH_CHALLENGE);
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink) pSink->SA_OnAddTempUser_D(tInfo);

	SendCmd_AddTempUserRep(tClientInfo, (PUCHAR)tServerInfo.tConnectInfo[1].szUserName, (PUCHAR)tServerInfo.tConnectInfo[1].szPassword);
	
	tClientInfo.bSrcType = 3; // 0-客户端 1-注册服务器 2-状态服务器 3-转发服务器
//	tClientInfo.dwServerID = 0;		// 注1 ServerID
//	tClientInfo.dwUserID = 0;		// 客户端UserID
//	tClientInfo.dwSessionID = 0;	// 客户端SessionID
//	tClientInfo.dwDeviceID = 0;		// 观看设备DeviceID
//	tClientInfo.bType = 0;			// 0:实时
	tClientInfo.tNetInfo.dwPublicIP = m_tNetInfo.dwPublicIP;
	tClientInfo.tNetInfo.wPublicPortTCP = m_tNetInfo.wPublicPortTCP;
	tClientInfo.tNetInfo.wPublicPortUDP = m_tNetInfo.wPublicPortUDP;
	tClientInfo.tNetInfo.wLocalPortUDP = m_tNetInfo.wLocalPortUDP;
	tClientInfo.tNetInfo.wNetworkType = m_tNetInfo.wNetworkType;
	tClientInfo.tNetInfo.listLocalIPs = m_tNetInfo.listLocalIPs;
	tClientInfo.nTransmitSessionMode = 0;
	SendCmd_TransClientInfo(tClientInfo, true);
#endif
	return 0;
}

int CAppServer::OnReportNetwork( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

#if defined(SERVERAPP_D)
	int nNeedLen = sizeof(DWORD) + 4*sizeof(WORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	NetInfo_t tNetInfo;
	WORD wCount = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tNetInfo.dwPublicIP >> tNetInfo.wPublicPortTCP >> tNetInfo.wPublicPortUDP >> tNetInfo.wLocalPortUDP >> wCount;
	nNeedLen = wCount * sizeof(DWORD) + sizeof(WORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	m_tNetInfo.dwPublicIP = tNetInfo.dwPublicIP;
	m_tNetInfo.wPublicPortTCP = tNetInfo.wPublicPortTCP;
	m_tNetInfo.wPublicPortUDP = tNetInfo.wPublicPortUDP;
	m_tNetInfo.wLocalPortUDP = tNetInfo.wLocalPortUDP;
	for (WORD i = 0; i < wCount; i++)
	{
		DWORD dwIP = 0;
		bufferGet >> dwIP;
		m_tNetInfo.listLocalIPs.push_back(dwIP);
	}
	bufferGet >> m_tNetInfo.wNetworkType;
#endif
	return 0;
}

int CAppServer::OnQiniu_GetUploadToken(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	
	int nNeedLen = sizeof(BYTE) + 3*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTag.bSrcType >> tTag.dwTagID1 >> tTag.dwTagID2 >> tTag.dwStoreID;
	LOG_DEBUG(LOG_DB_SERVER,"%d %d %d %d\n",tTag.bSrcType, tTag.dwTagID1, tTag.dwTagID2, tTag.dwStoreID);
	
#if defined(SERVERAPP_STORAGE_BUSINESS)
	IStoreHandle* pStoreHandle = IStore_GetHandle();
	LOG_DEBUG(LOG_DB_SERVER,"pHandle1 = %p\n", pStoreHandle);
	LOG_ASSERT_RET(LOG_MAIN, pStoreHandle, -1);
	if(false == pStoreHandle->Qiniu_GetStorageAccount(tTag))
	{
		return SendCmd_Qiniu_UploadTokenRep(tTag, NULL, ERROR_SYSTEM);
	}
	LOG_DEBUG(LOG_DB_SERVER,"pHandle2 = %p\n", pStoreHandle);

	//待删除
	// 从数据库查询storeid对应的云存储账号信息，产生上传凭证
//	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
//	if (-1 == m_pDataBase->Qiniu_GetStorageAccount(tTag))
//	{
//		return SendCmd_Qiniu_UploadTokenRep(tTag, NULL, ERROR_SYSTEM);
//	}
#endif
	return 0;
}

int CAppServer::OnRepoartAlarmStatus(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CAppServer::%s\n",__FUNCTION__);
	int nNeedLen = sizeof(DWORD) * 4 + LENGTH_TIMESTAMP;
	if(nLen < nNeedLen){
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	AlarmStatus_t alarmStatus;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> alarmStatus.dwDeviceID >> alarmStatus.dwRoomID >> alarmStatus.dwType >> alarmStatus.dwSubType;
	bufferGet >> CByteArrayBuffer(alarmStatus.szTimeStamp, LENGTH_TIMESTAMP);
	LOG_DEBUG(LOG_MAIN, "DevID = %d, RoomID = %d, Type = %d, SubType = %d, alarmStatus.szTimeStamp = %s\n", alarmStatus.dwDeviceID, alarmStatus.dwRoomID, alarmStatus.dwType, alarmStatus.dwSubType, alarmStatus.szTimeStamp);
#if defined(SERVERAPP_STORAGE_BUSINESS)
	IStoreHandle* pStoreHandle = IStore_GetHandle();
	LOG_DEBUG(LOG_DB_SERVER,"pHandle1 = %p\n", pStoreHandle);
	LOG_ASSERT_RET(LOG_MAIN, pStoreHandle, -1);
	if(false == pStoreHandle->RepoartAlarmStatus(alarmStatus))
	{
		LOG_DEBUG(LOG_DB_SERVER,"pHandle2 = %p\n", pStoreHandle);
		return -1;
	}
	//待删除
//	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
//	if (-1 == m_pDataBase->RepoartAlarmStatus(alarmStatus)) return -1;
#endif
	return 0;
}

int CAppServer::OnQiniu_GetUploadTokenRep(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = sizeof(BYTE) + 3*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTag.bSrcType >> tTag.dwTagID1 >> tTag.dwTagID2 >> tTag.dwStoreID;

	BYTE szUploadToken[LENGTH_UPLOADTOKEN+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szUploadToken, LENGTH_UPLOADTOKEN, nLen, nNeedLen)) return -1;

#if defined(SERVERAPP_D)
	// 解析上传凭证
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink) pSink->SA_OnQiniu_GetUploadToken_Storage(tTag, (PUCHAR)szUploadToken);
#endif
	return 0;
}

#if defined(SERVERAPP_STORAGE_BUSINESS)
int CAppServer::OnQiniu_GetStorageAccount(StorageTag_t& tTag, StorageAccount_t& tInfo)
{
	if ((tInfo.dwStoreID != tTag.dwStoreID) || 
		(strlen((const char*)tInfo.szAccessKey) <= 0) ||
		(strlen((const char*)tInfo.szSecretKey) <= 0) ||
		(strlen((const char*)tInfo.szBucket) <= 0) ||
		(strlen((const char*)tInfo.szDomain) <= 0)
		)
	{
		return SendCmd_Qiniu_UploadTokenRep(tTag, NULL, ERROR_INVALID_USERNAME);
	}

	BYTE szUploadToken[LENGTH_UPLOADTOKEN+1] = {0};
	Qiniu_CreateUploadToken((const char*)tInfo.szAccessKey, (const char*)tInfo.szSecretKey, (const char*)tInfo.szBucket, (char*)szUploadToken);
	return SendCmd_Qiniu_UploadTokenRep(tTag, (PUCHAR)szUploadToken, ERROR_NO);
}
#endif

int CAppServer::OnQiniu_GetDownloadUrls(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"(#photos:5#)\n",__FUNCTION__);
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = 3*sizeof(BYTE) + 7*sizeof(DWORD) + LENGTH_TIMESTAMP2;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTag.bSrcType >> tTag.dwTagID1 >> tTag.dwTagID2 >> tTag.dwStoreID;

	StoreKey_t tKey; memset(&tKey, 0, sizeof(StoreKey_t));
	bufferGet >> tKey.dwDeviceID >> tKey.dwRoomID >> tKey.dwSize >> tKey.dwStoreID >> tKey.bType >> tKey.bRecReason;
	bufferGet >> CByteArrayBuffer((PUCHAR)tKey.szTimeStamp, LENGTH_TIMESTAMP2);

#if defined(SERVERAPP_STORAGE_BUSINESS)
	IStoreHandle* pStoreHandle = IStore_GetHandle();
	LOG_DEBUG(LOG_DB_SERVER,"pHandle1 = %p\n", pStoreHandle);
	LOG_ASSERT_RET(LOG_MAIN, pStoreHandle, -1);
	if(false == pStoreHandle->Qiniu_GetStorageKeys(tTag, tKey))
	{
		LIST_STORE_KEYURL lstInfo;
		return SendCmd_Qiniu_DownloadUrlsRep(tTag, lstInfo, ERROR_SYSTEM);
	}
	LOG_DEBUG(LOG_DB_SERVER,"pHandle2 = %p\n", pStoreHandle);

	//待删除
	// 1 从数据库查询userid+deviceid对应的房号
	// 2 从数据库查询deviceid+roomid对应的key列表
//	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
//	if (-1 == m_pDataBase->Qiniu_GetStorageKeys(tTag, tKey))
//	{
//		LIST_STORE_KEYURL lstInfo;
//		return SendCmd_Qiniu_DownloadUrlsRep(tTag, lstInfo, ERROR_SYSTEM);
//	}
#endif
	return 0;
}

int CAppServer::OnQiniu_GetDownloadUrls2(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "CAppServer::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = 3*sizeof(BYTE) + 9*sizeof(DWORD) + LENGTH_TIMESTAMP2;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTag.bSrcType >> tTag.dwTagID1 >> tTag.dwTagID2 >> tTag.dwStoreID;

	StoreVisitor_t tVisitor; memset(&tVisitor, 0, sizeof(StoreVisitor_t));
	bufferGet >> tVisitor.tKey.dwDeviceID >> tVisitor.tKey.dwRoomID >> tVisitor.tKey.dwSize >> tVisitor.tKey.dwStoreID 
		>> tVisitor.tKey.bType >> tVisitor.tKey.bRecReason;
	bufferGet >> CByteArrayBuffer((PUCHAR)tVisitor.tKey.szTimeStamp, LENGTH_TIMESTAMP2);
	bufferGet >> tVisitor.startIndex >> tVisitor.dwCount;

#if defined(SERVERAPP_STORAGE_BUSINESS)
	IStoreHandle* pStoreHandle = IStore_GetHandle();
	LOG_DEBUG(LOG_DB_SERVER,"pHandle1 = %p\n", pStoreHandle);
	LOG_ASSERT_RET(LOG_MAIN, pStoreHandle, -1);
	if( false == pStoreHandle->Qiniu_GetStorageKeys2(tTag, tVisitor))
	{
		LIST_STORE_KEYURL lstInfo;
		return SendCmd_Qiniu_DownloadUrlsRep(tTag, lstInfo, ERROR_SYSTEM);
	}
	LOG_DEBUG(LOG_DB_SERVER,"pHandle2 = %p\n", pStoreHandle);
	//待删除
	// 1 从数据库查询userid+deviceid对应的房号
	// 2 从数据库查询deviceid+roomid对应的key列表
//	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
//	if (-1 == m_pDataBase->Qiniu_GetStorageKeys2(tTag, tVisitor))
//	{
//		LIST_STORE_KEYURL lstInfo;
//		return SendCmd_Qiniu_DownloadUrlsRep(tTag, lstInfo, ERROR_SYSTEM);
//	}
#endif
	return 0;
}

int CAppServer::OnQiniu_GetDownloadUrlsRep(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "CAppServer::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = sizeof(BYTE) + 3*sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	int nCount = 0;
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTag.bSrcType >> tTag.dwTagID1 >> tTag.dwTagID2 >> tTag.dwStoreID >> nCount;

	LIST_STORE_KEYURL lstInfo;
	StoreKeyUrl_t tKeyUrl;
	for (int i = 0; i < nCount; i++)
	{
		nNeedLen = 2*sizeof(BYTE) + 4*sizeof(DWORD) + sizeof(UINT) + LENGTH_TIMESTAMP2;
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		memset(&tKeyUrl, 0, sizeof(StoreKeyUrl_t));
		bufferGet >> tKeyUrl.tKey.dwDeviceID >> tKeyUrl.tKey.dwRoomID >> tKeyUrl.tKey.dwSize
			>> tKeyUrl.tKey.dwStoreID >> tKeyUrl.tKey.bType >> tKeyUrl.tKey.bRecReason;
		bufferGet >> CByteArrayBuffer((PUCHAR)tKeyUrl.tKey.szTimeStamp, LENGTH_TIMESTAMP2);
		if (false == GetVariableStr(bufferGet, (PUCHAR)tKeyUrl.szUrl, LENGTH_STOREURL, nLen, nNeedLen)) return -1;

		lstInfo.push_back(tKeyUrl);
	}
#if defined(SERVERAPP_D)
	IServerAppHandleSink* pSink = CAppServerMgr::Instance()->GetSink();
	if (pSink) pSink->SA_OnQiniu_GetDownloadUrls_Storage(tTag, lstInfo);
#endif
}

#if defined(SERVERAPP_STORAGE_BUSINESS)
int CAppServer::OnQiniu_GetStorageKeys(StorageTag_t& tTag, LIST_STORE_ACCOUNTKEYS& lstAccountKeys)
{
	LOG_DEBUG(LOG_MAIN,"CAppServer::%s\n",__FUNCTION__);
	StoreKeyUrl_t tKeyUrl;
	LIST_STORE_KEYURL lstKeyUrl;
	LIST_STORE_ACCOUNTKEYS::iterator iterAccountKeys = lstAccountKeys.begin();
	for (; iterAccountKeys != lstAccountKeys.end(); iterAccountKeys++)
	{
		LIST_STOREKEY::iterator iterKey = iterAccountKeys->lstKey.begin();
		for (; iterKey != iterAccountKeys->lstKey.end(); iterKey++)
		{
			memset(&tKeyUrl, 0, sizeof(StoreKeyUrl_t));
			tKeyUrl.tKey = *iterKey;

			BYTE szKey[LENGTH_STOREKEY+1];
			GenerateStoreKey(*iterKey, szKey);
			Qiniu_GetDowndloadUrls((const char*)iterAccountKeys->tAccount.szAccessKey, 
				(const char*)iterAccountKeys->tAccount.szSecretKey, 
				(const char*)iterAccountKeys->tAccount.szDomain,
				(const char*)szKey,
				(char*)tKeyUrl.szUrl);
			lstKeyUrl.push_back(tKeyUrl);
		}
	}
	return SendCmd_Qiniu_DownloadUrlsRep(tTag, lstKeyUrl, ERROR_NO);
}
#endif

int CAppServer::OnQiniu_ReportUploadResult( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(BYTE) + 4*sizeof(DWORD) + LENGTH_TIMESTAMP2;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	StoreKey_t tKey; memset(&tKey, 0, sizeof(StoreKey_t));
	bufferGet >> tKey.dwDeviceID >> tKey.dwRoomID >> tKey.dwSize >> tKey.dwStoreID >> tKey.bType >> tKey.bRecReason;
	bufferGet >> CByteArrayBuffer((PUCHAR)tKey.szTimeStamp, LENGTH_TIMESTAMP2);
	LOG_DEBUG(LOG_MAIN, "Qiniu_ReportUploadResult failed(DeviceID:%d RoomID:%d dwSize:%d dwStoreID:%d Type:%d Reason:%d Time:%s)\n",
		tKey.dwDeviceID, tKey.dwRoomID, tKey.dwSize, tKey.dwStoreID, tKey.bType, tKey.bRecReason, tKey.szTimeStamp);

#if defined(SERVERAPP_STORAGE_BUSINESS)
	IStoreHandle* pStoreHandle = IStore_GetHandle();
	LOG_DEBUG(LOG_DB_SERVER,"pHandle1 = %p\n", pStoreHandle);
	LOG_ASSERT_RET(LOG_MAIN, pStoreHandle, -1);
	if (false == pStoreHandle->Qiniu_ReportUploadResult(tKey))
	{
		LOG_DEBUG(LOG_MAIN, "Qiniu_ReportUploadResult failed(DeviceID:%d RoomID:%d dwSize:%d dwStoreID:%d Type:%d Reason:%d Time:%s)\n",
				tKey.dwDeviceID, tKey.dwRoomID, tKey.dwSize, tKey.dwStoreID, tKey.bType, tKey.bRecReason, tKey.szTimeStamp);
	}

	// 写入数据库
//	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
//	if (-1 == m_pDataBase->Qiniu_ReportUploadResult(tKey))
//	{
//		LOG_DEBUG(LOG_MAIN, "Qiniu_ReportUploadResult failed(DeviceID:%d RoomID:%d dwSize:%d dwStoreID:%d Type:%d Reason:%d Time:%s)\n",
//			tKey.dwDeviceID, tKey.dwRoomID, tKey.dwSize, tKey.dwStoreID, tKey.bType, tKey.bRecReason, tKey.szTimeStamp);
//		return -1;
//	}
#endif
	return 0;
}

IMPLEMENT_SINGLETON(CAppServerMgr)
CAppServerMgr::CAppServerMgr()
{
	m_pSink = NULL;
}

bool CAppServerMgr::SA_AddServer(ServerInfo_t& tInfo, INetConnection* pCon)
{
	CAppServer* p = GetElem(tInfo.dwServerID);
	LOG_DEBUG(LOG_MAIN, "%s ServerType %d ServerID %d UserName %s pCon %p CAppServer %p\n", 
		__FUNCTION__, tInfo.bServerType, tInfo.dwServerID, tInfo.szUserName, pCon, p);
	if (p == NULL)
	{
		try
		{
			p = new CAppServer();
		}
		catch(std::bad_alloc &memExp)
		{
			if (pCon) { pCon->Disconnect(); NetworkDestroyConnection(pCon); }
			return false;
		}
		AddElem(tInfo.dwServerID, p);
	}

	if (pCon)
	{
		p->SetServerInfo(tInfo);
		p->SetNetConnection(pCon);
	}
	else p->Start(tInfo);
	return true;
}

bool CAppServerMgr::SA_GetDServers( DWORD& dwVendorID, DWORD& dwConfigureIndex, LIST_SERVERINFO& listInfo )
{
	LOG_DEBUG(LOG_MAIN, "%s dwVendorID %d dwConfigureIndex %d\n", __FUNCTION__, dwVendorID, dwConfigureIndex);
	if (m_mapServerIndex.empty())
	{
		LOG_ERR(LOG_MAIN, "%s m_mapServerIndex Empty!\n", __FUNCTION__); return false;
	}

	dwVendorID = CalcRealVendorID(dwVendorID);
	MAP_DWORD::iterator pos = m_mapServerIndex.find(dwVendorID);
	if (pos != m_mapServerIndex.end())
	{
		dwConfigureIndex = pos->second;
		LOG_DEBUG(LOG_MAIN, "%s 1. dwVendorID %d dwConfigureIndexNew %d\n", __FUNCTION__, dwVendorID, dwConfigureIndex);
	}
	else
	{
		dwVendorID = 0;
		dwConfigureIndex = 0;
		LOG_ERR(LOG_MAIN, "%s 1. Can't Find RegisterServer dwVendorID %d\n", __FUNCTION__, dwVendorID); return false;
	}

	MAP_SERVERINFO::iterator iter = m_mapServerInfo.find(dwVendorID);
	if (iter == m_mapServerInfo.end())
	{
		LOG_ERR(LOG_MAIN, "%s 2. Can't Find RegisterServer dwVendorID %d\n", __FUNCTION__, dwVendorID); return false;
	}
	listInfo.insert(listInfo.end(), iter->second.begin(), iter->second.end());
	return true;
}

bool CAppServerMgr::SA_DserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	MAP_DWORD::iterator iter = m_mapServerIndex.find(dwVendorID);
	if (iter != m_mapServerIndex.end()) iter->second = dwConfigureIndex;
	else m_mapServerIndex.insert(std::make_pair(dwVendorID, dwConfigureIndex));

	MAP_SERVERINFO::iterator pos = m_mapServerInfo.find(dwVendorID);
	if (pos != m_mapServerInfo.end())
	{
		pos->second.clear();
		pos->second.insert(pos->second.end(), listInfo.begin(), listInfo.end());
	}
	else m_mapServerInfo.insert(std::make_pair(dwVendorID, listInfo));
	return true;
}
//发送短信
bool CAppServerMgr::SA_Sms_Notification( PUCHAR pMsg, LIST_SMSINFO& listSmsInfo, DWORD dwClientID )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer((BYTE)SERVER_TYPE_NOTIFICATION);
	LOG_ASSERT_RET(LOG_MAIN, p, false);

	LIST_SMSINFO::iterator iter = listSmsInfo.begin();
	for (; iter != listSmsInfo.end(); iter++)
	{
		int nMobilePhoneLen = strlen((const char*)iter->szMobilePhone);
		if (nMobilePhoneLen <= 0) continue;

		Notify_SmsInfo_t tInfo; memset(&tInfo, 0, sizeof(Notify_SmsInfo_t));
		snprintf((char*)tInfo.szMessageContent, LENGTH_MSGCONTENT, "%s", pMsg);
		tInfo.dwVendorID = iter->dwVendorID;
		memcpy(tInfo.szMobilePhone, iter->szMobilePhone, LENGTH_MOBILEPHONE);
		p->SendCmd_Sms(tInfo, dwClientID);
	}
	return true;
}
//推送2
bool CAppServerMgr::SA_Push_Notification(DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, DWORD dwDeviceID, LIST_PUSHINFO& listToken)
{
	LOG_DEBUG(LOG_MAIN, "%s dwStatus %d dwDeviceID %d Message %s\n", __FUNCTION__, dwStatus, dwDeviceID, pMsg);
	CAppServer* p = GetServer(SERVER_TYPE_NOTIFICATION);
	LOG_ASSERT_RET(LOG_MAIN, p, false);

	typedef std::map<DWORD, LIST_NOTIFY_PUSHINFO> MAP_NOTIFY_PUSHINFO;
	MAP_NOTIFY_PUSHINFO::iterator iterMap;
	MAP_NOTIFY_PUSHINFO mapInfo;
	LIST_PUSHINFO::iterator iter = listToken.begin();
	for (; iter != listToken.end(); iter++)
	{
		Notify_PushInfo_t tInfo; memset(&tInfo, 0, sizeof(Notify_PushInfo_t));
		GenerateDeviceStatusMsg((PUCHAR)iter->szDeviceName, pMsg, pTimeStamp, (PUCHAR)tInfo.szMessageContent);
//		LOG_DEBUG(LOG_MAIN, "NewMessage %s\n", tInfo.szMessageContent);

		tInfo.dwVendorID = iter->dwVendorID;
		tInfo.bPushType = iter->bPushType;
		tInfo.bLanguage = iter->bLanguage;
		memcpy(tInfo.szToken, iter->szToken, LENGTH_TOKEN);

		iterMap = mapInfo.find(iter->dwUserID);
		if (iterMap == mapInfo.end())
		{
			LIST_NOTIFY_PUSHINFO listInfo;
			listInfo.push_back(tInfo);
			mapInfo.insert(std::make_pair(iter->dwUserID, listInfo));
		}
		else
		{
			iterMap->second.push_back(tInfo);
		}
	}

	iterMap = mapInfo.begin();
	for (; iterMap != mapInfo.end(); iterMap++)
	{
		p->SendCmd_Push(dwStatus, dwDeviceID, iterMap->second);
	}
	return true;
}

CAppServer* CAppServerMgr::GetServer(BYTE bType)
{
	LOG_DEBUG(LOG_MAIN, "%s type: %d\n", __FUNCTION__, bType);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		if ( (iter->second->GetServerType() == bType) && 
			 (iter->second->IsServerOnline())
			 ) return iter->second;
	}
	return NULL;
}

CAppServer* CAppServerMgr::GetServer( DWORD dwServerID )
{
	LOG_DEBUG(LOG_MAIN, "%s serverid: %d\n", __FUNCTION__, dwServerID);
	CAppServer* p = GetElem(dwServerID);
	if (p && p->IsServerOnline() ) return p;
	return NULL;
}

bool CAppServerMgr::SA_ReportDeviceStatus_Status( DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2 )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
#if defined(SERVERAPP_D)
	CAppServer* p = GetServer(SERVER_TYPE_STATUS);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No status server\n");
		return false;
	}
	p->SendCmd_ReportDeviceStatus(dwDeviceID, dwStatus, pMsg, pTimeStamp, list1, list2);
#endif
	return true;
}

bool CAppServerMgr::SA_ReportUserStatus_Status( UserStatus_t& tInfo )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STATUS);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No status server\n");
		return false;
	}
	p->SendCmd_ReportUserStatus(tInfo);
	return true;
}

bool CAppServerMgr::SA_TransServerInfo_Status( TransServerInfo_t& tInfo )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STATUS);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No status server\n");
		return false;
	}
	p->SendCmd_TransServerInfo(tInfo);
	return true;
}

bool CAppServerMgr::SA_TransServerInfo_Relay( TransServerInfo_t& tInfo, DWORD dwServerID )
{
	LOG_DEBUG(LOG_MAIN, "%s dwServerID %d\n", __FUNCTION__, dwServerID);
	CAppServer* p = GetServer(dwServerID);
	if((dwServerID == SERVER_TYPE_STATUS) && (!p)) {
		LOG_ERR(LOG_MAIN, "No status server\n");
		return false;
	}
	LOG_ASSERT_RET(LOG_MAIN, p, false);
	p->SendCmd_TransServerInfo(tInfo);
	return true;
}

bool CAppServerMgr::SA_GetDeviceStatus_Status( GetDeviceStatus_t& tInfo )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STATUS);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No status server\n");
		return false;
	}
	p->SendCmd_GetDeviceStatus(tInfo.dwUserID, tInfo.dwSessionID, tInfo.listDeviceID);
	return true;
}

bool CAppServerMgr::SA_GetDeviceStatusRep_Status( DWORD dwServerID, GetDeviceStatusRep_t& tInfo )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STATUS);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No status server\n");
		return false;
	}
	p->SetHeader_reserved3(dwServerID);
	p->SendCmd_GetDeviceStatusRep(tInfo.dwUserID, tInfo.dwSessionID, tInfo.listInfo);
	return true;
}

bool CAppServerMgr::SA_AddTempUser_Relay( TransClientInfo_t& tInfo, bool bRelay, DWORD dwAutoRelay )
{
	LOG_DEBUG(LOG_MAIN, "%s bRelay %d dwAutoRelay %d\n", __FUNCTION__, bRelay, dwAutoRelay);
	if (false == bRelay) return false;
	if (RELAY_TYPE_NO == dwAutoRelay) return false;
	CAppServer* p = GetServer(SERVER_TYPE_NB);
	LOG_ASSERT_RET(LOG_MAIN, p, false);
	p->SendCmd_AddTempUser(tInfo);
	return true;
}

bool CAppServerMgr::SA_TransClientInfo_Status( TransClientInfo_t& tInfo, bool bRelay )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STATUS);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No status server\n");
		return false;
	}
	p->SendCmd_TransClientInfo(tInfo, bRelay);
	return true;
}

bool CAppServerMgr::SA_SdkTunnel_Status(SdkTunnel_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STATUS);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No status server\n");
		return false;
	}
	p->SendCmd_SdkTunnel(tInfo);
	return true;
}

bool CAppServerMgr::SA_SdkTunnelRep_Status(SdkTunnel_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STATUS);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No status server\n");
		return false;
	}
	p->SendCmd_SdkTunnelRep(tInfo);
	return true;
}

bool CAppServerMgr::SA_ReportAlarmStatus(AlarmStatus_t& alarmStatus)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STORAGE);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No storage server\n");
		return false;
	}
	p->SendCmd_AlarmStatus(alarmStatus, ERROR_NO);
	return true;
}

bool CAppServerMgr::SA_GetUploadToken_Storage(StorageTag_t& tTag)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STORAGE);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No storage server\n");
		return false;
	}
	p->SendCmd_Qiniu_UploadToken(tTag, ERROR_NO);
	return true;
}

bool CAppServerMgr::SA_ReportUploadResult_Storage(StoreKey_t& tKey)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STORAGE);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No storage server\n");
		return false;
	}
	p->SendCmd_Qiniu_ReportUploadResult(tKey);
	return true;
}

bool CAppServerMgr::SA_GetDownloadUrls_Storage(StorageTag_t& tTag, StoreKey_t& tKey)
{
	LOG_DEBUG(LOG_MAIN, "CAppServerMgr::%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STORAGE);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No storage server\n");
		return false;
	}
	p->SendCmd_Qiniu_DownloadUrls(tTag, tKey, ERROR_NO);
	return true;
}

bool CAppServerMgr::SA_GetDownloadUrls_Storage2(StorageTag_t& tTag, StoreVisitor_t& tVisitor)
{
	LOG_DEBUG(LOG_MAIN, "CAppServerMgr::%s\n", __FUNCTION__);
	CAppServer* p = GetServer(SERVER_TYPE_STORAGE);
	if(!p) {
		LOG_ERR(LOG_MAIN, "No storage server\n");
		return false;
	}
	p->SendCmd_Qiniu_DownloadUrls2(tTag, tVisitor, ERROR_NO);
	return true;
}

#if defined(SERVERAPP_STATUS)
bool CAppServerMgr::ReportDeviceStatus_D( DWORD dwServerID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2 )
{
	LOG_DEBUG(LOG_MAIN, "%s ServerID %d DeviceID %d Status %d\n", __FUNCTION__, dwServerID, dwDeviceID, dwStatus);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tInfo; memset(&tInfo, 0, sizeof(ServerInfo_t));
		p->GetServerInfo(tInfo);
		if ( (tInfo.bServerType != SERVER_TYPE_D) || (tInfo.dwServerID == dwServerID) )
		{
			LOG_DEBUG(LOG_MAIN, "Skip The DServer Type %d ServerID %d(SrcServerID %d)\n", tInfo.bServerType, tInfo.dwServerID, dwServerID);
			continue;
		}
		p->SendCmd_ReportDeviceStatus(dwDeviceID, dwStatus, pMsg, pTimeStamp, list1, list2);
	}
	return true;
}

bool CAppServerMgr::GetDeviceStatus_D(DWORD dwSrcServerID, DWORD dwDstServerID, DWORD dwUserID, DWORD dwSessionID, LIST_DWORD& lstDeviceID)
{
	LOG_DEBUG(LOG_MAIN, "%s dwSrcServerID %d dwDstServerID %d\n", __FUNCTION__, dwSrcServerID, dwDstServerID);
	if (dwSrcServerID == dwDstServerID) return true;
	CAppServer* p = GetServer(dwDstServerID);
	LOG_ASSERT_RET(LOG_MAIN, p, false);
	p->SetHeader_reserved3(dwSrcServerID);
	p->SendCmd_GetDeviceStatus(dwUserID, dwSessionID, lstDeviceID);
	return true;
}

bool CAppServerMgr::GetDeviceStatusRep_D(DWORD dwDstServerID, DWORD dwUserID, DWORD dwSessionID, LIST_DEVICESTATUS& lstInfo)
{
	LOG_DEBUG(LOG_MAIN, "%s dwDstServerID %d\n", __FUNCTION__, dwDstServerID);
	CAppServer* p = GetServer(dwDstServerID);
	LOG_ASSERT_RET(LOG_MAIN, p, false);
	p->SendCmd_GetDeviceStatusRep(dwUserID, dwSessionID, lstInfo);
	return true;
}

#endif
