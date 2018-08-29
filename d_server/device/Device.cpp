#include "Device.h"
#include "ServerAppInterface.h"
#include "getbuffer.h"
#include "Log.h"
#include <errno.h>
#include <unistd.h>

const WORD TIMER_CACHE_STATUS = 10;

const WORD g_wCurHeaderVersion = 1;
const BYTE g_bCurPayloadVersion = 1;

const BYTE UpdateDevRoomType_UserIndex = 1;
const BYTE UpdateDevRoomType_PushIndex = 2;
const BYTE UpdateDevRoomType_CardIndex = 4;
const BYTE UpdateDevRoomType_Other = 8;
const BYTE UpdateDevRoomType_Delete = 16;
const BYTE UpdateDevRoomType_PushSwitchIndex = 32;

const BYTE RoomIndexType_UserIndex = 0;
const BYTE RoomIndexType_PushIndex = 1;
const BYTE RoomIndexType_CardIndex = 2;
const BYTE RoomIndexType_Other = 3;
const BYTE RoomIndexType_PushSwitchIndex = 4;

const int UpdateDevCfgType_Ucpaas = 1;
const int UpdateDevCfgType_PhoneNum = 2;

const int AT_DOOR_UNLOCK = 13;
const int AT_DOOR_LOCK = 14;

BYTE CDevice::m_szBuffer[MAX_PACKET_LEN] = {0};
const CDevice::HandlerEntry CDevice::mHandlers[] =
{
	{ CMD_REGISTER,					&CDevice::OnRegister			},
	{ CMD_REPORT_NETWORK,			&CDevice::OnReportNetwork		},
	{ CMD_REPORT_DEVICE_STATUS,		&CDevice::OnReportDeviceStatus	},
	{ CMD_GET_DEVICEUSER_INFO,		&CDevice::OnGetDeviceUserInfo	},
	{ CMD_GET_DEVICEPUSH_INFO,		&CDevice::OnGetDevicePushInfo	},
	{ CMD_GET_REGISTER_INFO,		&CDevice::OnGetRegisterInfo		},

	{ CMD_GET_DEVICE_ROOM_SUM,			&CDevice::OnGetDeviceRoomSum			},
	{ CMD_GET_DEVICE_ROOM_SUM_REP_ACK,	&CDevice::OnGetDeviceRoomSumRepAck	},
	{ CMD_GET_DEVICE_ROOMINFO,			&CDevice::OnGetDeviceRoomInfo		},
	{ CMD_GET_DEVICE_ROOMINFO_REP_ACK,	&CDevice::OnGetDeviceRoomInfoRepAck	},

	{ CMD_SDK_TUNNEL_REP,			&CDevice::OnSdkTunnel				},

	{ CMD_GET_UPLOAD_TOKEN,			&CDevice::OnQiniu_GetUploadToken	},
	{ CMD_GET_DOWNLOAD_URLS,		&CDevice::OnQiniu_GetDownloadUrls	},
	{ CMD_REPORT_UPLOAD_RESULT,		&CDevice::OnQiniu_ReportUploadResult},
	{ CMD_GET_DEVICE_CFG,			&CDevice::OnGetDeviceCfg	},
	{ DD_CMD_GET_SERVERTIME,		&CDevice::OnGetServerTime	},
	{ DD_PUSH_UNLOCK_RECORD,		&CDevice::OnReportUnlockLog	},
	{ DD_UPDATE_BULLETIN,			&CDevice::OnGetNoticeIndex  },
	{ CMD_GET_ADVERT_INFO,			&CDevice::OnGetAdvertIndex	},
	{ CMD_UPDATE_VISITORCFG,	&CDevice::OnGetVisitorCfg	},
//	{ CMD_REPORT_UPLOAD_RESULT,		&CDevice::OnQiniu_ReportUploadResult2},//test d_sdb
	{ CMD_GET_BIND_DEV_STATE,		&CDevice::OnGetBindDevStatus	},
	{ CMD_CALL_INDOOR_DEV,			&CDevice::OnCallIndoorDev	},
	//storagebusiness
	{ CMD_GET_ADVERTURLS,			&CDevice::OnGetAdvertByID2     }, //通过ADID更新广告
	{ CMD_REPORT_PROGRESS,		&CDevice::OnReportDownloadProgress2	},
};

CDevice::CDevice(DWORD dwClientID)
{
	m_dwClientID = dwClientID;
	m_pCon = NULL;
	m_pDataBase = NULL;
	m_pStorageDB = NULL;
	memset(&m_tHeader, 0, sizeof(PacketHeader_t));
	memset(&m_tDeviceInfo, 0, sizeof(DeviceInfo_t));
	m_wHeaderVersion = 0;
	m_dwCmdFlag = 1;
	m_wSegFlag = 1;
#ifdef CACHE_DEVICE_STATUS
	AddTimer(TIMER_NORMAL, TIMER_CACHE_STATUS, this);
#endif
}

CDevice::~CDevice()
{
#ifdef CACHE_DEVICE_STATUS
	DelTimer(TIMER_NORMAL, this);
#endif
	DelMulPkt(this);
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	if (m_pDataBase) { UnRegisterDataBase(m_pDataBase); m_pDataBase = NULL; }
	if (m_pStorageDB) { UnRegStorageDB(m_pStorageDB); m_pStorageDB = NULL; }
	if (m_pStorageB)	{UnRegStorageBusiness(m_pStorageB); m_pStorageB = NULL;	}
}

void CDevice::SetNetConnection( INetConnection* pCon )
{
	if (NULL == pCon) return;
	if (m_pCon == pCon) return;
	if (m_pCon) { m_pCon->Disconnect(0); NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	m_pCon = pCon; m_pCon->SetSink(this);
}

int CDevice::OnDisconnect( int nReason, INetConnection* pCon )
{
	if (pCon != m_pCon) return -1;
	LOG_DEBUG(LOG_MAIN, "CDevice::%s nReason %d pCon %p DeviceID %d\n", __FUNCTION__, nReason, pCon, m_tDeviceInfo.dwDeviceID);
	DelMulPkt(this);

	CDeviceMgr* pDevMgr = CDeviceMgr::Instance();
	if(IsYunShangCheng()) pDevMgr->AddNotify_OnOff(m_tDeviceInfo.szSerialNO, 0);
	pDevMgr->DelDevice(m_tDeviceInfo.dwDeviceID);
	pDevMgr->DelElem(m_dwClientID);

	return 0;
}

int CDevice::OnReceive( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return OnCommand(pData, nLen, pCon);
}

int CDevice::OnCommand( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return ProcessCommand(pData, nLen);
}

int CDevice::OnPeerIPChange( DWORD dwPeerAddr, WORD wPort, INetConnection *pCon )
{
	if (pCon == NULL) return 0;
	m_tNetInfo.dwPublicIP = dwPeerAddr;
	m_tNetInfo.wPublicPortTCP = wPort;
	m_tNetInfo.wPublicPortUDP = wPort;
	LOG_DEBUG(LOG_MAIN, "CDevice::%s dwPeerAddr %d wPort %d\n", __FUNCTION__, dwPeerAddr, wPort);
	return 0;
}

int CDevice::ProcessCommand( PUCHAR pData, int nLen )
{
	if ( false == ParsePacketHeader(pData, nLen, m_tHeader) ) return -1;
	if (m_tHeader.groupcode != GROUPCODE_D_DEVICE) return -1;
	//////////////////////////////////////////////////////////////////////////
	static int g_cameraHandlersCount = sizeof( mHandlers) / sizeof( mHandlers[0] );
	for ( int i = 0; i < g_cameraHandlersCount; ++i )
	{
		if ( mHandlers[i].bCommand != m_tHeader.commandid ) continue;
		PMFHANDLER pHandler = mHandlers[i].pmfHandler;
		return (this->*pHandler)(pData+PACKET_HEADER_SIZE, nLen-PACKET_HEADER_SIZE);
	}
	return -1;
}

bool CDevice::IsYunShangCheng()
{
	if(17 != m_tDeviceInfo.dwVendorID) return false;
	DWORD dwSNID = SN2ID((char*)m_tDeviceInfo.szSerialNO);
	if((dwSNID < 800001) || (dwSNID > 810000)) {
		LOG_DEBUG(LOG_MAIN, "%s err id=%d sn=%s\n", __FUNCTION__, dwSNID, m_tDeviceInfo.szSerialNO);	
		return false;
	}
	return true;
}

int CDevice::OnGetDeviceInfo(DeviceInfo_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN,"Registered | %s \n",__FUNCTION__ );
	DWORD dwDserverConfigureIndex = 0;
	LIST_SERVERINFO listServerInfo;
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	CDeviceMgr* pDevMgr = CDeviceMgr::Instance();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->SA_GetDServers(tInfo.dwVendorID, dwDserverConfigureIndex, listServerInfo);

	tInfo.dwDeviceType = m_tDeviceInfo.dwDeviceType;
	memcpy((PUCHAR)tInfo.szImageVer, (PUCHAR)m_tDeviceInfo.szImageVer, LENGTH_IMAGEVERSION);

	WORD wError = ERROR_INVALID_SERIALNO;
	if (tInfo.dwDeviceID != 0)
	{
		LOG_DEBUG(LOG_MAIN, "%s_1 cursn=%s insn=%s\n", __FUNCTION__, m_tDeviceInfo.szSerialNO, tInfo.szSerialNO);	
		memcpy(&m_tDeviceInfo, &tInfo, sizeof(DeviceInfo_t));
		LOG_DEBUG(LOG_MAIN, "%s_2 cursn=%s\n", __FUNCTION__, m_tDeviceInfo.szSerialNO);	

		wError = ERROR_NO;
		pDevMgr->AddDevice(m_tDeviceInfo.dwDeviceID, this);

		LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
		m_pDataBase->GetDeviceRoomSum(m_tDeviceInfo.dwDeviceID);
		// m_pDataBase->GetDeviceCfg(m_tDeviceInfo.dwDeviceID, UpdateDevCfgType_Ucpaas);

		if(IsYunShangCheng()) pDevMgr->AddNotify_OnOff(m_tDeviceInfo.szSerialNO, 1);
	}
	return SendCmd_RegisterRep(dwDserverConfigureIndex, wError);
}

int CDevice::OnGetDeviceUserInfo(DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo)
{
	if (dwDeviceID != m_tDeviceInfo.dwDeviceID) return 0;
	return SendCmd_DeviceUser(dwDeviceID, dwIndex, listInfo);
}

int CDevice::OnGetDevicePushInfo(DWORD dwDeviceID, DWORD dwIndex, LIST_PUSHINFO& listInfo)
{
	if (dwDeviceID != m_tDeviceInfo.dwDeviceID) return 0;
	return SendCmd_DevicePush(dwDeviceID, dwIndex, listInfo);
}

int CDevice::OnGetDeviceRoomSum( DWORD dwDeviceID, LIST_ROOMSUM& lstInfo, WORD totalsegment, WORD subsegment )
{
	if (dwDeviceID != m_tDeviceInfo.dwDeviceID) return 0;

	MAP_ROOMSUM::iterator iter = m_mapDeviceRoomSum.find(subsegment);
	if (iter != m_mapDeviceRoomSum.end())
	{
		iter->second.clear();
		iter->second.insert(iter->second.end(), lstInfo.begin(), lstInfo.end());
	}
	else
	{
		m_mapDeviceRoomSum.insert(std::make_pair(subsegment, lstInfo));
	}

	if ( (totalsegment == subsegment) && (totalsegment == m_mapDeviceRoomSum.size()) )
	{
		LIST_ROOMSUM lstTempInfo;
		MAP_ROOMSUM::iterator pos = m_mapDeviceRoomSum.begin();
		for (; pos != m_mapDeviceRoomSum.end(); pos++)
		{
			lstTempInfo.insert(lstTempInfo.end(), pos->second.begin(), pos->second.end());
		}
		SendCmd_DeviceRoomSum(dwDeviceID, lstTempInfo);
		m_mapDeviceRoomSum.clear();
	}
	return 0;
}

int CDevice::OnGetDeviceRoomUser( DWORD dwDeviceID, LIST_ROOMUSER& lstInfo, WORD totalsegment, WORD subsegment )
{
	if (dwDeviceID != m_tDeviceInfo.dwDeviceID) return 0;
	MAP_ROOMUSER::iterator iter = m_mapDeviceRoomUser.find(subsegment);
	if (iter != m_mapDeviceRoomUser.end())
	{
		iter->second.clear();
		iter->second.insert(iter->second.end(), lstInfo.begin(), lstInfo.end());
	}
	else
	{
		m_mapDeviceRoomUser.insert(std::make_pair(subsegment, lstInfo));
	}

	if ( (totalsegment == subsegment) && (totalsegment == m_mapDeviceRoomUser.size()) )
	{
		LIST_ROOMUSER lstTempInfo;
		MAP_ROOMUSER::iterator pos = m_mapDeviceRoomUser.begin();
		for (; pos != m_mapDeviceRoomUser.end(); pos++)
		{
			lstTempInfo.insert(lstTempInfo.end(), pos->second.begin(), pos->second.end());
		}
		SendCmd_DeviceRoomUser(dwDeviceID, lstTempInfo);
		m_mapDeviceRoomUser.clear();
	}
	return 0;
}

int CDevice::OnGetDeviceRoomPush( DWORD dwDeviceID, LIST_ROOMPUSH& lstInfo, WORD totalsegment, WORD subsegment )
{
	if (dwDeviceID != m_tDeviceInfo.dwDeviceID) return 0;
	MAP_ROOMPUSH::iterator iter = m_mapDeviceRoomPush.find(subsegment);
	if (iter != m_mapDeviceRoomPush.end())
	{
		iter->second.clear();
		iter->second.insert(iter->second.end(), lstInfo.begin(), lstInfo.end());
	}
	else
	{
		m_mapDeviceRoomPush.insert(std::make_pair(subsegment, lstInfo));
	}

	if ( (totalsegment == subsegment) && (totalsegment == m_mapDeviceRoomPush.size()) )
	{
		LIST_ROOMPUSH lstTempInfo;
		MAP_ROOMPUSH::iterator pos = m_mapDeviceRoomPush.begin();
		for (; pos != m_mapDeviceRoomPush.end(); pos++)
		{
			lstTempInfo.insert(lstTempInfo.end(), pos->second.begin(), pos->second.end());
		}
		SendCmd_DeviceRoomPush(dwDeviceID, lstTempInfo);
		m_mapDeviceRoomPush.clear();
	}
	return 0;
}

int CDevice::OnGetDeviceRoomCard( DWORD dwDeviceID, LIST_ROOMCARD& lstInfo, WORD totalsegment, WORD subsegment )
{
	if (dwDeviceID != m_tDeviceInfo.dwDeviceID) return 0;
	MAP_ROOMCARD::iterator iter = m_mapDeviceRoomCard.find(subsegment);
	if (iter != m_mapDeviceRoomCard.end())
	{
		iter->second.clear();
		iter->second.insert(iter->second.end(), lstInfo.begin(), lstInfo.end());
	}
	else
	{
		m_mapDeviceRoomCard.insert(std::make_pair(subsegment, lstInfo));
	}

	if ( (totalsegment == subsegment) && (totalsegment == m_mapDeviceRoomCard.size()) )
	{
		LIST_ROOMCARD lstTempInfo;
		MAP_ROOMCARD::iterator pos = m_mapDeviceRoomCard.begin();
		for (; pos != m_mapDeviceRoomCard.end(); pos++)
		{
			lstTempInfo.insert(lstTempInfo.end(), pos->second.begin(), pos->second.end());
		}
		SendCmd_DeviceRoomCard(dwDeviceID, lstTempInfo);
		m_mapDeviceRoomCard.clear();
	}
	return 0;
}

int CDevice::OnGetDeviceRoomOther( DWORD dwDeviceID, LIST_ROOMOTHER& lstInfo, WORD totalsegment, WORD subsegment )
{
	if (dwDeviceID != m_tDeviceInfo.dwDeviceID) return 0;
	MAP_ROOMOTHER::iterator iter = m_mapDeviceRoomOther.find(subsegment);
	if (iter != m_mapDeviceRoomOther.end())
	{
		iter->second.clear();
		iter->second.insert(iter->second.end(), lstInfo.begin(), lstInfo.end());
	}
	else
	{
		m_mapDeviceRoomOther.insert(std::make_pair(subsegment, lstInfo));
	}

	if ( (totalsegment == subsegment) && (totalsegment == m_mapDeviceRoomOther.size()) )
	{
		LIST_ROOMOTHER lstTempInfo;
		MAP_ROOMOTHER::iterator pos = m_mapDeviceRoomOther.begin();
		for (; pos != m_mapDeviceRoomOther.end(); pos++)
		{
			lstTempInfo.insert(lstTempInfo.end(), pos->second.begin(), pos->second.end());
		}
		SendCmd_DeviceRoomOther(dwDeviceID, lstTempInfo);
		m_mapDeviceRoomOther.clear();
	}
	return 0;
}

//2.2.17  获取室内机信息
int CDevice::OnGetDeviceRoomIndoor(DWORD dwDeviceID, LIST_ROOMINDOOR2& lstInfo, WORD totalsegment, WORD subsegment)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	if (dwDeviceID != m_tDeviceInfo.dwDeviceID) return 0;
	MAP_ROOMINDOOR::iterator iter = m_mapDeviceRoomIndoor.find(subsegment);
	if (iter != m_mapDeviceRoomIndoor.end())
	{
		iter->second.clear();
		iter->second.insert(iter->second.end(), lstInfo.begin(), lstInfo.end());
	}
	else
	{
		m_mapDeviceRoomIndoor.insert(std::make_pair(subsegment, lstInfo));
	}

	if ( (totalsegment == subsegment) && (totalsegment == m_mapDeviceRoomIndoor.size()) )
	{
		LIST_ROOMINDOOR2 lstTempInfo;
		MAP_ROOMINDOOR::iterator pos = m_mapDeviceRoomIndoor.begin();
		for (; pos != m_mapDeviceRoomIndoor.end(); pos++)
		{
			lstTempInfo.insert(lstTempInfo.end(), pos->second.begin(), pos->second.end());
		}
		SendCmd_DeviceRoomIndoor(dwDeviceID, lstTempInfo);
		m_mapDeviceRoomIndoor.clear();
	}
	return 0;
}
//公告
int CDevice::OnGetBulletinIndex(DWORD dwNoticeIndex)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << dwNoticeIndex;
	LOG_DEBUG(LOG_MAIN,"NoticeIndex=%d\n",dwNoticeIndex);
	return SendPacket(buffer, DD_UPDATE_BULLETIN_REP, ERROR_NO);
}
//广告
int CDevice::OnGetAdvertIndex(DWORD dwAdvertIndex, DWORD dwAdvertType)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << dwAdvertIndex << dwAdvertType;
	LOG_DEBUG(LOG_MAIN,"AdvertIndex=%d\n",dwAdvertIndex);
	return SendPacket(buffer, CMD_GET_ADVERT_INFO_REP, ERROR_NO);
}
//访客配置
int CDevice::OnGetVisitorCfg(DWORD dwVisitorCfg)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << dwVisitorCfg;
	LOG_DEBUG(LOG_MAIN,"dwVisitorCfg=%d\n",dwVisitorCfg);
	return SendPacket(buffer, CMD_UPDATE_VISITORCFG_REP, ERROR_NO);
}
//StorageBusiness
int CDevice::OnGetAdvertUrls_Rep(AdvertInfoRep_t& tAdvertRep)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = 1;
	buffer << dwCount;
	buffer << tAdvertRep.tInfo.dwAdID << tAdvertRep.tInfo.dwAdType
		<< tAdvertRep.tInfo.dwUsePosition << tAdvertRep.tInfo.dwTimeStamp;
	PutVariableStr(buffer, tAdvertRep.tInfo.szFormat);
	PutVariableStr(buffer, tAdvertRep.szUrl);
	LOG_DEBUG(LOG_MAIN, "AdID %d, ApType %d, UPosition %d, Time %d, Format %s\n", 
		tAdvertRep.tInfo.dwAdID, tAdvertRep.tInfo.dwAdType, tAdvertRep.tInfo.dwUsePosition, tAdvertRep.tInfo.dwTimeStamp, tAdvertRep.tInfo.szFormat);
	LOG_DEBUG(LOG_MAIN, "AdUrl = %s\n", tAdvertRep.szUrl);
	return SendPacket(buffer, CMD_GET_ADVERTURLS_REP, ERROR_NO);
}


int CDevice::OnGetDeviceCfg(DWORD dwDeviceID, int nType, UcpaasInfo_t& tUcpaas)
{
	if (dwDeviceID != m_tDeviceInfo.dwDeviceID) return 0;
	return SendCmd_DeviceCfg(dwDeviceID, nType, tUcpaas);
}
int CDevice::OnGetSystemCfg(DWORD dwVendorID, int nType, SystemCfg_t& tCfgInfo)
{
	LOG_DEBUG(LOG_MAIN,"UserID:%s | AccountSid:%s | AuthToken:%s | AppKey:%s | PhoneNumber:%s\n",tCfgInfo.szUserID, tCfgInfo.szAccountSid, tCfgInfo.szAuthToken, tCfgInfo.szAppKey, tCfgInfo.szPhoneNumber);
	if (dwVendorID != m_tDeviceInfo.dwVendorID) return 0;
	return SendCmd_SystemCfg(dwVendorID, nType, tCfgInfo);
}
//回调室内机绑定的门口机在线离线状态
int CDevice::OnGetIndoorBindDevStatus(LIST_DEVSTATUS& lstDevStatus)
{
	LOG_DEBUG(LOG_MAIN,"[1011] CDevice::%s\n",__FUNCTION__);
	LIST_DEVSTATUS::iterator iter = lstDevStatus.begin();
	IDeviceHandleSink* pSink = CDeviceMgr::Instance()->GetSink();
	if (pSink) pSink->Dev_OnGetIndoorBindDevStatus(lstDevStatus);
	LOG_DEBUG(LOG_MAIN,"[1012] CDevice::%s\n",__FUNCTION__);
	return SendCmd_IndoorBindDevStatus(lstDevStatus);
}
//
int CDevice::OnAddUnlockItem_Rep(UnlockRep_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN,"2006 %s\n",__FUNCTION__);
	return SendCmd_UnlockRecords(tInfo);
}

//1
int CDevice::OnRegister( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "Registered | %s | nLen:%d \n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = LENGTH_SERIALNO + LENGTH_PASSWORD + LENGTH_IMAGEVERSION;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	BYTE szSerialNO[LENGTH_SERIALNO+1] = {0};
	DWORD dwDserverConfigureIndex = 0, dwConfigureIndex1 = 0, dwConfigureIndex2 = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> CByteArrayBuffer((PUCHAR)szSerialNO, LENGTH_SERIALNO);
	bufferGet >> CByteArrayBuffer((PUCHAR)m_tDeviceInfo.szPassword, LENGTH_PASSWORD);
	memset(m_tDeviceInfo.szImageVer, 0, sizeof(m_tDeviceInfo.szImageVer));
	bufferGet >> CByteArrayBuffer((PUCHAR)m_tDeviceInfo.szImageVer, LENGTH_IMAGEVERSION);
	memset(m_tDeviceInfo.szDeviceName, 0, sizeof(m_tDeviceInfo.szDeviceName));
	if (false == GetVariableStr(bufferGet, (PUCHAR)m_tDeviceInfo.szDeviceName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	nNeedLen += 4*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	bufferGet >> m_tDeviceInfo.dwDeviceType >> dwDserverConfigureIndex >> dwConfigureIndex1 >> dwConfigureIndex2;

	m_wHeaderVersion = m_tHeader.version;

	LOG_DEBUG(LOG_MAIN, "SerialNO %s ImageVersion %s DeviceName %s DeviceType %d DserverIndex %d Index1 %d Index2 %d HeaderVer %d\n",
		szSerialNO, m_tDeviceInfo.szImageVer, m_tDeviceInfo.szDeviceName, m_tDeviceInfo.dwDeviceType,
		dwDserverConfigureIndex, dwConfigureIndex1, dwConfigureIndex2, m_wHeaderVersion);
	
	if ( (m_tDeviceInfo.dwDeviceID == 0) || memcmp(szSerialNO, m_tDeviceInfo.szSerialNO, LENGTH_SERIALNO) )
	{
		if (m_pDataBase == NULL)
		{
			m_pDataBase = RegisterDataBase(this);
			LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
		}
		if (m_pStorageDB == NULL)
		{
			m_pStorageDB =RegStorageDB(this);
			LOG_ASSERT_RET(LOG_MAIN, m_pStorageDB, -1);
		}
		if (m_pStorageB == NULL)
		{
			m_pStorageB = RegStorageBusiness(this);
			LOG_ASSERT_RET(LOG_MAIN, m_pStorageB, -1);
		}
		if (-1 == m_pDataBase->GetDeviceInfo((PUCHAR)szSerialNO))
		{
			return SendCmd_RegisterRep(dwDserverConfigureIndex, ERROR_SYSTEM);
		}
	}
	return 0;
}

int CDevice::OnReportNetwork( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
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

	//m_tNetInfo.dwPublicIP = tNetInfo.dwPublicIP;
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
//推送
int CDevice::OnReportDeviceStatus( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d dwDeviceID %d\n", __FUNCTION__, nLen, m_tDeviceInfo.dwDeviceID);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	//////////////////////////////////////////////////////////////////////////
	int nNeedLen = 2*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwDeviceID = 0, dwStatus = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwDeviceID >> dwStatus;
	LOG_DEBUG(LOG_MAIN,"CDevice::%s DeviceID:%d, Status:%d\n",__FUNCTION__,dwDeviceID, dwStatus);

	BYTE szMsg[LENGTH_MSGCONTENT+1] = {0};
	if (false == GetBase64Str(bufferGet, (PUCHAR)szMsg, LENGTH_MSGCONTENT, nLen, nNeedLen)) return -1;
	
	nNeedLen += 2*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwSmsCount = 0, dwPushCount = 0;
	bufferGet >> dwSmsCount >> dwPushCount;

	LIST_SMSINFO listSmsInfo;
	SmsInfo_t tInfo1;
	for (UINT i = 0; i < dwSmsCount; i++)
	{
		nNeedLen += 2*sizeof(DWORD) + sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		memset(&tInfo1, 0, sizeof(SmsInfo_t));
		bufferGet >> tInfo1.dwUserID >> tInfo1.dwVendorID >> tInfo1.bLanguage;

		if (false == GetBase64Str(bufferGet, (PUCHAR)tInfo1.szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo1.szDeviceName, LENGTH_NAME, nLen, nNeedLen)) return -1;
		listSmsInfo.push_back(tInfo1);
	}
	
	LIST_PUSHINFO listPushInfo;
	PushInfo_t tInfo2;
	for (UINT j = 0; j < dwPushCount; j++)
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
		LOG_DEBUG(LOG_MAIN, "dwUserID %d dwVendorID %d bPushType %d bLanguage %d szToken %s szDeviceName %s\n", 
			tInfo2.dwUserID, tInfo2.dwVendorID, tInfo2.bPushType, tInfo2.bLanguage, tInfo2.szToken, tInfo2.szDeviceName);
		listPushInfo.push_back(tInfo2);
	}

	//上报报警状态
	if(dwStatus == AT_DOOR_LOCK || dwStatus ==AT_DOOR_UNLOCK)
	{
		AlarmStatus_t alarmStatus;
		alarmStatus.dwDeviceID = dwDeviceID;
		alarmStatus.dwRoomID = 0;
		alarmStatus.dwType = 1;
		if(dwStatus == AT_DOOR_LOCK) alarmStatus.dwSubType = 1;
		else alarmStatus.dwSubType = 0;
		if (false == GetVariableStr(bufferGet, (PUCHAR)alarmStatus.szTimeStamp, LENGTH_TIMESTAMP, nLen, nNeedLen)) return -1;
		return ReportAlarmStatus(alarmStatus);
	}
	//
	ReportDevStatus devStatus;
	devStatus.dwDeviceID = dwDeviceID;
	devStatus.dwStatus = dwStatus;
	// 通过dispatch通知到viewermger
	IDeviceHandleSink* pSink = CDeviceMgr::Instance()->GetSink();
	if (pSink) pSink->Dev_OnReportDeviceStatus(m_tDeviceInfo.dwDeviceID, dwStatus, (PUCHAR)szMsg, listSmsInfo, listPushInfo, devStatus);
	//3.1.2  门口机呼叫室内机回应室内机
	ReportDeviceStatusRep(devStatus);
	return 0;
}

//3.1.2  门口机呼叫室内机回应室内机
bool CDevice::ReportDeviceStatusRep(ReportDevStatus& devStatus)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << devStatus.dwResult << devStatus.dwStatus;
	SendPacket(buffer, CMD_REPORT_DEVICE_STATUS_REP, ERROR_NO);
	return true;
}

int CDevice::OnGetDeviceUserInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	bufferGet >> dwDeviceID;
	LOG_DEBUG(LOG_MAIN, "dwDeviceID %d\n", dwDeviceID);
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->GetDeviceUserInfo(m_tDeviceInfo.dwDeviceID))
	{
		LIST_SMSINFO listInfo;
		return SendCmd_DeviceUser(dwDeviceID, 0, listInfo, ERROR_SYSTEM);
	}
	return 0;
}

int CDevice::OnGetDevicePushInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	bufferGet >> dwDeviceID;
	LOG_DEBUG(LOG_MAIN, "dwDeviceID %d\n", dwDeviceID);
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if (-1 == m_pDataBase->GetDevicePushInfo(m_tDeviceInfo.dwDeviceID))
	{
		LIST_PUSHINFO listInfo;
		return SendCmd_DevicePush(dwDeviceID, 0, listInfo, ERROR_SYSTEM);
	}
	return 0;
}

int CDevice::OnGetDeviceRoomSum(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	bufferGet >> dwDeviceID;

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	m_pDataBase->GetDeviceRoomSum(m_tDeviceInfo.dwDeviceID);
	return 0;
}

int CDevice::OnGetDeviceRoomSumRepAck(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	bufferGet >> dwDeviceID;
	LOG_DEBUG(LOG_MAIN, "dwDeviceID %d CommandFlag %d SegmentFlag %d\n", dwDeviceID, m_tHeader.commandflag, m_tHeader.segmentflag);

	AddMulPktAck(dwDeviceID, MP_Sum, m_tHeader.commandflag);
	return 0;
}

const BYTE DB_ROOMTYPE_USERINDEX = 0;
const BYTE DB_ROOMTYPE_PUSHINDEX = 1;
const BYTE DB_ROOMTYPE_CARDINDEX = 2;
const BYTE DB_ROOMTYPE_OTHER = 3;
const BYTE DB_ROOMTYPE_INDOORINDEX = 4;
//const BYTE DB_ROOMTYPE_PUSHSWITCHINDEX = 4;
int CDevice::OnGetDeviceRoomInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"===================================================\n");
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD) + sizeof(BYTE) + sizeof(int);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	BYTE bRoomIndexType = 0;
	int nCount = 0;
	bufferGet >> dwDeviceID >> bRoomIndexType >> nCount;
	LOG_DEBUG(LOG_MAIN, "dwDeviceID %d bRoomIndexType %d nCount %d\n", dwDeviceID, bRoomIndexType, nCount);

	nNeedLen += nCount*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	LIST_DWORD lstRoomID;
	for (int i = 0; i < nCount; i++)
	{
		DWORD dwRoomID = 0;
		bufferGet >> dwRoomID;
		LOG_DEBUG(LOG_MAIN,"RoomID:%d\n",dwRoomID);
		lstRoomID.push_back(dwRoomID);
	}

	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	m_pDataBase->GetDeviceRoomInfo(m_tDeviceInfo.dwDeviceID, bRoomIndexType, lstRoomID);
	LOG_DEBUG(LOG_MAIN,"===================================================\n");

	return 0;
}

int CDevice::OnGetDeviceRoomInfoRepAck(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	BYTE bRoomIndexType = 0;
	bufferGet >> dwDeviceID >> bRoomIndexType;
	LOG_DEBUG(LOG_MAIN, "dwDeviceID %d bRoomIndexType %d CommandFlag %d SegmentFlag %d\n", dwDeviceID, bRoomIndexType, m_tHeader.commandflag, m_tHeader.segmentflag);

	MulPkt_e eMulPkt = MP_NULL;
	if (bRoomIndexType == DB_ROOMTYPE_USERINDEX) eMulPkt = MP_UserIndex;
	else if (bRoomIndexType == DB_ROOMTYPE_PUSHINDEX) eMulPkt = MP_PushIndex;
	else if (bRoomIndexType == DB_ROOMTYPE_CARDINDEX) eMulPkt = MP_CardIndex;
	else if (bRoomIndexType == DB_ROOMTYPE_OTHER) eMulPkt = MP_Other;
	else if (bRoomIndexType == DB_ROOMTYPE_INDOORINDEX) eMulPkt = MP_IndoorIndex;
//	else if (bRoomIndexType == DB_ROOMTYPE_PUSHSWITCHINDEX) eMulPkt = MP_PushSwitchIndex;
	AddMulPktAck(dwDeviceID, eMulPkt, m_tHeader.commandflag);
	return 0;
}

int CDevice::OnGetDeviceCfg(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);
	//判断类型tType
	DWORD tID = 0;
	DWORD tType = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tID >> tType;
	if(0 == tType){
		LOG_DEBUG(LOG_MAIN, "%s | not recv tType\n", __FUNCTION__);
		return 0;
	}
	if(1 == tType)
	{
		LOG_DEBUG(LOG_MAIN, "%s | nLen:%d | tType:%d \n", __FUNCTION__, nLen, tType);
//		m_tDeviceInfo.dwDeviceID = tID;
		if (-1 == m_pDataBase->GetDeviceCfg(m_tDeviceInfo.dwDeviceID, UpdateDevCfgType_Ucpaas))
		{
			UcpaasInfo_t tUcpaas;
			memset(&tUcpaas, 0, sizeof(UcpaasInfo_t));
			return SendCmd_DeviceCfg(m_tDeviceInfo.dwDeviceID, UpdateDevCfgType_Ucpaas, tUcpaas, ERROR_SYSTEM);
		}
	}
	else if(2 == tType)
	{
		LOG_DEBUG(LOG_MAIN, "%s | nLen:%d | tType:%d \n", __FUNCTION__, nLen, tType);
		
		//查询厂商手机号码
//		m_tDeviceInfo.dwVendorID = tID;
		if (-1 == m_pDataBase->GetDeviceCfg(m_tDeviceInfo.dwVendorID,UpdateDevCfgType_PhoneNum))
		{
			//获取系统配置信息
			SystemCfg_t tCfgInfo;
			memset(&tCfgInfo, 0, sizeof(tCfgInfo));
			if (false == GetSystemCfg(tCfgInfo)) return false;
			return SendCmd_SystemCfg(m_tDeviceInfo.dwVendorID, UpdateDevCfgType_PhoneNum, tCfgInfo, ERROR_SYSTEM);
		}
	}
	return 0;
}

int CDevice::OnGetServerTime( PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s | Len:%d \n", __FUNCTION__, nLen);
	return SendCmd_ServerTimeRep(ERROR_NO);
}

//开门记录2001
int CDevice::OnReportUnlockLog( PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"2001:%s, nLen:%d \n",__FUNCTION__, nLen);
	PUCHAR pBuff = pData-PACKET_HEADER_SIZE;
	int nBufLen  = nLen +PACKET_HEADER_SIZE;
	
	LOG_ASSERT_RET(LOG_MAIN, m_pStorageDB, -1);
	if( -1 == m_pStorageDB->SDB_UnlockItemTunel(pBuff, nBufLen) )
	{
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);
		return -1;
	}
	return 0;
}

//门禁状态
int CDevice::ReportAlarmStatus(AlarmStatus_t& alarmStatus)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s DevID %d RoomID %d Type %d SubType %d TimeStamp %s\n",
		__FUNCTION__,alarmStatus.dwDeviceID, alarmStatus.dwRoomID, alarmStatus.dwType, alarmStatus.dwSubType, alarmStatus.szTimeStamp);
	IDeviceHandleSink* pSink = CDeviceMgr::Instance()->GetSink();
	if (pSink) pSink->Dev_ReportAlarmStatus(alarmStatus);
	return 0;
}

//获取公告Index
int CDevice::OnGetNoticeIndex(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s, nLen:%d \n",__FUNCTION__, nLen);
	DWORD dwGroupID = m_tDeviceInfo.dwGroupID;
	DWORD dwDeviceID = m_tDeviceInfo.dwDeviceID;
	LOG_DEBUG(LOG_MAIN,"dwGroupID = %d, dwDeviceID = %d\n",dwGroupID, dwDeviceID);
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if( -1 == m_pDataBase->GetNoticeIndex(dwGroupID,dwDeviceID))
	{
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);
		return -1;
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////
int CDevice::OnGetAdvertByID2(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"%s\n",__FUNCTION__);
	DWORD dwCount = 0;
	DWORD dwAdvertID = 0;
	DWORD dwDeviceID = m_tDeviceInfo.dwDeviceID;
	LIST_DWORD lstAdvertID;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwCount;
	LOG_DEBUG(LOG_MAIN,"%s Count = %d\n",__FUNCTION__, dwCount);
	if (dwCount == 0)		return 0;

	int nNeedLen = sizeof(DWORD) + dwCount * sizeof(DWORD);
	if(nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	for(int i = 0; i < dwCount; i++)
	{
		bufferGet >> dwAdvertID;
		LOG_DEBUG(LOG_MAIN,"%s AdID = %d\n",__FUNCTION__, dwAdvertID);
		lstAdvertID.push_back(dwAdvertID);
	}
	//////////////////////////////////////////////////////////////////////////
	LOG_ASSERT_RET(LOG_MAIN, m_pStorageB, -1);
	if( -1 == m_pStorageB->GetAdvertInfo(dwDeviceID, lstAdvertID))
	{
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);
		return -1;
	}
	return 0;
}
int CDevice::OnReportDownloadProgress2(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s, nLen:%d \n",__FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >sizeof(DWORD), -1);
	DWORD dwCount = 0;
	DWORD dwAdID = 0;
	DWORD dwProgress = 0;
	BYTE		 bSuccess = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwCount;
	LOG_DEBUG(LOG_MAIN, "%s Count = %d\n", __FUNCTION__, dwCount);

	int nNeedLen = sizeof(DWORD) + dwCount * ( 2*sizeof(DWORD) + sizeof(BYTE));
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	Progress_t tInfo;
	LIST_PROGRESS lstProgress;
	for (int i = 0; i < dwCount; i++)
	{
		bufferGet >> tInfo.dwAdID >> tInfo.dwProgress >> tInfo.cFinish;
		LOG_DEBUG(LOG_MAIN, "%s AdID = %d Progress [%d%] Finish = %d\n", __FUNCTION__,tInfo.dwAdID, tInfo.dwProgress, tInfo.cFinish);
		lstProgress.push_back(tInfo);
	}
	LOG_ASSERT_RET(LOG_MAIN, m_pStorageB, -1);
	if( -1 == m_pStorageB->ReportAdvertProgress(m_tDeviceInfo.dwDeviceID,lstProgress))
	{
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);
		return -1;
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////
//广告
int CDevice::OnGetAdvertIndex(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s, nLen:%d \n",__FUNCTION__, nLen);
	DWORD dwGroupID = m_tDeviceInfo.dwGroupID;
	DWORD dwDeviceID = m_tDeviceInfo.dwDeviceID;
	LOG_DEBUG(LOG_MAIN,"dwVillageid = %d, dwDeviceID = %d\n",dwGroupID, dwDeviceID);
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if( -1 == m_pDataBase->GetAdvertIndex(dwGroupID,dwDeviceID))
	{
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);
		return -1;
	}
	return 0;
}
//配置
int CDevice::OnGetVisitorCfg(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s, nLen:%d \n",__FUNCTION__, nLen);
	DWORD dwGroupID = m_tDeviceInfo.dwGroupID;
	DWORD dwDeviceID = m_tDeviceInfo.dwDeviceID;
	LOG_DEBUG(LOG_MAIN,"dwVillageid = %d, dwDeviceID = %d\n",dwGroupID, dwDeviceID);
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if( -1 == m_pDataBase->GetVisitorCfg(dwGroupID,dwDeviceID))
	{
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);
		return -1;
	}
	return 0;
}

//3.1.20  获取绑定设备状态
int CDevice::OnGetBindDevStatus(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= sizeof(DWORD), -1);
	DWORD dwIndoorID;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwIndoorID;
	LOG_DEBUG(LOG_MAIN,"CDevice::%s %d\n",__FUNCTION__, dwIndoorID);
	LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
	if( -1 == m_pDataBase->GetBindDevStatus(dwIndoorID))
	{
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);
		return -1;
	}
	return 0;
}

//3.1.21 云平台呼叫到所有绑定的室内机
int CDevice::OnCallIndoorDev(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwCount = 0, dwStatus = 0;
	DWORD nNeedLen = 2 * sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	bufferGet >> dwCount >> dwStatus;
	LOG_DEBUG(LOG_MAIN,"Count:%d, Status:%d\n", dwCount, dwStatus);

	nNeedLen += dwCount * sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	LIST_DWORD lstIndoorID;
	for(int i = 0; i < dwCount; i++)
	{
		DWORD dwIndoorID = 0;
		bufferGet >> dwIndoorID;
		LOG_DEBUG(LOG_MAIN,"IndoorID:%d\n",dwIndoorID);
		lstIndoorID.push_back(dwIndoorID);
	}
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_CallIndoor(lstIndoorID, dwStatus, m_tDeviceInfo.dwDeviceID);
	return 0;
}

//门口机离线
int CDevice::GetBindIndoorID(DWORD dwDeviceID)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	if( -1 == m_pDataBase->GetBindIndoorID(dwDeviceID))
	{
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);
		return -1;
	}
	return 0;
}


int CDevice::OnSdkTunnel(PUCHAR pData, int nLen)
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

	IDeviceHandleSink* pSink = CDeviceMgr::Instance()->GetSink();
	if (pSink) pSink->Dev_OnSdkTunnel(tTunnel);
	return 0;
}

int CDevice::OnQiniu_GetUploadToken(PUCHAR pData, int nLen)
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

	tTag.bSrcType = SRC_TYPE_DEVICE;
	tTag.dwTagID1 = m_tDeviceInfo.dwDeviceID;
	tTag.dwTagID2 = 0;
	tTag.dwStoreID = m_tDeviceInfo.dwStoreID;
	IDeviceHandleSink* pSink = CDeviceMgr::Instance()->GetSink();
	if (pSink) pSink->Dev_OnQiniu_GetUploadToken(tTag);
	return 0;
}

int CDevice::OnQiniu_GetDownloadUrls(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	StoreKey_t tKey; memset(&tKey, 0, sizeof(StoreKey_t));
	IDeviceHandleSink* pSink = CDeviceMgr::Instance()->GetSink();
	if (pSink) pSink->Dev_OnQiniu_GetDownloadUrls(tTag, tKey);
	return 0;
}

int CDevice::OnQiniu_ReportUploadResult(PUCHAR pData, int nLen)
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

	IDeviceHandleSink* pSink = CDeviceMgr::Instance()->GetSink();
	if (pSink) pSink->Dev_OnQiniu_ReportUploadResult(tKey);
	return 0;
}
//////////////////////////////////////////////////////////////////////////
//改上面代码，直发给sdb
int CDevice::OnQiniu_ReportUploadResult2(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s, nLen:%d \n",__FUNCTION__, nLen);
	PUCHAR pBuff = pData-PACKET_HEADER_SIZE;
	int nBufLen  = nLen +PACKET_HEADER_SIZE;

	LOG_ASSERT_RET(LOG_MAIN, m_pStorageDB, -1);
	if( -1 == m_pStorageDB->SDB_ReportUploadResult(pBuff, nBufLen) )
	{
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);
		return -1;
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////
int CDevice::OnGetRegisterInfo( PUCHAR pData, int nLen )
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

	LIST_SERVERINFO listInfo;
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->SA_GetDServers(dwVendorID, dwConfigureIndex, listInfo);
	return SendCmd_RegisterInfo(dwVendorID, dwConfigureIndex, listInfo);
}

//////////////////////////////////////////////////////////////////////////

void CDevice::PacketHeader(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg /* = 1 */, WORD wSubSeg /* = 1 */)
{
	int nLen = buffer.GetFilledSize();
	buffer.SetOffset(0);
	// Header
	buffer << (BYTE)m_tHeader.groupcode << (WORD)wCommand/*Command ID*/ << (BYTE)m_tHeader.reserved0/*Reserved0*/
		<< (WORD)g_wCurHeaderVersion/*Version*/ << (WORD)m_tHeader.reserved1/*Reserved1*/
		<< (DWORD)m_tHeader.destinationid/*Source ID*/
		<< (DWORD)m_tHeader.sourceid/*Destination ID*/
		<< (DWORD)m_tHeader.commandflag/*Command Flag*/
		<< (WORD)wTotalSeg/*Total Segment*/ << (WORD)wSubSeg/*Sub Segment*/
		<< (WORD)0/*Segment Flag*/ << (WORD)m_tHeader.reserved2/*Reserved2*/
		<< (DWORD)m_tHeader.reserved3/*Reserved3*/;
	// Payload
	buffer << (WORD)wError/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)g_bCurPayloadVersion/*Payload Version*/ << (WORD)0/*Payload Length*/;
	buffer.SetOffset(nLen);
	LOG_DEBUG(LOG_MAIN, "ToDevice pCon %p PacketHeader cmd:0x%04x err:0x%04x len:%d\n", m_pCon, wCommand, wError, nLen);
}

int CDevice::SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg /* = 1 */, WORD wSubSeg /* = 1 */)
{
	LOG_ASSERT_RET(LOG_MAIN, m_pCon, -1);
	int nLen = buffer.GetFilledSize();
	buffer.SetOffset(0);
	// Header
	buffer << (BYTE)m_tHeader.groupcode << (WORD)wCommand/*Command ID*/ << (BYTE)m_tHeader.reserved0/*Reserved0*/
		<< (WORD)g_wCurHeaderVersion/*Version*/ << (WORD)m_tHeader.reserved1/*Reserved1*/
		<< (DWORD)m_tHeader.destinationid/*Source ID*/
		<< (DWORD)m_tHeader.sourceid/*Destination ID*/
		<< (DWORD)m_tHeader.commandflag/*Command Flag*/
		<< (WORD)wTotalSeg/*Total Segment*/ << (WORD)wSubSeg/*Sub Segment*/
		<< (WORD)0/*Segment Flag*/ << (WORD)m_tHeader.reserved2/*Reserved2*/
		<< (DWORD)m_tHeader.reserved3/*Reserved3*/;
	// Payload
	buffer << (WORD)wError/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)g_bCurPayloadVersion/*Payload Version*/ << (WORD)0/*Payload Length*/;
	buffer.SetOffset(nLen);
	LOG_DEBUG(LOG_MAIN, "ToDevice pCon %p SendData cmd:0x%04x err:0x%04x len:%d\n", m_pCon, wCommand, wError, nLen);
	return m_pCon->SendCommand((PUCHAR)buffer, nLen);
}

int CDevice::SendCmd_TransClientInfo(TransClientInfo_t& tInfo)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.bSrcType << tInfo.dwServerID << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwDeviceID << tInfo.bType;
	PutBuffer_NetInfo(buffer, tInfo.tNetInfo);
	buffer << tInfo.nTransmitSessionMode;
	return SendPacket(buffer, CMD_TRANS_CLIENTINFO, ERROR_NO);
}

int CDevice::SendCmd_SdlTunnel(SdkTunnel_t& tInfo)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.bSrcType << tInfo.dwServerID << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwDeviceID << (WORD)tInfo.nTunnelDataLen;
	buffer << CByteArrayBuffer(tInfo.pTunnelData, tInfo.nTunnelDataLen);
	return SendPacket(buffer, CMD_SDK_TUNNEL, ERROR_NO);
}

void CDevice::UpdateDeviceRoomInfo(BYTE bType, LIST_DWORD& lstRoomID)
{
	// 删除该房号
	if (bType & UpdateDevRoomType_Delete)
	{
		ClearRooms(lstRoomID);
	}

	LOG_ASSERT_RETVOID(LOG_MAIN, m_pDataBase);
	// 更新其他房号和密码信息
	if (bType & UpdateDevRoomType_Other)
	{
		if (m_wHeaderVersion >= 2)
		{
			if (-1 == m_pDataBase->GetDeviceRoomInfo(m_tDeviceInfo.dwDeviceID, RoomIndexType_Other, lstRoomID)) return;
		}
	}
	// 更新房号用户信息
	if (bType & UpdateDevRoomType_UserIndex)
	{
		if (m_wHeaderVersion >= 2)
		{
			if (-1 == m_pDataBase->GetDeviceRoomInfo(m_tDeviceInfo.dwDeviceID, RoomIndexType_UserIndex, lstRoomID)) return;
		}
		else
		{
			if (-1 == m_pDataBase->GetDeviceUserInfo(m_tDeviceInfo.dwDeviceID)) return;
		}
	}
	// 更新房号推送信息
	if (bType & UpdateDevRoomType_PushIndex)
	{
		if (m_wHeaderVersion >= 2)
		{
			if (-1 == m_pDataBase->GetDeviceRoomInfo(m_tDeviceInfo.dwDeviceID, RoomIndexType_PushIndex, lstRoomID)) return;
		}
		else
		{
			if (-1 == m_pDataBase->GetDevicePushInfo(m_tDeviceInfo.dwDeviceID)) return;
		}
	}
	// 更新房号刷卡卡信息
	if (bType & UpdateDevRoomType_CardIndex)
	{
		if (m_wHeaderVersion >= 2)
		{
			if (-1 == m_pDataBase->GetDeviceRoomInfo(m_tDeviceInfo.dwDeviceID, RoomIndexType_CardIndex, lstRoomID)) return;
		}
	}

	if (bType & UpdateDevRoomType_PushSwitchIndex)
	{
		if (m_wHeaderVersion >= 2)
		{
			if (-1 == m_pDataBase->GetDeviceRoomInfo(m_tDeviceInfo.dwDeviceID, RoomIndexType_PushSwitchIndex, lstRoomID)) return;
		}
	}
}
//08
void CDevice::SendDeviceDeadLine(DeviceDeadLineInfo_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN,"108:%s, DeviceID:%d, DevDeadLine:%d\n",__FUNCTION__ , tInfo.dwDeviceID, tInfo.dwDevDeadLine);
	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.dwDevDeadLine;
	SendPacket(buffer,CMD_DEV_DEADLINE,ERROR_NO);
}
//UnlcokRecordsRep
int CDevice::SendCmd_UnlockRecords(UnlockRep_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN,"2007 %s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.dwResult << tInfo.dwCount;
	LOG_DEBUG(LOG_MAIN,"Result:%d,Count:%d\n",tInfo.dwResult, tInfo.dwCount);
	for(int i = 0; i < tInfo.dwCount; i++)
	{
		buffer << tInfo.UnlockIndex[i];
		LOG_DEBUG(LOG_MAIN,"Index:%d\n",tInfo.UnlockIndex[i]);
	}
	SendPacket(buffer,DD_PUSH_UNLOCK_RECORD_REP,ERROR_NO);
}
//77.更新物業公告
void CDevice::SendDevicePropertyAnnounce(DWORD dwNoticeIndex)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s NoticeIndex = %d\n",__FUNCTION__, dwNoticeIndex);
	DECLARE_PUTBUFFER( buffer )
	buffer << dwNoticeIndex;
	SendPacket(buffer,DD_UPDATE_BULLETIN_REP,ERROR_NO);
}
//更新广告
void CDevice::SendDeviceAdvertInfo(DWORD dwAdvertIndex)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s AdvertIndex = %d\n",__FUNCTION__, dwAdvertIndex);
	DECLARE_PUTBUFFER( buffer )
	buffer << dwAdvertIndex ;
	SendPacket(buffer,CMD_GET_ADVERT_INFO_REP,ERROR_NO);
}


//3.1.21 云平台呼叫到所有绑定的室内机
void CDevice::SendCallIndoor(DWORD dwDeviceID, DWORD dwStatus)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s dwDeviceID=%d, dwStatus=%d\n",__FUNCTION__, dwDeviceID, dwStatus);
	DECLARE_PUTBUFFER( buffer )
	buffer << dwDeviceID << dwStatus;
	SendPacket(buffer, CMD_CALL_INDOOR_DEV_REP, ERROR_NO);
}

void CDevice::SendFirmwareRequest(DevUpgrad_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN,"%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << CByteArrayBuffer(tInfo.DevVersion, LENGTH_IMAGEVERSION);
	LOG_DEBUG(LOG_MAIN,"version=%s\n",tInfo.DevVersion);
//	SendPacket(buffer,DD_UPDATE_FIRMWARE_REQUEST,ERROR_NO);
}

//3.1.20 通知室内机其绑定设备的在线离线状态
void CDevice::UpdateDevStat(DevStatus& tDevStat)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = 1;
	DevStatus tIndoorStat;
	buffer << dwCount << tIndoorStat.dwDeviceID << tIndoorStat.dwStatus;
	SendPacket(buffer,CMD_GET_BIND_DEV_STATE_REP,ERROR_NO);
}

void CDevice::ClearRooms(LIST_DWORD& lstRoomID)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	if (m_wHeaderVersion >= 2)
	{
		SendCmd_DeleteRoom(m_tDeviceInfo.dwDeviceID, lstRoomID);
	}
	else
	{
		if (-1 == m_pDataBase->GetDeviceUserInfo(m_tDeviceInfo.dwDeviceID)) return;
		if (-1 == m_pDataBase->GetDevicePushInfo(m_tDeviceInfo.dwDeviceID)) return;
	}
}

void CDevice::UpdateDevice()
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	LIST_DWORD lstRoomID;
	BYTE bUpdateDevRoomType = UpdateDevRoomType_UserIndex | UpdateDevRoomType_PushIndex | UpdateDevRoomType_CardIndex | UpdateDevRoomType_Other;
	UpdateDeviceRoomInfo(bUpdateDevRoomType, lstRoomID);
}

void CDevice::UpdateDeviceEx(int nType)
{
	LOG_DEBUG(LOG_MAIN, "%s nType %d\n", __FUNCTION__, nType);
	if (nType & UpdateDevCfgType_Ucpaas)
	{
		if (m_wHeaderVersion >= 2)
		{
			if (-1 == m_pDataBase->GetDeviceCfg(m_tDeviceInfo.dwDeviceID, nType)) return;
		}
	}
}


int CDevice::SendCmd_RegisterInfo( DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo )
{
	struct sockaddr_in* psinPeer = NULL;
	m_pCon->GetOpt( NETWORK_TRANSPORT_OPT_GET_PEER_ADDR, &psinPeer );
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

int CDevice::SendCmd_DeviceUser(DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo, WORD wError /* = 0 */)
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

int CDevice::SendCmd_DevicePush(DWORD dwDeviceID, DWORD dwIndex, LIST_PUSHINFO& listInfo, WORD wError /* = 0 */)
{
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		buffer << dwDeviceID << dwIndex << dwCount;
		return SendPacket(buffer, CMD_GET_DEVICEPUSH_INFO_REP, wError);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DevicePush(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_PUSHINFO::iterator iter = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwIndex << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << iter->dwUserID << iter->dwVendorID << iter->bPushType << iter->bLanguage;
			PutVariableStr(buffer, (PUCHAR)iter->szToken);
			iter++;
		}
		SendPacket(buffer, CMD_GET_DEVICEPUSH_INFO_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}
//9
int CDevice::SendCmd_RegisterRep( DWORD dwDserverConfigureIndex, WORD wError )
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s | wError:%d \n",__FUNCTION__, wError);
	LOG_ASSERT_RET(LOG_MAIN, m_pCon, -1);
	struct sockaddr_in* psinPeer = NULL;
	m_pCon->GetOpt( NETWORK_TRANSPORT_OPT_GET_PEER_ADDR, &psinPeer );
	LOG_ASSERT_RET(LOG_MAIN, psinPeer, -1);
	m_tNetInfo.dwPublicIP = ntohl( psinPeer->sin_addr.s_addr );
	m_tNetInfo.wPublicPortTCP = ntohs( psinPeer->sin_port );
	m_tNetInfo.wPublicPortUDP = ntohs( psinPeer->sin_port );

	DWORD dwTime = (DWORD)time(NULL);
	DECLARE_PUTBUFFER( buffer )
	buffer << m_tDeviceInfo.dwDeviceID;
	buffer << dwDserverConfigureIndex;         // dserverindex表格的configureindex字段
	buffer << m_tDeviceInfo.dwConfigureIndex;  // 0000房号的dwUserIndex
	buffer << m_tDeviceInfo.dwConfigureIndex2; // 0000房号的dwPushIndex
	buffer << m_tDeviceInfo.dwVendorID;
	buffer << CByteArrayBuffer((PUCHAR)m_tDeviceInfo.szSerialNO, LENGTH_SERIALNO);
	PutVariableStr(buffer, (PUCHAR)m_tDeviceInfo.szDeviceName);
	buffer << m_tNetInfo.dwPublicIP << m_tNetInfo.wPublicPortTCP << dwTime << (DWORD)0;
	buffer << CByteArrayBuffer((PUCHAR)m_tDeviceInfo.szDeadLine,LENGTH_DEADLINE);
	LOG_DEBUG(LOG_MAIN,"%s DeviceID:%d DeadLine=%s\n",__FUNCTION__, m_tDeviceInfo.dwDeviceID, m_tDeviceInfo.szDeadLine);
	return SendPacket(buffer, CMD_REGISTER_REP, wError);
}

int CDevice::SendCmd_ServerTimeRep(DWORD wError)
{
	DWORD dwTime = (DWORD)time(NULL);
	DECLARE_PUTBUFFER( buffer )
	buffer << dwTime;
	LOG_DEBUG(LOG_MAIN, "%s | Time:%d \n", __FUNCTION__, dwTime);

	return SendPacket(buffer, DD_CMD_GET_SERVERTIME_REP, wError);
}

void CDevice::GetDeviceNetInfo( NetInfo_t& tNetInfo, DWORD& dwAutoRelay )
{
	dwAutoRelay = m_tDeviceInfo.dwAutoRelay;
	if ( (dwAutoRelay == RELAY_TYPE_AUTO) || (dwAutoRelay == RELAY_TYPE_NO) )
	{
		tNetInfo.dwPublicIP = m_tNetInfo.dwPublicIP;
		tNetInfo.wPublicPortTCP = m_tNetInfo.wPublicPortTCP;
		tNetInfo.wPublicPortUDP = m_tNetInfo.wPublicPortUDP;
		tNetInfo.wLocalPortUDP = m_tNetInfo.wLocalPortUDP;
		tNetInfo.wNetworkType = m_tNetInfo.wNetworkType;
		tNetInfo.listLocalIPs = m_tNetInfo.listLocalIPs;
		LOG_DEBUG(LOG_MAIN,"IP %s %d\n",IpDword2Str(m_tNetInfo.dwPublicIP), m_tNetInfo.wPublicPortTCP);
	}
}

void CDevice::GetDevicePassword( PUCHAR pPassword )
{
	memcpy(pPassword, m_tDeviceInfo.szPassword, LENGTH_PASSWORD);
}

int CDevice::SendCmd_DeviceRoomSum(DWORD dwDeviceID, LIST_ROOMSUM& listInfo)
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		PacketHeader(buffer, CMD_GET_DEVICE_ROOM_SUM_REP, ERROR_NO);
		AddMulPkt(dwDeviceID, MP_Sum, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		m_dwCmdFlag++;
		m_wSegFlag++;
		StartSendMulPkt(dwDeviceID, MP_Sum, this);
		return 0;
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DeviceRoomSum(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_ROOMSUM::iterator iter = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << (DWORD)iter->dwRoomID;
			PutVariableStr(buffer, (PUCHAR)iter->szRoom);
			buffer << (DWORD)iter->dwPushIndex << (DWORD)iter->dwUserIndex << (DWORD)iter->dwCardIndex;
			buffer << (BYTE)0x10;
			buffer << CByteArrayBuffer((PUCHAR)iter->szPassword, LENGTH_ROOMPWD);
			buffer << (DWORD)0;
			//LOG_DEBUG(LOG_MAIN, "PacketRoomSum: RoomID %d UserIndex %d PushIndex %d CardIndex %d Room %s Pwd %s\n",
			//	iter->dwRoomID, iter->dwUserIndex, iter->dwPushIndex, iter->dwCardIndex, iter->szRoom, iter->szPassword);
			iter++;
		}

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		PacketHeader(buffer, CMD_GET_DEVICE_ROOM_SUM_REP, ERROR_NO, wTotalSeg, wSubSeg);
		AddMulPkt(dwDeviceID, MP_Sum, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		m_dwCmdFlag++;
	}
	m_wSegFlag++;
	StartSendMulPkt(dwDeviceID, MP_Sum, this);
	return 0;
}

int CDevice::SendCmd_DeviceRoomUser(DWORD dwDeviceID, LIST_ROOMUSER& listInfo)
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		PacketHeader(buffer, CMD_GET_DEVICE_ROOMUSER_REP, ERROR_NO);
		AddMulPkt(dwDeviceID, MP_UserIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		m_dwCmdFlag++;
		m_wSegFlag++;
		StartSendMulPkt(dwDeviceID, MP_UserIndex, this);
		return 0;
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DeviceRoomUser(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_ROOMUSER::iterator iter = listInfo.begin();
	LIST_SMSINFO::iterator pos;
	bool bChange = true;
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		if (listInfo.end() == iter) break;

		if (bChange) pos = iter->lstUserInfo.begin();

		DWORD dwHighBit16 = ( (*posCount) & 0xffff0000 ) >> 16; // 高16位 0表示不同的RoomInfo，其他表示同一个RoomInfo
		DWORD dwPacketCount = ( (*posCount) & 0x0000ffff ); // 低16位 高16位为0时RoomInfo个数 / 高16位不为0时PushInfo个数

		if (dwHighBit16 == 0)
		{
			DECLARE_PUTBUFFER( buffer )
			buffer << dwDeviceID << dwPacketCount;

			for (int i = 0; i < dwPacketCount; ++i)
			{
				Packet_RoomUserInfo(buffer, *iter);
				iter++;
				bChange = true;
			}

			m_tHeader.commandflag = m_dwCmdFlag;
			m_tHeader.segmentflag = m_wSegFlag;
			PacketHeader(buffer, CMD_GET_DEVICE_ROOMUSER_REP, ERROR_NO, wTotalSeg, wSubSeg);
			AddMulPkt(dwDeviceID, MP_UserIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			m_dwCmdFlag++;
		}
		else
		{
			bChange = false;

			DECLARE_PUTBUFFER( buffer )
			buffer << dwDeviceID << (int)1 << iter->dwRoomID << iter->dwUserIndex << dwPacketCount;

			for (int i = 0; i < dwPacketCount; ++i)
			{
				buffer << pos->dwUserID << pos->dwVendorID << pos->bLanguage;
				PutVariableStr(buffer, (PUCHAR)pos->szMobilePhone);
				PutVariableStr(buffer, (PUCHAR)pos->szDeviceName);
				//LOG_DEBUG(LOG_UTIL, "Packet_RoomUserInfo2: RoomID %d UserIndex %d UserID %d VendorID %d Language %d Name %s DevName %s\n",
				//	iter->dwRoomID, iter->dwUserIndex, pos->dwUserID, pos->dwVendorID, pos->bLanguage, pos->szMobilePhone, pos->szDeviceName);

				pos++;
				if (pos == iter->lstUserInfo.end())
				{
					iter++;
					bChange = true;
					if (listInfo.end() != iter) pos = iter->lstUserInfo.begin();
					break;
				}
			}

			m_tHeader.commandflag = m_dwCmdFlag;
			m_tHeader.segmentflag = m_wSegFlag;
			PacketHeader(buffer, CMD_GET_DEVICE_ROOMUSER_REP, ERROR_NO, wTotalSeg, wSubSeg);
			AddMulPkt(dwDeviceID, MP_UserIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			m_dwCmdFlag++;
		}
		posCount++;
	}
	m_wSegFlag++;
	StartSendMulPkt(dwDeviceID, MP_UserIndex, this);
	return 0;
}

int CDevice::SendCmd_DeviceRoomPush( DWORD dwDeviceID, LIST_ROOMPUSH& listInfo )
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		PacketHeader(buffer, CMD_GET_DEVICE_ROOMPUSH_REP, ERROR_NO);
		AddMulPkt(dwDeviceID, MP_PushIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		m_dwCmdFlag++;
		m_wSegFlag++;
		StartSendMulPkt(dwDeviceID, MP_PushIndex, this);
		return 0;
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DeviceRoomPush(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_ROOMPUSH::iterator iter = listInfo.begin();
	LIST_PUSHINFO::iterator pos;
	bool bChange = true;
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		if (listInfo.end() == iter) break;

		if (bChange) pos = iter->lstPushInfo.begin();

		DWORD dwHighBit16 = ( (*posCount) & 0xffff0000 ) >> 16; // 高16位 0表示不同的RoomInfo，其他表示同一个RoomInfo
		DWORD dwPacketCount = ( (*posCount) & 0x0000ffff ); // 低16位 高16位为0时RoomInfo个数 / 高16位不为0时PushInfo个数

		if (dwHighBit16 == 0)
		{
			DECLARE_PUTBUFFER( buffer )
			buffer << dwDeviceID << dwPacketCount;

			for (int i = 0; i < dwPacketCount; ++i)
			{
				Packet_RoomPushInfo(buffer, *iter);
				iter++;
				bChange = true;
			}

			m_tHeader.commandflag = m_dwCmdFlag;
			m_tHeader.segmentflag = m_wSegFlag;
			PacketHeader(buffer, CMD_GET_DEVICE_ROOMPUSH_REP, ERROR_NO, wTotalSeg, wSubSeg);
			AddMulPkt(dwDeviceID, MP_PushIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			m_dwCmdFlag++;
		}
		else
		{
			bChange = false;

			DECLARE_PUTBUFFER( buffer )
			buffer << dwDeviceID << (int)1 << iter->dwRoomID << iter->dwPushIndex << dwPacketCount;

			for (int i = 0; i < dwPacketCount; ++i)
			{
				buffer << pos->dwUserID << pos->dwVendorID << pos->bPushType << pos->bLanguage;
				PutVariableStr(buffer, (PUCHAR)pos->szToken);
				//LOG_DEBUG(LOG_UTIL, "Packet_RoomPushInfo2: RoomID %d PushIndex %d UserID %d VendorID %d PushType %d Language %d Token %s\n",
				//	iter->dwRoomID, iter->dwPushIndex, pos->dwUserID, pos->dwVendorID, pos->bPushType, pos->bLanguage, pos->szToken);
				pos++;
				if (pos == iter->lstPushInfo.end())
				{
					iter++;
					bChange = true;
					if (listInfo.end() != iter) pos = iter->lstPushInfo.begin();
					break;
				}
			}

			m_tHeader.commandflag = m_dwCmdFlag;
			m_tHeader.segmentflag = m_wSegFlag;
			PacketHeader(buffer, CMD_GET_DEVICE_ROOMPUSH_REP, ERROR_NO, wTotalSeg, wSubSeg);
			AddMulPkt(dwDeviceID, MP_PushIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			m_dwCmdFlag++;
		}
		posCount++;
	}
	m_wSegFlag++;
	StartSendMulPkt(dwDeviceID, MP_PushIndex, this);
	return 0;
}

int CDevice::SendCmd_DeviceRoomCard(DWORD dwDeviceID, LIST_ROOMCARD& listInfo)
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		PacketHeader(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO);
		AddMulPkt(dwDeviceID, MP_CardIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		m_dwCmdFlag++;
		m_wSegFlag++;
		StartSendMulPkt(dwDeviceID, MP_CardIndex, this);
		return 0;
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DeviceRoomCard(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_ROOMCARD::iterator iter = listInfo.begin(), iter_limit = listInfo.begin();
	LIST_CARDINFO::iterator pos, pos_limit;
	bool bChange = true;
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		if (listInfo.end() == iter) break;

		if (bChange) 
		{
			pos = iter->lstCardInfo.begin();
			pos_limit = iter_limit->lstCardInfo.begin();
		}

		DWORD dwHighBit16 = ( (*posCount) & 0xffff0000 ) >> 16; // 高16位 0表示不同的RoomInfo，其他表示同一个RoomInfo
		DWORD dwPacketCount = ( (*posCount) & 0x0000ffff ); // 低16位 高16位为0时RoomInfo个数 / 高16位不为0时PushInfo个数

		if (dwHighBit16 == 0)
		{
			DECLARE_PUTBUFFER( buffer )
			buffer << dwDeviceID << dwPacketCount;

			for (int i = 0; i < dwPacketCount; ++i)
			{
				Packet_RoomCardInfo(buffer, *iter);
				iter++;
				bChange = true;
			}

			for (int j = 0; j < dwPacketCount; ++j)
			{
				Packet_RoomCardTimeLimit(buffer, *iter_limit);
				iter_limit++;
				bChange = true;
			}

			m_tHeader.commandflag = m_dwCmdFlag;
			m_tHeader.segmentflag = m_wSegFlag;
			PacketHeader(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO, wTotalSeg, wSubSeg);
			AddMulPkt(dwDeviceID, MP_CardIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			m_dwCmdFlag++;
		}
		else
		{
			bChange = false;

			DECLARE_PUTBUFFER( buffer )
			buffer << dwDeviceID << (int)1 << iter->dwRoomID << iter->dwCardIndex << dwPacketCount;

			for (int i = 0; i < dwPacketCount; ++i)
			{
				PutVariableStr(buffer, (PUCHAR)pos->szCard);
				buffer << pos->bCardType;
				//LOG_DEBUG(LOG_UTIL, "Packet_RoomCardInfo2: RoomID %d Card %s CardType %d\n",
					//iter->dwRoomID, iter->dwCardIndex, pos->szCard, pos->bCardType);
				pos++;
				if (pos == iter->lstCardInfo.end())
				{
					iter++;
					bChange = true;
					if (listInfo.end() != iter) pos = iter->lstCardInfo.begin();
					break;
				}
			}

			for (int j = 0; j < dwPacketCount; ++j)
			{
				buffer << pos_limit->dwCardTimeLimit;
				pos_limit++;
				if (pos_limit == iter_limit->lstCardInfo.end())
				{
					iter_limit++;
					bChange = true;
					if(listInfo.end() != iter_limit) iter_limit->lstCardInfo.begin();
					break;
				}
			}

			m_tHeader.commandflag = m_dwCmdFlag;
			m_tHeader.segmentflag = m_wSegFlag;
			PacketHeader(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO, wTotalSeg, wSubSeg);
			AddMulPkt(dwDeviceID, MP_CardIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			m_dwCmdFlag++;
		}
		posCount++;
	}
	m_wSegFlag++;
	StartSendMulPkt(dwDeviceID, MP_CardIndex, this);
	return 0;
}

int CDevice::SendCmd_DeviceRoomIndoor(DWORD dwDeviceID, LIST_ROOMINDOOR2& listInfo)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s\n",__FUNCTION__);
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;
		LOG_DEBUG(LOG_MAIN,"DeviceID=%d dwCount=%d\n",dwDeviceID, dwCount);
		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		PacketHeader(buffer, CMD_GET_DEVICE_ROOMINDOOR_REP, ERROR_NO);
		AddMulPkt(dwDeviceID, MP_IndoorIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		m_dwCmdFlag++;
		m_wSegFlag++;
		StartSendMulPkt(dwDeviceID, MP_IndoorIndex, this);
		return 0;
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DeviceRoomIndoor(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_ROOMINDOOR2::iterator iter = listInfo.begin();
	LIST_INDOORINFO::iterator pos;
	bool bChange = true;
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		if (listInfo.end() == iter) break;

		if (bChange) pos = iter->lstIndoorInfo.begin();

		DWORD dwHighBit16 = ( (*posCount) & 0xffff0000 ) >> 16; // 高16位 0表示不同的RoomInfo，其他表示同一个RoomInfo
		DWORD dwPacketCount = ( (*posCount) & 0x0000ffff ); // 低16位 高16位为0时RoomInfo个数 / 高16位不为0时PushInfo个数

		if (dwHighBit16 == 0)
		{
			LOG_DEBUG(LOG_MAIN,"1 \n");
			DECLARE_PUTBUFFER( buffer )
			buffer << dwDeviceID << dwPacketCount;
			LOG_DEBUG(LOG_MAIN,"DeviceID=%d dwCount=%d\n",dwDeviceID, dwPacketCount);
			for (int i = 0; i < dwPacketCount; ++i)
			{
//				LOG_DEBUG(LOG_MAIN,"RoomID=%d IndoorIndex=%d Count=%d\n",iter->dwRoomID, iter->dwIndoorIndex, iter->lstIndoorInfo.size());
				Packet_RoomIndoorInfo(buffer, *iter);
				iter++;
				bChange = true;
			}
			LOG_DEBUG(LOG_MAIN,"bufflen=%d\n",buffer.GetFilledSize());
			m_tHeader.commandflag = m_dwCmdFlag;
			m_tHeader.segmentflag = m_wSegFlag;
			PacketHeader(buffer, CMD_GET_DEVICE_ROOMINDOOR_REP, ERROR_NO, wTotalSeg, wSubSeg);
			AddMulPkt(dwDeviceID, MP_IndoorIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			m_dwCmdFlag++;
		}
		else
		{
			bChange = false;
			LOG_DEBUG(LOG_MAIN,"2 \n");
			DECLARE_PUTBUFFER( buffer )
			buffer << dwDeviceID << (int)1 << iter->dwRoomID << iter->dwIndoorIndex << dwPacketCount;
			LOG_DEBUG(LOG_MAIN,"DeviceID=%d RoomID=%d IndoorIndex=%d PacketCount=%d\n",dwDeviceID, iter->dwRoomID, iter->dwIndoorIndex, dwPacketCount);
			for (int i = 0; i < dwPacketCount; ++i)
			{
				PutVariableStr(buffer, (PUCHAR)pos->szSerialNO);
				buffer << pos->dwInDoorID;
				LOG_DEBUG(LOG_MAIN,"IndoorID=%d szSerialNO=%s\n",pos->dwInDoorID, pos->szSerialNO);
				pos++;
				if (pos == iter->lstIndoorInfo.end())
				{
					iter++;
					bChange = true;
					if (listInfo.end() != iter) pos = iter->lstIndoorInfo.begin();
					break;
				}
			}

			m_tHeader.commandflag = m_dwCmdFlag;
			m_tHeader.segmentflag = m_wSegFlag;
			PacketHeader(buffer, CMD_GET_DEVICE_ROOMINDOOR_REP, ERROR_NO, wTotalSeg, wSubSeg);
			AddMulPkt(dwDeviceID, MP_IndoorIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			m_dwCmdFlag++;
		}
		posCount++;
	}
	m_wSegFlag++;
	StartSendMulPkt(dwDeviceID, MP_IndoorIndex, this);
	return 0;
}

int CDevice::SendCmd_DeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& listInfo)
{
	LOG_DEBUG(LOG_MAIN,"CDevice::%s DeviceID %d\n", __FUNCTION__, dwDeviceID);
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		PacketHeader(buffer, CMD_GET_DEVICE_ROOMOTHER_REP, ERROR_NO);
		AddMulPkt(dwDeviceID, MP_Other, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		m_dwCmdFlag++;
		m_wSegFlag++;
		StartSendMulPkt(dwDeviceID, MP_Other, this);
		return 0;
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DeviceRoomOther(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_ROOMOTHER::iterator iter = listInfo.begin(), posAttr = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwPacketCount;
		LOG_DEBUG(LOG_MAIN,"DeviceID = %d, PacketCount = %d\n", dwDeviceID, dwPacketCount);
		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << (DWORD)iter->dwRoomID;
			PutVariableStr(buffer, (PUCHAR)iter->szRoom);
			buffer << (BYTE)0x10;
			buffer << CByteArrayBuffer((PUCHAR)iter->szPassword, LENGTH_ROOMPWD);
			buffer << (DWORD)0;
			iter++;
		}
		for(int j = 0; j < dwPacketCount; ++j)
		{
			buffer << (DWORD)posAttr->dwRoomID << (DWORD)posAttr->dwRoomAttr;
			LOG_DEBUG(LOG_MAIN,"RoomID = %d, RoomAttr = %d\n", posAttr->dwRoomID, posAttr->dwRoomAttr);
			posAttr++;
		}

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		PacketHeader(buffer, CMD_GET_DEVICE_ROOMOTHER_REP, ERROR_NO, wTotalSeg, wSubSeg);
		AddMulPkt(dwDeviceID, MP_Other, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		m_dwCmdFlag++;
	}
	m_wSegFlag++;
	StartSendMulPkt(dwDeviceID, MP_Other, this);
	return 0;
}

int CDevice::OnSendNext( DWORD dwCmdFlag, PUCHAR pData, int nLen )
{
	//LOG_DEBUG(LOG_MAIN, "%s dwCmdFlag %d nLen %d\n", __FUNCTION__, dwCmdFlag, nLen);
	if (m_pCon) return m_pCon->SendCommand(pData, nLen);
	return 0;
}

int CDevice::SendCmd_DeviceCfg(DWORD dwDeviceID, int nType, UcpaasInfo_t& tUcpaas, WORD wError /* = 0 */)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << dwDeviceID << nType;
	LOG_DEBUG(LOG_MAIN," CDevice::%s dwDeviceID=%d, nType=%d\n",__FUNCTION__, dwDeviceID, nType);
	PutVariableStr(buffer, tUcpaas.szUsername);
	PutVariableStr(buffer, tUcpaas.szPassword);
	PutVariableStr(buffer, tUcpaas.szAppid);
	LOG_DEBUG(LOG_MAIN,"szUsername=%s, szPassword=%s, szAppid=%s\n", tUcpaas.szUsername, tUcpaas.szPassword, tUcpaas.szAppid);
	return SendPacket(buffer, CMD_GET_DEVICE_CFG_REP, wError);
}

int CDevice::SendCmd_SystemCfg(DWORD dwVendorID, int nType, SystemCfg_t& tCfgInfo, WORD wError /* = 0 */)
{
	LOG_DEBUG(LOG_MAIN,"UserID:%s | AccountSid:%s | AuthToken:%s | AppKey:%s | PhoneNumber:%s\n",tCfgInfo.szUserID, tCfgInfo.szAccountSid, tCfgInfo.szAuthToken, tCfgInfo.szAppKey, tCfgInfo.szPhoneNumber);
	DECLARE_PUTBUFFER( buffer )
	buffer << dwVendorID << nType;
	PutVariableStr(buffer, tCfgInfo.szUserID);
	PutVariableStr(buffer, tCfgInfo.szAccountSid);
	PutVariableStr(buffer, tCfgInfo.szAuthToken);
	PutVariableStr(buffer, tCfgInfo.szAppKey);
	PutVariableStr(buffer, tCfgInfo.szPhoneNumber);
	return SendPacket(buffer, CMD_GET_DEVICE_CFG_REP, wError);
}

int CDevice::SendCmd_IndoorBindDevStatus(LIST_DEVSTATUS& lstDevStatus)
{
	LOG_DEBUG(LOG_MAIN,"[1013] CDevice::%s\n",__FUNCTION__);
	LIST_DEVSTATUS::iterator iter = lstDevStatus.begin();
	DWORD dwCount = lstDevStatus.size();
	DECLARE_PUTBUFFER( buffer )
	buffer << dwCount;
	for(; iter != lstDevStatus.end(); iter++)
	{
		buffer << iter->dwDeviceID << iter->dwStatus;
		LOG_DEBUG(LOG_MAIN,"DeviceID:%d - Status:%d\n",iter->dwDeviceID, iter->dwStatus);
	}
	LOG_DEBUG(LOG_MAIN,"[1014] CDevice::%s\n",__FUNCTION__);
	return SendPacket(buffer, CMD_GET_BIND_DEV_STATE_REP, ERROR_NO);
	return 0;
}

int CDevice::SendCmd_DeleteRoom( DWORD dwDeviceID, LIST_DWORD& lstRoomID )
{
	LOG_ASSERT_RET(LOG_MAIN, m_pCon, -1);
	DWORD dwCount = lstRoomID.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;
		return SendPacket(buffer, CMD_DELETE_DEVICE_ROOM, ERROR_NO);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DeleteRoom(lstRoomID, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_DWORD::iterator iter = lstRoomID.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			DWORD dwRoomID = *iter;
			buffer << dwRoomID;
			iter++;
		}
		SendPacket(buffer, CMD_DELETE_DEVICE_ROOM, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CDevice::SendCmd_Qiniu_UploadTokenRep(StorageTag_t& tTag, PUCHAR pUploadToken, WORD wError)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID;
	PutVariableStr(buffer, pUploadToken);
	return SendPacket(buffer, CMD_GET_UPLOAD_TOKEN_REP, wError);
}

#ifdef CACHE_DEVICE_STATUS
void CDevice::OnTimer(TimerReason_e eReason, ITimerSink* pSink)
{
	MAP_DEVST::iterator iter = m_mapDevSt.begin(), iterTemp;
	while (iter != m_mapDevSt.end())
	{
		iterTemp = iter; iterTemp++;
		BYTE bTTL = iter->second.bTTL++;
		if (bTTL > 6) m_mapDevSt.erase(iter); 
		iter = iterTemp;
	}
}
#endif

void CDevice::CacheDeviceStatus( DWORD dwUserID, DWORD dwStatus, PUCHAR pStatusMsg )
{
#ifdef CACHE_DEVICE_STATUS
	MAP_DEVST::iterator iter = m_mapDevSt.find(dwUserID);
	if (iter != m_mapDevSt.end())
	{
		iter->second.dwStatus = dwStatus;
		memcpy(iter->second.szStatusMsg, pStatusMsg, LENGTH_MSGCONTENT+1);
		iter->second.bTTL = 0;
	}
	else
	{
		CacheDeviceStatus_t tInfo; memset(&tInfo, 0, sizeof(CacheDeviceStatus_t));
		tInfo.dwStatus = dwStatus;
		memcpy(tInfo.szStatusMsg, pStatusMsg, LENGTH_MSGCONTENT+1);
		m_mapDevSt.insert(std::make_pair(dwUserID, tInfo));
	}
#endif
}

void CDevice::GetDeviceStatus(DWORD dwUserID, DeviceStatus_t& tInfo)
{
	tInfo.dwDeviceID = m_tDeviceInfo.dwDeviceID;
#ifdef CACHE_DEVICE_STATUS
	MAP_DEVST::iterator iter = m_mapDevSt.find(dwUserID);
	if (iter != m_mapDevSt.end())
	{
		tInfo.dwStatus = iter->second.dwStatus;
		memcpy(tInfo.szStatusMsg, iter->second.szStatusMsg, LENGTH_MSGCONTENT+1);
		LOG_DEBUG(LOG_MAIN,"DeviceID:%d,Status:%d\n",tInfo.dwDeviceID,tInfo.dwStatus);
		m_mapDevSt.erase(iter);
	}
	else tInfo.dwStatus = AT_ONLINE;
#else
	tInfo.dwStatus = AT_ONLINE;
#endif
}

void CDevice::GetDeviceStatus( DeviceStatus_t& tInfo )
{
	tInfo.dwDeviceID = m_tDeviceInfo.dwDeviceID;
#ifdef CACHE_DEVICE_STATUS
	if (m_mapDevSt.empty()) tInfo.dwStatus = AT_ONLINE;
	else tInfo.dwStatus = m_mapDevSt.begin()->second.dwStatus;
#else
	tInfo.dwStatus = AT_ONLINE;
#endif
}

void CDevice::ClearDeviceStatus( DWORD dwUserID )
{
#ifdef CACHE_DEVICE_STATUS
	MAP_DEVST::iterator iter = m_mapDevSt.find(dwUserID);
	if (iter != m_mapDevSt.end()) m_mapDevSt.erase(iter);
#endif
}

IMPLEMENT_SINGLETON(CDeviceMgr)
CDeviceMgr::CDeviceMgr()
{
	m_pSink = NULL;
	m_dwClientID = 0;
	m_nCapacity = 0;
	m_ePVID = PVID_Unknown;
}
CDeviceMgr::~CDeviceMgr()
{
#ifdef _TEST_STORAGE_
	DelTimer(TIMER_NORMAL, this);
#endif
	ThreadStop();
	UnRegisterNetListen(GROUPCODE_D_DEVICE, this);
}

bool CDeviceMgr::Start()
{
#ifdef _TEST_STORAGE_
	m_tTime = 0;
	time(&m_tTime);
	LOG_DEBUG(LOG_MAIN, "CDeviceMgr::Start Time %d\n", m_tTime);
	m_tTime -= 30*24*3600;
	struct tm tmTime;
	localtime_r(&m_tTime, &tmTime);
	BYTE szTimeStamp[LENGTH_TIMESTAMP2+1] = {0};
	snprintf((char*)szTimeStamp, LENGTH_TIMESTAMP2+1, "%04d%02d%02d%02d%02d%02d",
		tmTime.tm_year+1900, tmTime.tm_mon+1, tmTime.tm_mday, tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec);
	LOG_DEBUG(LOG_MAIN, "CDeviceMgr::Start Time %d:%s\n", m_tTime, szTimeStamp);

	m_nSize = 1000;
	AddTimer(TIMER_NORMAL, 3, this);
#endif
	InitPlatformVID();
	RegisterNetListen(GROUPCODE_D_DEVICE, this); 
	ThreadStart();

	if(PVID_Dong == m_ePVID) AddNotify_OnOff((PUCHAR)"FFFFFFFFFFFFFFFFFFFF", 0);
	return true;
}
void CDeviceMgr::InitPlatformVID()
{
	DWORD dwEth0 = Net_GetEthIP("eth0");
	DWORD dwEth1 = Net_GetEthIP("eth1");

	if((0x7928A1E4 == dwEth1) || (0x7928A1E4 == dwEth0)) m_ePVID = PVID_Dong;
	else
	if((0xB65CB789 == dwEth1) || (0xB65CB789 == dwEth0)) m_ePVID = PVID_Aurine;
	else
	if((0x792B9901 == dwEth1) || (0x792B9901 == dwEth0)) m_ePVID = PVID_Mili;
	else
	if((0x781A84FD == dwEth1) || (0x781A84FD == dwEth0)) m_ePVID = PVID_Fuju;

	LOG_DEBUG(LOG_MAIN, "eth0: 0x%08x eth1: 0x%08x PVID: %d\n", dwEth0, dwEth1, m_ePVID);
}
bool CDeviceMgr::AddDevice(DWORD dwDeviceID, CDevice* pDevice)
{
	CDevice* pPreDevice = Template_GetDevice(dwDeviceID);
	if(pPreDevice)
	{
		DWORD dwID = pPreDevice->GetElemID();
		DWORD dwClientID = pDevice->GetElemID();
		LOG_DEBUG(LOG_MAIN,"New dwClientID = %d\n", dwClientID);
		DelElem(dwID);
		Template_ReplaceDevice(dwDeviceID, pDevice);
		LOG_DEBUG(LOG_MAIN, "dwDeviceID %d Register Again\n", dwDeviceID);	
		return false;
	}

	// NULL == pPreDevice
	Template_AddDevice(dwDeviceID, pDevice);
	int nCurOnline = Template_GetSize();
	LOG_DEBUG(LOG_MAIN, "CDeviceMgr::%s dwDeviceID %d pDevice %p Cur Online %d\n", __FUNCTION__, dwDeviceID, pDevice, nCurOnline);

	if (m_pSink)
	{
		BYTE szMsg[LENGTH_MSGCONTENT+1] = {0};
		LIST_SMSINFO list1;
		LIST_PUSHINFO list2;
		ReportDevStatus devStatus;
		m_pSink->Dev_OnReportDeviceStatus(dwDeviceID, AT_ONLINE, (PUCHAR)szMsg, list1, list2, devStatus);
	}
	
	//通知sdb设备上线存入数据库
	AlarmStatus_t tStatus;
	tStatus.dwDeviceID = dwDeviceID;
	tStatus.dwRoomID  = 0;
	tStatus.dwType = 2;
	tStatus.dwSubType = 1;
	tStatus.dwTimeStamp = time(NULL) +  (8 * 3600);
	pPreDevice->ReportAlarmStatus(tStatus);

	return true;
}
void CDeviceMgr::DelDevice(DWORD dwDeviceID)
{
	CDevice* pPreDevice = Template_GetDevice(dwDeviceID);
	//通知sdb设备下线存入数据库
	AlarmStatus_t tStatus;
	tStatus.dwDeviceID = dwDeviceID;
	tStatus.dwRoomID  = 0;
	tStatus.dwType = 2;
	tStatus.dwSubType = 0;
	tStatus.dwTimeStamp = time(NULL) +  (8 * 3600);
	pPreDevice->ReportAlarmStatus(tStatus);
	//////////////////////////////////////////////////////////////////////////
	Template_DelDevice(dwDeviceID);
	int nCurCount = Template_GetSize();
	LOG_DEBUG(LOG_MAIN, "CDeviceMgr::%s dwDeviceID %d Cur Online %d\n", __FUNCTION__, dwDeviceID, nCurCount);

	if(m_pSink)
	{
		BYTE szMsg[LENGTH_MSGCONTENT+1] = {0};
		LIST_SMSINFO list1;
		LIST_PUSHINFO list2;
		ReportDevStatus devStatus;
		m_pSink->Dev_OnReportDeviceStatus(dwDeviceID, AT_OFFLINE, (PUCHAR)szMsg, list1, list2, devStatus);
	}
}

bool CDeviceMgr::AddNotify_OnOff(PUCHAR pSN, char cOnOff)
{
	LOG_DEBUG(LOG_MAIN, "%s_1 sn: %s st: %d\n", __FUNCTION__, pSN, cOnOff);
	// 通知到云上城平台的php文件不存在
	if(access(PATH_NOTIFY_ONOFF_YSC,  F_OK)) {
		LOG_EMERG(LOG_MAIN, "nofile: %s\n", PATH_NOTIFY_ONOFF_YSC);
		return false;
	}
	LOG_ASSERT_RET(LOG_MAIN, pSN, false);

	Lock();
	if (m_listOnOffLine.size() > 1000)
	{
		LOG_EMERG(LOG_MAIN, "m_listOnOffLine.size() %d busy\n", m_listOnOffLine.size());
		UnLock();
		return false;
	}
	OnOff_t tInfo;
	tInfo.strSN.assign((const char*)pSN, strlen((const char*)pSN));
	tInfo.cOnOff = cOnOff ? 1 : 0;
	m_listOnOffLine.push_back(tInfo);
	UnLock();
	ActivateThread();
	return true;
}
#define DM_MAX_CMD_LEN 100
void CDeviceMgr::ThreadLoop()
{
	while (m_bRunning)
	{
		HangUpThread();
		ProcessThreadCommand();
	}
	if(PVID_Dong == m_ePVID) 
	{
		char szCmdBuf[DM_MAX_CMD_LEN] = {0};
		const char* pSN = "FFFFFFFFFFFFFFFFFFFF";
		snprintf((char*)szCmdBuf, DM_MAX_CMD_LEN, "php %s %s %d", PATH_NOTIFY_ONOFF_YSC, pSN, 0);
		ExecCommand((const char*)szCmdBuf);
	}
	LOG_DEBUG(LOG_MAIN, "CDeviceMgr::%s TreadID:%d Exit\n", __FUNCTION__, GetThreadID());
}
void CDeviceMgr::ProcessThreadCommand()
{
	// get command list
	LIST_ONOFF listCommand; listCommand.clear();
	Lock();
	if (m_listOnOffLine.empty()) { UnLock(); return; }
	listCommand.insert(listCommand.end(), m_listOnOffLine.begin(), m_listOnOffLine.end());
	m_listOnOffLine.clear();
	UnLock();

	char szCmdBuf[DM_MAX_CMD_LEN];
	LIST_ONOFF::iterator iter = listCommand.begin();
	for (; iter != listCommand.end(); iter++)
	{
		// 示例: php /home/dong_server/d/YunSC/YunSC_Notify.php NCOQ12FDCZ00CZTECLEK 1
		memset(szCmdBuf, 0, DM_MAX_CMD_LEN);
		snprintf((char*)szCmdBuf, DM_MAX_CMD_LEN, "php %s %s %d", PATH_NOTIFY_ONOFF_YSC, iter->strSN.c_str(), iter->cOnOff);
		ExecCommand((const char*)szCmdBuf);
	}
}
#define MAX_LINE_LEN 484
bool CDeviceMgr::ExecCommand( const char* pCommand)
{
	static DWORD s_dwIndex = 0; s_dwIndex++;
	LOG_DEBUG(LOG_MAIN, "ExecCommand(Index=%d): %s\n", s_dwIndex, pCommand);

	FILE* fp = popen(pCommand, "r");
	if (!fp) {
		LOG_ERR(LOG_MAIN, "popen %s failed\n", pCommand);
		return false;
	}
	char szCommand[MAX_LINE_LEN+1] = {0};
	while( fgets(szCommand, sizeof(szCommand), fp) ) 
	{
		LOG_DEBUG(LOG_MAIN, "CmdRep(Index=%d):%s\n", s_dwIndex, szCommand);
		memset(szCommand, 0, MAX_LINE_LEN);
	}
	if (-1 == pclose(fp))
	{
		LOG_DEBUG(LOG_MAIN, "pclose(err:%d:%s)\n", errno, strerror(errno)); return false;
	}
	return true;
}

int CDeviceMgr::OnDispatchConnection( INetConnection* pCon, int nNetType, PUCHAR pData, int nLen )
{
	int nCurOnline = Template_GetSize();
	if ( m_nCapacity && (nCurOnline >= m_nCapacity) )
	{
		NetworkDestroyConnection(pCon); ToServerCapacity(); return 0;
	}

	CDevice* p = NULL;
	try
	{
		p = new CDevice(++m_dwClientID);
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

void CDeviceMgr::Dev_GetDeviceStatus( DWORD dwUserID, LIST_DWORD& listDeviceID, LIST_DEVICESTATUS& listInfo )
{
	LOG_DEBUG(LOG_MAIN, "CDeviceMgr::%s\n", __FUNCTION__);
	LIST_DWORD::iterator iter = listDeviceID.begin(), iterTemp;
	iterTemp = iter;
	while (iter != listDeviceID.end())
	{
		iterTemp++;
		CDevice* pDevice = Template_GetDevice(*iter);
		if (pDevice)
		{
			DeviceStatus_t tInfo; memset(&tInfo, 0, sizeof(DeviceStatus_t));
			pDevice->GetDeviceStatus(dwUserID, tInfo);
			listInfo.push_back(tInfo);

			listDeviceID.erase(iter);
		}
		iter = iterTemp;
	}
}
//室内机获取门口机在线离线
void CDeviceMgr::Dev_GetDeviceStatus(LIST_DEVSTATUS& lstDevStatus)
{
	LOG_DEBUG(LOG_MAIN, "[2005] CDeviceMgr::%s\n", __FUNCTION__);
	LIST_DEVSTATUS::iterator iter = lstDevStatus.begin();
	for(; iter != lstDevStatus.end(); iter++)
	{
		CDevice* pDevice = Template_GetDevice(iter->dwDeviceID);
		if(pDevice)	iter->dwStatus = AT_ONLINE;
		else	iter->dwStatus = AT_OFFLINE;
		LOG_DEBUG(LOG_MAIN,"DeviceID:%d - Status:%d\n",iter->dwDeviceID, iter->dwStatus);
	}
}

bool CDeviceMgr::Dev_GetDeviceConnectInfo(TransClientInfo_t& tClientInfo, TransServerInfo_t& tServerInfo, bool bTransClientInfo, DWORD& dwAutoRelay)
{
	tServerInfo.bSrcType = tClientInfo.bSrcType;
	tServerInfo.dwServerID = tClientInfo.dwServerID;
	tServerInfo.dwUserID = tClientInfo.dwUserID;
	tServerInfo.dwSessionID = tClientInfo.dwSessionID;
	tServerInfo.dwDeviceID = tClientInfo.dwDeviceID;
	tServerInfo.bType = tClientInfo.bType;
	LOG_DEBUG(LOG_MAIN, "CDeviceMgr::%s bSrcType %d dwServerID %d dwUserID %d dwSessionID %d dwDeviceID %d bType %d\n",
		__FUNCTION__, tServerInfo.bSrcType, tServerInfo.dwServerID, tServerInfo.dwUserID, tServerInfo.dwSessionID,
		tServerInfo.dwDeviceID, tServerInfo.bType);

	//////////////////////////////////////////////////////////////////////////
	// 测试
	//tServerInfo.tConnectInfo[0].dwID = tServerInfo.dwDeviceID;
	//tServerInfo.tConnectInfo[0].tNetInfo.dwPublicIP = IpStr2Dword((char*)"192.168.68.124");
	//tServerInfo.tConnectInfo[0].tNetInfo.wPublicPortTCP = 9529;
	//tServerInfo.tConnectInfo[0].tNetInfo.wPublicPortUDP = 9529;
	//tServerInfo.tConnectInfo[0].tNetInfo.wLocalPortUDP = 0;
	//tServerInfo.tConnectInfo[0].tNetInfo.wNetworkType = 3;
	//tServerInfo.tConnectInfo[0].tNetInfo.listLocalIPs = m_tNetInfo.listLocalIPs;
	//memcpy(tServerInfo.tConnectInfo[0].szPassword, "123456", 6);
	//return true;
	//////////////////////////////////////////////////////////////////////////
	CDevice* pDevice = Template_GetDevice(tServerInfo.dwDeviceID);
	if (NULL == pDevice) return false;
	tServerInfo.tConnectInfo[0].dwID = tServerInfo.dwDeviceID;
	pDevice->GetDeviceNetInfo(tServerInfo.tConnectInfo[0].tNetInfo, dwAutoRelay);
	pDevice->GetDevicePassword(tServerInfo.tConnectInfo[0].szPassword);
	if (bTransClientInfo) pDevice->SendCmd_TransClientInfo(tClientInfo);
	//////////////////////////////////////////////////////////////////////////
	// 当有用户请求观看时将状态清除
	pDevice->ClearDeviceStatus(tClientInfo.dwUserID);
	return true;
}

void CDeviceMgr::Dev_TransClientInfo(TransClientInfo_t& tClientInfo)
{
	CDevice* pDevice = Template_GetDevice(tClientInfo.dwDeviceID);
	if (NULL == pDevice) return;
	pDevice->SendCmd_TransClientInfo(tClientInfo);
}
DWORD CDeviceMgr::Dev_GetDeviceType(DWORD dwDeviceID)
{
	CDevice* pDevice = Template_GetDevice(dwDeviceID);
	if (NULL == pDevice) return 0;
	return pDevice->GetDeviceType();
}
PUCHAR CDeviceMgr::Dev_GetDeviceImgVer(DWORD dwDeviceID)
{
	CDevice* pDevice = Template_GetDevice(dwDeviceID);
	if (NULL == pDevice) return NULL;
	return pDevice->GetDeviceImgVer();
}

void CDeviceMgr::Dev_GetDeviceList( LIST_DEVICESTATUS& listInfo )
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	for (int i = 0; i < MAP_COUNT_DEVICE; i++)
	{
		ELEM_MAP::iterator iter = m_mapDevice[i].begin();
		for (; iter != m_mapDevice[i].end(); iter++)
		{
			DeviceStatus_t tInfo; memset(&tInfo, 0, sizeof(DeviceStatus_t));
			iter->second->GetDeviceStatus(tInfo);
			listInfo.push_back(tInfo);
		}
	}
}

void CDeviceMgr::Dev_UpdateDeviceRoomInfo( DWORD dwVendorID, DWORD dwDeviceID, BYTE bType, LIST_DWORD& lstRoomID )
{
	LOG_DEBUG(LOG_MAIN, "%s dwVendorID %d dwDeviceID %d bType %d RoomCount %d\n",
		__FUNCTION__, dwVendorID, dwDeviceID, bType, lstRoomID.size());
	CDevice* pDevice = Template_GetDevice(dwDeviceID);
	if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, dwDeviceID); return; }
	pDevice->UpdateDeviceRoomInfo(bType, lstRoomID);
}
//发送截至日期07
void CDeviceMgr::Dev_SendDeviceDeadLine(DeviceDeadLineInfo_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN,"107:%s\n",__FUNCTION__);
	CDevice* pDevice = Template_GetDevice(tInfo.dwDeviceID);
	if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, tInfo.dwDeviceID); return; }
	pDevice->SendDeviceDeadLine(tInfo);
}

//66.更新物業公告
void CDeviceMgr::Dev_SendPropertyAnnounce(DWORD dwNoticeIndex, LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_MAIN,"CDeviceMgr::%s NoticeIndex = %d\n",__FUNCTION__, dwNoticeIndex);
	LIST_DWORD::iterator iter = lstDevID.begin();
	for(; iter != lstDevID.end(); iter++)
	{
		CDevice* pDevice = Template_GetDevice(*iter);
		if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, *iter); continue; }
		pDevice->SendDevicePropertyAnnounce(dwNoticeIndex);
	}
}
//更新广告
void CDeviceMgr::Dev_SendAdvertInfo(LIST_ADVERT& lstDevAdvert)
{
	LOG_DEBUG(LOG_MAIN,"CDeviceMgr::%s\n",__FUNCTION__);
	LIST_ADVERT::iterator iter = lstDevAdvert.begin();
	for(; iter != lstDevAdvert.end(); iter++)
	{
		CDevice* pDevice = Template_GetDevice(iter->dwDeviceID);
		if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, iter->dwDeviceID); continue; }
		pDevice->SendDeviceAdvertInfo(iter->dwAdvertIndex);
	}
}


//3.1.21 云平台呼叫到所有绑定的室内机
void CDeviceMgr::Dev_CallIndoor(LIST_DWORD& lstIndoorID, DWORD dwStatus, DWORD dwDeviceID)
{
	LOG_DEBUG(LOG_MAIN,"CDeviceMgr::%s\n",__FUNCTION__);
	LIST_DWORD::iterator iter = lstIndoorID.begin();
	for(; iter != lstIndoorID.end(); iter++)
	{
		DWORD dwIndoorID = *iter;
		CDevice* pDevice = Template_GetDevice(dwIndoorID);
		if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No IndoorID %d\n", __FUNCTION__, dwIndoorID); return; }
		pDevice->SendCallIndoor(dwDeviceID, dwStatus);
	}
}
//
void CDeviceMgr::Dev_SendFirmwareRequest(DevUpgrad_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN,"%s\n",__FUNCTION__);
	CDevice* pDevice = Template_GetDevice(tInfo.dwDeviceID);
	if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, tInfo.dwDeviceID); return; }
	pDevice->SendFirmwareRequest(tInfo);
}

//删除在线设备
void CDeviceMgr::Dev_SendDeleteDeviceOnline(LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_MAIN,"%s\n",__FUNCTION__);
	LIST_DWORD::iterator iter = lstDevID.begin();
	for(; iter != lstDevID.end(); iter++)
	{
		DWORD dwDeviceID = *iter;
		LOG_DEBUG(LOG_MAIN,"DeviceID=%d\n",dwDeviceID);
		CDevice* pDevice = Template_GetDevice(dwDeviceID);
		if(pDevice) DelElem(pDevice->GetElemID());
		DelDevice(dwDeviceID);
	}
}

void CDeviceMgr::Dev_ClearRooms( LIST_DWORD& lstDeviceID )
{
	LOG_DEBUG(LOG_MAIN, "%s DeviceCount %d\n", __FUNCTION__, lstDeviceID.size());
	LIST_DWORD::iterator iter = lstDeviceID.begin();
	for ( ; iter != lstDeviceID.end(); iter++)
	{
		CDevice* pDevice = Template_GetDevice(*iter);
		if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, *iter); continue; }
		LIST_DWORD lstRoomID;
		pDevice->ClearRooms(lstRoomID);
	}
}

void CDeviceMgr::Dev_UpdateDevice( LIST_DWORD& lstDeviceID )
{
	LOG_DEBUG(LOG_MAIN, "%s DeviceCount %d\n", __FUNCTION__, lstDeviceID.size());
	LIST_DWORD::iterator iter = lstDeviceID.begin();
	for ( ; iter != lstDeviceID.end(); iter++)
	{
		CDevice* pDevice = Template_GetDevice(*iter);
		if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, *iter); continue; }
		pDevice->UpdateDevice();
	}
}

void CDeviceMgr::Dev_UpdateDeviceEx(int nType, LIST_DWORD& lstDeviceID)
{
	LOG_DEBUG(LOG_MAIN, "%s nType %d DeviceCount %d\n", __FUNCTION__, nType, lstDeviceID.size());
	LIST_DWORD::iterator iter = lstDeviceID.begin();
	for ( ; iter != lstDeviceID.end(); iter++)
	{
		CDevice* pDevice = Template_GetDevice(*iter);
		if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, *iter); continue; }
		pDevice->UpdateDeviceEx(nType);
	}
}
//3.1.20 通知室内机其绑定设备的在线离线状态
void CDeviceMgr::Dev_GetDevBindIndoorID(DevStatus& tDevStat,LIST_DWORD& lstIndoorID)
{
	LOG_DEBUG(LOG_MAIN, "CDeviceMgr::%s\n", __FUNCTION__);
	LIST_DWORD::iterator iter = lstIndoorID.begin();
	for ( ; iter != lstIndoorID.end(); iter++)
	{
		CDevice* pDevice = Template_GetDevice(*iter);
		if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, *iter); continue; }
		pDevice->UpdateDevStat(tDevStat);
	}
}

void  CDeviceMgr::Dev_SmsSpecialCrowd(SmsInfo2_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "CDeviceMgr::%s\n", __FUNCTION__);
	if (m_pSink) m_pSink->Dev_OnSmsSpecialCrowd(tInfo);
}

void CDeviceMgr::Dev_VideoAdvertUrl(DWORD dwDeviceID, AdvertInfoRep_t& tAdvertRep)
{
	CDevice* pDevice = Template_GetDevice(dwDeviceID);
	if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, dwDeviceID); return; }
	pDevice->OnGetAdvertUrls_Rep(tAdvertRep);
}

void CDeviceMgr::Dev_Capacity( int nCapacity )
{
	//LOG_DEBUG(LOG_MAIN, "%s nCapacity %d\n", __FUNCTION__, nCapacity);
	m_nCapacity = nCapacity;

	ToServerCapacity();
}

void CDeviceMgr::Dev_CacheDeviceStatus( DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg )
{
	CDevice* pDevice = Template_GetDevice(dwDeviceID);
	if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, dwDeviceID); return; }
	BYTE szMessage[LENGTH_MSGCONTENT+1] = {0};
	sprintf((char*)szMessage, "C0%s|%lu|%lu", pStatusMsg, dwStatus, dwDeviceID);
	pDevice->CacheDeviceStatus(dwUserID, dwStatus, szMessage);
}

bool CDeviceMgr::Dev_SdkTunnel(SdkTunnel_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
	CDevice* pDevice = Template_GetDevice(tInfo.dwDeviceID);
	if (NULL == pDevice) return false;
	pDevice->SendCmd_SdlTunnel(tInfo);
	return true;
}

void CDeviceMgr::Dev_Qiniu_UploadToken(StorageTag_t& tTag, PUCHAR pUploadToken)
{
	LOG_DEBUG(LOG_MAIN, "%s UploadToken %s\n", __FUNCTION__, pUploadToken);
	CDevice* pDevice = Template_GetDevice(tTag.dwTagID1);
	if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, tTag.dwTagID1); return; }
	pDevice->SendCmd_Qiniu_UploadTokenRep(tTag, pUploadToken, ERROR_NO);
}

void CDeviceMgr::Dev_Qiniu_DownloadUrls(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo)
{

}

void CDeviceMgr::ToServerCapacity()
{
	int nOnline = Template_GetSize();
	if ( (m_nCapacity == 0) || (nOnline <= m_nCapacity) ) return;

	int nDelete = nOnline - m_nCapacity;
	for (int i = 0; i < nDelete; i++)
	{
		OfflineDevice();
	}
}

void CDeviceMgr::OfflineDevice()
{
	CDevice* pDevice = Template_GetBegin();
	DWORD dwID = 0;
	if(pDevice) dwID = pDevice->GetElemID();
	DelElem(dwID);
}
//同步1
void  CDeviceMgr::Dev_SendDeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& listDeviceRoomOther)
{
	LOG_DEBUG(LOG_MAIN,"CDeviceMgr::%s DeviceID %d\n", __FUNCTION__, dwDeviceID);
	CDevice* pDevice = Template_GetDevice(dwDeviceID);
	if (NULL == pDevice) { LOG_DEBUG(LOG_MAIN, "%s No DeviceID %d\n", __FUNCTION__, dwDeviceID); return; }
	pDevice->SendCmd_DeviceRoomOther(dwDeviceID, listDeviceRoomOther);
}


#ifdef _TEST_STORAGE_
void CDeviceMgr::OnTimer(TimerReason_e eReason, ITimerSink* pSink)
{
	StoreKey_t tKey; memset(&tKey, 0, sizeof(StoreKey_t));
	tKey.dwSize = m_nSize++;
	tKey.dwStoreID = (m_nSize % 2) + 1;
	if (tKey.dwStoreID % 2) // wuye123
	{
		tKey.dwDeviceID = 11;
		tKey.dwRoomID = 128;
	}
	else // mltest
	{
		tKey.dwDeviceID = 35;
		tKey.dwRoomID = 82;
	}
	tKey.bType = 1;
	tKey.bRecReason = 10;
	struct tm tmTime;
	localtime_r(&m_tTime, &tmTime);
	snprintf((char*)tKey.szTimeStamp, LENGTH_TIMESTAMP2+1, "%04d%02d%02d%02d%02d%02d",
		tmTime.tm_year+1900, tmTime.tm_mon+1, tmTime.tm_mday, tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec);

	if (m_pSink) m_pSink->Dev_OnQiniu_ReportUploadResult(tKey);
	m_tTime += 10;
}
#endif
