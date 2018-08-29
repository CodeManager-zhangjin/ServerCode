#include "View.h"
#include "../device/Device.h" //?
#include "UtilityInterface.h"
#include "getbuffer.h"
#include "Log.h"
#include "DeviceInterface.h"
#include "ServerAppInterface.h"
#include <algorithm>

DWORD g_dwUserID = 0;
CSTRING g_token;
BYTE g_bPushType = 0;


BYTE CView::m_szBuffer[MAX_PACKET_LEN] = {0};
const CView::HandlerEntry CView::mHandlers[] =
{
	{ CMD_GET_CHALLENGE,		&CView::OnGetChallenge			},
	{ CMD_CLIENT_AUTH,			&CView::OnLogin					},
	{ CMD_REPORT_NETWORK,		&CView::OnReportNetwork			},
	{ CMD_GET_USERDEVICE_INFO,	&CView::OnGetUserDeviceInfo		},
	{ CMD_GET_USERGROUP_INFO,	&CView::OnGetUserGroupInfo		},
	{ CMD_GET_USERROOM_INFO,	&CView::OnGetUserRoomInfo		},
	{ CMD_ADD_DEVICE,			&CView::OnAddDevice				},
	{ CMD_ADD_DEL_PUSH_INFO,	&CView::OnAddDelPushInfo		},
	{ CMD_GET_DEVICE_STATUS,	&CView::OnGetDeviceStatus		},
	{ CMD_TRANS_CLIENTINFO,		&CView::OnTransClientInfo		},
	{ CMD_GET_REGISTER_INFO,	&CView::OnGetRegisterInfo		},
	{ CMD_SET_DEVICE_NAME,		&CView::OnSetDeviceName			},
	{ CMD_DEL_DEVICE,			&CView::OnDelDevice				},
	{ CMD_AUTHORIZE,			&CView::OnAuthorize				},
	{ CMD_AUTHORIZE2,			&CView::OnAuthorize2			},
	{ CMD_GET_DEVICEUSER_INFO,	&CView::OnGetDeviceUserInfo		},

	{ CMD_ADD_DEL_PUSH_INFO_EX,	&CView::OnAddDelPushInfoEx		},
	{ CMD_SDK_TUNNEL,			&CView::OnSdkTunnel				},
	{ CMD_GET_UPLOAD_TOKEN,		&CView::OnQiniu_GetUploadToken	},
	{ CMD_GET_DOWNLOAD_URLS,	&CView::OnQiniu_GetDownloadUrls	},
	{ CMD_GET_DOWNLOAD_URLS2,	&CView::OnQiniu_GetDownloadUrls2},
	{ CMD_PUSH_SWITCH,			&CView::OnSetPushSwitch			},
	//室内机绑定门口机
	{ CMD_INDOOR_BIND_DEVICE,	&CView::OnIndoorBindDevice		},
};

CView::CView(DWORD dwSessionD)
{
	m_dwSessionID = dwSessionD;
	m_dwAppVendorID = 0;
	m_pCon = NULL;
	m_pDataBase = NULL;
	memset(&m_tHeader, 0, sizeof(PacketHeader_t));
	m_bAuth = false;
	memset(&m_tUserInfo, 0, sizeof(UserInfo_t));
	memset(&m_tGtPushInfo, 0, sizeof(PushInfo_t));
	memset(&m_tBaiduPushInfo, 0, sizeof(PushInfo_t));
	m_dwClientVersion = 0;
	m_bPermissionPass = true;
	LOG_DEBUG(LOG_MAIN, "CView New %p\n", this);
}

CView::~CView()
{
	LOG_DEBUG(LOG_MAIN, "CView delete %p\n", this);
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	if (m_pDataBase) { UnRegisterDataBase(m_pDataBase); m_pDataBase = NULL; }
}

void CView::GetUserStatus(UserStatus_t& tStatus)
{
	tStatus.dwUserID = m_tUserInfo.dwUserID;
	tStatus.dwSessionID = m_dwSessionID;
	tStatus.dwStatus = AT_ONLINE;
	tStatus.bPushType = m_tGtPushInfo.bPushType;
	memcpy(tStatus.szToken, m_tGtPushInfo.szToken, LENGTH_TOKEN);
}

void CView::SetNetConnection(INetConnection* pCon, int nNetType)
{
	if (NULL == pCon) return;
	if (m_pCon) { m_pCon->Disconnect(0); NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	m_pCon = pCon; m_pCon->SetSink(this);
	LOG_DEBUG(LOG_MAIN, "CView::SetNetConnection m_pCon %p m_dwSessionID %d\n", m_pCon, m_dwSessionID);
}

INetConnection* CView::GetNetConnection()
{
	return m_pCon;
}

int CView::OnDisconnect( int nReason, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nReason %d pCon %p\n", __FUNCTION__, nReason, pCon);
	if (NULL == pCon) return -1;
	if (m_pCon != pCon) return -1;

	if (m_tUserInfo.dwUserID) CViewMgr::Instance()->DelViewSession(m_tUserInfo.dwUserID, m_dwSessionID, this);
	IViewHandleSink* pSink = CViewMgr::Instance()->GetSink();
	if (pSink)
	{
		UserStatus_t tStatus; memset(&tStatus, 0, sizeof(UserStatus_t));
		tStatus.dwUserID = m_tUserInfo.dwUserID;
		tStatus.dwSessionID = m_dwSessionID;
		tStatus.dwStatus = AT_OFFLINE;
		tStatus.bPushType = m_tGtPushInfo.bPushType;
		memcpy(tStatus.szToken, m_tGtPushInfo.szToken, LENGTH_TOKEN);
		pSink->View_OnReportUserStatus(tStatus);
	}

	CViewMgr::Instance()->DelElem(m_dwSessionID);
	return 0;
}

int CView::OnReceive( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return OnCommand(pData, nLen, pCon);
}

int CView::OnCommand( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return ProcessCommand(pData, nLen, pCon);
}

int CView::OnPeerIPChange( DWORD dwPeerAddr, WORD wPort, INetConnection *pCon )
{
	if (pCon == NULL) return 0;
	m_tNetInfo.dwPublicIP = dwPeerAddr;
	int nType = pCon->GetType();
	if (NETWORK_CONNECT_TYPE_TCP == nType) m_tNetInfo.wPublicPortTCP = wPort;
	else if (NETWORK_CONNECT_TYPE_UDP == nType) m_tNetInfo.wPublicPortUDP = wPort;
	LOG_DEBUG(LOG_MAIN, "CView::%s dwPeerAddr %d wPort %d Type %d", __FUNCTION__, dwPeerAddr, wPort, nType);
	return 0;
}

bool CView::IsValidVendorID(DWORD dwVendorID)
{
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, false);
	DWORD dwDserverConfigureIndex = 0;
	LIST_SERVERINFO listInfo;
	pHandle->SA_GetDServers(dwVendorID, dwDserverConfigureIndex, listInfo);
	if (listInfo.size() <= 0)
	{
		LOG_DEBUG(LOG_MAIN, "CView::%s false\n", __FUNCTION__);
		return false;
	}
	return true;
}

static const char* g_szTestAccount[] = {
	"17682339368",
	"18518213143",
	"15736749766",
	"13597678942",
	"13971703884",
	"18758565273",
	"18767158276",
	"13750831423",
	"dddemo",
	"ddwuye",
	"606"
};
bool CView::IsTestAccount(PUCHAR pPhone)
{
	for(int i = 0; i < 11; i++)
	{
		if(strcmp(g_szTestAccount[i], (char*)pPhone)) continue;
		LOG_DEBUG(LOG_MAIN, "%s TestAccount: %s\n", __FUNCTION__, pPhone);
		return true;
	}
	LOG_DEBUG(LOG_MAIN, "%s TestAccount: %s\n",__FUNCTION__, pPhone);
	return false;
}

int CView::OnGetUserInfo(UserInfo_t& tInfo, ClientTokenArray_t& tArray)
{
	LOG_DEBUG(LOG_MAIN,"CView::%s\n",__FUNCTION__);
	DWORD dwDserverConfigureIndex = 0;
	LIST_SERVERINFO listInfo;

	WORD wError = ERROR_INVALID_USERNAME;
	if((0 == tInfo.dwUserID) && (1 == m_cUserType))
	{
		BYTE szDigist[LENGTH_CHALLENGE+1] = {0};
		CalcAuthDigistA(szDigist, tInfo.szUserName, tInfo.szUserName, m_szChallenge);
		if (memcmp(szDigist, m_szDigist, LENGTH_CHALLENGE)) wError = ERROR_INVALID_PASSWORD;
		else
		{
			return SendCmd_AuthRep(dwDserverConfigureIndex, wError, (PUCHAR)tInfo.szUrl, tArray);
		}
	}
	else if(tInfo.dwUserID)
	{
		// Digist = md5(UserName +md5(Password,16) + Challenge, 16)
		BYTE szDigist[LENGTH_CHALLENGE+1] = {0};
		CalcAuthDigistA(szDigist, tInfo.szUserName, tInfo.szPassword, m_szChallenge);

		if (memcmp(szDigist, m_szDigist, LENGTH_CHALLENGE)) wError = ERROR_INVALID_PASSWORD;
		else
		{
			wError = ERROR_NO;
			m_bAuth = true;
			memcpy(&m_tUserInfo, &tInfo, sizeof(UserInfo_t));

			IServerAppHandle* pHandle = ServerApp_GetHandle();
			LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
			pHandle->SA_GetDServers(m_dwAppVendorID, dwDserverConfigureIndex, listInfo);

			CViewMgr::Instance()->AddViewSession(m_tUserInfo.dwUserID, m_dwSessionID, this);
			IViewHandleSink* pSink = CViewMgr::Instance()->GetSink();
			if (pSink)
			{
				UserStatus_t tStatus; memset(&tStatus, 0, sizeof(UserStatus_t));
				tStatus.dwUserID = tInfo.dwUserID;
				tStatus.dwSessionID = m_dwSessionID;
				tStatus.dwStatus = AT_ONLINE;
				tStatus.bPushType = m_tGtPushInfo.bPushType;
				memcpy(tStatus.szToken, m_tGtPushInfo.szToken, LENGTH_TOKEN);
				pSink->View_OnReportUserStatus(tStatus);
			}
		}
	}

	DWORD dwDSvrIndex = dwDserverConfigureIndex;
	LOG_DEBUG(LOG_MAIN,"%s UserName=%s\n",__FUNCTION__, tInfo.szUserName);
//	if(!IsTestAccount(tInfo.szUserName)) dwDSvrIndex++;

	return SendCmd_AuthRep(dwDSvrIndex, wError, (PUCHAR)tInfo.szUrl, tArray);
}

int CView::OnGetUserDeviceInfo(DWORD dwUserID, DWORD dwIndex, LIST_DEVICEINFO& listInfo, MAP_DEVROOMINFO& mapDevRoomInfo)
{
	LOG_DEBUG(LOG_MAIN,"CView::%s\n",__FUNCTION__);
	return SendCmd_UserDevice(dwUserID, m_tUserInfo.dwConfigureIndex, listInfo, mapDevRoomInfo);
}

int CView::OnGetUserGroupInfo(DWORD dwUserID, DWORD dwIndex, LIST_GROUPINFO& listInfo)
{
	return SendCmd_UserGroup(dwUserID, m_tUserInfo.dwConfigureIndex, listInfo);
}

int CView::OnGetUserRoomInfo(DWORD dwUserID, DWORD dwIndex, LIST_ROOMINFO& listInfo)
{
	return SendCmd_UserRoom(dwUserID, dwIndex, listInfo);
}

int CView::OnGetIndoorBindDev(LIST_DEVICEINFO& lstDevInfo)
{
	LOG_DEBUG(LOG_MAIN,"[1021] CView::%s\n",__FUNCTION__);
	return SendCmd_IndoorDev(lstDevInfo);//
}

int CView::OnGetDeviceUserInfo(DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo)
{
	return SendCmd_DeviceUser(dwDeviceID, dwIndex, listInfo);
}

int CView::OnAddDevice(int nReason, PUCHAR pUser)
{
	DECLARE_PUTBUFFER( buffer )
	PutVariableStr(buffer, pUser);
	return SendPacket(buffer, CMD_ADD_DEVICE_REP, (WORD)nReason);
}

int CView::OnAddDelPushInfoEx(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo)
{
	if (m_tHeader.payloadversion >= 1) return SendCmd_SetPushInfoEx((WORD)nReason, bOpr, tInfo);
	return SendCmd_SetPushInfo(ERROR_NO, bOpr, tInfo);
}

int CView::OnSetDeviceName(int nReason)
{
	return SendCmd_Error(CMD_SET_DEVICE_NAME_REP, (WORD)nReason);
}

int CView::OnDelDevice(int nReason, DWORD dwUserID, DWORD dwDeviceID)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << dwUserID << dwDeviceID;
	return SendPacket(buffer, CMD_DEL_DEVICE_REP, (WORD)nReason);
}

int CView::OnAuthorize(int nReason, DWORD dwUserID)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << dwUserID;
	return SendPacket(buffer, CMD_AUTHORIZE_REP, (WORD)nReason);
}

int CView::OnAuthorize2(int nReason)
{
	DECLARE_PUTBUFFER( buffer )
	return SendPacket(buffer, CMD_AUTHORIZE2_REP, (WORD)nReason);
}

int CView::OnSetPushSwitch(DWORD dwUserID, int nSwitch)
{
	return SendCmd_SetPushSwitch(dwUserID, nSwitch, ERROR_NO);
}

int CView::ProcessCommand( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN,"CView::%s Len %d\n",__FUNCTION__, nLen);
	if ( false == ParsePacketHeader(pData, nLen, m_tHeader) ) return -1;
	if (m_tHeader.groupcode != GROUPCODE_D_CLIENT)
	{
		LOG_DEBUG(LOG_MAIN, "%s nLen %d Wrong groupcode 0x%02x\n", __FUNCTION__, nLen, m_tHeader.groupcode);
		return -1;
	}
	if ((m_bAuth == false) &&
		(m_tHeader.commandid != CMD_CLIENT_AUTH) &&
		(m_tHeader.commandid != CMD_GET_CHALLENGE)
		)
	{
		LOG_DEBUG(LOG_MAIN, "%s nLen %d Wrong commandid 0x%04x\n", __FUNCTION__, nLen, m_tHeader.commandid);
		return -1;
	}
	//////////////////////////////////////////////////////////////////////////
	static int g_viewHandlersCount = sizeof( mHandlers) / sizeof( mHandlers[0] );
	for ( int i = 0; i < g_viewHandlersCount; ++i )
	{
		if ( mHandlers[i].bCommand != m_tHeader.commandid ) continue;
		PMFHANDLER pHandler = mHandlers[i].pmfHandler;
		return (this->*pHandler)(pData+PACKET_HEADER_SIZE, nLen-PACKET_HEADER_SIZE, pCon);
	}
	return -1;
}

int CView::OnGetChallenge(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d pCon %p\n", __FUNCTION__, nLen, pCon);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	if (m_bPermissionPass == false)
	{
		BYTE szUrl[LENGTH_URL+1] = {0};
		ClientTokenArray_t tArray; memset(&tArray, 0, sizeof(ClientTokenArray_t));
		return SendCmd_AuthRep(0, ERROR_PERMISSION, (PUCHAR)szUrl, tArray);
	}
	return SendCmd_Challenge();
}

int CView::OnLogin( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d pCon %p\n", __FUNCTION__, nLen, pCon);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	
	int nNeedLen = 0;
	BYTE szUserName[LENGTH_NAME+1] = {0};
	CGetBuffer bufferGet(pData, nLen);
	if (false == GetVariableStr(bufferGet, (PUCHAR)szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;

	nNeedLen += LENGTH_CHALLENGE + 2*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwDserverConfigureIndex = 0;
	bufferGet >> CByteArrayBuffer((PUCHAR)m_szDigist, LENGTH_CHALLENGE);
	bufferGet >> dwDserverConfigureIndex >> m_tUserInfo.dwConfigureIndex;
	LOG_DEBUG(LOG_MAIN, "szUserName %s\n", szUserName);
	memcpy(m_tUserInfo.szUserName, szUserName, LENGTH_NAME);//test
	//////////////////////////////////////////////////////////////////////////
	if ( (m_tUserInfo.dwUserID == 0) || memcmp(szUserName, m_tUserInfo.szUserName, LENGTH_NAME) )
	{
		if (m_pDataBase == NULL)
		{
			m_pDataBase = RegisterDataBase(this);
			LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
		}

		//////////////////////////////////////////////////////////////////////////
		// 获取VendorID
		nNeedLen += 2*sizeof(DWORD) + 3*sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_MAIN, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}

		PushInfo_t tInfo; memset(&tInfo, 0, sizeof(PushInfo_t));
		BYTE bOpr = 0;
		bufferGet >> bOpr >> tInfo.dwUserID >> tInfo.dwVendorID >> tInfo.bPushType >> tInfo.bLanguage;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;
		
		// 检查定制APP厂商ID
		m_dwAppVendorID = tInfo.dwVendorID;
		if (false == IsValidVendorID(m_dwAppVendorID))
		{
			BYTE szUrl[LENGTH_URL+1] = {0};
			ClientTokenArray_t tArray; memset(&tArray, 0, sizeof(ClientTokenArray_t));
			return SendCmd_AuthRep(dwDserverConfigureIndex, ERROR_INVALID_VENDORID, (PUCHAR)szUrl, tArray);
		}

		//////////////////////////////////////////////////////////////////////////
		// 客户端版本号
		// 0-不支持转发 1-支持转发
		nNeedLen += sizeof(DWORD);
		if (nLen >= nNeedLen) bufferGet >> m_dwClientVersion;
		else m_dwClientVersion = 0;
		//0: 缺省值，表示手机客户端APP；
		//1: 室内机APP；
		m_cUserType = 0;
		nNeedLen += sizeof(BYTE);
		if(nLen >= nNeedLen)bufferGet >>m_cUserType;
		LOG_DEBUG(LOG_MAIN,"UserType:%d\n",m_cUserType);
		if(m_cUserType == 1)//室内机
		{
			m_bAuth = true;
			memcpy(szIndoorSN,(PUCHAR)szUserName,20);
			LOG_DEBUG(LOG_MAIN,"===>InDoor %s UserType:%d\n",__FUNCTION__, m_cUserType);
			BYTE szUrl[LENGTH_URL+1] = {0};
			ClientTokenArray_t tArray; memset(&tArray, 0, sizeof(ClientTokenArray_t));
			return SendCmd_AuthRep(dwDserverConfigureIndex, ERROR_NO, (PUCHAR)szUrl, tArray);
		}
		else if (-1 == m_pDataBase->GetUserInfo((PUCHAR)szUserName, m_dwAppVendorID))
		{
			BYTE szUrl[LENGTH_URL+1] = {0};
			ClientTokenArray_t tArray; memset(&tArray, 0, sizeof(ClientTokenArray_t));
			return SendCmd_AuthRep(dwDserverConfigureIndex, ERROR_SYSTEM, (PUCHAR)szUrl, tArray);
		}
	}
	else
	{
		ClientTokenArray_t tArray; memset(&tArray, 0, sizeof(ClientTokenArray_t));
		OnGetUserInfo(m_tUserInfo, tArray);
	}

	//////////////////////////////////////////////////////////////////////////
	//int nOffset = bufferGet.GetCurrentOffset();
	//OnAddDelPushInfo(pData+nOffset, nLen-nOffset, pCon);
	return 0;
}

//室内机登录，获取设备列表
int CView::LoginGetDevList(PUCHAR pSN)
{
	LOG_DEBUG(LOG_MAIN,"CView::%s\n",__FUNCTION__);
	//判断是用序列号登录还是用indoorid登录
//	if(strlen(pSN) < 20){
//		DWORD dwIndoorID = atoi(pSN);
//		return -1;
//	}

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->GetIndoorDevList(pSN))
	{
		LIST_DEVICEINFO listInfo;
		MAP_DEVROOMINFO mapDevRoomInfo;
		return 0;
	}
	return 0;
}


int CView::OnReportNetwork( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
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
	m_tNetInfo.listLocalIPs.clear();
	for (WORD i = 0; i < wCount; i++)
	{
		DWORD dwIP = 0;
		bufferGet >> dwIP;
		m_tNetInfo.listLocalIPs.push_back(dwIP);
	}
	bufferGet >> m_tNetInfo.wNetworkType;
	return 0;
}
//获取设备列表1
int CView::OnGetUserDeviceInfo( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "[1001] CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	
	//如果等于1，获取室内机列表
	if(m_cUserType == 1)
	{
		LOG_DEBUG(LOG_MAIN,"[1002] CView::%s\n",__FUNCTION__);
		LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
		m_pDataBase->GetIndoorDevList(szIndoorSN);
		LOG_DEBUG(LOG_MAIN,"[1003] CView::%s\n",__FUNCTION__);
		return 0;
	}
	//////////////////////////////////////////////////////////////////////////
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwUserID = 0;
	
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwUserID;
	if (dwUserID != m_tUserInfo.dwUserID)
	{
		LIST_DEVICEINFO listInfo;
		MAP_DEVROOMINFO mapDevRoomInfo;
		return SendCmd_UserDevice(dwUserID, 0, listInfo, mapDevRoomInfo, ERROR_INVALID_USERID);
	}
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);

	if (-1 == m_pDataBase->GetUserDeviceInfo(dwUserID))
	{
		LIST_DEVICEINFO listInfo;
		MAP_DEVROOMINFO mapDevRoomInfo;
		return SendCmd_UserDevice(dwUserID, 0, listInfo, mapDevRoomInfo,ERROR_SYSTEM);
	}
	return 0;
}

int CView::OnGetUserGroupInfo( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwUserID = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwUserID;
	if (dwUserID != m_tUserInfo.dwUserID)
	{
		LIST_GROUPINFO listInfo;
		return SendCmd_UserGroup(dwUserID, 0, listInfo, ERROR_INVALID_USERID);
	}
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->GetUserGroupInfo(dwUserID))
	{
		LIST_GROUPINFO listInfo;
		return SendCmd_UserGroup(dwUserID, 0, listInfo, ERROR_SYSTEM);
	}
	return 0;
}
//1
int CView::OnGetUserRoomInfo( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwUserID = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwUserID;
	LOG_DEBUG(LOG_MAIN,"UserID:%d\n",dwUserID);
	if (dwUserID != m_tUserInfo.dwUserID)
	{
		LIST_ROOMINFO listInfo;
		return SendCmd_UserRoom(dwUserID, 0, listInfo, ERROR_INVALID_USERID);
	}
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->GetUserRoomInfo(dwUserID))
	{
		LIST_ROOMINFO listInfo;
		return SendCmd_UserRoom(dwUserID, 0, listInfo, ERROR_SYSTEM);
	}
	return 0;
}

// 通知数据库
int CView::OnAddDevice(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = sizeof(DWORD) + LENGTH_SERIALNO;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwUserID = 0;
	BYTE bDevNameLen = 0;
	BYTE szSerialNO[LENGTH_SERIALNO+1] = {0};
	bufferGet >> dwUserID;
	bufferGet >> CByteArrayBuffer((PUCHAR)szSerialNO, LENGTH_SERIALNO);
	
	BYTE szDevName[LENGTH_NAME+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szDevName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	
	// 旧版本默认将设备添加到“0000”房号下
	BYTE szRoom[LENGTH_ROOM+1] = {0};
	memcpy(szRoom, "0000", 4);
	if (nLen > nNeedLen)
	{
		if (false == GetVariableStr(bufferGet, (PUCHAR)szRoom, LENGTH_ROOM, nLen, nNeedLen)) return -1;
	}

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->AddDevice(dwUserID, (PUCHAR)szSerialNO, (PUCHAR)szDevName, (PUCHAR)szRoom))
	{
		return SendCmd_Error(CMD_ADD_DEVICE_REP, ERROR_SYSTEM);
	}
	return 0;
}

const BYTE PUSHINFO_OPR_FORCE_ADD = 1;
const BYTE PUSHINFO_OPR_DEL = 0;
const BYTE PUSHINFO_OPR_KEEP = 2;
const BYTE PUSHINFO_OPR_TRY_ADD = 3;
int CView::OnAddDelPushInfo( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = 2*sizeof(DWORD) + 3*sizeof(BYTE);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	ClientTokenArray_t tInfo; memset(&tInfo, 0, sizeof(ClientTokenArray_t));
	tInfo.nCount = 1;
	tInfo.tToken[0].bMainFlag = 1;

	BYTE bOpr = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> bOpr >> tInfo.dwUserID >> tInfo.dwVendorID >> tInfo.tToken[0].bPushType >> tInfo.bLanguage;
	LOG_DEBUG(LOG_MAIN, "payloadversion %d, bOpr %d dwUserID %d dwVendorID %d bPushType %d bLanguage %d Token %s\n", 
		m_tHeader.payloadversion, bOpr, tInfo.dwUserID, tInfo.dwVendorID, tInfo.tToken[0].bPushType, tInfo.bLanguage, tInfo.tToken[0].szToken);
	if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.tToken[0].szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;
	LOG_DEBUG(LOG_MAIN, "payloadversion %d, bOpr %d dwUserID %d dwVendorID %d bPushType %d bLanguage %d Token %s\n", 
		m_tHeader.payloadversion, bOpr, tInfo.dwUserID, tInfo.dwVendorID, tInfo.tToken[0].bPushType, tInfo.bLanguage, tInfo.tToken[0].szToken);
	if ( (0 == tInfo.dwUserID) || (0 == tInfo.dwVendorID) )
	{
		return SendCmd_SetPushInfo(ERROR_INVALID_USERID, bOpr, tInfo);
	}

	if (bOpr == PUSHINFO_OPR_KEEP) return 0;

	if ( (bOpr != PUSHINFO_OPR_FORCE_ADD) && (bOpr != PUSHINFO_OPR_DEL) && (bOpr != PUSHINFO_OPR_TRY_ADD) )
	{
		return SendCmd_SetPushInfo(ERROR_INVALID_PUSHOPR, bOpr, tInfo);
	}

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->AddDelPushInfoEx(bOpr, tInfo))
	{
		return SendCmd_SetPushInfo(ERROR_SYSTEM, bOpr, tInfo);
	}

	SetThirdPartyPushInfo(tInfo);
	return 0;
}

int CView::OnAddDelPushInfoEx( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d SessionID %d\n", __FUNCTION__, nLen, m_dwSessionID);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	
	int nNeedLen = 2*sizeof(DWORD) + 2*sizeof(BYTE) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	ClientTokenArray_t tInfo; memset(&tInfo, 0, sizeof(ClientTokenArray_t));
	BYTE bOpr = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> bOpr >> tInfo.dwUserID >> tInfo.dwVendorID >> tInfo.bLanguage >> tInfo.nCount;
	if ( (0 == tInfo.dwUserID) || (0 == tInfo.dwVendorID) )
	{
		return SendCmd_SetPushInfoEx(ERROR_INVALID_USERID, bOpr, tInfo);
	}

	if (bOpr == PUSHINFO_OPR_KEEP) return 0;

	if ( (bOpr != PUSHINFO_OPR_FORCE_ADD) && (bOpr != PUSHINFO_OPR_DEL) && (bOpr != PUSHINFO_OPR_TRY_ADD) )
	{
		return SendCmd_SetPushInfoEx(ERROR_INVALID_PUSHOPR, bOpr, tInfo);
	}

	for (int i = 0; i < tInfo.nCount; i++)
	{
		nNeedLen += 2*sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tInfo.tToken[i].bMainFlag >> tInfo.tToken[i].bPushType;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.tToken[i].szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;

		LOG_DEBUG(LOG_MAIN, "bOpr %d dwUserID %d dwVendorID %d bPushType %d bLanguage %d Token %s\n", 
			bOpr, tInfo.dwUserID, tInfo.dwVendorID, tInfo.tToken[i].bPushType, tInfo.bLanguage, tInfo.tToken[i].szToken);

	}
	tInfo.dwView = (DWORD)this;
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->AddDelPushInfoEx(bOpr, tInfo))
	{
		return SendCmd_SetPushInfoEx(ERROR_SYSTEM, bOpr, tInfo);
	}

	SetThirdPartyPushInfo(tInfo);
	return 0;
}

int CView::OnSdkTunnel( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
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

	tTunnel.bSrcType = SRC_TYPE_CLIENT;
	IViewHandleSink* pSink = CViewMgr::Instance()->GetSink();
	if (pSink) pSink->View_OnSdkTunnel(tTunnel);
	return 0;
}

int CView::OnQiniu_GetUploadToken( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	// 用于测试
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	tTag.bSrcType = SRC_TYPE_DEVICE;
	tTag.dwTagID1 = 152;
	tTag.dwTagID2 = 0;
	tTag.dwStoreID = 1;
	IViewHandleSink* pSink = CViewMgr::Instance()->GetSink();
	if (pSink) pSink->View_OnQiniu_GetUploadToken(tTag);
	return 0;
}

int CView::OnQiniu_GetDownloadUrls( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
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

	tTag.bSrcType = SRC_TYPE_CLIENT;
	tTag.dwTagID1 = m_tUserInfo.dwUserID;
	tTag.dwTagID2 = m_dwSessionID;
	tTag.dwStoreID = 0;

	IViewHandleSink* pSink = CViewMgr::Instance()->GetSink();
	if (pSink) pSink->View_OnQiniu_GetDownloadUrls(tTag, tKey);
	return 0;
}

int CView::OnQiniu_GetDownloadUrls2(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 12, -1);
	
	DWORD dwDeviceID, startIndex, Count;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwDeviceID >> startIndex >> Count;
	
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	tTag.bSrcType = SRC_TYPE_CLIENT;
	tTag.dwTagID1 = m_tUserInfo.dwUserID;
	tTag.dwTagID2 = m_dwSessionID;
	tTag.dwStoreID = 0;

	StoreVisitor_t tVisitor; memset(&tVisitor, 0, sizeof(StoreVisitor_t));
	tVisitor.tKey.dwDeviceID = dwDeviceID;
	tVisitor.startIndex = startIndex;
	tVisitor.dwCount    = Count;

	IViewHandleSink* pSink = CViewMgr::Instance()->GetSink();
	if (pSink) pSink->View_OnQiniu_GetDownloadUrls2(tTag, tVisitor);
	return 0;
}

int CView::OnSetPushSwitch(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	
	int nNeedLen = sizeof(DWORD) + sizeof(int);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwUserID = 0;
	int nSwitch = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwUserID >> nSwitch;

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->SetPushSwitch(dwUserID, nSwitch))
	{
		return SendCmd_SetPushSwitch(dwUserID, nSwitch, ERROR_SYSTEM);
	}
	return 0;
}

//绑定门口机
int CView::OnIndoorBindDevice(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN,"===================================================================\n");
	LOG_DEBUG(LOG_MAIN," CView::%s, nLen:%d \n",__FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = LENGTH_SERIALNO + sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	BYTE szIndoorSN[LENGTH_SERIALNO + 1] = {0};
	DWORD dwCount = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> CByteArrayBuffer(szIndoorSN, LENGTH_SERIALNO);
	bufferGet >> dwCount;
	LOG_DEBUG(LOG_MAIN,"SN:%s,Count:%d\n",szIndoorSN, dwCount);
	nNeedLen += dwCount * sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	MAP_DWORD map;
	for(int i = 0; i < dwCount; ++i)
	{
		DWORD dwDeviceID, dwRoomID;
		bufferGet >> dwDeviceID;
		bufferGet >> dwRoomID;
		map.insert(std::make_pair(dwRoomID, dwDeviceID));
		LOG_DEBUG(LOG_MAIN,"Befor DeviceID:%d,RoomID:%d\n",dwDeviceID, dwRoomID);
	}
	BindInfo_t tBindInfo;
	LIST_BIND_INFO listBindInfo;
	MAP_DWORD::iterator iter = map.begin();
	for(; iter != map.end(); iter++)
	{
		memset(&tBindInfo, 0, sizeof(BindInfo_t));
		tBindInfo.dwDeviceID = iter->second;
		tBindInfo.dwRoomID = iter->first;
		LOG_DEBUG(LOG_MAIN,"After DeviceID:%d,RoomID:%d\n",tBindInfo.dwDeviceID, tBindInfo.dwRoomID);
		listBindInfo.push_back(tBindInfo);
	}

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if( -1 == m_pDataBase->IndoorBindDevice(szIndoorSN, listBindInfo))
	{
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);	return -1;
	}
	LOG_DEBUG(LOG_MAIN,"===================================================================\n");
	return 0;
}

//获取设备状态
int CView::OnGetDeviceStatus(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "[2001] CView::%s nLen %d\n", __FUNCTION__, nLen);
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
	LOG_DEBUG(LOG_MAIN,"[2002] UserID:%d SessionID:%d Count:%d\n",tInfo.dwUserID,tInfo.dwSessionID,nCount);

	nNeedLen += nCount * sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	if(m_cUserType == 1)
	{
		LOG_DEBUG(LOG_MAIN,"[2003]  CView::%s\n",__FUNCTION__);
		LIST_DEVSTATUS lstDevStatus;
		for(UINT i = 0; i < nCount; i++)
		{
			DevStatus devStatus;
			bufferGet >> devStatus.dwDeviceID;
			lstDevStatus.push_back(devStatus);
		}
		IDeviceHandleSink* pSink = CDeviceMgr::Instance()->GetSink();
		if (pSink) pSink->Dev_OnGetIndoorBindDevStatus(lstDevStatus);
		LOG_DEBUG(LOG_MAIN,"[2007]  CView::%s\n",__FUNCTION__);
		SendCmd_DevStatus(lstDevStatus);
		LOG_DEBUG(LOG_MAIN,"[2009]  CView::%s\n",__FUNCTION__);
		return 0;
	}

	for (UINT i = 0; i < nCount; i++)
	{
		DWORD dwDeviceID = 0;
		bufferGet >> dwDeviceID;
//		LOG_DEBUG(LOG_MAIN,"DeviceID:%d\n",dwDeviceID);
		tInfo.listDeviceID.push_back(dwDeviceID);
	}
	
	IViewHandleSink* pSink = CViewMgr::Instance()->GetSink();
	if (pSink) pSink->View_OnGetDeviceStatus(tInfo);

	return 0;
}

// 连接设备请求
int CView::OnTransClientInfo(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
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
	LOG_DEBUG(LOG_MAIN, "bSrcType %d dwServerID %d dwUserID %d dwSessionID %d dwDeviceID %d bType %d\n",
		tInfo.bSrcType, tInfo.dwServerID, tInfo.dwUserID, tInfo.dwSessionID, tInfo.dwDeviceID, tInfo.bType);
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

	tInfo.tNetInfo.dwPublicIP = m_tNetInfo.dwPublicIP;
	tInfo.tNetInfo.wPublicPortTCP = m_tNetInfo.wPublicPortTCP;
	tInfo.tNetInfo.wPublicPortUDP = m_tNetInfo.wPublicPortUDP;
	tInfo.tNetInfo.wLocalPortUDP = m_tNetInfo.wLocalPortUDP;
	tInfo.tNetInfo.listLocalIPs = m_tNetInfo.listLocalIPs;
	tInfo.tNetInfo.wNetworkType = m_tNetInfo.wNetworkType;

	//m_cUserType == 1，是室内机
	if(m_cUserType == 1){
		LOG_DEBUG(LOG_MAIN,"%s === indoor ===\n",__FUNCTION__);
		TransServerInfo_t tServerInfo;
		tServerInfo.bSrcType = SRC_TYPE_INDOOR;
		IViewHandleSink* pSink = CViewMgr::Instance()->GetSink();
		if (pSink) pSink->View_OnTransClientInfo(tInfo, (m_dwClientVersion >= 1), tServerInfo);
		LOG_DEBUG(LOG_MAIN,"%d\n",m_cUserType);
		SendCmd_TransServerInfo(tServerInfo);
	}
	else
	{
		LOG_DEBUG(LOG_MAIN,"%s === device ===\n",__FUNCTION__);
		TransServerInfo_t tServerInfo;
		tInfo.bSrcType = SRC_TYPE_CLIENT;
		IViewHandleSink* pSink = CViewMgr::Instance()->GetSink();
		if (pSink) pSink->View_OnTransClientInfo(tInfo, (m_dwClientVersion >= 1), tServerInfo);
	}
	return 0;
}

int CView::OnGetRegisterInfo( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = 3*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwCameraID = 0, dwVendorID = 0, dwConfigureIndex = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwCameraID >> dwVendorID >> dwConfigureIndex;

	LIST_SERVERINFO listInfo;
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->SA_GetDServers(dwVendorID, dwConfigureIndex, listInfo);

	DWORD dwDSvrIndex = dwConfigureIndex;
	LOG_DEBUG(LOG_MAIN,"%s UserName=%s\n", __FUNCTION__, m_tUserInfo.szUserName);
/*	if(!IsTestAccount(m_tUserInfo.szUserName))
	{
		dwDSvrIndex++;
		LOG_DEBUG(LOG_MAIN,"%s Connect 118.178.192.103\n", __FUNCTION__);
		DWORD dwIP = IpStr2Dword((char*)"118.178.192.103");
		LIST_SERVERINFO::iterator pos = listInfo.begin();
		for(; listInfo.end() != pos; ++pos) pos->dwIP = dwIP;
	}
*/
	return SendCmd_RegisterInfo(dwVendorID, dwDSvrIndex, listInfo, pCon);
}

int CView::OnSetDeviceName(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = sizeof(DWORD)*3; char cFrom  = 0;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwVendorID = 0, dwUserID = 0, dwDeviceID = 0;
	bufferGet >> dwVendorID >> dwUserID >> dwDeviceID;
	BYTE szDevName[LENGTH_NAME+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szDevName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	if ( (dwUserID == 0) || (dwDeviceID == 0) ) { cFrom = 1; goto SETFAILED; }
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->SetDeviceName(dwVendorID, dwUserID, dwDeviceID, (PUCHAR)szDevName))
	{
		cFrom = 2; goto SETFAILED;
	}
	return 0;

SETFAILED:
	LOG_DEBUG(LOG_MAIN, "%s failed from %d\n", __FUNCTION__, cFrom);
	return SendCmd_Error(CMD_SET_DEVICE_NAME_REP, ERROR_SYSTEM);
}

int CView::OnDelDevice(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwUserID = 0, dwDeviceID = 0;
	bufferGet >> dwUserID >> dwDeviceID;
	LOG_DEBUG(LOG_MAIN, "dwUserID %d dwDeviceID %d\n", dwUserID, dwDeviceID);

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->DelDevice(dwUserID, dwDeviceID))
	{
		return SendCmd_Error(CMD_DEL_DEVICE_REP, ERROR_SYSTEM);
	}
	return 0;
}

int CView::OnAuthorize(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	BYTE szUserName[LENGTH_NAME+1] = {0};
	DWORD dwOwnerID = 0, dwDeviceID = 0;
	bufferGet >> dwOwnerID;
	if (false == GetVariableStr(bufferGet, (PUCHAR)szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	nNeedLen += sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	bufferGet >> dwDeviceID;
	LOG_DEBUG(LOG_MAIN, "dwOwnerID %d szUserName %s dwDeviceID %d\n", dwOwnerID, szUserName, dwDeviceID);

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->Authorize(dwOwnerID, szUserName, dwDeviceID))
	{
		return SendCmd_Error(CMD_AUTHORIZE, ERROR_SYSTEM);
	}
	return 0;
}

int CView::OnAuthorize2(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	
	DWORD dwUserID = 0, dwDeviceID = 0;
	bufferGet >> dwUserID >> dwDeviceID;
	BYTE szDeviceName[LENGTH_NAME+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szDeviceName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	BYTE szRoom[LENGTH_ROOM+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szRoom, LENGTH_ROOM, nLen, nNeedLen)) return -1;
	LOG_DEBUG(LOG_MAIN, "UserID %d DeviceID %d DeviceName %s Room %s\n", dwUserID, dwDeviceID, szDeviceName, szRoom);

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->Authorize2(dwUserID, dwDeviceID, (PUCHAR)szDeviceName, (PUCHAR)szRoom))
	{
		return SendCmd_Error(CMD_AUTHORIZE, ERROR_SYSTEM);
	}
	return 0;
}

int CView::OnGetDeviceUserInfo(PUCHAR pData, int nLen, INetConnection* pCon)
{
	LOG_DEBUG(LOG_MAIN, "CView::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	bufferGet >> dwDeviceID;
	LOG_DEBUG(LOG_MAIN, "dwDeviceID %d\n", dwDeviceID);
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->GetDeviceUserInfo(dwDeviceID))
	{
		LIST_SMSINFO listInfo;
		return SendCmd_DeviceUser(dwDeviceID, 0, listInfo, ERROR_SYSTEM);
	}
	return 0;
}

int CView::SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg /* = 1 */, WORD wSubSeg /* = 1 */)
{
	int nLen = buffer.GetFilledSize();
	buffer.SetOffset(0);
	// Header
	buffer << (BYTE)m_tHeader.groupcode << (WORD)wCommand/*Command ID*/ << (BYTE)0/*Reserved0*/
		<< (WORD)0/*Version*/ << (WORD)0/*Reserved1*/
		<< (DWORD)m_tHeader.destinationid/*Source ID*/
		<< (DWORD)m_tHeader.sourceid/*Destination ID*/
		<< (DWORD)0/*Command Flag*/
		<< (WORD)wTotalSeg/*Total Segment*/ << (WORD)wSubSeg/*Sub Segment*/
		<< (WORD)0/*Segment Flag*/ << (WORD)0/*Reserved2*/
		<< (DWORD)0/*Reserved3*/;
	// Payload
	buffer << (WORD)wError/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;
	buffer.SetOffset(nLen);
	int nSendLen = m_pCon->SendCommand((PUCHAR)buffer, nLen);
	LOG_DEBUG(LOG_MAIN, "ToView pCon %p SendData cmd:0x%04x err:0x%04x len:%d sendlen:%d\n", m_pCon, wCommand, wError, nLen, nSendLen);
	return nSendLen;
}
//回调给手机端状态
int CView::SendCmd_DevStatus(LIST_DEVSTATUS& lstDevStatus)
{
	LOG_DEBUG(LOG_MAIN,"[2008] CView::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = lstDevStatus.size();
	buffer.Skip(2 * sizeof(DWORD));
	buffer << dwCount;
	LIST_DEVSTATUS::iterator iter = lstDevStatus.begin();
	for(; iter != lstDevStatus.end(); iter++)
	{
		buffer << iter->dwDeviceID << iter->dwStatus;
		LOG_DEBUG(LOG_MAIN,"deviceid:%d,status:%d\n",iter->dwDeviceID, iter->dwStatus);
	}
	SendPacket(buffer, CMD_GET_DEVICE_STATUS_REP, ERROR_NO);
}

int CView::SendCmd_DeviceUser(DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo, WORD wError /* = 0 */)
{
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		buffer << dwDeviceID << dwIndex << dwCount;
		return SendPacket(buffer, CMD_GET_DEVICEUSER_INFO_REP, wError);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DeviceUser(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_SMSINFO::iterator iter = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwIndex << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << iter->dwUserID << iter->dwVendorID << iter->bLanguage;
			PutBase64Str(buffer, (PUCHAR)iter->szMobilePhone);
			PutVariableStr(buffer, (PUCHAR)iter->szDeviceName);
			iter++;
		}
		SendPacket(buffer, CMD_GET_DEVICEUSER_INFO_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CView::SendCmd_TransServerInfo(TransServerInfo_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN,"==>CView::%s bSrcType %d dwServerID %d dwUserID %d dwSessionID %d dwDeviceID %d bType %d\n",
		__FUNCTION__, tInfo.bSrcType, tInfo.dwServerID, tInfo.dwUserID, tInfo.dwSessionID,
		tInfo.dwDeviceID, tInfo.bType);

	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.bSrcType << tInfo.dwServerID << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwDeviceID << tInfo.bType;
	PutBuffer_ConnectInfo(buffer, tInfo.tConnectInfo[0]);
	PutBuffer_ConnectInfo(buffer, tInfo.tConnectInfo[1]);
	return SendPacket(buffer, CMD_TRANS_SERVERINFO, ERROR_NO);
}

int CView::SendCmd_UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID, DWORD dwConfigureIndex)
{
	LOG_DEBUG(LOG_MAIN,"CView::%s\n",__FUNCTION__);
	/*
	m_tUserInfo.dwConfigureIndex = dwConfigureIndex;
	DECLARE_PUTBUFFER( buffer )
	buffer << dwVendorID << dwUserID << dwConfigureIndex;
	SendPacket(buffer, CMD_USER_CONFIGUREINDEX, ERROR_NO);

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->GetDeviceUserInfo(dwDeviceID))
	{
		LIST_SMSINFO listInfo;
		return SendCmd_DeviceUser(dwDeviceID, 0, listInfo, ERROR_SYSTEM);
	}
	*/
	return 0;
}

int CView::SendCmd_RegisterInfo( DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo, INetConnection* pCon )
{
	LOG_ASSERT_RET(LOG_MAIN, pCon, -1);
	struct sockaddr_in* psinPeer = NULL;
	pCon->GetOpt( NETWORK_TRANSPORT_OPT_GET_PEER_ADDR, &psinPeer );
	if (NULL == psinPeer) return -1;
	DWORD dwIP = ntohl(psinPeer->sin_addr.s_addr); // 设备/客户端IP

	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = listInfo.size();
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

int CView::SendCmd_Challenge()
{
	DECLARE_PUTBUFFER(bufferPut)
	GenerateChallenge((PUCHAR)m_szChallenge);
	bufferPut << CByteArrayBuffer((PUCHAR)m_szChallenge, LENGTH_CHALLENGE);
	return SendPacket(bufferPut, CMD_GET_CHALLENGE_REP, ERROR_NO);
}

int CView::SendCmd_AuthRep( DWORD dwDserverConfigureIndex, WORD wError, PUCHAR pUrl, ClientTokenArray_t& tArray )
{
	LOG_ASSERT_RET(LOG_MAIN, m_pCon, -1);
	struct sockaddr_in* psinPeer = NULL;
	m_pCon->GetOpt( NETWORK_TRANSPORT_OPT_GET_PEER_ADDR, &psinPeer );
	LOG_ASSERT_RET(LOG_MAIN, psinPeer, -1);
	DWORD dwPublicIP = ntohl( psinPeer->sin_addr.s_addr );
	WORD wPublicPort = ntohs( psinPeer->sin_port );

	DECLARE_PUTBUFFER( buffer )
	buffer << m_tUserInfo.dwUserID << dwDserverConfigureIndex << m_tUserInfo.dwConfigureIndex << m_dwSessionID;
	PutBase64Str(buffer, (PUCHAR)m_tUserInfo.szMobilePhone);
	buffer << dwPublicIP << wPublicPort;
	PutVariableStr(buffer, pUrl);

	/////////////////////////////////////////////////////
	buffer << tArray.nCount;
	for (int i = 0; i < tArray.nCount; ++i)
	{
		buffer << tArray.tToken[i].bPushType;
		PutVariableStr(buffer, (PUCHAR)tArray.tToken[i].szToken);
	}
	buffer << tArray.nPushSwitch;
	/////////////////////////////////////////////////////

	return SendPacket(buffer, CMD_AUTH_REP, wError);
}
//获取设备列表5
int CView::SendCmd_UserDevice(DWORD dwUserID, DWORD dwIndex, LIST_DEVICEINFO& listInfo, MAP_DEVROOMINFO& mapDevRoomInfo, WORD wError /* = 0 */)
{
	LOG_DEBUG(LOG_MAIN,"CView::%s\n",__FUNCTION__);	
	
	DWORD dwMapCount = mapDevRoomInfo.size();
	DWORD dwCount = listInfo.size();
	LOG_DEBUG(LOG_MAIN,"Count=%d,MapCount=%d\n",dwCount,dwMapCount);

	if (dwCount == 0 || dwMapCount == 0 || dwCount != dwMapCount)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwIndex << dwCount;
		return SendPacket(buffer, CMD_GET_USERDEVICE_INFO_REP, wError);
	}

	LOG_DEBUG(LOG_MAIN,"Count=%d,MapCount=%d\n",dwCount,dwMapCount);
	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_UserDevice(mapDevRoomInfo, listInfo, listCount);

	BYTE szImgVer[LENGTH_IMAGEVERSION+1] = {0};

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_DEVICEINFO::iterator iter = listInfo.begin(), iter2 = listInfo.begin(), iter4 = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwIndex << dwPacketCount;
		
		for (int i = 0; i < dwPacketCount; i++, iter++)
		{
			buffer << iter->dwDeviceID << iter->dwVendorID << iter->dwGroupID;
			buffer << CByteArrayBuffer((PUCHAR)iter->szSerialNO, LENGTH_SERIALNO);
     		PutVariableStr(buffer, (PUCHAR)iter->szDeviceName);
			buffer << CByteArrayBuffer((PUCHAR)iter->szPassword, LENGTH_PASSWORD);	
			LOG_DEBUG(LOG_MAIN,"DeviceID1:%d\n",iter->dwDeviceID);
		}
		//////////////////////////////////////////////////////////////////////////
		for (int i = 0; i < dwPacketCount; i++, iter2++)
		{
			buffer << Device_GetHandle()->Dev_GetDeviceType(iter2->dwDeviceID);
			PUCHAR pVer = Device_GetHandle()->Dev_GetDeviceImgVer(iter2->dwDeviceID);
			if(pVer) buffer << CByteArrayBuffer(pVer, LENGTH_IMAGEVERSION);
			else buffer << CByteArrayBuffer((PUCHAR)szImgVer, LENGTH_IMAGEVERSION);
			buffer << CByteArrayBuffer((PUCHAR)iter2->szRoom, LENGTH_USERROOM);
			LOG_DEBUG(LOG_MAIN,"Deviceid2:%d,Room:%s\n",iter2->dwDeviceID,iter2->szRoom);	
		}
		//////////////////////////////////////////////////////////////////////////
		MAP_DEVROOMINFO::iterator iter3 = mapDevRoomInfo.begin();
		LOG_DEBUG(LOG_MAIN,"--------------------------------------------\n");
		for (int i = 0; i < dwPacketCount; i++, iter4++)
		{
			for(; iter3 != mapDevRoomInfo.end(); iter3++)
			{
				if(iter3->first == iter4->dwDeviceID)
				{
					DWORD dwDeviceID = iter3->first;
					DWORD dwRoomCount = iter3->second.size();
					buffer << dwDeviceID << dwRoomCount;
					LOG_DEBUG(LOG_MAIN,"DeviceID3:%d  Count:%d\n", dwDeviceID, dwRoomCount);
					LIST_ROOMINFO2::iterator it = iter3->second.begin();
					for(; it != iter3->second.end(); it++)
					{
						DWORD dwRoomID = it->dwRoomID;
						buffer << dwRoomID;
						buffer << CByteArrayBuffer(it->szRoom,LENGTH_USERROOM);
						LOG_DEBUG(LOG_MAIN,"RoomID:%d - Room:%s\n",dwRoomID,it->szRoom);
					}
					iter3 = mapDevRoomInfo.begin();
					break;
				}
			}
		}
		LOG_DEBUG(LOG_MAIN,"--------------------------------------------\n");

		SendPacket(buffer, CMD_GET_USERDEVICE_INFO_REP, wError, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CView::SendCmd_UserGroup(DWORD dwUserID, DWORD dwIndex, LIST_GROUPINFO& listInfo, WORD wError /* = 0 */)
{
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		buffer << dwUserID << dwIndex << dwCount;
		return SendPacket(buffer, CMD_GET_USERGROUP_INFO_REP, wError);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_UserGroup(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_GROUPINFO::iterator iter = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwIndex << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << iter->dwGroupID << iter->dwParentID << iter->dwSequence;
			PutVariableStr(buffer, (PUCHAR)iter->szGroupName);
			iter++;
		}
		SendPacket(buffer, CMD_GET_USERGROUP_INFO_REP, wError, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CView::SendCmd_UserRoom(DWORD dwUserID, DWORD dwIndex, LIST_ROOMINFO& listInfo, WORD wError /* = 0 */)
{
	LOG_DEBUG(LOG_MAIN,"CView::%s dwUserID = %d, dwIndex = %d, listRoomInfo.size() = %d, ErrCode = %d\n", __FUNCTION__, dwUserID, dwIndex, listInfo.size(), wError);
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		buffer << dwUserID << dwIndex << dwCount;
		return SendPacket(buffer, CMD_GET_USERROOM_INFO_REP, wError);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_UserRoom(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_ROOMINFO::iterator iter = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwIndex << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << iter->dwRoomID << iter->dwDeviceID;
			buffer << CByteArrayBuffer((PUCHAR)iter->szPassword, LENGTH_PASSWORD);
			PutVariableStr(buffer, (PUCHAR)iter->szRoom);
			LOG_DEBUG(LOG_MAIN,"DevID = %d, RoomID = %d,Room = %s\n", iter->dwDeviceID, iter->dwRoomID, iter->szRoom);
			iter++;
		}
		SendPacket(buffer, CMD_GET_USERROOM_INFO_REP, wError, wTotalSeg, wSubSeg);
	}
	return 0;
}

//获取室内机列表
int CView::SendCmd_IndoorDev(LIST_DEVICEINFO& lstDevInfo)
{
	LOG_DEBUG(LOG_MAIN,"[1022] CView::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = lstDevInfo.size();
	if(dwCount == 0)
	{
		DWORD dwUserID = 0;
		buffer << dwUserID;
		buffer.Skip(sizeof(DWORD));
		buffer << dwCount;
		SendPacket(buffer, CMD_GET_USERDEVICE_INFO_REP, ERROR_NO);
	}
//	buffer << dwUserID << dwIndex << dwPacketCount;
	DWORD dwUserID = 0;
	buffer << dwUserID;
	buffer.Skip(sizeof(DWORD));
	buffer << lstDevInfo.size();

	LIST_DEVICEINFO::iterator iter = lstDevInfo.begin();
	for (; iter != lstDevInfo.end(); iter++)
	{
		buffer << iter->dwDeviceID << iter->dwVendorID << iter->dwGroupID;
		buffer << CByteArrayBuffer((PUCHAR)iter->szSerialNO, LENGTH_SERIALNO);
		PutVariableStr(buffer, (PUCHAR)iter->szDeviceName);
		buffer.Skip(LENGTH_PASSWORD);
		LOG_DEBUG(LOG_MAIN,"DeviceID %d, VendorID %d, GroupID %d, SN %s, DevName %s\n", iter->dwDeviceID, iter->dwVendorID, iter->dwGroupID, iter->szSerialNO, iter->szDeviceName);
	}
	LOG_DEBUG(LOG_MAIN,"[1023] CView::%s\n",__FUNCTION__);
	SendPacket(buffer, CMD_GET_USERDEVICE_INFO_REP, ERROR_NO);
	return 0;
}

int CView::SendCmd_Error(WORD wCommand, WORD wError)
{
	DECLARE_PUTBUFFER( buffer )
	return SendPacket(buffer, wCommand, wError);
}

int CView::SendCmd_SetPushInfo(WORD wError, BYTE bOpr, ClientTokenArray_t& tInfo)
{
	PushInfo_t tPushInfo; memset(&tPushInfo, 0, sizeof(PushInfo_t));
	for (int i = 0; i < tInfo.nCount; i++)
	{
		if (tInfo.tToken[i].bMainFlag)
		{
			tPushInfo.bPushType = tInfo.tToken[i].bPushType;
			memcpy(tPushInfo.szToken, tInfo.tToken[i].szToken, LENGTH_TOKEN+1);
	
			DECLARE_PUTBUFFER( buffer )
			buffer << bOpr << tInfo.dwUserID << tInfo.dwVendorID << tPushInfo.bPushType << tInfo.bLanguage;
			PutVariableStr(buffer, tPushInfo.szToken);
			buffer << tInfo.bLoginOtherPlaceFlag;
			PutVariableStr(buffer, tInfo.szCreated);
			return SendPacket(buffer, CMD_ADD_DEL_PUSH_INFO_REP, wError);
		}
	}
	return 0;
}

int CView::SendCmd_SetPushInfoEx(WORD wError, BYTE bOpr, ClientTokenArray_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "SendCmd_SetPushInfoEx GtToken %s BaiduToken %s HWToken %s MIToken %s JGToken %s MZToken %s SessionID %d bOpr %d LoginOtherPlaceFlag %d Created %s\n", 
		m_tGtPushInfo.szToken, m_tBaiduPushInfo.szToken, m_tHWPushInfo.szToken, m_tMIPushInfo.szToken, m_tJGPushInfo.szToken, m_tMZPushInfo.szToken, m_dwSessionID, bOpr, tInfo.bLoginOtherPlaceFlag, tInfo.szCreated);
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << bOpr << tInfo.dwUserID << tInfo.dwVendorID << tInfo.bLanguage << tInfo.nCount;
	for (int i = 0; i < tInfo.nCount; i++)
	{
		LOG_DEBUG(LOG_MAIN,"====>%d\n",tInfo.tToken[i].bPushType);
		bufferPut << (BYTE)tInfo.tToken[i].bMainFlag << (BYTE)tInfo.tToken[i].bPushType;
		PutVariableStr(bufferPut, (PUCHAR)tInfo.tToken[i].szToken);
	}
	bufferPut << tInfo.bLoginOtherPlaceFlag;
	PutVariableStr(bufferPut, tInfo.szCreated);
	return SendPacket(bufferPut, CMD_ADD_DEL_PUSH_INFO_EX_REP, wError);
}

int CView::SendCmd_SetPushSwitch(DWORD dwUserID, int nSwitch, WORD wError)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwUserID << nSwitch;
	return SendPacket(bufferPut, CMD_PUSH_SWITCH_REP, wError);
}
//3.2.14 室内机绑定门口机
int CView::OnIndoorBindDevice_Rep(BindInfoRep_t& tBindInfo)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	SendCmd_IndoorBindDevice(tBindInfo);
	return 0;
}
//3.2.14 室内机绑定门口机回调
void CView::SendCmd_IndoorBindDevice(BindInfoRep_t& tBindInfo)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << tBindInfo.dwResult << tBindInfo.dwIndoorID;
	LOG_DEBUG(LOG_MAIN,"Result:%d,IndoorID:%d\n",tBindInfo.dwResult, tBindInfo.dwIndoorID);
	SendPacket(buffer,CMD_INDOOR_BIND_DEVICE_REP,ERROR_NO);
}

int CView::SendCmd_DeviceStatus( DWORD dwUserID, DWORD dwSessionID, LIST_DEVICESTATUS& listInfo )
{
	UINT nCount = listInfo.size();
	if (nCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwSessionID << nCount;
		return SendPacket(buffer, CMD_GET_DEVICE_STATUS_REP, ERROR_NO);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_GetDeviceStatusRep(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_DEVICESTATUS::iterator iter = listInfo.begin(), iter2 = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwSessionID << nCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << (DWORD)iter->dwDeviceID << (DWORD)iter->dwStatus;
			iter++;
		}
		for (int j = 0; j < dwPacketCount; j++)
		{
			PutBase64Str(buffer, (PUCHAR)iter2->szStatusMsg);
			iter2++;
		}
		SendPacket(buffer, CMD_GET_DEVICE_STATUS_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CView::SendCmd_DeviceStatus(DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << dwUserID << m_dwSessionID << (UINT)1 << (DWORD)dwDeviceID << (DWORD)dwStatus;
	LOG_DEBUG(LOG_MAIN,"%s dwUserID=%d, SessionID=%d,DeviceID=%d,Status=%d\n",__FUNCTION__,dwUserID, m_dwSessionID, dwDeviceID, dwStatus);
	PutBase64Str(buffer, pStatusMsg);
	LOG_DEBUG(LOG_MAIN, "SendCmd_DeviceStatus StatusMsg:%s\n", pStatusMsg);
	return SendPacket(buffer, CMD_GET_DEVICE_STATUS_REP, ERROR_NO);
}

int CView::SendCmd_SdlTunnelRep(SdkTunnel_t& tInfo)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.bSrcType << tInfo.dwServerID << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwDeviceID << (WORD)tInfo.nTunnelDataLen;
	buffer << CByteArrayBuffer(tInfo.pTunnelData, tInfo.nTunnelDataLen);
	return SendPacket(buffer, CMD_SDK_TUNNEL_REP, ERROR_NO);
}

bool CView::CompareToken( ClientTokenArray_t& tInfo )
{
	if ( (strlen((const char*)m_tGtPushInfo.szToken) <= 0) && (strlen((const char*)m_tBaiduPushInfo.szToken) <= 0) )
	{
		LOG_DEBUG(LOG_MAIN, "CompareToken SessionID %d Token Empty\n", m_dwSessionID);
		return true;
	}
	for (int i = 0; i < tInfo.nCount; i++)
	{
		BYTE bPushType = tInfo.tToken[i].bPushType;
		if ( (bPushType == PUSH_TYPE_IOS_GT) || (bPushType == PUSH_TYPE_ANDROID_GT) )
		{
			LOG_DEBUG(LOG_MAIN, "CompareToken SessionID %d szToken %s myGtToken %s\n", m_dwSessionID, tInfo.tToken[i].szToken, m_tGtPushInfo.szToken);
			if (0 == memcmp(tInfo.tToken[i].szToken, m_tGtPushInfo.szToken, LENGTH_TOKEN+1)) return true;
		}
		else if ( (bPushType == PUSH_TYPE_IOS_BAIDU) || (bPushType == PUSH_TYPE_ANDROID_BAIDU) )
		{
			LOG_DEBUG(LOG_MAIN, "CompareToken SessionID %d szToken %s myBaiduToken %s\n", m_dwSessionID, tInfo.tToken[i].szToken, m_tBaiduPushInfo.szToken);
			if (0 == memcmp(tInfo.tToken[i].szToken, m_tBaiduPushInfo.szToken, LENGTH_TOKEN+1)) return true;
		}
	}
	return false;
}

void CView::SetThirdPartyPushInfo( ClientTokenArray_t& tInfo )
{
	for (int i = 0; i < tInfo.nCount; i++)
	{
		BYTE bPushType = tInfo.tToken[i].bPushType;
		if ( (bPushType == PUSH_TYPE_IOS_GT) || (bPushType == PUSH_TYPE_ANDROID_GT) )
		{
			m_tGtPushInfo.bPushType = bPushType;
			m_tGtPushInfo.dwUserID = m_tUserInfo.dwUserID;
			m_tGtPushInfo.dwVendorID = tInfo.dwVendorID;
			m_tGtPushInfo.bLanguage = tInfo.bLanguage;
			memcpy(m_tGtPushInfo.szToken, tInfo.tToken[i].szToken, LENGTH_TOKEN+1);
			LOG_DEBUG(LOG_MAIN, "SetThirdPartyPushInfo SessionID %d myGtToken %s\n", m_dwSessionID, m_tGtPushInfo.szToken);
		}
		if ( (bPushType == PUSH_TYPE_IOS_BAIDU) || (bPushType == PUSH_TYPE_ANDROID_BAIDU) )
		{
			m_tBaiduPushInfo.bPushType = bPushType;
			m_tBaiduPushInfo.dwUserID = m_tUserInfo.dwUserID;
			m_tBaiduPushInfo.dwVendorID = tInfo.dwVendorID;
			m_tBaiduPushInfo.bLanguage = tInfo.bLanguage;
			memcpy(m_tBaiduPushInfo.szToken, tInfo.tToken[i].szToken, LENGTH_TOKEN+1);
			LOG_DEBUG(LOG_MAIN, "SetThirdPartyPushInfo SessionID %d myBaiduToken %s\n", m_dwSessionID, m_tBaiduPushInfo.szToken);
		}
		//华为
		if ( bPushType == PUSH_TYPE_ANDROID_HW )
		{
			m_tHWPushInfo.bPushType = bPushType;
			m_tHWPushInfo.dwUserID = m_tUserInfo.dwUserID;
			m_tHWPushInfo.dwVendorID = tInfo.dwVendorID;
			m_tHWPushInfo.bLanguage = tInfo.bLanguage;
			memcpy(m_tHWPushInfo.szToken, tInfo.tToken[i].szToken, LENGTH_TOKEN+1);
			LOG_DEBUG(LOG_MAIN, "SetThirdPartyPushInfo SessionID %d myHWToken %s\n", m_dwSessionID, m_tHWPushInfo.szToken);
		}
		//小米
		if ( bPushType == PUSH_TYPE_ANDROID_MI )
		{
			m_tMIPushInfo.bPushType = bPushType;
			m_tMIPushInfo.dwUserID = m_tUserInfo.dwUserID;
			m_tMIPushInfo.dwVendorID = tInfo.dwVendorID;
			m_tMIPushInfo.bLanguage = tInfo.bLanguage;
			memcpy(m_tMIPushInfo.szToken, tInfo.tToken[i].szToken, LENGTH_TOKEN+1);
			LOG_DEBUG(LOG_MAIN, "SetThirdPartyPushInfo SessionID %d myMIToken %s\n", m_dwSessionID, m_tMIPushInfo.szToken);
		}
		//极光
		if ( (bPushType == PUSH_TYPE_IOS_JG) || (bPushType == PUSH_TYPE_ANDROID_JG) )
		{
			m_tJGPushInfo.bPushType = bPushType;
			m_tJGPushInfo.dwUserID = m_tUserInfo.dwUserID;
			m_tJGPushInfo.dwVendorID = tInfo.dwVendorID;
			m_tJGPushInfo.bLanguage = tInfo.bLanguage;
			memcpy(m_tJGPushInfo.szToken, tInfo.tToken[i].szToken, LENGTH_TOKEN+1);
			LOG_DEBUG(LOG_MAIN, "SetThirdPartyPushInfo SessionID %d myJGToken %s\n", m_dwSessionID, m_tJGPushInfo.szToken);
		}
		//魅族
		if ( bPushType == PUSH_TYPE_ANDROID_MZ )
		{
			m_tMZPushInfo.bPushType = bPushType;
			m_tMZPushInfo.dwUserID = m_tUserInfo.dwUserID;
			m_tMZPushInfo.dwVendorID = tInfo.dwVendorID;
			m_tMZPushInfo.bLanguage = tInfo.bLanguage;
			memcpy(m_tMZPushInfo.szToken, tInfo.tToken[i].szToken, LENGTH_TOKEN+1);
			LOG_DEBUG(LOG_MAIN, "SetThirdPartyPushInfo SessionID %d myMZToken %s\n", m_dwSessionID, m_tMZPushInfo.szToken);
		}
	}
}

int CView::SendCmd_Qiniu_DownloadUrlsRep( StorageTag_t& tTag, LIST_STORE_KEYURL& listInfo, WORD wError )
{
	LOG_DEBUG(LOG_MAIN,"========================================================================\n",__FUNCTION__);
	LOG_DEBUG(LOG_MAIN,"CView::%s\n",__FUNCTION__);
	UINT nCount = listInfo.size();
	if (nCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID << nCount;
		return SendPacket(buffer, CMD_GET_DOWNLOAD_URLS_REP, wError);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DownloadUrlsRep(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_STORE_KEYURL::iterator iter = listInfo.begin(), iter2 = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << iter->tKey.dwDeviceID << iter->tKey.dwRoomID << iter->tKey.dwSize
				<< iter->tKey.dwStoreID << iter->tKey.bType << iter->tKey.bRecReason;
			buffer << CByteArrayBuffer((PUCHAR)iter->tKey.szTimeStamp, LENGTH_TIMESTAMP2);
			PutVariableStr(buffer, (PUCHAR)iter->szUrl);
			LOG_DEBUG(LOG_MAIN, "Url:%s\n", iter->szUrl);
			iter++;
		}

		SendPacket(buffer, CMD_GET_DOWNLOAD_URLS_REP, wError, wTotalSeg, wSubSeg);
	}
	LOG_DEBUG(LOG_MAIN,"========================================================================\n");
	return 0;
}

int CView::SendCmd_IndoorStatus(ReportDevStatus& devStatus)
{
	LOG_DEBUG(LOG_MAIN,"CView::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << devStatus.dwDeviceID << devStatus.dwStatus;
	SendPacket(buffer,  CMD_CALL_INDOOR_DEV_REP, ERROR_NO);
	return 0;
}


//////////////////////////////////////////////////////////////////////////
IMPLEMENT_SINGLETON(CViewMgr)
CViewMgr::CViewMgr()
{
	m_pSink = NULL;
	m_dwSessionID = 0;
	m_nPermission = 0;
}

CViewMgr::~CViewMgr()
{
	UnRegisterNetListen(GROUPCODE_D_CLIENT, this);
}

bool CViewMgr::Start()
{
	RegisterNetListen(GROUPCODE_D_CLIENT, this);
	return true;
}

int CViewMgr::OnDispatchConnection( INetConnection* pCon, int nNetType, PUCHAR pData, int nLen )
{
	CView* p = NULL;
	try
	{
		p = new CView(++m_dwSessionID);
	}
	catch(std::bad_alloc &memExp)
	{
		LOG_ASSERT_RET(LOG_MAIN, p, -1);
	}
	AddElem(m_dwSessionID, p);
	p->SetNetConnection(pCon, nNetType);
	p->SetPermissionPass(PermissionPass(m_nPermission));
	p->OnCommand(pData, nLen, pCon);
	return 0;
}

void CViewMgr::View_UserConfigureIndex( DWORD dwVendorID, DWORD dwUserID, DWORD dwConfigureIndex )
{
	LOG_DEBUG(LOG_MAIN, "%s dwVendorID %d dwUserID %d dwConfigureIndex %d\n",
		__FUNCTION__, dwVendorID, dwUserID, dwConfigureIndex);

	ELEM_MAP mapElem; Template_GetViewSession(dwUserID, mapElem);

	//LIST_PUSHINFO lstView;
	ELEM_MAP::iterator pos = mapElem.begin();
	for (; pos != mapElem.end(); pos++)
	{
		CView* p = pos->second;
		if (NULL == p) continue;

		//////////////////////////////////////////////////////////////////////////
		// 检查相同pushtype+token的用户，视为同一个手机客户端
		//g_token.assign((const char*)p->GetToken());
		//g_bPushType = p->GetPushType();
		//if (g_token.size())
		//{
		//	LIST_PUSHINFO::iterator findIter = std::find_if(lstView.begin(), lstView.end(), FindViewByToken2());
		//	if (findIter != lstView.end())
		//	{
		//		LOG_DEBUG(LOG_MAIN, "Same Client pCon %p (pushtype %d token %s) Skip Commmand Notify\n", p->GetNetConnection(), g_bPushType, g_token.c_str());
		//		continue;
		//	}
		//	PushInfo_t tInfo; memset(&tInfo, 0, sizeof(PushInfo_t));
		//	tInfo.bPushType = g_bPushType;
		//	memcpy(tInfo.szToken, p->GetToken(), LENGTH_TOKEN);
		//	lstView.push_back(tInfo);
		//}
		//////////////////////////////////////////////////////////////////////////

		p->SendCmd_UserConfigureIndex(dwVendorID, dwUserID, dwConfigureIndex);
	}
}

void CViewMgr::ReportDeviceStatus(DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg)
{
	LOG_DEBUG(LOG_MAIN, "%s dwUserID %d\n", __FUNCTION__, dwUserID);
	
	//////////////////////////////////////////////////////////////////////////
	if (m_pSink) m_pSink->View_CacheDeviceStatus(dwUserID, dwDeviceID, dwStatus, pStatusMsg);

	//////////////////////////////////////////////////////////////////////////
	BYTE szStatusMsg[LENGTH_MSGCONTENT+1];
	sprintf((char*)szStatusMsg, "C0%s|%lu|%lu", pStatusMsg, dwStatus, dwDeviceID);

	ELEM_MAP mapElem; Template_GetViewSession(dwUserID, mapElem);

	//LIST_PUSHINFO lstView;
	ELEM_MAP::iterator pos = mapElem.begin();
	for (; pos != mapElem.end(); pos++)
	{
		CView* p = pos->second;
		if (NULL == p) continue;

		//g_bPushType = p->GetPushType();
		//g_token.assign((const char*)p->GetToken());

		//////////////////////////////////////////////////////////////////////////
		// 1 检查推送列表中是否有在线用户，有在线用户的过滤推送，直接消息通知
		//LIST_PUSHINFO::iterator pos2 = std::find_if(list2.begin(), list2.end(), FindViewByToken2());
		//if (pos2 != list2.end())
		//{
		//	LOG_DEBUG(LOG_MAIN, "Skip Push, Commmand Notify! User(pushtype %d token %s) Online\n", g_bPushType, g_token.c_str());
		//	list2.erase(pos2);
		//}
		//////////////////////////////////////////////////////////////////////////
		
		//////////////////////////////////////////////////////////////////////////
		// 2 检查相同pushtype+token的用户，视为同一个手机客户端
		//if (g_token.size())
		//{
		//	LIST_PUSHINFO::iterator findIter = std::find_if(lstView.begin(), lstView.end(), FindViewByToken2());
		//	if (findIter != lstView.end())
		//	{
		//		LOG_DEBUG(LOG_MAIN, "Same Client pCon %p (pushtype %d token %s) Skip Commmand Notify\n", p->GetNetConnection(), g_bPushType, g_token.c_str());
		//		continue;
		//	}
		//	PushInfo_t tInfo; memset(&tInfo, 0, sizeof(PushInfo_t));
		//	tInfo.bPushType = g_bPushType;
		//	memcpy(tInfo.szToken, p->GetToken(), LENGTH_TOKEN);
		//	lstView.push_back(tInfo);
		//}
		//////////////////////////////////////////////////////////////////////////
		p->SendCmd_DeviceStatus(dwUserID, dwDeviceID, dwStatus, (PUCHAR)szStatusMsg);
		ReportDevStatus devStatus;
		memset(&devStatus, 0, sizeof(ReportDevStatus));
		devStatus.dwDeviceID = dwDeviceID;
		devStatus.dwStatus = dwStatus;
		View_ReportIndoorStatus(dwUserID,devStatus);
	}
}

void CViewMgr::View_ReportDeviceStatus(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);

	MAP_DWORD_STRING mapTemp;
	LIST_SMSINFO::iterator iter = list1.begin();
	for (; iter != list1.end(); iter++)
	{
		CSTRING devicename; devicename.assign((const char*)iter->szDeviceName);
		mapTemp.insert(std::make_pair(iter->dwUserID, devicename));
	}
	LIST_PUSHINFO::iterator pos = list2.begin();
	for (; pos != list2.end(); pos++)
	{
		CSTRING devicename; devicename.assign((const char*)pos->szDeviceName);
		mapTemp.insert(std::make_pair(pos->dwUserID, devicename));
	}

	MAP_DWORD_STRING::iterator iterUserID = mapTemp.begin();
	for (; iterUserID != mapTemp.end(); iterUserID++)
	{
		BYTE szStatusMsg[LENGTH_MSGCONTENT+1];
		GenerateDeviceStatusMsg((PUCHAR)iterUserID->second.c_str(), pMsg, pTimeStamp, (PUCHAR)szStatusMsg);
		ReportDeviceStatus(iterUserID->first, dwDeviceID, dwStatus, (PUCHAR)szStatusMsg);
	}
}


void CViewMgr::View_GetDeviceStatusRep(GetDeviceStatusRep_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CView* p = Template_GetViewSession(tInfo.dwUserID, tInfo.dwSessionID);
	if (p) p->SendCmd_DeviceStatus(tInfo.dwUserID, tInfo.dwSessionID, tInfo.listInfo);
}

void CViewMgr::View_TransServerInfo(TransServerInfo_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CView* p = Template_GetViewSession(tInfo.dwUserID, tInfo.dwSessionID);
	if (p) p->SendCmd_TransServerInfo(tInfo);
}

void CViewMgr::View_GetUserList( LIST_USERSTATUS& listInfo )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
}

void CViewMgr::AddViewSession( DWORD dwUserID, DWORD dwSessionID, CView* pView )
{
	if (NULL == pView) return;

	// 查找相同的pushtype+token
	//CView* pSameView = GetSameView(dwUserID, dwSessionID - 1, pView);
	//if (pSameView)
	//{
	//	int nNetType = NETWORK_CONNECT_TYPE_TCP;
	//	INetConnection* pCon = pView->GetNetConnection(nNetType);
	//	if (pCon) pSameView->SetNetConnection(pCon, nNetType);
	//	DelElem(dwSessionID);
	//	LOG_DEBUG(LOG_MAIN, "Find Same ViewSession\n");
	//	return;
	//}

	Template_AddViewSession(dwUserID, dwSessionID, pView);
}

void CViewMgr::DelViewSession( DWORD dwUserID, DWORD dwSessionID, CView* pView )
{
	Template_DelViewSession(dwUserID, dwSessionID, pView);
}

void CViewMgr::View_LoginOtherPlace(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	ELEM_MAP mapElem; Template_GetViewSession(tInfo.dwUserID, mapElem);

	ELEM_MAP::iterator pos = mapElem.begin();
	for (; pos != mapElem.end(); pos++)
	{
		// 过滤相同token值的客户端
		if (pos->second->CompareToken(tInfo))
		{
			LOG_DEBUG(LOG_MAIN, "%s continue\n", __FUNCTION__);
			continue;
		}
		pos->second->SendCmd_SetPushInfoEx((WORD)nReason, bOpr, tInfo);
	}
}

void CViewMgr::View_Permission( int nPermission )
{
	LOG_DEBUG(LOG_MAIN, "%s nPermission %d\n", __FUNCTION__, nPermission);
	m_nPermission = nPermission;
}

void CViewMgr::View_SdkTunnelRep( SdkTunnel_t& tInfo )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CView* p = Template_GetViewSession(tInfo.dwUserID, tInfo.dwSessionID);
	if (p) p->SendCmd_SdlTunnelRep(tInfo);
}

void CViewMgr::View_Qiniu_UploadToken(StorageTag_t& tTag, PUCHAR pUploadToken)
{

}

void CViewMgr::View_Qiniu_DownloadUrls(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo)
{
	LOG_DEBUG(LOG_MAIN, "CViewMgr::%s\n", __FUNCTION__);
	CView* p = Template_GetViewSession(tTag.dwTagID1, tTag.dwTagID2);
	if (p) p->SendCmd_Qiniu_DownloadUrlsRep(tTag, lstInfo, ERROR_NO);
}

void CViewMgr::View_ReportIndoorStatus(DWORD dwUserID, ReportDevStatus& devStatus)
{
	LOG_DEBUG(LOG_MAIN, "CViewMgr::%s\n", __FUNCTION__);
	CView* p = Template_GetViewSession(dwUserID, m_dwSessionID);
	if(p) p->SendCmd_IndoorStatus(devStatus);
}