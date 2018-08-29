#include "DBClientHandle.h"
#include "getbuffer.h"
#include "UtilityInterface.h"
#include "CGetUserInfoMgr.h"
#include "CGetUserGroupMgr.h"
#include "CGetUserDeviceMgr.h"
#include "CGetUserRoomMgr.h"
#include "DataBaseA.h"
#include "DataBaseB.h"
#include "DataBaseC.h"
#include <algorithm>
#include <map>

const CDBClientHandle::HandlerEntry CDBClientHandle::m_handlers[] =
{
	{ CMD_GET_CHALLENGE_REP,		&CDBClientHandle::OnChallenge			},
	{ CMD_AUTH_REP,					&CDBClientHandle::OnAuth				},
	{ CMD_GET_SERVER_INFO_REP,		&CDBClientHandle::OnGetServerInfo		},
	{ CMD_QUERY_USER_REP,			&CDBClientHandle::OnQueryMobilePhone	},
	{ CMD_SET_SECRET_REP,			&CDBClientHandle::OnSetSecret			},
	{ CMD_GET_USER_INFO_REP,		&CDBClientHandle::OnGetUserInfo		},
	{ CMD_GET_USERDEVICE_INFO_REP,	&CDBClientHandle::OnGetUserDeviceInfo	},
	{ CMD_GET_USERGROUP_INFO_REP,	&CDBClientHandle::OnGetUserGroupInfo	},
	{ CMD_GET_USERROOM_INFO_REP,	&CDBClientHandle::OnGetUserRoomInfo	},
	{ CMD_GET_DEVICE_INFO_REP,		&CDBClientHandle::OnGetDeviceInfo		},
	{ CMD_GET_DEVICEUSER_INFO_REP,	&CDBClientHandle::OnGetDeviceUserInfo	},
	{ CMD_GET_DEVICEPUSH_INFO_REP,	&CDBClientHandle::OnGetDevicePushInfo	},
	{ CMD_ADD_DEVICE_REP,			&CDBClientHandle::OnAddDevice			},
	{ CMD_ADD_DEL_PUSH_INFO_EX_REP,	&CDBClientHandle::OnAddDelPushInfoEx	},
	{ CMD_SET_DEVICE_NAME_REP,		&CDBClientHandle::OnSetDeviceName		},
	{ CMD_DEL_DEVICE_REP,			&CDBClientHandle::OnDelDevice			},
	{ CMD_AUTHORIZE_REP,			&CDBClientHandle::OnAuthorize			},
	{ CMD_AUTHORIZE2_REP,			&CDBClientHandle::OnAuthorize2		},
	{ CMD_PIECE_OF_SERIALNO,		&CDBClientHandle::OnPieceOfSerialNO	},
	{ CMD_DSERVER_CONFIGUREINDEX,	&CDBClientHandle::OnDserverConfigureIndex	},
	{ CMD_USER_CONFIGUREINDEX,		&CDBClientHandle::OnUserConfigureIndex	},
	{ CMD_DEVICE_CONFIGUREINDEX,	&CDBClientHandle::OnDeviceConfigureIndex	},
	{ CMD_ERROR,					&CDBClientHandle::OnError				},

	{ CMD_GET_DEVICE_ROOM_SUM_REP,	&CDBClientHandle::OnGetDeviceRoomSum	},
	{ CMD_GET_DEVICE_ROOMUSER_REP,	&CDBClientHandle::OnGetDeviceRoomUser	},
	{ CMD_GET_DEVICE_ROOMPUSH_REP,	&CDBClientHandle::OnGetDeviceRoomPush	},
	{ CMD_GET_DEVICE_ROOMCARD_REP,	&CDBClientHandle::OnGetDeviceRoomCard	},
	{ CMD_GET_DEVICE_ROOMOTHER_REP,	&CDBClientHandle::OnGetDeviceRoomOther	},
	{ CMD_GET_DEVICE_ROOMINDOOR_REP,&CDBClientHandle::OnGetDeviceRoomIndoor	},

	{ CMD_UPDATE_DEVICE_ROOMINFO,	&CDBClientHandle::OnUpdateDeviceRoomInfo	},
	{ CMD_CLEAR_ROOMS,				&CDBClientHandle::OnClearRooms			},
	{ CMD_UPDATE_DEVICE,			&CDBClientHandle::OnUpdateDevice			},

	{ CMD_GET_DSERVER_INFO_REP,		&CDBClientHandle::OnGetDServerInfo		},
	
	{ CMD_GET_STORAGEACCOUNT_REP,	&CDBClientHandle::OnQiniu_GetStorageAccount	},
	{ CMD_GET_STORAGEKEYS_REP,		&CDBClientHandle::OnQiniu_GetStorageKeys		},

	{ CMD_UPDATE_DEVICE_EX,			&CDBClientHandle::OnUpdateDeviceEx		},
	{ CMD_GET_DEVICE_CFG_REP,		&CDBClientHandle::OnGetDeviceCfg			},
    { CMD_PUSH_SWITCH_REP,			&CDBClientHandle::OnSetPushSwitch			},
	{ CMD_GET_DEVICE_DEADLINE_REP,	&CDBClientHandle::OnGetDeviceDeadLine		},
	{ DD_PUSH_UNLOCK_RECORD_REP,	&CDBClientHandle::OnUnlockRecords			},
	{ CMD_NOTICE_INFO,				&CDBClientHandle::OnUpdatePropertyAnnounce	},
	{ CMD_ADVERT_INFO,				&CDBClientHandle::OnUpdateAdvertInfo		},
	//请求回应
	{ DD_UPDATE_BULLETIN_REP,		&CDBClientHandle::OnUpdateBulletin			},
	{ CMD_GET_ADVERT_INFO_REP,		&CDBClientHandle::OnGetAdvertInfo			},
	{ CMD_UPDATE_VISITORCFG_REP,	&CDBClientHandle::OnGetVisitorCfg		},

	{ DD_UPDATE_FIRMWARE_REQUEST,   &CDBClientHandle::OnUpdateFirmwareRequest   },
	{ DD_DELETE_DEVICE_ONLINE,		&CDBClientHandle::OnDeleteDeviceOnline		},
	//3.2.14 室内机绑定门口机
	{ CMD_INDOOR_BIND_DEVICE_REP,	&CDBClientHandle::OnIndoorBindDeviceRep		},
	{ CMD_GET_BIND_DEV_STATE_REP,	&CDBClientHandle::OnGetIndoorBindDevList	},//门口机
	{ CMD_GET_DEV_BIND_IDNOOR_REP,  &CDBClientHandle::OnGetDevBindIndoorID		},//app
	{ CMD_GET_LIST_DEV_STAT_REP,	&CDBClientHandle::OnGetListDevStat			},//列表
	{ CMD_SMS,									&CDBClientHandle::OnSmsSpecialCrowd },

	//StorageBusiness
	{ CMD_GET_ADVERTURLS_REP,		&CDBClientHandle::OnGetAdvertUrls_Rep		},
};

CDBClientHandle::CDBClientHandle()
{
	m_pCon = NULL;
	m_bGroupCode = 0;
	m_pSink = NULL;
	m_pSdbSink = NULL;
	m_pSbSink = NULL;
	memset(m_szSerialNO, 0, LENGTH_SERIALNO+1);
	memset(m_szUserName, 0, LENGTH_NAME+1);
	memset(m_szPassword, 0, LENGTH_PASSWORD+1);
}

CDBClientHandle::~CDBClientHandle()
{

}

int CDBClientHandle::SendDeviceRoomSumRepAck(DWORD dwDeviceID)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwDeviceID;
	return SendMsg(bufferPut, CMD_GET_DEVICE_ROOM_SUM_REP_ACK, 0);
}

int CDBClientHandle::SendDeviceRoomInfoRepAck(DWORD dwDeviceID, BYTE bType)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwDeviceID << bType;
	return SendMsg(bufferPut, CMD_GET_DEVICE_ROOMINFO_REP_ACK, 0);
}

int CDBClientHandle::SendMsg(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg /* = 1 */, WORD wSubSeg /* = 1 */)
{
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pCon, -1);
	int nLen = buffer.GetFilledSize();
	buffer.SetOffset(0);
	// Header
	buffer << (BYTE)m_bGroupCode << (WORD)wCommand/*Command ID*/ << (BYTE)0/*Reserved0*/
		<< (WORD)0x0001/*Version*/ << (WORD)0/*Reserved1*/
		<< (DWORD)m_dwDataBaseAID/*Source ID*/
		<< (DWORD)0/*Destination ID*/
		<< (DWORD)m_tHeader.commandflag/*Command Flag*/
		<< (WORD)wTotalSeg/*Total Segment*/ << (WORD)wSubSeg/*Sub Segment*/
		<< (WORD)m_tHeader.segmentflag/*Segment Flag*/ << (WORD)0/*Reserved2*/
		<< (DWORD)0/*Reserved3*/;
	// Payload
	buffer << (WORD)wError/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;
	// Payload Data
	buffer.SetOffset(nLen);
	int nRealLen = m_pCon->SendCommand((PUCHAR)buffer, nLen);
	LOG_DEBUG(LOG_DB_CLIENT, "pCon %p SendData cmd:0x%04x err:0x%04x len:%d reallen %d SrcID %d\n", m_pCon, wCommand, wError, nLen, nRealLen, m_dwDataBaseAID);
	return nRealLen;
}

int CDBClientHandle::GetChallenge()
{
	DECLARE_PUTBUFFER(bufferPut)
	return SendMsg(bufferPut, CMD_GET_CHALLENGE, 0);
}

int CDBClientHandle::Auth(PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pChallenge)
{
	LOG_DEBUG(LOG_DB_CLIENT, "CDBClientHandle::%s\n", __FUNCTION__);
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << CByteArrayBuffer(pSN, LENGTH_SERIALNO);
	PutVariableStr(bufferPut, pUserName);
	BYTE szDigist[LENGTH_CHALLENGE+1] = {0};
	CalcAuthDigist((PUCHAR)szDigist, pUserName, pPassword, pChallenge);
	LOG_DEBUG(LOG_DB_CLIENT, "szDigist %s pUserName %s pPassword %s pChallenge %s\n", szDigist, pUserName, pPassword, pChallenge);
	bufferPut << CByteArrayBuffer(szDigist, LENGTH_CHALLENGE);
	return SendMsg(bufferPut, CMD_SERVER_AUTH, 0);
}

int CDBClientHandle::ProcessCommand(PUCHAR pData, int nLen)
{
	if ( false == ParsePacketHeader(pData, nLen, m_tHeader) ) return -1;
	if (m_tHeader.groupcode != m_bGroupCode) return -1;

	const int nHandlerCount = sizeof ( m_handlers )/sizeof(HandlerEntry);
	for ( int i = 0; i < nHandlerCount; ++i )
	{
		if ( m_handlers[i].wCommand != m_tHeader.commandid ) continue;	
		PMFHANDLER pmfHandler = m_handlers[i].pmfHandler;
		(this->*pmfHandler)(pData+PACKET_HEADER_SIZE, nLen-PACKET_HEADER_SIZE);
		break;
    }
	return -1;
}

int CDBClientHandle::OnChallenge( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);

	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	if (m_tHeader.error)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "DataBase Auth Failed (error:%d)\n", m_tHeader.error); return -1;
	}

	int nNeedLen = LENGTH_CHALLENGE;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	BYTE szChallengeStr[LENGTH_CHALLENGE+1] = {0};
	bufferGet >> CByteArrayBuffer((PUCHAR)szChallengeStr, LENGTH_CHALLENGE);
	LOG_DEBUG(LOG_DB_CLIENT, "szChallengeStr = %s\n", szChallengeStr);

	Auth((PUCHAR)m_szSerialNO, (PUCHAR)m_szUserName, (PUCHAR)m_szPassword, (PUCHAR)szChallengeStr);
	return 0;
}

const int DATABASE_STATUS_AUTH = 1;
int CDBClientHandle::OnAuth( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	if (m_tHeader.error)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "2 DataBase Auth Failed (error:%d)\n", m_tHeader.error); return -1;
	}

	if(m_pSdbSink){
		LOG_DEBUG(LOG_MAIN, "m_pSdbSink = %p\n", m_pSdbSink);
		m_pSdbSink->OnStorageDBStatus(DATABASE_STATUS_AUTH);
	}
	else if(m_pSink) {
		LOG_DEBUG(LOG_MAIN, "m_pSink = %p\n", m_pSink);
		m_pSink->OnDataBaseStatus(DATABASE_STATUS_AUTH);
	}
	else if(m_pSbSink) {
		LOG_DEBUG(LOG_MAIN, "m_pSbSink = %p\n", m_pSbSink);
		m_pSbSink->OnStorageBusinessStatus(DATABASE_STATUS_AUTH);
	}
	return 0;
}

int CDBClientHandle::OnGetServerInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	UINT nCount = 0;
	bufferGet >> nCount;

	if (m_tHeader.subsegment == 1) m_listDserverInfo.clear();

	ServerInfo_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += 2*sizeof(DWORD) + sizeof(BYTE) + LENGTH_SERIALNO;
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		memset(&tInfo, 0, sizeof(ServerInfo_t));
		bufferGet >> tInfo.dwServerID >> tInfo.dwVendorID >> tInfo.bServerType;
		bufferGet >> CByteArrayBuffer(tInfo.szSerialNO, LENGTH_SERIALNO);

		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;
		
		nNeedLen += 2*sizeof(DWORD) + LENGTH_PASSWORD;
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> CByteArrayBuffer(tInfo.szPassword, LENGTH_PASSWORD);
		bufferGet >> tInfo.dwIP >> tInfo.nNetID;

		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szPosition, LENGTH_POSITION, nLen, nNeedLen)) return -1;

		LOG_DEBUG(LOG_DB_CLIENT, "server id:%d, type:%d, sn:%s, name:%s, username:%s, ip:%s netid:%d, position:%s\n",
			tInfo.dwServerID, tInfo.bServerType, tInfo.szSerialNO, tInfo.szName, tInfo.szUserName,
			IpDword2Str(tInfo.dwIP), tInfo.nNetID, tInfo.szPosition);

		m_listDserverInfo.push_back(tInfo);
	}
	if (m_tHeader.totalsegment == m_tHeader.subsegment) pSink->OnGetServerInfo(m_listDserverInfo);
	return 0;
}

int CDBClientHandle::OnQueryMobilePhone( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	CGetBuffer bufferGet(pData, nLen);
	int nNeedLen = 0;
	BYTE szMobilePhone[LENGTH_MOBILEPHONE+1] = {0};
	if (false == GetBase64Str(bufferGet, (PUCHAR)szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;

	return pSink->OnQueryMobilePhone((PUCHAR)szMobilePhone, (int)m_tHeader.error);
}

int CDBClientHandle::OnSetSecret(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;
	return pSink->OnSetSecret((int)m_tHeader.error);
}
int CDBClientHandle::ParseOnUserInfoEx(CGetBuffer& bufferGet, int nNeedLen, int nLen, ClientTokenArray_t& tArray)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s\n", __FUNCTION__);
	nNeedLen += sizeof(int);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	bufferGet >> tArray.nCount;
	if (tArray.nCount > 5)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "2 wrong count:%d\n", tArray.nCount); return -1;
	}
	for (int i = 0; i < tArray.nCount; ++i)
	{
		nNeedLen += sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tArray.tToken[i].bPushType;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tArray.tToken[i].szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;
	}
	nNeedLen += sizeof(BYTE);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "4 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	bufferGet >> tArray.nPushSwitch;
	return 0;
}
int CDBClientHandle::OnGetUserInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"%s,%d 用户登录\n",__FUNCTION__,__LINE__);
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	
	int nNeedLen = 2*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	UserInfo_t tInfo; memset(&tInfo, 0, sizeof(UserInfo_t));
	bufferGet >> tInfo.dwUserID >> tInfo.dwConfigureIndex;
	
	if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;

	nNeedLen += LENGTH_PASSWORD;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	bufferGet >> CByteArrayBuffer(tInfo.szPassword, LENGTH_PASSWORD);

	if (false == GetBase64Str(bufferGet, (PUCHAR)tInfo.szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;
	if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szUrl, LENGTH_URL, nLen, nNeedLen)) return -1;

	/////////////////////////////////////////////////////
	ClientTokenArray_t tArray; memset(&tArray, 0, sizeof(ClientTokenArray_t));
	if(ParseOnUserInfoEx(bufferGet, nNeedLen, nLen, tArray)) {
		tArray.nPushSwitch = 1;
	}
	/////////////////////////////////////////////////////

	CGetUserInfo* p = CGetUserInfoMgr::Instance()->GetElem(m_tHeader.destinationid);
	if (p) p->CallbackUserInfo(tInfo, tArray);
	return 0;
}
//获取设备列表4
int CDBClientHandle::OnGetUserDeviceInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"|===============================================================================|\n");
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwUserID = 0, dwIndex = 0;
	UINT nCount = 0;
	bufferGet >> dwUserID >> dwIndex >> nCount;
	LOG_DEBUG(LOG_DB_CLIENT, "%s dwUserID %d dwIndex %d nCount %d\n", __FUNCTION__, dwUserID, dwIndex, nCount);

	if (m_tHeader.subsegment == 1) {
		m_listUserDeviceInfo.clear();
		mapDevRoomInfo.clear();
		lstRoomInfo.clear();
	}
	DeviceInfo_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += 3*sizeof(DWORD) + LENGTH_SERIALNO;
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}

		memset(&tInfo, 0, sizeof(DeviceInfo_t));
		bufferGet >> tInfo.dwDeviceID >> tInfo.dwVendorID >> tInfo.dwGroupID;
		bufferGet >> CByteArrayBuffer(tInfo.szSerialNO, LENGTH_SERIALNO);

		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szDeviceName, LENGTH_NAME, nLen, nNeedLen)) return -1;

		nNeedLen += LENGTH_PASSWORD;
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> CByteArrayBuffer(tInfo.szPassword, LENGTH_PASSWORD);
		bufferGet >> CByteArrayBuffer(tInfo.szRoom, LENGTH_USERROOM);
//		LOG_DEBUG(LOG_DB_CLIENT,"UserID:%d=>DeviceID:%d,Room:%s\n",dwUserID,tInfo.dwDeviceID,tInfo.szRoom);
		m_listUserDeviceInfo.push_back(tInfo);
		//////////////////////////////////////////////////////////////////////////
		nNeedLen += 2 * sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "4 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		DWORD dwDeviceID = 0;
		DWORD dwRoomCount = 0;
		bufferGet >> dwDeviceID >> dwRoomCount;
		LOG_DEBUG(LOG_DB_CLIENT,"===> DeviceID:%d  RoomCount %d <===\n",dwDeviceID, dwRoomCount);
		RoomInfo_t2 tInfo2;
		memset(&tInfo2, 0, sizeof(RoomInfo_t2));
		for(int j = 0; j < dwRoomCount; j++)
		{
			nNeedLen += sizeof(DWORD) + LENGTH_USERROOM;
			bufferGet >> tInfo2.dwRoomID;
			bufferGet >> CByteArrayBuffer(tInfo2.szRoom, LENGTH_USERROOM);
			LOG_DEBUG(LOG_MAIN,"RoomID:%d - Room:%s\n",tInfo2.dwRoomID,tInfo2.szRoom);
			lstRoomInfo.push_back(tInfo2);
		}
		mapDevRoomInfo.insert(std::make_pair(dwDeviceID,lstRoomInfo));

		lstRoomInfo.clear();
	}
	
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s listCout %d, mapCount %d\n",__FUNCTION__, m_listUserDeviceInfo.size(), mapDevRoomInfo.size());
	if (m_tHeader.totalsegment == m_tHeader.subsegment)
	{
		CGetUserDevice* p = CGetUserDeviceMgr::Instance()->GetElem(m_tHeader.destinationid);
		if (p) p->CallbackUserDevice(dwUserID, dwIndex, m_listUserDeviceInfo, mapDevRoomInfo);
	}
	LOG_DEBUG(LOG_DB_CLIENT,"|===============================================================================|\n");
	return 0;
}

int CDBClientHandle::OnGetUserGroupInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwUserID = 0, dwIndex = 0;
	UINT nCount = 0;
	bufferGet >> dwUserID >> dwIndex >> nCount;
	LOG_DEBUG(LOG_DB_CLIENT, "%s dwUserID %d dwIndex %d nCount %d\n", __FUNCTION__, dwUserID, dwIndex, nCount);

	if (m_tHeader.subsegment == 1) m_listUserGroupInfo.clear();
	GroupInfo_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += 3*sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}

		memset(&tInfo, 0, sizeof(GroupInfo_t));
		bufferGet >> tInfo.dwGroupID >> tInfo.dwParentID >> tInfo.dwSequence;

		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szGroupName, LENGTH_NAME, nLen, nNeedLen)) return -1;
		m_listUserGroupInfo.push_back(tInfo);
	}

	if (m_tHeader.totalsegment == m_tHeader.subsegment)
	{
		CGetUserGroup* p = CGetUserGroupMgr::Instance()->GetElem(m_tHeader.destinationid);
		if (p) p->CallbackUserGroup(dwUserID, dwIndex, m_listUserGroupInfo);
	}
	return 0;
}

int CDBClientHandle::OnGetUserRoomInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwUserID = 0, dwIndex = 0;
	UINT nCount = 0;
	bufferGet >> dwUserID >> dwIndex >> nCount;
	LOG_DEBUG(LOG_DB_CLIENT, "%s dwUserID %d dwIndex %d nCount %d\n", __FUNCTION__, dwUserID, dwIndex, nCount);

	if (m_tHeader.subsegment == 1) m_listUserRoomInfo.clear();
	RoomInfo_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += 2*sizeof(DWORD) + LENGTH_PASSWORD;
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}

		memset(&tInfo, 0, sizeof(RoomInfo_t));
		bufferGet >> tInfo.dwRoomID >> tInfo.dwDeviceID;
		bufferGet >> CByteArrayBuffer((PUCHAR)tInfo.szPassword, LENGTH_PASSWORD);
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szRoom, LENGTH_ROOM, nLen, nNeedLen)) return -1;
		m_listUserRoomInfo.push_back(tInfo);
	}

	if (m_tHeader.totalsegment == m_tHeader.subsegment)
	{
		CGetUserRoom* p = CGetUserRoomMgr::Instance()->GetElem(m_tHeader.destinationid);
		if (p) p->CallbackUserRoom(dwUserID, dwIndex, m_listUserRoomInfo);
	}
	return 0;
}
//7
int CDBClientHandle::OnGetDeviceInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "Registered | %s \n", __FUNCTION__);
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;
	
	int nNeedLen = 5*sizeof(DWORD) + LENGTH_SERIALNO;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	CGetBuffer bufferGet(pData, nLen);
	DeviceInfo_t tInfo; memset(&tInfo, 0, sizeof(DeviceInfo_t));
	bufferGet >> tInfo.dwDeviceID >> tInfo.dwConfigureIndex >> tInfo.dwConfigureIndex2 >> tInfo.dwVendorID >> tInfo.dwAutoRelay;
	bufferGet >> CByteArrayBuffer(tInfo.szSerialNO, LENGTH_SERIALNO);

	if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szDeviceName, LENGTH_NAME, nLen, nNeedLen)) goto CALLBACK;
	nNeedLen += sizeof(DWORD);
	if (nLen < nNeedLen) goto CALLBACK;
	bufferGet >> tInfo.dwStoreID;
	GetVariableStr(bufferGet, (PUCHAR)tInfo.szDeadLine, LENGTH_DEADLINE, nLen, nNeedLen);
	nNeedLen += sizeof(DWORD);
	if (nLen < nNeedLen) goto CALLBACK;
	bufferGet >> tInfo.dwGroupID;
CALLBACK:
	return pSink->OnGetDeviceInfo(tInfo);
}

int CDBClientHandle::OnGetDeviceUserInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;
	
	int nNeedLen = 2*sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0, dwIndex = 0;
	UINT nCount = 0;
	bufferGet >> dwDeviceID >> dwIndex >> nCount;

	if (m_tHeader.subsegment == 1) m_listDeviceUserInfo.clear();
	SmsInfo_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += 2*sizeof(DWORD) + sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		memset(&tInfo, 0, sizeof(SmsInfo_t));
		bufferGet >> tInfo.dwUserID >> tInfo.dwVendorID >> tInfo.bLanguage;
		
		if (false == GetBase64Str(bufferGet, (PUCHAR)tInfo.szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szDeviceName, LENGTH_NAME, nLen, nNeedLen)) return -1;

		m_listDeviceUserInfo.push_back(tInfo);
	}
	if (m_tHeader.totalsegment == m_tHeader.subsegment) pSink->OnGetDeviceUserInfo(dwDeviceID, dwIndex, m_listDeviceUserInfo);
	return 0;
}

int CDBClientHandle::OnGetDevicePushInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = 2*sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0, dwIndex = 0;
	UINT nCount = 0;
	bufferGet >> dwDeviceID >> dwIndex >> nCount;

	if (m_tHeader.subsegment == 1) m_listDevicePushInfo.clear();
	PushInfo_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		memset(&tInfo, 0, sizeof(PushInfo_t));
		nNeedLen += 2*sizeof(DWORD) + 2*sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}

		bufferGet >> tInfo.dwUserID >> tInfo.dwVendorID >> tInfo.bPushType >> tInfo.bLanguage;

		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;
		m_listDevicePushInfo.push_back(tInfo);
	}
	if (m_tHeader.totalsegment == m_tHeader.subsegment) pSink->OnGetDevicePushInfo(dwDeviceID, dwIndex, m_listDevicePushInfo);
	return 0;
}

int CDBClientHandle::OnAddDevice(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = 0;
	BYTE szUserName[LENGTH_NAME+1] = {0};
	CGetBuffer bufferGet(pData, nLen);
	if (false == GetVariableStr(bufferGet, (PUCHAR)szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	return pSink->OnAddDevice((int)m_tHeader.error, (PUCHAR)szUserName);
}

const BYTE PUSHINFO_OPR_FORCE_ADD = 1;
const BYTE PUSHINFO_OPR_DEL = 0;
const BYTE PUSHINFO_OPR_KEEP = 2;
const BYTE PUSHINFO_OPR_TRY_ADD = 3;
int CDBClientHandle::OnAddDelPushInfoEx(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(DWORD) + 2*sizeof(BYTE) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	ClientTokenArray_t tInfo; memset(&tInfo, 0, sizeof(ClientTokenArray_t));
	BYTE bOpr = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> bOpr >> tInfo.dwUserID >> tInfo.dwVendorID >> tInfo.bLanguage >> tInfo.nCount;
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s Count:%d\n",__FUNCTION__,tInfo.nCount);
	
	for (int i = 0; i < tInfo.nCount; i++)
	{
		nNeedLen += 2*sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tInfo.tToken[i].bMainFlag >> tInfo.tToken[i].bPushType;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.tToken[i].szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;
	}

	nNeedLen += sizeof(BYTE);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	bufferGet >> tInfo.bLoginOtherPlaceFlag;
	if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szCreated, LENGTH_TIMESTAMP, nLen, nNeedLen)) return -1;

	LOG_DEBUG(LOG_DB_CLIENT, "bOpr:%d bLoginOtherPlaceFlag:%d\n", bOpr, tInfo.bLoginOtherPlaceFlag);
	if (tInfo.bLoginOtherPlaceFlag && (bOpr == PUSHINFO_OPR_FORCE_ADD))// 提示其他Token用户账号在其他位置登录
	{
		LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);
		return m_pSink->OnLoginOtherPlace((int)m_tHeader.error, bOpr, tInfo);
	}

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;
	return pSink->OnAddDelPushInfoEx((int)m_tHeader.error, bOpr, tInfo);
}

int CDBClientHandle::OnSetDeviceName( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	return pSink->OnSetDeviceName((int)m_tHeader.error);
}

int CDBClientHandle::OnDelDevice( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = 2*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	DWORD dwUserID = 0;
	DWORD dwDeviceID = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwUserID >> dwDeviceID;

	return pSink->OnDelDevice((int)m_tHeader.error, dwUserID, dwDeviceID);
}

int CDBClientHandle::OnAuthorize( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwUserID = 0;
	bufferGet >> dwUserID;
	return pSink->OnAuthorize((int)m_tHeader.error, dwUserID);
}

int CDBClientHandle::OnAuthorize2( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	return pSink->OnAuthorize2((int)m_tHeader.error);
}

int CDBClientHandle::OnPieceOfSerialNO( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	int nNeedLen = sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	UINT nCount = 0;
	bufferGet >> nCount;
	nNeedLen += nCount* 3* sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	if (m_tHeader.subsegment == 1) m_listPieceOfSerialNO.clear();
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen total sub segment %d:%d nCount %d\n", __FUNCTION__, m_tHeader.totalsegment, m_tHeader.subsegment, nCount);
	PieceOfSerialNO_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		bufferGet >> tInfo.dwBegin >> tInfo.dwEnd >> tInfo.dwVendorID;
		m_listPieceOfSerialNO.push_back(tInfo);
		LOG_DEBUG(LOG_DB_CLIENT, "%s nLen Begin:End %d:%d VendorID %d m_listPieceOfSerialNO size %d\n", 
			__FUNCTION__, tInfo.dwBegin, tInfo.dwEnd, tInfo.dwVendorID, m_listPieceOfSerialNO.size());
	}
	if (m_tHeader.totalsegment == m_tHeader.subsegment) m_pSink->OnPieceOfSerialNO(m_listPieceOfSerialNO);
	return 0;
}

int CDBClientHandle::OnDserverConfigureIndex( PUCHAR pData, int nLen )
{
	printf("CDBClientHandle::%s\n",__FUNCTION__);
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	int nNeedLen = 2*sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	UINT nCount = 0;
	DWORD dwVendorID = 0, dwConfigureIndex = 0;
	bufferGet >> dwVendorID >> dwConfigureIndex >> nCount;
	LOG_DEBUG(LOG_DB_CLIENT, "%d:%d dwVendorID %d dwConfigureIndex %d nCount %d\n", 
		m_tHeader.totalsegment, m_tHeader.subsegment, dwVendorID, dwConfigureIndex, nCount);

	if (m_tHeader.subsegment == 1) m_listDserverInfo.clear();

	LIST_SERVERINFO listInfo;
	ServerInfo_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += 2*sizeof(DWORD) + sizeof(BYTE) + LENGTH_SERIALNO;
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		memset(&tInfo, 0, sizeof(ServerInfo_t));
		bufferGet >> tInfo.dwServerID >> tInfo.dwVendorID >> tInfo.bServerType;
		bufferGet >> CByteArrayBuffer(tInfo.szSerialNO, LENGTH_SERIALNO);

		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;

		nNeedLen += 2*sizeof(DWORD) + LENGTH_PASSWORD;
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> CByteArrayBuffer(tInfo.szPassword, LENGTH_PASSWORD);
		bufferGet >> tInfo.dwIP >> tInfo.nNetID;

		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szPosition, LENGTH_POSITION, nLen, nNeedLen)) return -1;
		m_listDserverInfo.push_back(tInfo);
	}
	
	if (m_tHeader.totalsegment == m_tHeader.subsegment) m_pSink->OnDserverConfigureIndex(dwVendorID, dwConfigureIndex, m_listDserverInfo);
	return 0;
}

int CDBClientHandle::OnUserConfigureIndex(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	int nNeedLen = 3*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwVendorID = 0, dwUserID = 0;
	bufferGet >> dwVendorID >> dwUserID;
	return m_pSink->OnUserConfigureIndex(dwVendorID, dwUserID);
}

int CDBClientHandle::OnDeviceConfigureIndex(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	int nNeedLen = 3*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwVendorID = 0, dwDeviceID = 0, dwRoomID = 0;
	BYTE bType = 0;
	bufferGet >> dwVendorID >> dwDeviceID >> dwRoomID >> bType;
	return m_pSink->OnDeviceConfigureIndex(dwVendorID, dwDeviceID, dwRoomID, bType);
}

int CDBClientHandle::OnError( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	return pSink->OnDataBaseError((int)m_tHeader.error);
}

int CDBClientHandle::OnGetDeviceRoomSum(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	UINT nCount = 0;
	bufferGet >> dwDeviceID >> nCount;
	LOG_DEBUG(LOG_DB_CLIENT, "%s did=%d t=%d s=%d cut=%d\n",
		__FUNCTION__, dwDeviceID, m_tHeader.totalsegment, m_tHeader.subsegment, nCount);

	//if (m_tHeader.subsegment == 1) m_listDeviceRoomSum.clear();
	LIST_ROOMSUM listDeviceRoomSum; RoomSum_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		memset(&tInfo, 0, sizeof(RoomSum_t));
		nNeedLen += sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tInfo.dwRoomID;
		//		LOG_DEBUG(LOG_DB_CLIENT, "%s dwRoomID:%d\n", __FUNCTION__, tInfo.dwRoomID);

		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szRoom, LENGTH_ROOM, nLen, nNeedLen)) return -1;
		//		LOG_DEBUG(LOG_DB_CLIENT, "%s szRoom:%s\n", __FUNCTION__, tInfo.szRoom);

		nNeedLen += 3*sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tInfo.dwPushIndex >> tInfo.dwUserIndex >> tInfo.dwCardIndex;
		//		LOG_DEBUG(LOG_DB_CLIENT, "%s dwRoomIndex:%d nPushCount:%d\n", __FUNCTION__, tInfo.dwRoomIndex, nPushCount);

		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szPassword, LENGTH_ROOMPWD, nLen, nNeedLen)) return -1;
		
		nNeedLen += sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "4 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		DWORD dwReserve = 0;
		bufferGet >> dwReserve;
		//m_listDeviceRoomSum.push_back(tInfo);
		listDeviceRoomSum.push_back(tInfo);
	}
	SendDeviceRoomSumRepAck(dwDeviceID);
	//if (m_tHeader.totalsegment == m_tHeader.subsegment) pSink->OnGetDeviceRoomSum(dwDeviceID, m_listDeviceRoomSum);
	pSink->OnGetDeviceRoomSum(dwDeviceID, listDeviceRoomSum, m_tHeader.totalsegment, m_tHeader.subsegment);
	return 0;
}

int CDBClientHandle::OnGetDeviceRoomUser(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	UINT nCount = 0;
	bufferGet >> dwDeviceID >> nCount;
	//	LOG_DEBUG(LOG_DB_CLIENT, "%s dwDeviceID:%d nCount:%d\n", __FUNCTION__, dwDeviceID, nCount);

	//if (m_tHeader.subsegment == 1) m_listDeviceRoomUser.clear();
	LIST_ROOMUSER listDeviceRoomUser;
	RoomUser_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += 2*sizeof(DWORD) + sizeof(UINT);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		int nUserCount = 0;
		bufferGet >> tInfo.dwRoomID >> tInfo.dwUserIndex >> nUserCount;

		tInfo.lstUserInfo.clear();
		for (int j = 0; j < nUserCount; j++)
		{
			nNeedLen += 2*sizeof(DWORD) + sizeof(BYTE);
			if (nLen < nNeedLen)
			{
				LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
			}
			SmsInfo_t tSmsInfo; memset(&tSmsInfo, 0, sizeof(SmsInfo_t));
			bufferGet >> tSmsInfo.dwUserID >> tSmsInfo.dwVendorID >> tSmsInfo.bLanguage;
			
			if (false == GetBase64Str(bufferGet, (PUCHAR)tSmsInfo.szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;
			if (false == GetVariableStr(bufferGet, (PUCHAR)tSmsInfo.szDeviceName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	
			tInfo.lstUserInfo.push_back(tSmsInfo);
		}

		//m_listDeviceRoomUser.push_back(tInfo);
		listDeviceRoomUser.push_back(tInfo);
	}
	SendDeviceRoomInfoRepAck(dwDeviceID, 0);
	//if (m_tHeader.totalsegment == m_tHeader.subsegment) pSink->OnGetDeviceRoomUser(dwDeviceID, m_listDeviceRoomUser);
	pSink->OnGetDeviceRoomUser(dwDeviceID, listDeviceRoomUser, m_tHeader.totalsegment, m_tHeader.subsegment);
	return 0;
}

int CDBClientHandle::OnGetDeviceRoomPush(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	UINT nCount = 0;
	bufferGet >> dwDeviceID >> nCount;
	//	LOG_DEBUG(LOG_DB_CLIENT, "%s dwDeviceID:%d nCount:%d\n", __FUNCTION__, dwDeviceID, nCount);

	//if (m_tHeader.subsegment == 1) m_listDeviceRoomPush.clear();
	LIST_ROOMPUSH listDeviceRoomPush;
	RoomPush_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += 2*sizeof(DWORD) + sizeof(UINT);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		int nPushCount = 0;
		bufferGet >> tInfo.dwRoomID >> tInfo.dwPushIndex >> nPushCount;

		tInfo.lstPushInfo.clear();
		for (int j = 0; j < nPushCount; j++)
		{
			nNeedLen += 2*sizeof(DWORD) + 2*sizeof(BYTE);
			if (nLen < nNeedLen)
			{
				LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
			}
			PushInfo_t tPushInfo; memset(&tPushInfo, 0, sizeof(PushInfo_t));
			bufferGet >> tPushInfo.dwUserID >> tPushInfo.dwVendorID >> tPushInfo.bPushType >> tPushInfo.bLanguage;
			if (false == GetVariableStr(bufferGet, (PUCHAR)tPushInfo.szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;

			tInfo.lstPushInfo.push_back(tPushInfo);
		}

		//m_listDeviceRoomPush.push_back(tInfo);
		listDeviceRoomPush.push_back(tInfo);
	}
	SendDeviceRoomInfoRepAck(dwDeviceID, 1);
	//if (m_tHeader.totalsegment == m_tHeader.subsegment) pSink->OnGetDeviceRoomPush(dwDeviceID, m_listDeviceRoomPush);
	pSink->OnGetDeviceRoomPush(dwDeviceID, listDeviceRoomPush, m_tHeader.totalsegment, m_tHeader.subsegment);
	return 0;
}

int CDBClientHandle::OnGetDeviceRoomCard(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	UINT nCount = 0;
	bufferGet >> dwDeviceID >> nCount;
	//	LOG_DEBUG(LOG_DB_CLIENT, "%s dwDeviceID:%d nCount:%d\n", __FUNCTION__, dwDeviceID, nCount);

	//if (m_tHeader.subsegment == 1) m_listDeviceRoomCard.clear();
	LIST_ROOMCARD listDeviceRoomCard;
	RoomCard_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += 2*sizeof(DWORD) + sizeof(UINT);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		int nCardCount = 0;
		bufferGet >> tInfo.dwRoomID >> tInfo.dwCardIndex >> nCardCount;

		tInfo.lstCardInfo.clear();
		for (int j = 0; j < nCardCount; j++)
		{
			CardInfo_t tCardInfo; memset(&tCardInfo, 0, sizeof(CardInfo_t));
			if (false == GetVariableStr(bufferGet, (PUCHAR)tCardInfo.szCard, LENGTH_CARDNUMBER, nLen, nNeedLen)) return -1;
			nNeedLen += sizeof(BYTE);
			if (nLen < nNeedLen)
			{
				LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
			}
			
			bufferGet >> tCardInfo.bCardType;

			tInfo.lstCardInfo.push_back(tCardInfo);
		}
		//m_listDeviceRoomCard.push_back(tInfo);
		listDeviceRoomCard.push_back(tInfo);
	}

	LIST_DWORD lstCardTimeLimit;
	for(int i = 0; i < nCount; i++)
	{
		nNeedLen += sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}

		int nCardCount2 = 0;
		bufferGet >> nCardCount2;
		for(int j = 0; j < nCardCount2; j++)
		{
			nNeedLen += sizeof(DWORD);
			DWORD dwCardTimeLimit = 0;
			bufferGet >> dwCardTimeLimit;
//			LOG_DEBUG(LOG_MAIN,"dwCardTimeLimit1 %d\n", dwCardTimeLimit);
			lstCardTimeLimit.push_back(dwCardTimeLimit);
		}
	}

	LIST_DWORD::iterator iter_limit = lstCardTimeLimit.begin();
	LIST_ROOMCARD::iterator iter = listDeviceRoomCard.begin();
	for (; iter != listDeviceRoomCard.end(); iter++)
	{
		LIST_CARDINFO::iterator iter_room = iter->lstCardInfo.begin();
		for(; iter_room != iter->lstCardInfo.end(); iter_room++)
		{
			if(iter_limit != lstCardTimeLimit.end())
			{
				DWORD dwCardTimeLimit = *iter_limit;
				iter_room->dwCardTimeLimit = dwCardTimeLimit;
				LOG_DEBUG(LOG_MAIN,"dwCardTimeLimit2 %d\n", iter_room->dwCardTimeLimit);
				iter_limit++;
			}
		}
	}

	SendDeviceRoomInfoRepAck(dwDeviceID, 2);
	//if (m_tHeader.totalsegment == m_tHeader.subsegment) pSink->OnGetDeviceRoomCard(dwDeviceID, m_listDeviceRoomCard);
	pSink->OnGetDeviceRoomCard(dwDeviceID, listDeviceRoomCard, m_tHeader.totalsegment, m_tHeader.subsegment);
	return 0;
}
//05
int CDBClientHandle::OnGetDeviceDeadLine(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "105:%s, nLen:%d, dstid:%d \n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 4, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);
	
	DWORD nCount= 0;
	CGetBuffer bufferGet(pData,nLen);
	bufferGet >> nCount;
	LOG_DEBUG(LOG_DB_CLIENT,"Count:%d\n",nCount);
	if(nCount > 169){	LOG_DEBUG(LOG_DB_CLIENT,"Failed nCount:%d\n",nCount); return -1;	}

	DWORD nNeedLen = nCount * 2 * sizeof(DWORD) + sizeof(DWORD);
	if (nLen < nNeedLen){
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet nCount:%d len:%d needlen:%d\n",nCount, nLen, nNeedLen); return -1;
	}

	LOG_DEBUG(LOG_DB_CLIENT,"nLen:%d,nNeedLen:%d\n",nLen,nNeedLen);
	DeviceDeadLineInfo_t tInfo;
	for(UINT i = 0; i < nCount; i++){
		bufferGet >> tInfo.dwDeviceID;
		bufferGet >> tInfo.dwDevDeadLine;
//		LOG_DEBUG(LOG_DB_CLIENT,"%s, DeviceID:%d, DevDeadLine:%d \n",__FUNCTION__,tInfo.dwDeviceID, tInfo.dwDevDeadLine);
		m_pSink->DeviceDeadLine(tInfo);
	}
	return 0;
}
//44.更新物業公告
int CDBClientHandle::OnUpdatePropertyAnnounce(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 8, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);
	
	DWORD dwNoticeIndex, dwCount = 0, dwDeviceId;
	CGetBuffer bufferGet(pData,nLen);
	bufferGet >> dwNoticeIndex >> dwCount;

	DWORD nNeedLen = 2 * sizeof(DWORD) + dwCount * sizeof(DWORD);
	if (nLen < nNeedLen){
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet nCount:%d len:%d needlen:%d\n",dwCount, nLen, nNeedLen); return -1;
	}

	LIST_DWORD lstDevID;
	for(int i = 0; i < dwCount; i++)
	{
		bufferGet >> dwDeviceId;
		lstDevID.push_back(dwDeviceId);
	}
	m_pSink->DevUpdatePropertyAnnounce(dwNoticeIndex,lstDevID);
	return 0;
}

int CDBClientHandle::OnUpdateAdvertInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= sizeof(DWORD) * 3, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	DWORD dwCount = 0, dwDeviceId, dwAdvertIndex;
	CGetBuffer bufferGet(pData,nLen);
	bufferGet >> dwCount;

	DWORD nNeedLen = sizeof(DWORD) + dwCount * sizeof(DWORD);
	if (nLen < nNeedLen){
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet nCount:%d len:%d needlen:%d\n",dwCount, nLen, nNeedLen); return -1;
	}

	LIST_ADVERT lstDevAdvert;
	for(int i = 0; i < dwCount; i++)
	{
		DevAdvert_t	tAdvert;
		bufferGet >> dwDeviceId >> dwAdvertIndex;
		tAdvert.dwDeviceID = dwDeviceId;
		tAdvert.dwAdvertIndex = dwAdvertIndex;
		lstDevAdvert.push_back(tAdvert);
	}
	m_pSink->DevUpdateAdvertInfo(lstDevAdvert);
	return 0;
}

//公告
int CDBClientHandle::OnUpdateBulletin(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	DWORD nNeedLen = sizeof(DWORD);
	if(nNeedLen > nLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n",nLen, nNeedLen); return -1;
	}
	DWORD dwNoticeIndex = 0, dwCount = 0;
	CGetBuffer bufferGet(pData,nLen);
	bufferGet >> dwNoticeIndex;

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) {
		LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s pSink is NULL\n",__FUNCTION__);
		return -1;
	}
	pSink->OnGetBulletinIndex(dwNoticeIndex);
	return 0;
}
//广告
int CDBClientHandle::OnGetAdvertInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	DWORD nNeedLen = 2 * sizeof(DWORD);
	if(nNeedLen > nLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwAdvertIndex = 0, dwAdvertType = 0;
	CGetBuffer bufferGet(pData,nLen);
	bufferGet >> dwAdvertIndex >> dwAdvertType;

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) {
		LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s pSink is NULL\n",__FUNCTION__);
		return -1;
	}
	pSink->OnGetAdvertIndex(dwAdvertIndex, dwAdvertType);
	return 0;
}
//访客配置
int CDBClientHandle::OnGetVisitorCfg(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	DWORD nNeedLen = sizeof(DWORD);
	if(nNeedLen > nLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwVisitorCfg = 0;
	CGetBuffer bufferGet(pData,nLen);
	bufferGet >> dwVisitorCfg;
	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) {
		LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s pSink is NULL\n",__FUNCTION__);	return -1;
	}
	pSink->OnGetVisitorCfg(dwVisitorCfg);
	return 0;
}

int CDBClientHandle::OnUpdateFirmwareRequest(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 4, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);
	DWORD dwCount;
	CGetBuffer bufferGet(pData,nLen);
	bufferGet >> dwCount;
	int nNeedLen = sizeof(DWORD) + dwCount * (sizeof(DWORD) + LENGTH_IMAGEVERSION);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DevUpgrad_t tInfo;
	for(int i = 0; i < dwCount; i++)
	{
		bufferGet >> tInfo.dwDeviceID;
		bufferGet >> CByteArrayBuffer(tInfo.DevVersion, LENGTH_IMAGEVERSION);
		m_pSink->DevUpdateFirmwareRequest(tInfo);
	}
	return 0;
}

int CDBClientHandle::OnDeleteDeviceOnline(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 4, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	DWORD dwCount;
	CGetBuffer bufferGet(pData,nLen);
	bufferGet >> dwCount;

	int nNeedLen = sizeof(DWORD) + dwCount * sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	LIST_DWORD lstDevID;
	DWORD dwDeviceID;
	for(int i = 0; i < dwCount; i++)
	{
		bufferGet >> dwDeviceID;
		lstDevID.push_back(dwDeviceID);
	}

	m_pSink->DevDeleteDeviceOnline(lstDevID);
	return 0;
}
//更改此函数
/*
int CDBClientHandle::OnGetDeviceRoomOther(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	UINT nCount = 0;
	bufferGet >> dwDeviceID >> nCount;
	//	LOG_DEBUG(LOG_DB_CLIENT, "%s dwDeviceID:%d nCount:%d\n", __FUNCTION__, dwDeviceID, nCount);

	//if (m_tHeader.subsegment == 1) m_listDeviceRoomOther.clear();
	LIST_ROOMOTHER listDeviceRoomOther;
	RoomOther_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		memset(&tInfo, 0, sizeof(RoomOther_t));
		nNeedLen += sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tInfo.dwRoomID;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szRoom, LENGTH_ROOM, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szPassword, LENGTH_ROOMPWD, nLen, nNeedLen)) return -1;

		nNeedLen += sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "4 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		DWORD dwReserve = 0;
		bufferGet >> dwReserve;
		//m_listDeviceRoomOther.push_back(tInfo);
		listDeviceRoomOther.push_back(tInfo);
	}
	LIST_ROOMATTR listRoomAttr;
	RoomAttr_t tRoomAttr;
	for (UINT j = 0; j < nCount; j++)
	{
		memset(&tRoomAttr, 0, sizeof(RoomAttr_t));
		nNeedLen += 2 * sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tRoomAttr.dwRoomID >> tRoomAttr.dwRoomAttr;
		listRoomAttr.push_back(tRoomAttr);
	}
	SendDeviceRoomInfoRepAck(dwDeviceID, 3);
	//if (m_tHeader.totalsegment == m_tHeader.subsegment) pSink->OnGetDeviceRoomOther(dwDeviceID, m_listDeviceRoomOther);
	pSink->OnGetDeviceRoomOther(dwDeviceID, listDeviceRoomOther, listRoomAttr, m_tHeader.totalsegment, m_tHeader.subsegment);
	return 0;
}
*/
//替换上面的函数1
int CDBClientHandle::OnGetDeviceRoomOther(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);
	int nNeedLen = sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	UINT nCount = 0;
	bufferGet >> dwDeviceID >> nCount;
	LIST_ROOMOTHER listDeviceRoomOther;
	RoomOther_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		memset(&tInfo, 0, sizeof(RoomOther_t));
		nNeedLen += sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tInfo.dwRoomID;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szRoom, LENGTH_ROOM, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szPassword, LENGTH_ROOMPWD, nLen, nNeedLen)) return -1;

		nNeedLen += sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		DWORD dwReserve = 0;
		bufferGet >> dwReserve;
		listDeviceRoomOther.push_back(tInfo);
	}

	for (UINT j = 0; j < nCount; j++)
	{
		RoomOther_t tAttrInfo;
		memset(&tAttrInfo, 0, sizeof(RoomOther_t));

		nNeedLen += 2 * sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "4 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tAttrInfo.dwRoomID >> tAttrInfo.dwRoomAttr;
		LIST_ROOMOTHER::iterator iter = listDeviceRoomOther.begin();
		for( ; listDeviceRoomOther.end() != iter; iter++)
		{
			if(iter->dwRoomID == tAttrInfo.dwRoomID) iter->dwRoomAttr = tAttrInfo.dwRoomAttr;
		}
	}
	m_pSink->Dev_GetDeviceRoomOther(dwDeviceID, listDeviceRoomOther);
	return 0;
}

//2.2.17  获取室内机信息
int CDBClientHandle::OnGetDeviceRoomIndoor(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	UINT nCount = 0;
	bufferGet >> dwDeviceID >> nCount;
	LOG_DEBUG(LOG_DB_CLIENT, "%s dwDeviceID:%d nCount:%d\n", __FUNCTION__, dwDeviceID, nCount);

	//if (m_tHeader.subsegment == 1) m_listDeviceRoomCard.clear();
	LIST_ROOMINDOOR2 listDeviceRoomIndoor;
	RoomIndoor2_t tInfo;
	for (UINT i = 0; i < nCount; i++)
	{
		nNeedLen += 2*sizeof(DWORD) + sizeof(UINT);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		int nIndoorCount = 0;
		bufferGet >> tInfo.dwRoomID >> tInfo.dwIndoorIndex >> nIndoorCount;
		LOG_DEBUG(LOG_DB_CLIENT,"RoomID=%d IndoorIndex=%d IndoorCount=%d\n",tInfo.dwRoomID, tInfo.dwIndoorIndex, nIndoorCount);
		tInfo.lstIndoorInfo.clear();
		for (int j = 0; j < nIndoorCount; j++)
		{
			InDoorInfo_t tIndoorInfo; memset(&tIndoorInfo, 0, sizeof(InDoorInfo_t));
//			if (false == GetVariableStr(bufferGet, (PUCHAR)tIndoorInfo.szSerialNO, LENGTH_SERIALNO, nLen, nNeedLen)) return -1;
			nNeedLen += LENGTH_SERIALNO + sizeof(DWORD);
			if (nLen < nNeedLen){ LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1; }
			bufferGet >> CByteArrayBuffer(tIndoorInfo.szSerialNO, LENGTH_SERIALNO);
			bufferGet >> tIndoorInfo.dwInDoorID;
			LOG_DEBUG(LOG_DB_CLIENT,"szSerialNO=%s IndoorID=%d\n", tIndoorInfo.szSerialNO,tIndoorInfo.dwInDoorID);
			tInfo.lstIndoorInfo.push_back(tIndoorInfo);
		}
		listDeviceRoomIndoor.push_back(tInfo);
	}
	SendDeviceRoomInfoRepAck(dwDeviceID, 4);
	//if (m_tHeader.totalsegment == m_tHeader.subsegment) pSink->OnGetDeviceRoomCard(dwDeviceID, m_listDeviceRoomCard);
	pSink->OnGetDeviceRoomIndoor(dwDeviceID, listDeviceRoomIndoor, m_tHeader.totalsegment, m_tHeader.subsegment);
	return 0;
}

int CDBClientHandle::OnUpdateDeviceRoomInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	int nNeedLen = 2*sizeof(DWORD) + sizeof(BYTE) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwVendorID = 0, dwDeviceID = 0;
	BYTE bType = 0;
	UINT nCount = 0;
	bufferGet >> dwVendorID >> dwDeviceID >> bType >> nCount;

	nNeedLen += nCount*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	LIST_DWORD lstRoomID;
	for (UINT i = 0; i < nCount; i++)
	{
		DWORD dwRoomID = 0;
		bufferGet >> dwRoomID;
		lstRoomID.push_back(dwRoomID);
	}
	return m_pSink->OnUpdateDeviceRoomInfo(dwVendorID, dwDeviceID, bType, lstRoomID);
}

int CDBClientHandle::OnClearRooms(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	int nNeedLen = sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	UINT nCount = 0;
	bufferGet >> nCount;

	nNeedLen += nCount*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	LIST_DWORD lstDeviceID;
	for (UINT i = 0; i < nCount; i++)
	{
		DWORD dwDeviceID = 0;
		bufferGet >> dwDeviceID;
		lstDeviceID.push_back(dwDeviceID);
	}
	return m_pSink->OnClearRooms(lstDeviceID);
}

int CDBClientHandle::OnUpdateDevice(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	int nNeedLen = sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	UINT nCount = 0;
	bufferGet >> nCount;

	nNeedLen += nCount*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	LIST_DWORD lstDeviceID;
	for (UINT i = 0; i < nCount; i++)
	{
		DWORD dwDeviceID = 0;
		bufferGet >> dwDeviceID;
		lstDeviceID.push_back(dwDeviceID);
	}
	return m_pSink->OnUpdateDevice(lstDeviceID);
}

int CDBClientHandle::OnUpdateDeviceEx(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	int nNeedLen = 2*sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	UINT nCount = 0;
	int nType = 0;
	bufferGet >> nType >> nCount;

	nNeedLen += nCount*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	LIST_DWORD lstDeviceID;
	for (UINT i = 0; i < nCount; i++)
	{
		DWORD dwDeviceID = 0;
		bufferGet >> dwDeviceID;
		lstDeviceID.push_back(dwDeviceID);
	}
	return m_pSink->OnUpdateDeviceEx(nType, lstDeviceID);
}

int CDBClientHandle::OnGetDServerInfo(PUCHAR pData, int nLen)
{
//	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = LENGTH_SERIALNO + 2*sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DServerInfo_t tInfo; memset(&tInfo, 0, sizeof(DServerInfo_t));
	bufferGet >> CByteArrayBuffer((PUCHAR)tInfo.szSerialNO, LENGTH_SERIALNO);
	bufferGet >> tInfo.nPermission >> tInfo.nCapacity;
	return pSink->OnGetDServerInfo(tInfo);
}

int CDBClientHandle::OnQiniu_GetStorageAccount(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(BYTE) + 4*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	StorageAccount_t tAccount; memset(&tAccount, 0, sizeof(StorageAccount_t));

	bufferGet >> tTag.bSrcType >> tTag.dwTagID1 >> tTag.dwTagID2 >> tTag.dwStoreID;
	bufferGet >> tAccount.dwStoreID;
	if (false == GetVariableStr(bufferGet, (PUCHAR)tAccount.szAccessKey, LENGTH_ACCESSKEY, nLen, nNeedLen)) return -1;
	if (false == GetVariableStr(bufferGet, (PUCHAR)tAccount.szSecretKey, LENGTH_SECRETKEY, nLen, nNeedLen)) return -1;
	if (false == GetVariableStr(bufferGet, (PUCHAR)tAccount.szBucket, LENGTH_BUCKET, nLen, nNeedLen)) return -1;
	if (false == GetVariableStr(bufferGet, (PUCHAR)tAccount.szDomain, LENGTH_DOMAIN, nLen, nNeedLen)) return -1;
	LOG_DEBUG(LOG_DB_CLIENT, "SrcType %d TagID1 %d TagID2 %d StoreID %d:%d AccessKey %s SecretKey %s Bucket %s Domain %s\n",
		tTag.bSrcType, tTag.dwTagID1, tTag.dwTagID2, tTag.dwStoreID,
		tAccount.dwStoreID, tAccount.szAccessKey, tAccount.szSecretKey, tAccount.szBucket, tAccount.szDomain);
	return pSink->OnQiniu_GetStorageAccount(tTag, tAccount);
}

//开门记录2005
int CDBClientHandle::OnUnlockRecords(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "2005:%s,nLen:%d,dstid:%d \n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	IStorageDBSink* pSink =  CDataBaseBMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(DWORD) * 3;
	if (nLen < nNeedLen){
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	UnlockRep_t rInfo;
	memset(&rInfo, 0, sizeof(UnlockRep_t));
	CGetBuffer bufferGet(pData,nLen);
	bufferGet >> rInfo.dwDeviceID >> rInfo.dwResult >> rInfo.dwCount;
	LOG_DEBUG(LOG_DB_CLIENT,"DeviceID:%d,Result:%d,Count:%d\n", rInfo.dwDeviceID, rInfo.dwResult, rInfo.dwCount);
	nNeedLen += rInfo.dwCount * sizeof(DWORD);
	if (nLen < nNeedLen){
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	for(int i = 0; i < rInfo.dwCount; i++)
	{
		bufferGet >> rInfo.UnlockIndex[i];
		LOG_DEBUG(LOG_DB_CLIENT,"Index:%d\n",rInfo.UnlockIndex[i]);
	}
	return pSink->OnAddUnlockItem_Rep(rInfo);
}

//////////////////////////////////////////////////////////////////////////
int CDBClientHandle::OnQiniu_GetStorageKeys(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(BYTE) + 3*sizeof(DWORD) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	int nCount = 0;
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTag.bSrcType >> tTag.dwTagID1 >> tTag.dwTagID2 >> tTag.dwStoreID >> nCount;

	LIST_STORE_ACCOUNTKEYS lstAccountKeys;
	StoreAccountKeys_t tAccountKeys;
	StoreKey_t tKey;
	for (int i = 0; i < nCount; i++)
	{
		nNeedLen += sizeof(DWORD);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}

		memset(&(tAccountKeys.tAccount), 0, sizeof(StorageAccount_t));
		bufferGet >> tAccountKeys.tAccount.dwStoreID;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tAccountKeys.tAccount.szAccessKey, LENGTH_ACCESSKEY, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tAccountKeys.tAccount.szSecretKey, LENGTH_SECRETKEY, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tAccountKeys.tAccount.szBucket, LENGTH_BUCKET, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tAccountKeys.tAccount.szDomain, LENGTH_DOMAIN, nLen, nNeedLen)) return -1;

		nNeedLen += sizeof(int);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		int nCount2 = 0;
		bufferGet >> nCount2;

		tAccountKeys.lstKey.clear();
		for (int j = 0; j < nCount2; j++)
		{
			nNeedLen += 2*sizeof(DWORD) + 3*sizeof(BYTE);
			if (nLen < nNeedLen)
			{
				LOG_DEBUG(LOG_DB_CLIENT, "3 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
			}
			memset(&tKey, 0, sizeof(StoreKey_t));
			bufferGet >> tKey.dwDeviceID >> tKey.dwRoomID >> tKey.dwSize >> tKey.dwStoreID >> tKey.bType >> tKey.bRecReason;
			bufferGet >> CByteArrayBuffer((PUCHAR)tKey.szTimeStamp, LENGTH_TIMESTAMP2);
			tAccountKeys.lstKey.push_back(tKey);
		}
		
		lstAccountKeys.push_back(tAccountKeys);
	}

	return pSink->OnQiniu_GetStorageKeys(tTag, lstAccountKeys);
}

int CDBClientHandle::OnGetDeviceCfg(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(UINT) + sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	DWORD dwID = 0;
	int nType = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwID >> nType;

	if(1 == nType)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "%s | nLen:%d | nType:%d \n", __FUNCTION__, nLen, nType);
		UcpaasInfo_t tUcpaas;
		memset(&tUcpaas, 0, sizeof(UcpaasInfo_t));
		if (false == GetVariableStr(bufferGet, (PUCHAR)tUcpaas.szUsername, LENGTH_UCPAAS_USERNAME, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tUcpaas.szPassword, LENGTH_UCPAAS_PASSWORD, nLen, nNeedLen)) return -1;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tUcpaas.szAppid, LENGTH_UCPAAS_APPID, nLen, nNeedLen)) return -1;
		return pSink->OnGetDeviceCfg(dwID, nType, tUcpaas);
	}
	else if(2 == nType)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "%s | nLen:%d | nType:%d \n", __FUNCTION__, nLen, nType);
		SystemCfg_t tCfgInfo;
		memset(&tCfgInfo, 0, sizeof(tCfgInfo));
		if (false == GetSystemCfg(tCfgInfo)) return false;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tCfgInfo.szPhoneNumber, LENGTH_SYSTEM_PHONENUMBER, nLen, nNeedLen)) return -1;
		LOG_DEBUG(LOG_DB_CLIENT, "%s | PhoneNumber:%s \n", __FUNCTION__, tCfgInfo.szPhoneNumber);
		LOG_DEBUG(LOG_MAIN,"UserID:%s | AccountSid:%s | AuthToken:%s | AppKey:%s | PhoneNumber:%s\n",tCfgInfo.szUserID, tCfgInfo.szAccountSid, tCfgInfo.szAuthToken, tCfgInfo.szAppKey, tCfgInfo.szPhoneNumber);
		return pSink->OnGetSystemCfg(dwID, nType, tCfgInfo);
	}
}

int CDBClientHandle::OnSetPushSwitch(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s nLen %d dstid %d\n", __FUNCTION__, nLen, m_tHeader.destinationid);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(UINT) + sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	DWORD dwUserID = 0;
	int nSwitch = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwUserID >> nSwitch;
	return pSink->OnSetPushSwitch(dwUserID, nSwitch);
}
//3.2.14 室内机绑定门口机
int CDBClientHandle::OnIndoorBindDeviceRep(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s 1\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= 0, -1);
	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) return -1;

	int nNeedLen = sizeof(DWORD) * 2;
	if(nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT,"1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	BindInfoRep_t tBindRep;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >>tBindRep.dwResult >> tBindRep.dwIndoorID;
	return pSink->OnIndoorBindDevice_Rep(tBindRep);
}
//
int CDBClientHandle::OnGetIndoorBindDevList(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"[1009] CDBClientHandle::%s 2\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= sizeof(DWORD), -1);

	int nNeedLen = sizeof(DWORD);
	DWORD dwCount;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwCount;

	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
	LIST_DEVICEINFO lstDevInfo;
	DeviceInfo_t tInfo;
	for(int i = 0; i < dwCount; i++)
	{
		nNeedLen += 3 * sizeof(DWORD) + LENGTH_SERIALNO;
		if(nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_CLIENT,"1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		memset(&tInfo, 0, sizeof(DeviceInfo_t));
		bufferGet >> tInfo.dwDeviceID >> tInfo.dwVendorID >> tInfo.dwGroupID;
		bufferGet >> CByteArrayBuffer(tInfo.szSerialNO, LENGTH_SERIALNO);
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.szDeviceName, LENGTH_NAME, nLen, nNeedLen)) return -1;
		lstDevInfo.push_back(tInfo);
	}
	LOG_DEBUG(LOG_DB_CLIENT,"[1010] CDBClientHandle::%s\n",__FUNCTION__);

	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) {
		LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s pSink is NULL\n",__FUNCTION__);
		return -1;
	}
	LOG_DEBUG(LOG_DB_CLIENT,"==>CDBClientHandle::%s\n",__FUNCTION__);
	pSink->OnGetIndoorBindDev(lstDevInfo);
	return 0;
}
//回调给室内机绑定的门口机列表
int CDBClientHandle::OnGetListDevStat(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s nLen = %d\n",__FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= sizeof(DWORD), -1);

	int nNeedLen = sizeof(DWORD);
	DWORD dwCount;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwCount;
	nNeedLen += dwCount * sizeof(DWORD);
	if(nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT,"1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	LIST_DEVSTATUS lstDevStatus;
	DevStatus tInfo;
	for(int i = 0; i < dwCount; i++)
	{
		memset(&tInfo, 0, sizeof(DevStatus));
		bufferGet >> tInfo.dwDeviceID;
		lstDevStatus.push_back(tInfo);
	}
	IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
	if (NULL == pSink) {
		LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s pSink is NULL\n",__FUNCTION__);
		return -1;
	}
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
	pSink->OnGetIndoorBindDevStatus(lstDevStatus);
	return 0;
}
//
int CDBClientHandle::OnGetDevBindIndoorID(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, nLen >= sizeof(DWORD), -1);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSink, -1);

	DWORD dwCount;
	DevStatus tDevStat;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tDevStat.dwDeviceID >> tDevStat.dwStatus >> dwCount;

	int nNeedLen = sizeof(DWORD) * 3 + dwCount * sizeof(DWORD);
	if(nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT,"1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	LIST_DWORD lstIndoorID;
	DWORD dwIndoorID;
	for(int i = 0; i < dwCount; i++)
	{
		bufferGet >> dwIndoorID;
		lstIndoorID.push_back(dwIndoorID);
	}
	m_pSink->OnGetDevBindIndoorID(tDevStat, lstIndoorID);
	return 0;

}

int CDBClientHandle::OnSmsSpecialCrowd(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSdbSink, -1);
	int nNeedLen = sizeof(DWORD) * 3 + sizeof(BYTE) + LENGTH_MOBILEPHONE * 2;
	if(nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT,"1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	SmsInfo2_t tInfo;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tInfo.dwUserID >> tInfo.dwVendorID >> tInfo.bLanguage;
	bufferGet >> CByteArrayBuffer(tInfo.szMobilePhone, LENGTH_MOBILEPHONE);
	bufferGet >> CByteArrayBuffer(tInfo.szPropertyPhone, LENGTH_MOBILEPHONE);
	DWORD dwCount;
	bufferGet >> dwCount;
	for(int i = 0; i < dwCount; i++)
	{
		Phone_t tPhone;
		BYTE	 szMobilePhone[LENGTH_MOBILEPHONE+1] = {0};
		bufferGet >> CByteArrayBuffer(szMobilePhone, LENGTH_MOBILEPHONE);
		memcpy(tPhone.szMobilePhone, szMobilePhone, sizeof(szMobilePhone));
		tInfo.lstPhone.push_back(tPhone);
	}
	LOG_DEBUG(LOG_DB_CLIENT,"UserID = %d, VendorID = %d, Language = %d, Phone = %s\n",tInfo.dwUserID, tInfo.dwVendorID, tInfo.bLanguage, tInfo.szMobilePhone);
	m_pSdbSink->DevSmsSpecialCrowd(tInfo);
	return 0;
}
//StorageBusiness
int CDBClientHandle::OnGetAdvertUrls_Rep(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s\n", __FUNCTION__);
	AdvertInfoRep_t tAdvertRep;
	memset(&tAdvertRep, 0 , sizeof(AdvertInfoRep_t));

	CGetBuffer bufferGet(pData, nLen);
	int nNeedLen = 8 * sizeof(DWORD);
	if(nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT,"1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwDeviceID = 0;
	bufferGet >> dwDeviceID;
	bufferGet >> tAdvertRep.tInfo.dwAdID >> tAdvertRep.tInfo.dwAdType >> tAdvertRep.tInfo.dwApplyID >> tAdvertRep.tInfo.dwApplyType;
	bufferGet >> tAdvertRep.tInfo.dwSize >> tAdvertRep.tInfo.dwStoreID >>tAdvertRep.tInfo.dwStoreType >> tAdvertRep.tInfo.dwTimeStamp;
	bufferGet >> tAdvertRep.tInfo.dwUsePosition >> tAdvertRep.tInfo.dwUseType;
	if (false == GetVariableStr(bufferGet, (PUCHAR)tAdvertRep.tInfo.szFormat, LENGTH_FORMAT, nLen, nNeedLen)) return -1;
	if (false == GetVariableStr(bufferGet, (PUCHAR)tAdvertRep.szUrl, LENGTH_URL, nLen, nNeedLen)) return -1;

	LOG_DEBUG(LOG_DB_CLIENT, "AdID %d, ApType %d, ApID %d, AdType %d, Time %d, Size %d, StoreID %d, StoreType %d, UType %d, UPosition %d\n", 
		tAdvertRep.tInfo.dwAdID, tAdvertRep.tInfo.dwAdType, tAdvertRep.tInfo.dwApplyID, tAdvertRep.tInfo.dwApplyType, tAdvertRep.tInfo.dwTimeStamp, tAdvertRep.tInfo.dwSize, tAdvertRep.tInfo.dwStoreID, tAdvertRep.tInfo.dwStoreType, tAdvertRep.tInfo.dwUseType, tAdvertRep.tInfo.dwUsePosition);
	LOG_DEBUG(LOG_DB_CLIENT, "Url = %s\n", tAdvertRep.szUrl);

	if (dwDeviceID == 0)
	{	
		IStorageBusinessSink* pSink =  CDataBaseCMgr::Instance()->GetDataBaseSink(m_tHeader.destinationid);
		if (NULL == pSink) {
			LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s pSink is NULL\n",__FUNCTION__);
			return -1;
		}
		LOG_DEBUG(LOG_DB_CLIENT,"CDBClientHandle::%s\n",__FUNCTION__);
		return pSink->OnGetAdvertUrls_Rep(tAdvertRep);
	}
	else
	{
		LOG_ASSERT_RET(LOG_DB_CLIENT, m_pSbSink, -1);
		m_pSbSink->Dev_VideoAdvertUrl(dwDeviceID, tAdvertRep);
	}
}
