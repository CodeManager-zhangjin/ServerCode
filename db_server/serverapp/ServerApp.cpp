#include "ServerApp.h"
#include "ServerAuth.h"
#include "getbuffer.h"
#include "Log.h"
#include "qiniuInterface.h"
#include "dbHandleInterface.h"
#include <unistd.h>

const int MAX_DATABASE_COMMAND_COUNT = 100000;
const int DATABASE_APPSERVER_TIMER = 300; // 5min
const int DATABASE_KEEPALIVE_TIMER = 3600*4; // 4hour

const BYTE NOTIFY_CMD_RESPONSE = 'a';

const WORD INNERCMD_PIECE_OF_SERIALNO		= 0x2001;
const WORD INNERCMD_DSERVER_CONFIGUREINDEX	= 0x2002;

const WORD OTHERCMD_SERVER_AUTH	= 0x3001;

BYTE CAppServer::m_szBuffer[MAX_PACKET_LEN] = {0};
BYTE CAppServerCmd::m_szBuffer[MAX_PACKET_LEN] = {0};
const CAppServerCmd::HandlerEntry CAppServerCmd::mHandlers[] =
{
	{ CMD_GET_SERVER_INFO,			&CAppServerCmd::OnGetServerInfo		},
	{ CMD_QUERY_USER,				&CAppServerCmd::OnQueryUser			},
	{ CMD_SET_SECRET,				&CAppServerCmd::OnSetSecret			},
	{ CMD_GET_USER_INFO,			&CAppServerCmd::OnGetUserInfo		},
	{ CMD_GET_USERDEVICE_INFO,		&CAppServerCmd::OnGetUserDeviceInfo	},
	{ CMD_GET_USERGROUP_INFO,		&CAppServerCmd::OnGetUserGroupInfo	},
	{ CMD_GET_USERROOM_INFO,		&CAppServerCmd::OnGetUserRoomInfo	},
	{ CMD_GET_DEVICE_INFO,			&CAppServerCmd::OnGetDeviceInfo		},
	{ CMD_GET_DEVICEUSER_INFO,		&CAppServerCmd::OnGetDeviceUserInfo	},
	{ CMD_GET_DEVICEPUSH_INFO,		&CAppServerCmd::OnGetDevicePushInfo	},
	{ CMD_ADD_DEVICE,				&CAppServerCmd::OnAddDevice			},
	{ CMD_ADD_DEL_PUSH_INFO_EX,		&CAppServerCmd::OnAddDelPushInfoEx	},
	{ CMD_SET_DEVICE_NAME,			&CAppServerCmd::OnSetDeviceName		},
	{ CMD_DEL_DEVICE,				&CAppServerCmd::OnDelDevice			},
	{ CMD_AUTHORIZE,				&CAppServerCmd::OnAuthorize			},
	{ CMD_AUTHORIZE2,				&CAppServerCmd::OnAuthorize2		},

	{ CMD_GET_DEVICE_ROOM_SUM,			&CAppServerCmd::OnGetDeviceRoomSum			},
	{ CMD_GET_DEVICE_ROOM_SUM_REP_ACK,	&CAppServerCmd::OnGetDeviceRoomSumRepAck	},
	{ CMD_GET_DEVICE_ROOMINFO,			&CAppServerCmd::OnGetDeviceRoomInfo			},
	{ CMD_GET_DEVICE_ROOMINFO_REP_ACK,	&CAppServerCmd::OnGetDeviceRoomInfoRepAck	},

	{ CMD_GET_DSERVER_INFO,			&CAppServerCmd::OnGetDServerInfo	},

	{ CMD_GET_STORAGEACCOUNT,		&CAppServerCmd::OnQiniu_GetStorageAccount		},
	{ CMD_GET_STORAGEKEYS,			&CAppServerCmd::OnQiniu_GetStorageKeys			},
	{ CMD_GET_STORAGEKEYS2,			&CAppServerCmd::OnQiniu_GetStorageKeys2			},
	{ CMD_REPORT_UPLOAD_RESULT,		&CAppServerCmd::OnQiniu_ReportUploadResult		},

	{ CMD_GET_DEVICE_CFG,			&CAppServerCmd::OnGetDeviceCfg		},
	{ DD_PUSH_UNLOCK_RECORD,		&CAppServerCmd::OnStorageDBUnlockRecords	},
	{ CMD_REPORT_ALARMSTATUS,  &CAppServerCmd::OnStorageDBAlarmRecords		},
	{ CMD_PUSH_SWITCH,				&CAppServerCmd::OnSetPushSwitch		},

	{ INNERCMD_PIECE_OF_SERIALNO,		&CAppServerCmd::OnInnerPieceOfSN		},
	{ INNERCMD_DSERVER_CONFIGUREINDEX,	&CAppServerCmd::OnInnerDsvrCfgIndex		},

	{ OTHERCMD_SERVER_AUTH,				&CAppServerCmd::OnServerAuth		},
	{ DD_UPDATE_BULLETIN,				&CAppServerCmd::OnGetNoticeIndex    },
	{ CMD_GET_ADVERT_INFO,				&CAppServerCmd::OnGetAdvertIndex    },
	{ CMD_UPDATE_VISITORCFG,			&CAppServerCmd::OnGetVisitorCfg		},
	//3.2.14 室内机绑定门口机
	{CMD_INDOOR_BIND_DEVICE,			&CAppServerCmd::OnIndoorBindDevice	},
	//3.1.20 获取绑定设备的在线离线状态
	{CMD_GET_BIND_DEV_STATE,			&CAppServerCmd::OnGetBindDevStatus	},//门口机
	{CMD_GET_DEV_BIND_IDNOOR,			&CAppServerCmd::OnGetIndoorID		},
	{CMD_GET_INDOOR_INFO,				&CAppServerCmd::OnGetIndoorInfo		},

	//下面是storagebusiness的ServerApp
	{ CMD_GET_ADVERTURLS,					&CAppServerCmd::OnGetAdvertUrl		},
	{ CMD_REPORT_PROGRESS,				&CAppServerCmd::OnReportProgress	},
};

CAppServer::CAppServer()
{
	memset(&m_tBaseInfo, 0, sizeof(ServerInfo_t));
	memset(&m_tHeader, 0, sizeof(PacketHeader_t));
	m_pCon = NULL;
	m_bGroupCode = GROUPCODE;
}

CAppServer::~CAppServer()
{
	if (m_pCon)
	{
		m_pCon->Disconnect();
		NetworkDestroyConnection(m_pCon);
		m_pCon = NULL;
	}
	DelTimer(TIMER_NORMAL, this);
}

void CAppServer::SetServerInfo( ServerInfo_t& tInfo )
{
	memcpy(&m_tBaseInfo, &tInfo, sizeof(ServerInfo_t));

	if (tInfo.bServerType == SERVER_TYPE_D) m_bGroupCode = GROUPCODE_DB_D;
	else if (tInfo.bServerType == SERVER_TYPE_NOTIFICATION) m_bGroupCode = GROUPCODE_DB_NOTIFICATION;
	else if (tInfo.bServerType == SERVER_TYPE_LOGIN) m_bGroupCode = GROUPCODE_DB_LOGIN;
	else if (tInfo.bServerType == SERVER_TYPE_STATUS) m_bGroupCode = GROUPCODE_DB_STATUS;
	else if (tInfo.bServerType == SERVER_TYPE_NB) m_bGroupCode = GROUPCODE_DB_NB;
//	else if (tInfo.bServerType == SERVER_TYPE_STORAGE) m_bGroupCode = GROUPCODE_SDB_SB;

	m_tHeader.groupcode = m_bGroupCode;
#if defined(DBSERVER_DB)
	if (tInfo.bServerType == SERVER_TYPE_LOGIN)
	{
		InnerCmd_Pieceofsn();
	}
	if ( (tInfo.bServerType == SERVER_TYPE_LOGIN) || (tInfo.bServerType == SERVER_TYPE_D) )
	{
		InnerCmd_Dsvrcfgindex(0, 0);
	}
#endif
}


void CAppServer::GetServerInfo( ServerInfo_t& tInfo )
{
	memcpy(&tInfo, &m_tBaseInfo, sizeof(ServerInfo_t));
}

void CAppServer::SetNetConnection( INetConnection* pCon )
{
	if (NULL == pCon) return;
	if (m_pCon == pCon) return;
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	m_pCon = pCon; m_pCon->SetSink(this);

	AddTimer(TIMER_NORMAL, DATABASE_APPSERVER_TIMER, this);
}

int CAppServer::InnerCmd_Pieceofsn()
{
	DECLARE_PUTBUFFER( buffer )
	return InnerCmd(buffer, INNERCMD_PIECE_OF_SERIALNO);
}

int CAppServer::InnerCmd_Dsvrcfgindex(DWORD dwVendorID, DWORD dwConfigureIndex)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << dwVendorID << dwConfigureIndex;
	return InnerCmd(buffer, INNERCMD_DSERVER_CONFIGUREINDEX);
}

int CAppServer::InnerCmd(CPutBuffer& buffer, WORD wCommand)
{
	int nLen = buffer.GetFilledSize();
	buffer.SetOffset(0);

	// Header
	buffer << (BYTE)m_bGroupCode << (WORD)wCommand/*Command ID*/ << (BYTE)m_tHeader.reserved0/*Reserved0*/
		<< (WORD)m_tHeader.version/*Version*/ << (WORD)m_tHeader.reserved1/*Reserved1*/
		<< (DWORD)m_tHeader.destinationid/*Source ID*/
		<< (DWORD)m_tHeader.sourceid/*Destination ID*/
		<< (DWORD)m_tHeader.commandflag/*Command Flag*/
		<< (WORD)1/*Total Segment*/ << (WORD)1/*Sub Segment*/
		<< (WORD)m_tHeader.segmentflag/*Segment Flag*/ << (WORD)m_tHeader.reserved2/*Reserved2*/
		<< (DWORD)m_tHeader.reserved3/*Reserved3*/;
	// Payload
	buffer << (WORD)0/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;
	buffer.SetOffset(nLen);
	LOG_DEBUG(LOG_DB_SERVER, "InnerCmd cmd:0x%04x len:%d\n", wCommand, nLen);

	MsgTag_t tMsgTag;
	tMsgTag.dwServerID = m_tBaseInfo.dwServerID;
	tMsgTag.bServerType = m_tBaseInfo.bServerType;
	tMsgTag.eMsgType = MsgType_cmd;
	return CAppServerMgr::Instance()->AddCommand(tMsgTag, (PUCHAR)buffer, nLen);
}

int CAppServer::SendPacket( PUCHAR pData, int nLen )
{
	LOG_ASSERT_RET(LOG_DB_SERVER, m_pCon, -1);
	return m_pCon->SendCommand((PUCHAR)pData, nLen);
}

int CAppServer::SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg /* = 1 */, WORD wSubSeg /* = 1 */)
{
	int nLen = buffer.GetFilledSize();
	buffer.SetOffset(0);
	// Header
	buffer << (BYTE)m_bGroupCode << (WORD)wCommand/*Command ID*/ << (BYTE)m_tHeader.reserved0/*Reserved0*/
		<< (WORD)m_tHeader.version/*Version*/ << (WORD)m_tHeader.reserved1/*Reserved1*/
		<< (DWORD)m_tHeader.destinationid/*Source ID*/
		<< (DWORD)m_tHeader.sourceid/*Destination ID*/
		<< (DWORD)m_tHeader.commandflag/*Command Flag*/
		<< (WORD)wTotalSeg/*Total Segment*/ << (WORD)wSubSeg/*Sub Segment*/
		<< (WORD)m_tHeader.segmentflag/*Segment Flag*/ << (WORD)m_tHeader.reserved2/*Reserved2*/
		<< (DWORD)m_tHeader.reserved3/*Reserved3*/;
	// Payload
	buffer << (WORD)wError/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;
	buffer.SetOffset(nLen);
	LOG_DEBUG(LOG_DB_SERVER, "SendData cmd:0x%04x err:0x%04x len:%d dstid %d\n", wCommand, wError, nLen, m_tHeader.sourceid);
	return SendPacket((PUCHAR)buffer, nLen);
}

const int LENGTH_PIECE_OF_SERIALNO = 3*sizeof(DWORD);
const int MAX_SEND_PIECE_OF_SERIALNO = (MAX_PACKET_LEN - PACKET_HEADER_SIZE - sizeof(UINT)) / LENGTH_PIECE_OF_SERIALNO;
int CAppServer::SendCmd_PieceOfSerialNO( LIST_PIECEOFSERIALNO& listInfo )
{
	UINT nCount = listInfo.size();
	if (nCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << nCount;
		return SendPacket(buffer, CMD_PIECE_OF_SERIALNO, ERROR_NO);
	}
	WORD wTotalSeg = nCount / MAX_SEND_PIECE_OF_SERIALNO;
	if(nCount % MAX_SEND_PIECE_OF_SERIALNO) wTotalSeg++;

	LIST_PIECEOFSERIALNO::iterator iter = listInfo.begin();
	UINT nPacketCount, nRemainCount = nCount, dwCount = MAX_SEND_PIECE_OF_SERIALNO;
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		nPacketCount = 0;
		if (dwCount > nRemainCount) dwCount = nRemainCount;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwCount;

		for(; listInfo.end() != iter; ++iter)
		{
			buffer << iter->dwBegin << iter->dwEnd << iter->dwVendorID;

			nRemainCount--;
			if(++nPacketCount < dwCount) continue;

			SendPacket(buffer, CMD_PIECE_OF_SERIALNO, ERROR_NO, wTotalSeg, wSubSeg);	
			nPacketCount = 0; ++iter; break;
		}
	}
	return 0;
}

int CAppServer::SendCmd_DserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo)
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwVendorID << dwConfigureIndex << dwCount;
		return SendPacket(buffer, CMD_DSERVER_CONFIGUREINDEX, ERROR_NO);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DserverConfigureIndex(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwVendorID << dwConfigureIndex << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << (DWORD)iter->dwServerID << (DWORD)iter->dwVendorID << (BYTE)iter->bServerType;
			buffer << CByteArrayBuffer((PUCHAR)iter->szSerialNO, LENGTH_SERIALNO);
			PutVariableStr(buffer, (PUCHAR)iter->szUserName);
			buffer << CByteArrayBuffer((PUCHAR)iter->szPassword, LENGTH_PASSWORD);
			buffer << (DWORD)iter->dwIP << (UINT)iter->nNetID;
			PutVariableStr(buffer, (PUCHAR)iter->szPosition);
			iter++;
		}
		SendPacket(buffer, CMD_DSERVER_CONFIGUREINDEX, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CAppServer::SendCmd_UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << dwVendorID << dwUserID;
	return SendPacket(buffer, CMD_USER_CONFIGUREINDEX, ERROR_NO);
}

int CAppServer::SendCmd_DeviceConfigureIndex( DWORD dwVendorID, DWORD dwDeviceID, DWORD dwRoomID, BYTE bType )
{
	DECLARE_PUTBUFFER( buffer )
	buffer << dwVendorID << dwDeviceID << dwRoomID << bType;
	return SendPacket(buffer, CMD_DEVICE_CONFIGUREINDEX, ERROR_NO);
}
//更改此函数
/*
int CAppServer::SendCmd_UpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, int nType, LIST_DWORD& lstRoomID)
{
	DWORD dwCount = lstRoomID.size();
	if (dwCount == 0) return 0;

	BYTE bType = nType;
	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_RoomIndex(lstRoomID, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_DWORD::iterator iter = lstRoomID.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwVendorID << dwDeviceID << bType << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			DWORD dwRoomID = *iter;
			buffer << dwRoomID;
			iter++;
		}
		SendPacket(buffer, CMD_UPDATE_DEVICE_ROOMINFO, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}
*/
//替换上面的函数
int CAppServer::SendCmd_UpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, int nType, LIST_DWORD& lstRoomID)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServer::%s VendorID %d, DeviceID %d, Type %d, RoomCount %d\n", __FUNCTION__, dwVendorID, dwDeviceID, nType, lstRoomID.size());
	DWORD dwCount = lstRoomID.size();
	if(0 == dwCount)	return 0;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	BYTE bType = nType;
	//更新其它房号和密码信息
	if(bType & UpdateDevRoomType_Other)
	{
		LIST_ROOMOTHER lstRoomOther;
		pDBHandle->Query_DeviceRoomOther(dwDeviceID, lstRoomID, lstRoomOther);
		return SendCmd_DeviceRoomOther(dwDeviceID, lstRoomOther);
	}
	//后面再加...
	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_RoomIndex(lstRoomID, listCount);
	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_DWORD::iterator iter = lstRoomID.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwVendorID << dwDeviceID << bType << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			DWORD dwRoomID = *iter;
			buffer << dwRoomID;
			iter++;
		}
		SendPacket(buffer, CMD_UPDATE_DEVICE_ROOMINFO, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}
//同步1
int CAppServer::SendCmd_DeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER lstRoomOther)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServer::%s DeviceID %d\n", __FUNCTION__, dwDeviceID);
	DWORD dwCount = lstRoomOther.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;
		SendPacket(buffer, CMD_GET_DEVICE_ROOMOTHER_REP, ERROR_NO);
		return 0;
	}
	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DeviceRoomOther(lstRoomOther, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_ROOMOTHER::iterator iter = lstRoomOther.begin(), posAttr = lstRoomOther.begin();
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
			buffer << (BYTE)0x10;
			buffer << CByteArrayBuffer((PUCHAR)iter->szPassword, LENGTH_ROOMPWD);
			buffer << (DWORD)0;
			iter++;
		}
		for(int j = 0; j < dwPacketCount; ++j)
		{
			buffer << (DWORD)posAttr->dwRoomID << (DWORD)posAttr->dwRoomAttr;
			posAttr++;
		}
		SendPacket(buffer, CMD_GET_DEVICE_ROOMOTHER_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////
int CAppServer::SendCmd_ClearRooms(MAP_DWORD& mapDeviceID)
{
	DWORD dwCount = mapDeviceID.size();
	if (dwCount == 0) return 0;

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_ClearRooms(mapDeviceID, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	MAP_DWORD::iterator iter = mapDeviceID.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			DWORD dwDeviceID = iter->first;
			buffer << dwDeviceID;
			iter++;
		}
		SendPacket(buffer, CMD_CLEAR_ROOMS, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CAppServer::SendCmd_UpdateDevice(MAP_DWORD& mapDeviceID)
{
	DWORD dwCount = mapDeviceID.size();
	if (dwCount == 0) return 0;

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_UpdateDevice(mapDeviceID, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	MAP_DWORD::iterator iter = mapDeviceID.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			DWORD dwDeviceID = iter->first;
			buffer << dwDeviceID;
			iter++;
		}
		SendPacket(buffer, CMD_UPDATE_DEVICE, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CAppServer::SendCmd_UpdateDeviceEx(int nType, LIST_DWORD& lstDevID)
{
	DWORD dwCount = lstDevID.size();
	if (dwCount == 0) return 0;

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_UpdateDeviceEx(lstDevID, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_DWORD::iterator iter = lstDevID.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << nType << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			DWORD dwDeviceID = *iter;
			buffer << dwDeviceID;
			iter++;
		}
		SendPacket(buffer, CMD_UPDATE_DEVICE_EX, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CAppServer::SendCmd_SetPushInfoEx(WORD wError, BYTE bOpr, ClientTokenArray_t& tInfo)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << bOpr << tInfo.dwUserID << tInfo.dwVendorID << tInfo.bLanguage << tInfo.nCount;
	LOG_DEBUG(LOG_MAIN,"CAppServer::%s Count:%d\n",__FUNCTION__, tInfo.nCount);
	for (int i = 0; i < tInfo.nCount; i++)
	{
		buffer << (BYTE)tInfo.tToken[i].bMainFlag << (BYTE)tInfo.tToken[i].bPushType;
		PutVariableStr(buffer, (PUCHAR)tInfo.tToken[i].szToken);
	}
	buffer << tInfo.bLoginOtherPlaceFlag;
	PutVariableStr(buffer, tInfo.szCreated);
	return SendPacket(buffer, CMD_ADD_DEL_PUSH_INFO_EX_REP, wError);
}

//03
int CAppServer::SendCmd_DeviceDeadLine(LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_DB_SERVER, "103:%s , DeviceCount:%d \n", __FUNCTION__, lstDevID.size());
	if(lstDevID.size() == 0) return 0;
	LIST_DEVICE_DEADLINE lstDeadLine;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_DeviceDeadLine(lstDevID,lstDeadLine);//查询截至日期写入lstDevID
	if(lstDeadLine.size() == 0) return 0;

//	DWORD MaxCount = (MAX_PACKET_LEN - PACKET_HEADER_SIZE - sizeof(DWORD)) / (2 * sizeof(DWORD));
	DWORD MaxCount = 169;
	DWORD wTotalSeg = lstDeadLine.size() / MaxCount + 1;
	DWORD nNeedCount = MaxCount;

	LIST_DEVICE_DEADLINE::iterator iter = lstDeadLine.begin();
	for(DWORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		DECLARE_PUTBUFFER( buffer )
		if(wSubSeg == wTotalSeg)
		{	
			nNeedCount = lstDeadLine.size() % MaxCount;
			if(nNeedCount == 0)	return 0;
		}
		
		buffer << nNeedCount;
		for(int i = 0; i < nNeedCount; i++,iter++)
		{
			LOG_ASSERT_RET(LOG_DB_SERVER,iter != lstDeadLine.end(), -1);
			buffer << iter->dwDeviceID;
			buffer << iter->dwDevDeadLine;
//			LOG_DEBUG(LOG_DB_SERVER,"DeviceID:%d,DeadLine:%d\n",iter->dwDeviceID,iter->dwDevDeadLine);
		}
		LOG_DEBUG(LOG_DB_SERVER,"NeedCount:%d\n",nNeedCount);
		SendPacket(buffer, CMD_GET_DEVICE_DEADLINE_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

//33.更新物業公告
int CAppServer::SendCmd_PropertyAnnounce(DWORD dwVillageId, DWORD dwNoticeIndex)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServer::%s\n", __FUNCTION__);
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	//////////////////////////////////////////////////////////////////////////
	LIST_DWORD lstDevID;
	pDBHandle->Query_GroupDevice(dwVillageId,lstDevID);
	if(lstDevID.size() == 0) return 0;
	LOG_DEBUG(LOG_DB_SERVER,"total=%04d\n",lstDevID.size());

	DWORD MaxCount = (MAX_PACKET_LEN - PACKET_HEADER_SIZE - sizeof(DWORD)*2) / sizeof(DWORD); 
	DWORD wTotalSeg = lstDevID.size() / MaxCount + 1;
	DWORD nNeedCount = MaxCount;

	LIST_DWORD::iterator iter = lstDevID.begin();
	for(DWORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		DECLARE_PUTBUFFER( buffer )
		if(wSubSeg == wTotalSeg)
		{	
			nNeedCount = lstDevID.size() % MaxCount;
			if(nNeedCount == 0)	return 0;
		}

		buffer << dwNoticeIndex <<nNeedCount;
		for(int i = 1; i <= nNeedCount; i++,iter++)
		{
			LOG_ASSERT_RET(LOG_DB_SERVER,iter != lstDevID.end(), -1);
			DWORD deviceid = *iter;
			buffer << deviceid;
			LOG_DEBUG(LOG_DB_SERVER,"%04d,Index:%d,DeviceID:%d\n",i,dwNoticeIndex, deviceid);
		}
		LOG_DEBUG(LOG_DB_SERVER,"NeedCount:%d\n",nNeedCount);
		SendPacket(buffer, CMD_NOTICE_INFO, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}
//更新广告信息
int CAppServer::SendCmd_UpdateAdvertInfo(DWORD dwVillageId, DWORD dwAdvertIndex)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServer::%s\n", __FUNCTION__);
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);

	LIST_DWORD lstDevID;
	pDBHandle->Query_GroupDevice(dwVillageId,lstDevID);
	if(lstDevID.size() == 0) return 0;
	LOG_DEBUG(LOG_DB_SERVER,"total=%d\n",lstDevID.size());
	//一个包最多可以通知多少个设备
	DWORD MaxCount = (MAX_PACKET_LEN - PACKET_HEADER_SIZE - sizeof(DWORD)) / (sizeof(DWORD) * 2); 
	DWORD wTotalSeg = 1;
	if(lstDevID.size()%MaxCount)	wTotalSeg = lstDevID.size() / MaxCount + 1;
	else wTotalSeg = lstDevID.size() / MaxCount;

	LIST_DWORD::iterator iter = lstDevID.begin();
	DWORD dwDeviceCount = MaxCount;
	for(DWORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		DECLARE_PUTBUFFER( buffer )
		if(wSubSeg != wTotalSeg) buffer << dwDeviceCount;
		else if(lstDevID.size()%MaxCount != 0)
		{
			dwDeviceCount = lstDevID.size()%MaxCount;
			buffer << dwDeviceCount;
		}
		LOG_DEBUG(LOG_DB_SERVER,"dwDeviceCount:%d\n",dwDeviceCount);	
		for(int i = 1; i <= dwDeviceCount; i++,iter++)
		{
			LOG_ASSERT_RET(LOG_DB_SERVER,iter != lstDevID.end(), -1);
			DWORD dwDeviceID = *iter;
			buffer << dwDeviceID << dwAdvertIndex;
			LOG_DEBUG(LOG_DB_SERVER,"%04d,DeviceID=%d, dwAdvertIndex=%d\n",i, dwDeviceID, dwAdvertIndex);
		}
		SendPacket(buffer, CMD_ADVERT_INFO, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}
int CAppServer::SendCmd_UpdateFirmwareRequest(PCHAR strVersion, LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s\n", __FUNCTION__);
	DWORD dwCount = lstDevID.size();
	if(dwCount == 0) return 0;
	DECLARE_PUTBUFFER( buffer )

	buffer << dwCount;
	LIST_DWORD::iterator iter = lstDevID.begin();
	for(; iter != lstDevID.end(); iter++)
	{
		DWORD deviceid = *iter;
		buffer << deviceid;
		buffer << CByteArrayBuffer((PUCHAR)strVersion, LENGTH_IMAGEVERSION);
		LOG_DEBUG(LOG_DB_SERVER,"deviceid=%d,version=%s\n",deviceid,strVersion);
		SendPacket(buffer, DD_UPDATE_FIRMWARE_REQUEST, ERROR_NO);
	}
	return 0;
}

int CAppServer::SendCmd_DeleteDeviceOnline(LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServer::%s\n", __FUNCTION__);
	DWORD dwCount = lstDevID.size();
	if(dwCount == 0) return 0;
	DECLARE_PUTBUFFER( buffer )

	buffer << dwCount;
	LIST_DWORD::iterator iter = lstDevID.begin();
	for(; iter != lstDevID.end(); iter++)
	{
		DWORD deviceid = *iter;
		buffer << deviceid;
		LOG_DEBUG(LOG_DB_SERVER,"DeviceID = %d\n",deviceid);
		SendPacket(buffer, DD_DELETE_DEVICE_ONLINE, ERROR_NO);
	}
	return 0;
}
//发短信啦
int CAppServer::SendCmd_SpecialCrowdSms(SmsInfo2_t& tSmsInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServer::%s\n", __FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << tSmsInfo.dwUserID << tSmsInfo.dwVendorID << tSmsInfo.bLanguage;
	buffer << CByteArrayBuffer(tSmsInfo.szMobilePhone, LENGTH_MOBILEPHONE);
	buffer << CByteArrayBuffer(tSmsInfo.szPropertyPhone, LENGTH_MOBILEPHONE);
	buffer << tSmsInfo.lstPhone.size();
	LOG_DEBUG(LOG_DB_SERVER,"UserID = %d, VendorID = %d, Language = %d, Phone = %s, ProPhone = %s, Count = %d\n", 
		tSmsInfo.dwUserID, tSmsInfo.dwVendorID, tSmsInfo.bLanguage, tSmsInfo.szMobilePhone, tSmsInfo.szPropertyPhone, tSmsInfo.lstPhone.size());

	LIST_PHONE::iterator iter = tSmsInfo.lstPhone.begin();
	while(iter != tSmsInfo.lstPhone.end())
	{
		buffer << CByteArrayBuffer(iter->szMobilePhone, LENGTH_MOBILEPHONE);
		LOG_DEBUG(LOG_DB_SERVER,"Phone %s\n", iter->szMobilePhone);
		iter++;
	}
	SendPacket(buffer, CMD_SMS, ERROR_NO);
	return 0;
}

int CAppServer::SendCmd_DownLoadAdvertInfo(DWORD dwDeviceID, AdvertInfo_t& tInfo, PUCHAR pUrl)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServer::%s dwDeviceID = %d, AdID = %d\n", __FUNCTION__, dwDeviceID, tInfo.dwAdID);
	LOG_DEBUG(LOG_DB_SERVER, "pUrl = %s\n", pUrl);
	DECLARE_PUTBUFFER( buffer )
	buffer << dwDeviceID << tInfo.dwAdID << tInfo.dwAdType << tInfo.dwApplyID << tInfo.dwApplyType << tInfo.dwSize << tInfo.dwStoreID << tInfo.dwStoreType
		<< tInfo.dwTimeStamp << tInfo.dwUsePosition << tInfo.dwUseType;
	PutVariableStr(buffer, tInfo.szFormat);
	PutVariableStr(buffer, pUrl);
	SendPacket(buffer, CMD_GET_ADVERTURLS_REP, ERROR_NO);
	return 0;
}

int CAppServer::SendError(WORD wError)
{
	DECLARE_PUTBUFFER( buffer )
	SendPacket(buffer, CMD_ERROR, wError);
}

void CAppServer::OnTimer(TimerReason_e eReason, ITimerSink* pSink)
{
	if (m_pCon == NULL)
	{
		CAppServerMgr::Instance()->DelElem(m_tBaseInfo.dwServerID);
	}
}

int CAppServer::OnDisconnect( int nReason, INetConnection* pCon )
{
	LOG_DEBUG(LOG_DB_SERVER, "ServerID %d OnDisconnect nReason %d\n", m_tBaseInfo.dwServerID, nReason);
	if (pCon != m_pCon) return -1;

	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	return 0;
}

int CAppServer::OnReceive( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return OnCommand(pData, nLen, pCon);
}

int CAppServer::OnCommand( PUCHAR pData, int nLen, INetConnection* pCon )
{
	if (pCon != m_pCon) return -1;
	WORD wError = ProcessCommand(pData, nLen);
	// LOG_DEBUG(LOG_DB_SERVER, "CAppServer::OnCommand pCon %p nLen %d wError %d\n", pCon, nLen, wError);
	if (ERROR_NO != wError)
	{
		SendError(wError);
	}
	return 0;
}

WORD CAppServer::ProcessCommand( PUCHAR pData, int nLen )
{
	if ( false == ParsePacketHeader(pData, nLen, m_tHeader) )
	{
		return ERROR_SYSTEM;
	}
	if (m_tHeader.groupcode != m_bGroupCode)
	{
		LOG_DEBUG(LOG_DB_SERVER, "m_tHeader.groupcode 0x%02x m_bGroupCode 0x%02x\n", m_tHeader.groupcode, m_bGroupCode);
		return ERROR_SYSTEM;
	}
	//////////////////////////////////////////////////////////////////////////
	MsgTag_t tMsgTag;
	tMsgTag.dwServerID = m_tBaseInfo.dwServerID;
	tMsgTag.bServerType = m_tBaseInfo.bServerType;
	tMsgTag.eMsgType = MsgType_cmd;
	return CAppServerMgr::Instance()->AddCommand(tMsgTag, pData, nLen);
}


CAppServerCmd::CAppServerCmd()
{
	m_bTimer_db = false;
	m_bOpr_db = false;
	memset(&m_tMsgTag, 0, sizeof(MsgTag_t));
	memset(&m_tHeader, 0, sizeof(PacketHeader_t));
	m_pLocalCon = NULL;
	m_sock = -1;
	m_wRawUdpPort = 0;
	m_dwCmdFlag = 0;
	m_wSegFlag = 0;
}

CAppServerCmd::~CAppServerCmd()
{
	if (m_pLocalCon)
	{
		m_pLocalCon->Disconnect();
		NetworkDestroyConnection(m_pLocalCon);
		m_pLocalCon = NULL;
	}

	if (m_sock != -1)
	{
		close(m_sock); m_sock = -1;
	}
}

bool CAppServerCmd::OpenSocket(WORD wRawUdpPort)
{
	m_wRawUdpPort = wRawUdpPort;

	m_pLocalCon = CreateRawUdpCon(this, INADDR_ANY, wRawUdpPort);
	LOG_ASSERT_RET(LOG_DB_SERVER, m_pLocalCon, false);

	m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_sock == -1) {
		printf("socket error\n"); return false;  
	}
	return true;
}

WORD CAppServerCmd::AddCommand(MsgTag_t& tMsgTag, PUCHAR pData, int nLen)
{
	Lock();
	if (m_listRequest.size() > MAX_DATABASE_COMMAND_COUNT)
	{
		LOG_DEBUG(LOG_DB_SERVER, "m_listRequest.size() %d database busy\n", m_listRequest.size());
		UnLock();
		return ERROR_DBSERVER_BUSY;
	}
	Msg_t tMsg;
	memcpy(&(tMsg.tMsgTag), &tMsgTag, sizeof(MsgTag_t));
	tMsg.strMsg.assign((const char*)pData, nLen);
	m_listRequest.push_back(tMsg);
//	LOG_DEBUG(LOG_DB_SERVER, "%s m_listRequest.size() = %d\n", __FUNCTION__, m_listRequest.size());
	UnLock();	
	ActivateThread();
	return ERROR_NO;
}

void CAppServerCmd::ProcessThreadCommand()
{
	// get command list
	LIST_MSG listCommand; listCommand.clear();
	Lock();
	if (m_listRequest.empty())
	{
		UnLock(); return;
	}
	listCommand.insert(listCommand.end(), m_listRequest.begin(), m_listRequest.end());
	m_listRequest.clear();
	UnLock();
	
	// 
	LIST_MSG::iterator iter = listCommand.begin();
	for (; iter != listCommand.end(); iter++)
	{
		PUCHAR pData = (PUCHAR)iter->strMsg.c_str();
		int nLen = iter->strMsg.size();
		if ( false == ParsePacketHeader(pData, nLen, m_tHeader) ) continue;
		static int g_nHandlersCount = sizeof( mHandlers) / sizeof( mHandlers[0] );
		for ( int i = 0; i < g_nHandlersCount; ++i )
		{
			if ( mHandlers[i].wCommand != m_tHeader.commandid ) continue;
			memcpy(&m_tMsgTag, &(iter->tMsgTag), sizeof(MsgTag_t));
			PMFHANDLER pHandler = mHandlers[i].pmfHandler;
			(this->*pHandler)(pData+PACKET_HEADER_SIZE, nLen-PACKET_HEADER_SIZE);
			break;
		}
	}
}

int CAppServerCmd::OnOprDB()
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_UpdateDeviceCfg();
	return 0;
}

int CAppServerCmd::OnTestCommand()
{
	// LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	// DWORD dwDeviceID = 223;
	// DWORD dwRoomID = 663;
	// IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	// LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	// int nStoreLimit = 0;
	// if ( (false == pDBHandle->Query_StoreLimit(dwDeviceID, nStoreLimit)) ||
	// 	 (nStoreLimit == 0)
	// 	 )
	// {
	// 	return -1;
	// }

	// ISDBHandle* pSDBHandle = DBHandle_GetSDBHandle();
	// LOG_ASSERT_RET(LOG_DB_SERVER, pSDBHandle, -1);
	// int nStoreCount = 0;
	// pSDBHandle->Query_RoomStoreCount(dwDeviceID, dwRoomID, nStoreCount);
	// LOG_DEBUG(LOG_DB_SERVER, "%s nStoreCount %d nStoreLimit %d\n", __FUNCTION__, nStoreCount, nStoreLimit);
	// if (nStoreCount >= nStoreLimit)
	// {
	// 	pSDBHandle->Delete_StoreKey(dwDeviceID, dwRoomID, nStoreCount - nStoreLimit + 1);
	// }
	// LOG_DEBUG(LOG_DB_SERVER, "Insert_StoreKey\n");
	// pSDBHandle->Query_RoomStoreCount(dwDeviceID, dwRoomID, nStoreCount);
	return 0;
}

void CAppServerCmd::ThreadLoop()
{
	while (m_bRunning)
	{
		HangUpThread();
		ProcessThreadCommand();

		if (m_bTimer_db)
		{
			DBHandle_Timer();
			m_bTimer_db = false;
		}
		if (m_bOpr_db)
		{
			m_bOpr_db = false;
			OnOprDB();
		}
	}
	LOG_DEBUG(LOG_DB_SERVER, "CAppServerCmd::%s TreadID:%d Exit\n", __FUNCTION__, GetThreadID());
}

int CAppServerCmd::OnReceive( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return OnCommand(pData, nLen, pCon);
}

int CAppServerCmd::OnCommand( PUCHAR pData, int nLen, INetConnection* pCon )
{
	if (m_pLocalCon != pCon) return -1;
	if (nLen <= 0) return -1;
	if (pData[0] == NOTIFY_CMD_RESPONSE)
	{
		LIST_MSG listResponse; listResponse.clear();
		Lock();
		if (m_listResponse.empty())
		{
			UnLock(); return 0;
		}
		listResponse.insert(listResponse.end(), m_listResponse.begin(), m_listResponse.end());
		m_listResponse.clear();
		UnLock();

		return OnCmdResponse(listResponse);
	}
}

void CAppServerCmd::AddCommandResponse(PUCHAR pData, int nLen)
{
	// LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d m_tMsgTag: eMsgType %d dwServerID %d bServerType %d\n",
	// 	__FUNCTION__, nLen, m_tMsgTag.eMsgType, m_tMsgTag.dwServerID, m_tMsgTag.bServerType);
	Lock();
	if (m_listResponse.size() > MAX_DATABASE_COMMAND_COUNT)
	{
		LOG_DEBUG(LOG_DB_SERVER, "m_listResponse.size() %d database busy\n", m_listResponse.size());
		m_listResponse.pop_front();
	}

	Msg_t tMsg;
	memcpy(&(tMsg.tMsgTag), &m_tMsgTag, sizeof(MsgTag_t));
	tMsg.strMsg.assign((const char*)pData, nLen);
	m_listResponse.push_back(tMsg);
	UnLock();
	NotifyMainThread();
}

void CAppServerCmd::NotifyMainThread()
{
	if (m_sock == -1) return;
	struct sockaddr_in addrto;
	memset(&addrto, 0, sizeof(struct sockaddr_in));
	addrto.sin_family = AF_INET;
	addrto.sin_port = htons(m_wRawUdpPort);
	addrto.sin_addr.s_addr = INADDR_ANY;
	char szData[2] = {0};
	szData[0] = NOTIFY_CMD_RESPONSE;
	sendto(m_sock, (char*)szData, 1, 0, (struct sockaddr*)&addrto, sizeof(struct sockaddr_in));
}

int CAppServerCmd::SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg /* = 1 */, WORD wSubSeg /* = 1 */)
{
	int nLen = buffer.GetFilledSize();
	buffer.SetOffset(0);
	// Header
	buffer << (BYTE)m_tHeader.groupcode << (WORD)wCommand/*Command ID*/ << (BYTE)m_tHeader.reserved0/*Reserved0*/
		<< (WORD)m_tHeader.version/*Version*/ << (WORD)m_tHeader.reserved1/*Reserved1*/
		<< (DWORD)m_tHeader.destinationid/*Source ID*/
		<< (DWORD)m_tHeader.sourceid/*Destination ID*/
		<< (DWORD)m_tHeader.commandflag/*Command Flag*/
		<< (WORD)wTotalSeg/*Total Segment*/ << (WORD)wSubSeg/*Sub Segment*/
		<< (WORD)m_tHeader.segmentflag/*Segment Flag*/ << (WORD)m_tHeader.reserved2/*Reserved2*/
		<< (DWORD)m_tHeader.reserved3/*Reserved3*/;
	// Payload
	buffer << (WORD)wError/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;
	buffer.SetOffset(nLen);
	LOG_DEBUG(LOG_DB_SERVER, "SendData cmd:0x%04x err:0x%04x len:%d dstid %d\n", wCommand, wError, nLen, m_tHeader.sourceid);

	AddCommandResponse((PUCHAR)buffer, nLen);
	return nLen;
}

int CAppServerCmd::OnInnerPieceOfSN(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);

	LIST_PIECEOFSERIALNO listInfo;
	pDBHandle->Query_PieceOfSerialNO(listInfo);
	return SendCmd_PieceOfSerialNO(listInfo);
}

int CAppServerCmd::OnInnerDsvrCfgIndex(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = 2 * sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwVendorID = 0;
	DWORD dwConfigureIndex = 0;
	bufferGet >> dwVendorID >> dwConfigureIndex;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	
	if (dwVendorID == 0)
	{
		MAP_DWORD mapInfo;
		pDBHandle->Query_DserverConfigureIndex(0, mapInfo);

		MAP_DWORD::iterator iter = mapInfo.begin();
		for (; iter != mapInfo.end(); iter++)
		{
			BYTE szType[2] = {0};
			szType[0] = SERVER_TYPE_D;
			LIST_SERVERINFO listInfo; listInfo.clear();
			pDBHandle->Query_ServerInfo((PUCHAR)szType, iter->first, listInfo);

			SendCmd_DserverConfigureIndex(iter->first, iter->second, listInfo);
		}
	}
	else
	{
		BYTE szType[2] = {0};
		szType[0] = SERVER_TYPE_D;
		LIST_SERVERINFO listInfo; listInfo.clear();
		pDBHandle->Query_ServerInfo((PUCHAR)szType, dwVendorID, listInfo);

		SendCmd_DserverConfigureIndex(dwVendorID, dwConfigureIndex, listInfo);
	}

	return 0;
}

int CAppServerCmd::OnServerAuth(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = LENGTH_SERIALNO;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	ServerInfo_t tInfo;
	memset(&tInfo, 0, sizeof(ServerInfo_t));
	bufferGet >> CByteArrayBuffer(tInfo.szSerialNO, LENGTH_SERIALNO);
	LOG_DEBUG(LOG_DB_SERVER, "szSerialNo = %s\n", tInfo.szSerialNO);
	
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_ServerInfo(tInfo);
	
	return SendCmd_ServerInfo(tInfo);
}

int CAppServerCmd::OnGetServerInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	LIST_SERVERINFO listInfo;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	DWORD dwVendorID = 0;
	if ( (m_tMsgTag.bServerType == SERVER_TYPE_LOGIN) || (m_tMsgTag.bServerType == SERVER_TYPE_NOTIFICATION) )
	{
		dwVendorID = 0;
	}
	pDBHandle->Query_ServerInfo(pData, dwVendorID, listInfo);
	return SendCmd_ServerInfo(listInfo);
}

int CAppServerCmd::OnQueryUser( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	CGetBuffer bufferGet(pData, nLen);
	int nNeedLen = 0;
	BYTE szMobilePhone[LENGTH_MOBILEPHONE+1] = {0};
	if (false == GetBase64Str(bufferGet, (PUCHAR)szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	bool bExist = pDBHandle->Query_User((PUCHAR)szMobilePhone);
	WORD wError = ERROR_NO;
	if (false == bExist) wError = ERROR_NOT_EXIST_MOBILEPHONE;
	DECLARE_PUTBUFFER( buffer )
	buffer << CByteArrayBuffer(pData, nLen);
	return SendPacket(buffer, CMD_QUERY_USER_REP, wError);
}

int CAppServerCmd::OnSetSecret( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwVendorID = 0;
	BYTE szUserName[LENGTH_NAME+1] = {0};
	BYTE bLanguage = 0;
	bufferGet >> dwVendorID >> bLanguage;
	if (false == GetVariableStr(bufferGet, szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	
	nNeedLen = LENGTH_PASSWORD;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	BYTE szPassword[LENGTH_PASSWORD+1] = {0};
	bufferGet >> CByteArrayBuffer(szPassword, LENGTH_PASSWORD);

	BYTE szMobilePhone[LENGTH_MOBILEPHONE+1] = {0};
	if (false == GetBase64Str(bufferGet, (PUCHAR)szMobilePhone, LENGTH_MOBILEPHONE, nLen, nNeedLen)) return -1;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	bool bRet = pDBHandle->Insert_User(dwVendorID, bLanguage, (PUCHAR)szUserName, (PUCHAR)szPassword, (PUCHAR)szMobilePhone);
	WORD wError = ERROR_NO;
	if (false == bRet) wError = ERROR_UNKNOW;
	DECLARE_PUTBUFFER( buffer )
	return SendPacket(buffer, CMD_SET_SECRET_REP, wError);
}

int CAppServerCmd::OnGetUserInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	CGetBuffer bufferGet(pData, nLen);
	int nNeedLen = 0;
	BYTE szUserName[LENGTH_NAME+1] = {0};
	if (false == GetVariableStr(bufferGet, szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	nNeedLen += sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwVendorID = 0;
	bufferGet >> dwVendorID;

	UserInfo_t tInfo; memset(&tInfo, 0, sizeof(UserInfo_t));
	ClientTokenArray_t tArray; memset(&tArray, 0, sizeof(ClientTokenArray_t));
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	bool bExist = pDBHandle->Query_UserInfo((PUCHAR)szUserName, dwVendorID, tInfo, tArray);
	WORD wError = ERROR_NO;
	DECLARE_PUTBUFFER( buffer )
	if (false == bExist) wError = ERROR_INVALID_USERNAME;
	
	buffer << (DWORD)tInfo.dwUserID << (DWORD)tInfo.dwConfigureIndex;
	PutVariableStr(buffer, (PUCHAR)tInfo.szUserName);
	buffer << CByteArrayBuffer((PUCHAR)tInfo.szPassword, LENGTH_PASSWORD);
	PutBase64Str(buffer, (PUCHAR)tInfo.szMobilePhone);
	PutVariableStr(buffer, (PUCHAR)tInfo.szUrl);

	/////////////////////////////////////////////////////
	buffer << tArray.nCount;
	for (int i = 0; i < tArray.nCount; ++i)
	{
		buffer << tArray.tToken[i].bPushType;
		PutVariableStr(buffer, (PUCHAR)tArray.tToken[i].szToken);
	}
	buffer << tArray.nPushSwitch;
	/////////////////////////////////////////////////////

	return SendPacket(buffer, CMD_GET_USER_INFO_REP, wError);
}
//获取设备列表2
int CAppServerCmd::OnGetUserDeviceInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwUserID = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwUserID;

	LIST_DEVICEINFO listInfo;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_UserDevice(dwUserID, listInfo);
	//根据listInfo里的deviceid来查询用户房号，取第一个房号给listInfo.szRoom
	LIST_ROOMINFO listRoom;
	pDBHandle->Query_UserRoom(dwUserID,listRoom);
	LIST_DEVICEINFO::iterator iter = listInfo.begin();
	LIST_ROOMINFO::iterator iter2; 
	for( ; iter != listInfo.end(); iter++)
	{	
		LOG_DEBUG(LOG_DB_SERVER,"Query_UserDevice->DeviceID:%d\n",iter->dwDeviceID);
		iter2 = listRoom.begin();
		for( ; iter2 != listRoom.end(); iter2++)
		{
			if(iter2->dwDeviceID == iter->dwDeviceID)
			{
				memcpy(iter->szRoom,iter2->szRoom,LENGTH_USERROOM); 
				LOG_DEBUG(LOG_DB_SERVER,"UserID:%d==>UserDevice:DeviceID:%d,Room:%s==>UserRoom:DeviceID:%d,Room:%s\n",dwUserID,iter2->dwDeviceID,iter2->szRoom,iter->dwDeviceID,iter->szRoom);
				break;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//查询用户所有设备下的房号信息
	MAP_DEVROOMINFO mapDevRoomInfo;
	mapDevRoomInfo.clear();
	LOG_DEBUG(LOG_DB_SERVER,"============================================================\n");
	pDBHandle->Query_UserDeviceRoomInfo(dwUserID, listInfo, mapDevRoomInfo);
	LOG_DEBUG(LOG_DB_SERVER,"============================================================\n");
	MAP_DEVROOMINFO::iterator it = mapDevRoomInfo.begin();
	for(; it != mapDevRoomInfo.end(); it++)
	{
		LOG_DEBUG(LOG_DB_SERVER,"DeviceID:%d,Count:%d\n",it->first,it->second.size());
		LIST_ROOMINFO2::iterator it2 = it->second.begin();
		for(; it2 != it->second.end(); it2++)
		{
			LOG_DEBUG(LOG_DB_SERVER,"RoomID:%d,Room:%s\n",it2->dwRoomID,it2->szRoom);
		}
	}
	LOG_DEBUG(LOG_DB_SERVER,"============================================================\n");
	//end
	LOG_DEBUG(LOG_DB_SERVER,"===>mapCount %d\n",mapDevRoomInfo.size());
	return SendCmd_UserDevice(dwUserID, 0, listInfo, mapDevRoomInfo);
}

int CAppServerCmd::OnGetUserGroupInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwUserID = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwUserID;

	LIST_GROUPINFO listInfo;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_UserGroup(dwUserID, listInfo);
	return SendCmd_UserGroup(dwUserID, 0, listInfo);
}

int CAppServerCmd::OnGetUserRoomInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	DWORD dwUserID = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwUserID;

	LIST_ROOMINFO listInfo;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_UserRoom(dwUserID, listInfo);
	return SendCmd_UserRoom(dwUserID, 0, listInfo);
}
//4
int CAppServerCmd::OnGetDeviceInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "Registered | %s | nLen:%d | srcid:%d \n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = LENGTH_SERIALNO;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	BYTE szSerialNO[LENGTH_SERIALNO+1] = {0};
	bufferGet >> CByteArrayBuffer(szSerialNO, LENGTH_SERIALNO);

	DeviceInfo_t tInfo; memset(&tInfo, 0, sizeof(DeviceInfo_t));
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	bool bExist = pDBHandle->Query_DeviceInfo((PUCHAR)szSerialNO, tInfo);
	LOG_DEBUG(LOG_DB_SERVER,"DeadLine=%s\n",tInfo.szDeadLine);
	WORD wError = ERROR_NO;
	if (false == bExist) wError = ERROR_INVALID_SERIALNO;
	return SendCmd_DeviceInfo(tInfo, wError);
}

int CAppServerCmd::OnGetDeviceUserInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	bufferGet >> dwDeviceID;

	LIST_SMSINFO listInfo;
	DWORD dwConfigureIndex = 0;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_DeviceUser(dwDeviceID, dwConfigureIndex, listInfo);
	return SendCmd_DeviceUser(dwDeviceID, dwConfigureIndex, listInfo);
}

int CAppServerCmd::OnGetDevicePushInfo( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	bufferGet >> dwDeviceID;

	LIST_PUSHINFO listInfo;
	DWORD dwConfigureIndex2 = 0;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_DevicePush(dwDeviceID, dwConfigureIndex2, listInfo);
	return SendCmd_DevicePush(dwDeviceID, dwConfigureIndex2, listInfo);
}

int CAppServerCmd::OnGetDeviceRoomSum(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	bufferGet >> dwDeviceID;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	LIST_ROOMSUM lstRoomSum;
	pDBHandle->Query_DeviceRoomSum(dwDeviceID, lstRoomSum);
	return SendCmd_DeviceRoomSum(dwDeviceID, lstRoomSum);
}

int CAppServerCmd::OnGetDeviceRoomSumRepAck(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	bufferGet >> dwDeviceID;
	LOG_DEBUG(LOG_DB_SERVER, "dwDeviceID %d CommandFlag %d SegmentFlag %d\n", dwDeviceID, m_tHeader.commandflag, m_tHeader.segmentflag);

	AddMulPktAck(dwDeviceID, MP_Sum, m_tHeader.commandflag);
	return 0;
}

const BYTE DB_ROOMTYPE_USERINDEX = 0;
const BYTE DB_ROOMTYPE_PUSHINDEX = 1;
const BYTE DB_ROOMTYPE_CARDINDEX = 2;
const BYTE DB_ROOMTYPE_OTHER = 3;
const BYTE DB_ROOMTYPE_INDOORINDEX = 4;
int CAppServerCmd::OnGetDeviceRoomInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD) + sizeof(BYTE) + sizeof(int);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	BYTE bRoomIndexType = 0;
	int nCount = 0;
	bufferGet >> dwDeviceID >> bRoomIndexType >> nCount;
//	bRoomIndexType = 4;//test
	LOG_DEBUG(LOG_DB_SERVER, "dwDeviceID %d bRoomIndexType %d nCount %d\n", dwDeviceID, bRoomIndexType, nCount);

	nNeedLen += nCount*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	LIST_DWORD lstRoomID;
	for (int i = 0; i < nCount; i++)
	{
		DWORD dwRoomID = 0;
		bufferGet >> dwRoomID;
		lstRoomID.push_back(dwRoomID);
		LOG_DEBUG(LOG_DB_SERVER,"RoomID=%d\n",dwRoomID);
	}

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	if (bRoomIndexType == DB_ROOMTYPE_USERINDEX)
	{
		LIST_ROOMUSER lstRoomUser;
		pDBHandle->Query_DeviceRoomUser(dwDeviceID, lstRoomID, lstRoomUser);
		return SendCmd_DeviceRoomUser(dwDeviceID, lstRoomUser);
	}

	if (bRoomIndexType == DB_ROOMTYPE_PUSHINDEX)
	{
		LIST_ROOMPUSH lstRoomPush;
		pDBHandle->Query_DeviceRoomPush(dwDeviceID, lstRoomID, lstRoomPush);
		return SendCmd_DeviceRoomPush(dwDeviceID, lstRoomPush);
	}

	if (bRoomIndexType == DB_ROOMTYPE_CARDINDEX)
	{
		LIST_ROOMCARD lstRoomCard;
		pDBHandle->Query_DeviceRoomCard(dwDeviceID, lstRoomID, lstRoomCard);
		return SendCmd_DeviceRoomCard(dwDeviceID, lstRoomCard);
	}

	if (bRoomIndexType == DB_ROOMTYPE_OTHER)
	{
		LIST_ROOMOTHER lstRoomOther;
		pDBHandle->Query_DeviceRoomOther(dwDeviceID, lstRoomID, lstRoomOther);
		return SendCmd_DeviceRoomOther(dwDeviceID, lstRoomOther);
	}

	if (bRoomIndexType == DB_ROOMTYPE_INDOORINDEX)
	{
		LIST_ROOMINDOOR2 lstRoomIndoorInfo;
		pDBHandle->Query_DeviceRoomIndoor(dwDeviceID, lstRoomID, lstRoomIndoorInfo);
		LIST_ROOMINDOOR2::iterator iter = lstRoomIndoorInfo.begin();
		for(; iter != lstRoomIndoorInfo.end(); iter++)
		{
			LOG_DEBUG(LOG_DB_SERVER,"Index=%d RoomID=%d Count=%d\n",iter->dwIndoorIndex,iter->dwRoomID,iter->lstIndoorInfo.size());
			LIST_INDOORINFO::iterator iter2 = iter->lstIndoorInfo.begin();
			for(; iter2 != iter->lstIndoorInfo.end(); iter2++)
			{
				LOG_DEBUG(LOG_DB_SERVER,"IndoorID=%d SN=%s\n",iter2->dwInDoorID, iter2->szSerialNO);
			}
		}
		return SendCmd_DeviceRoomIndoor(dwDeviceID, lstRoomIndoorInfo);
	}
	return 0;
}

int CAppServerCmd::OnGetDeviceRoomInfoRepAck(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwDeviceID = 0;
	BYTE bRoomIndexType = 0;
	bufferGet >> dwDeviceID >> bRoomIndexType;
	LOG_DEBUG(LOG_DB_SERVER, "dwDeviceID %d bRoomIndexType %d CommandFlag %d SegmentFlag %d\n", dwDeviceID, bRoomIndexType, m_tHeader.commandflag, m_tHeader.segmentflag);

	MulPkt_e eMulPkt = MP_NULL;
	if (bRoomIndexType == DB_ROOMTYPE_USERINDEX) eMulPkt = MP_UserIndex;
	else if (bRoomIndexType == DB_ROOMTYPE_PUSHINDEX) eMulPkt = MP_PushIndex;
	else if (bRoomIndexType == DB_ROOMTYPE_CARDINDEX) eMulPkt = MP_CardIndex;
	else if (bRoomIndexType == DB_ROOMTYPE_OTHER) eMulPkt = MP_Other;
	else if (bRoomIndexType == DB_ROOMTYPE_INDOORINDEX) eMulPkt = MP_PushSwitchIndex;
	// AddMulPktAck(dwDeviceID, eMulPkt, m_tHeader.commandflag);
	return 0;
}

int CAppServerCmd::OnAddDevice( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = sizeof(DWORD) + LENGTH_SERIALNO;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwUserID = 0;
	BYTE szSerialNO[LENGTH_SERIALNO+1] = {0};
	bufferGet >> dwUserID;
	bufferGet >> CByteArrayBuffer((PUCHAR)szSerialNO, LENGTH_SERIALNO);
	BYTE szDevName[LENGTH_NAME+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szDevName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	BYTE szRoom[LENGTH_ROOM+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szRoom, LENGTH_ROOM, nLen, nNeedLen)) return -1;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	WORD wError = ERROR_NO;
	BYTE szUserName[LENGTH_NAME] = {0};
	if (false == pDBHandle->Insert_UserDevice(dwUserID, (PUCHAR)szSerialNO, (PUCHAR)szDevName, (PUCHAR)szUserName, (PUCHAR)szRoom))
	{
		wError = (WORD)DBHandle_GetError();
	}
	DECLARE_PUTBUFFER( buffer )
	PutVariableStr(buffer, (PUCHAR)szUserName);
	return SendPacket(buffer, CMD_ADD_DEVICE_REP, wError);
}

const BYTE PUSHINFO_OPR_FORCE_ADD = 1;
const BYTE PUSHINFO_OPR_DEL = 0;
const BYTE PUSHINFO_OPR_KEEP = 2;
const BYTE PUSHINFO_OPR_TRY_ADD = 3;
int CAppServerCmd::OnAddDelPushInfoEx( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	
	int nNeedLen = 2*sizeof(DWORD) + 2*sizeof(BYTE) + sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	ClientTokenArray_t tInfo; memset(&tInfo, 0, sizeof(ClientTokenArray_t));
	BYTE bOpr = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> bOpr >> tInfo.dwUserID >> tInfo.dwVendorID >> tInfo.bLanguage >> tInfo.nCount;
	for (int i = 0; i < tInfo.nCount; i++)
	{
		nNeedLen += 2*sizeof(BYTE);
		if (nLen < nNeedLen)
		{
			LOG_DEBUG(LOG_DB_SERVER, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
		}
		bufferGet >> tInfo.tToken[i].bMainFlag >> tInfo.tToken[i].bPushType;
		if (false == GetVariableStr(bufferGet, (PUCHAR)tInfo.tToken[i].szToken, LENGTH_TOKEN, nLen, nNeedLen)) return -1;
	}
	bufferGet >> tInfo.dwView;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	
	if (bOpr == PUSHINFO_OPR_FORCE_ADD)
	{
		pDBHandle->Insert_PushInfo(tInfo, false);
		if (tInfo.bLoginOtherPlaceFlag)
		{
			CAppServerMgr::Instance()->LoginOtherPlace(0, bOpr, tInfo); return 0;
		}
	}
	else if (bOpr == PUSHINFO_OPR_DEL)
	{
		pDBHandle->Delete_PushInfo(tInfo);
	}
	else if (bOpr == PUSHINFO_OPR_TRY_ADD)
	{
		pDBHandle->Insert_PushInfo(tInfo, true);
	}
	//LOG_DEBUG(LOG_DB_SERVER, "Opr %d(1-Force 3-Try 0-Delete) LoginOtherPlaceFlag %d UserID %d VendorID %d OS %d Token %s\n",
	//	bOpr, bLoginOtherPlaceFlag, tInfo.dwUserID, tInfo.dwVendorID, tInfo.bPushType, tInfo.szToken);
	return SendCmd_SetPushInfoEx(0, bOpr, tInfo);
}

int CAppServerCmd::OnSetDeviceName( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwVendorID = 0, dwUserID = 0, dwDeviceID = 0;
	bufferGet >> dwVendorID >> dwUserID >> dwDeviceID;
	BYTE szDevName[LENGTH_NAME+1] = {0};
	if (false == GetVariableStr(bufferGet, (PUCHAR)szDevName, LENGTH_NAME, nLen, nNeedLen)) return -1;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	WORD wError = ERROR_NO;
	if (false == pDBHandle->Update_DeviceName(dwVendorID, dwUserID, dwDeviceID, (PUCHAR)szDevName))
	{
		wError = (WORD)DBHandle_GetError();
	}
	DECLARE_PUTBUFFER( buffer )
	return SendPacket(buffer, CMD_SET_DEVICE_NAME_REP, wError);
}

int CAppServerCmd::OnDelDevice( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwUserID = 0, dwDeviceID = 0;
	bufferGet >> dwUserID >> dwDeviceID;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	WORD wError = ERROR_NO;
	if (false == pDBHandle->Delete_DeviceUser(dwUserID, dwDeviceID))
	{
		wError = (WORD)DBHandle_GetError();
	}
	DECLARE_PUTBUFFER( buffer )
	buffer << dwUserID << dwDeviceID;
	return SendPacket(buffer, CMD_DEL_DEVICE_REP, wError);
}

int CAppServerCmd::OnAuthorize( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	BYTE szUserName[LENGTH_NAME+1] = {0};
	DWORD dwOwnerID = 0, dwDeviceID = 0;
	bufferGet >> dwOwnerID;
	if (false == GetVariableStr(bufferGet, (PUCHAR)szUserName, LENGTH_NAME, nLen, nNeedLen)) return -1;
	nNeedLen += sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	bufferGet >> dwDeviceID;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	WORD wError = ERROR_NO;
	DWORD dwUserID = 0;
	if (false == pDBHandle->Insert_UserDevice(dwOwnerID, (PUCHAR)szUserName, dwDeviceID, dwUserID))
	{
		wError = (WORD)DBHandle_GetError();
	}
	DECLARE_PUTBUFFER( buffer )
	buffer << dwUserID;
	return SendPacket(buffer, CMD_AUTHORIZE_REP, wError);
}

int CAppServerCmd::OnAuthorize2( PUCHAR pData, int nLen )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
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

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	WORD wError = ERROR_NO;
	if (false == pDBHandle->Insert_UserDevice(dwUserID, dwDeviceID, (PUCHAR)szDeviceName, (PUCHAR)szRoom))
	{
		wError = (WORD)DBHandle_GetError();
	}
	DECLARE_PUTBUFFER( buffer )
	return SendPacket(buffer, CMD_AUTHORIZE2_REP, wError);
}

int CAppServerCmd::OnGetDServerInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = LENGTH_SERIALNO;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DServerInfo_t tInfo; memset(&tInfo, 0, sizeof(DServerInfo_t));
	bufferGet >> CByteArrayBuffer((PUCHAR)tInfo.szSerialNO, LENGTH_SERIALNO);

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_DServerInfo(tInfo);

	DECLARE_PUTBUFFER( buffer )
	buffer << CByteArrayBuffer((PUCHAR)tInfo.szSerialNO, LENGTH_SERIALNO);
	buffer << tInfo.nPermission << tInfo.nCapacity;
	return SendPacket(buffer, CMD_GET_DSERVER_INFO_REP, ERROR_NO);
}
//公告
int CAppServerCmd::OnGetNoticeIndex(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 8, -1);
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwGroupId = 0, dwDeviceId = 0;
	bufferGet >> dwGroupId >> dwDeviceId;
	LOG_DEBUG(LOG_DB_SERVER,"%s dwGroupID=%d, dwDeviceID=%d\n", __FUNCTION__, dwGroupId, dwDeviceId);
	if(dwGroupId == 0) {
		LOG_DEBUG(LOG_DB_SERVER,"%s GroupID is NULL\n",__FUNCTION__);
		return -1;
	}

	DWORD dwNoticeIndex = 0;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_NoticeIndex(dwGroupId, dwNoticeIndex);
	LOG_DEBUG(LOG_DB_SERVER,"%s %d\n",__FUNCTION__, dwNoticeIndex);

	DWORD dwCount = 1;
	DECLARE_PUTBUFFER( buffer )
	buffer << dwNoticeIndex << dwCount << dwDeviceId;
	return SendPacket(buffer, DD_UPDATE_BULLETIN_REP, ERROR_NO);
}
//广告
int CAppServerCmd::OnGetAdvertIndex(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 8, -1);
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwGroupId = 0, dwDeviceId = 0;
	bufferGet >> dwGroupId >> dwDeviceId;
	LOG_DEBUG(LOG_DB_SERVER,"dwGroupID=%d, dwDeviceID=%d\n",dwGroupId, dwDeviceId);

	if(dwGroupId == 0) {
		LOG_DEBUG(LOG_DB_SERVER,"GroupID is Invalid\n");
		return -1;
	}

	DWORD dwAdvertIndex = 0;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_AdvertIndex(dwGroupId, dwAdvertIndex);
	LOG_DEBUG(LOG_DB_SERVER,"%s %d\n",__FUNCTION__, dwAdvertIndex);

	DWORD dwCount = 1;
	DWORD dwAdvertType = 0;
	DECLARE_PUTBUFFER( buffer )
	buffer << dwAdvertIndex << dwAdvertType << dwCount << dwDeviceId;
	return SendPacket(buffer, CMD_GET_ADVERT_INFO_REP, ERROR_NO);
}
//访客配置
int CAppServerCmd::OnGetVisitorCfg(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 8, -1);
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwGroupId = 0, dwDeviceId = 0;
	bufferGet >> dwGroupId >> dwDeviceId;
	LOG_DEBUG(LOG_DB_SERVER,"dwGroupID=%d, dwDeviceID=%d\n",dwGroupId, dwDeviceId);
	
	DWORD dwVisitorCfg = 0;
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_VisitorCfg(dwGroupId, dwVisitorCfg);
	DECLARE_PUTBUFFER( buffer )
	buffer << dwVisitorCfg;
	return SendPacket(buffer, CMD_UPDATE_VISITORCFG_REP, ERROR_NO);
}


int CAppServerCmd::OnGetDeviceCfg(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d srcid %d\n", __FUNCTION__, nLen, m_tHeader.sourceid);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);
	int nNeedLen = sizeof(DWORD) + sizeof(int);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	int nType = 0;
	DWORD dwID = 0;
	bufferGet >> dwID >> nType;
	if(1 == nType)//云之讯
	{
		LOG_DEBUG(LOG_DB_SERVER, "云之讯 | %s | nLen:%d | nType:%d \n", __FUNCTION__, nLen, nType);
		IDBHandle* pDBHandle = DBHandle_GetDBHandle();
		LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
		UcpaasInfo_t tUcpaas; memset(&tUcpaas, 0, sizeof(UcpaasInfo_t));
		pDBHandle->Query_DeviceCfg(dwID, nType, tUcpaas);
		LOG_DEBUG(LOG_DB_SERVER,"szUsername=%s, szPassword=%s, szAppid=%s\n", tUcpaas.szUsername, tUcpaas.szPassword, tUcpaas.szAppid);

		DECLARE_PUTBUFFER( buffer )
		buffer << dwID << nType;
		PutVariableStr(buffer, tUcpaas.szUsername);
		PutVariableStr(buffer, tUcpaas.szPassword); // des+base64
		PutVariableStr(buffer, tUcpaas.szAppid);
		return SendPacket(buffer, CMD_GET_DEVICE_CFG_REP, ERROR_NO);
	}
	else if(2 == nType)//容联云
	{
		LOG_DEBUG(LOG_DB_SERVER, "容联云 | %s | nLen:%d | nType:%d \n", __FUNCTION__, nLen, nType);
		IDBHandle* pDBHandle = DBHandle_GetDBHandle();
		LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
		SystemCfg_t tPhone; memset(&tPhone, 0, sizeof(SystemCfg_t));
		pDBHandle->Query_VendorPhone(dwID, nType, tPhone); 
		LOG_DEBUG(LOG_DB_SERVER, "%s | nLen:%d | nType:%d | PhoneNumber:%s \n", __FUNCTION__, nLen, nType, tPhone.szPhoneNumber);

		DECLARE_PUTBUFFER( buffer )
		buffer << dwID << nType;
		PutVariableStr(buffer, (unsigned char*)tPhone.szPhoneNumber);
		return SendPacket(buffer, CMD_GET_DEVICE_CFG_REP, ERROR_NO);
	}
}

int CAppServerCmd::OnSetPushSwitch(PUCHAR pData, int nLen)
{
//	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = sizeof(DWORD) + sizeof(int);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DWORD dwUserID = 0;
	int nSwitch = 0;
	bufferGet >> dwUserID >> nSwitch;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	pDBHandle->Query_UpdatePushSwitch(dwUserID, nSwitch);

	DECLARE_PUTBUFFER( buffer )
	buffer << dwUserID << nSwitch;
	return SendPacket(buffer, CMD_PUSH_SWITCH_REP, ERROR_NO);
}

int CAppServerCmd::OnQiniu_GetStorageAccount(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = sizeof(BYTE) + 3*sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTag.bSrcType >> tTag.dwTagID1 >> tTag.dwTagID2 >> tTag.dwStoreID;

	ISDBHandle* pSDBHandle = DBHandle_GetSDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSDBHandle, -1);
	StorageAccount_t tAccount; memset(&tAccount, 0, sizeof(StorageAccount_t));
	tAccount.dwStoreID = tTag.dwStoreID;
	pSDBHandle->Query_StorageInfo(tAccount);
	return SendCmd_Qiniu_StorageAccount(tTag, tAccount);
}
//开门记录2003
int CAppServerCmd::OnStorageDBUnlockRecords(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "2003:%s,nLen:%d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 4, -1);

	DWORD Count = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> Count;
	DWORD MaxCount =  (MAX_PACKET_LEN - PACKET_HEADER_SIZE - sizeof(DWORD)) / (6 * sizeof(DWORD) + LENGTH_COMNUMBER); //设备一次最多发20条记录
	if(Count > MaxCount){
		LOG_ERR(LOG_DB_SERVER,"Count:%d,MaxCount:%d,Count failed\n",Count,MaxCount);
		return -1;
	}

	DWORD nNeedLen = Count * (6 * sizeof(DWORD) + LENGTH_COMNUMBER) + sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}

	DWORD Index[Count];
	UnlockInfo_t tInfo;
	LIST_DEVICE_UNLOCK listInfo;
	for(int i = 0; i < Count; i++)
	{
		memset(&tInfo, 0, sizeof(UnlockInfo_t));
		bufferGet >> tInfo.dwUnlockIndex >> tInfo.dwUnlockType >> tInfo.dwDeviceID >> tInfo.dwRoomID >> tInfo.dwUserID ;
		bufferGet >> CByteArrayBuffer(tInfo.ComNumber,LENGTH_COMNUMBER);
		bufferGet >> tInfo.dwTimestamp;
		LOG_DEBUG(LOG_DB_SERVER,"UnlockIndex:%d,DeviceID:%d,RoomID:%d,UserID:%d,Unlock:%d,TimeStamp:%d\n", tInfo.dwUnlockIndex, tInfo.dwDeviceID, tInfo.dwRoomID, tInfo.dwUserID, tInfo.dwUnlockType, tInfo.dwTimestamp);
		listInfo.push_back(tInfo);
		Index[i] = tInfo.dwUnlockIndex;
	}

	ISDBHandle* pSDBHandle = DBHandle_GetSDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSDBHandle, -1);
	pSDBHandle->StorageUnlockList(listInfo);

	UnlockRep_t rInfo;
	rInfo.dwResult = 0;
	rInfo.dwDeviceID =  tInfo.dwDeviceID;
	rInfo.dwCount    =  Count;
	
	LOG_DEBUG(LOG_DB_SERVER,"Result:%d,DeviceID:%d,Count:%d\n",rInfo.dwResult,rInfo.dwDeviceID,rInfo.dwCount);
	memcpy(rInfo.UnlockIndex,Index,sizeof(Index));
//	for(int i = 0; i < rInfo.dwCount; i++){
//		LOG_DEBUG(LOG_DB_SERVER,"UnlockIndex%d:%d\n",i,rInfo.UnlockIndex[i]);
//	}

	SendCmd_UnlockRecordsRep(rInfo);
	return 0;
}
//存储报警状态信息到数据库
int CAppServerCmd::OnStorageDBAlarmRecords(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServerCmd::%s\n",__FUNCTION__);
	int nNeedLen = 4 * sizeof(DWORD) + LENGTH_TIMESTAMP;
	if(nLen < nNeedLen) {
		LOG_DEBUG(LOG_DB_SERVER, "1 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	LIST_ALARMSTATUS listInfo;
	listInfo.clear();
	AlarmStatus_t tInfo;
	memset(&tInfo, 0, sizeof(AlarmStatus_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tInfo.dwDeviceID >> tInfo.dwRoomID >> tInfo.dwType >> tInfo.dwSubType;
	bufferGet >> CByteArrayBuffer(tInfo.szTimeStamp, LENGTH_TIMESTAMP);
	LOG_DEBUG(LOG_DB_SERVER,"%s DevID %d RoomID %d Type %d SubType %d TimeStamp %s\n",
		__FUNCTION__,tInfo.dwDeviceID, tInfo.dwRoomID, tInfo.dwType, tInfo.dwSubType, tInfo.szTimeStamp);
	
	listInfo.push_back(tInfo);

	ISDBHandle* pSDBHandle = DBHandle_GetSDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSDBHandle, -1);
	pSDBHandle->StorageAlarmRecordList(listInfo);
	return 0;
}

int CAppServerCmd::OnQiniu_GetStorageKeys(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = 3*sizeof(BYTE) + 7*sizeof(DWORD) + LENGTH_TIMESTAMP2;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTag.bSrcType >> tTag.dwTagID1 >> tTag.dwTagID2 >> tTag.dwStoreID;

	StoreKey_t tKey; memset(&tKey, 0, sizeof(StoreKey_t));
	bufferGet >> tKey.dwDeviceID >> tKey.dwRoomID >> tKey.dwSize >> tKey.dwStoreID >> tKey.bType >> tKey.bRecReason;
	bufferGet >> CByteArrayBuffer((PUCHAR)tKey.szTimeStamp, LENGTH_TIMESTAMP2);

	ISDBHandle* pSDBHandle = DBHandle_GetSDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSDBHandle, -1);
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	LIST_DWORD lstRoomID;
	pDBHandle->Query_UserDeviceRoom(tTag.dwTagID1, tKey.dwDeviceID, lstRoomID);
	LIST_STORE_ACCOUNTKEYS lstAccountKeys;
	pSDBHandle->Query_StorageKeys(tKey.dwDeviceID, lstRoomID, lstAccountKeys);

	tTag.dwStoreID = tKey.dwDeviceID; // dwStoreID字段存放deviceid
	return SendCmd_Qiniu_StorageKeys(tTag, lstAccountKeys);
}

int CAppServerCmd::OnQiniu_GetStorageKeys2(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "CAppServerCmd::%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = 3*sizeof(BYTE) + 9*sizeof(DWORD) + LENGTH_TIMESTAMP2;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	StorageTag_t tTag; memset(&tTag, 0, sizeof(StorageTag_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tTag.bSrcType >> tTag.dwTagID1 >> tTag.dwTagID2 >> tTag.dwStoreID;

	StoreVisitor_t tVisitor; memset(&tVisitor, 0, sizeof(StoreVisitor_t));
	bufferGet >> tVisitor.tKey.dwDeviceID >> tVisitor.tKey.dwRoomID >> tVisitor.tKey.dwSize >> tVisitor.tKey.dwStoreID 
		>> tVisitor.tKey.bType >> tVisitor.tKey.bRecReason;
	bufferGet >> CByteArrayBuffer((PUCHAR)tVisitor.tKey.szTimeStamp, LENGTH_TIMESTAMP2);
	bufferGet >> tVisitor.startIndex >> tVisitor.dwCount;

	ISDBHandle* pSDBHandle = DBHandle_GetSDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSDBHandle, -1);
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	LIST_DWORD lstRoomID;
	LIST_STORE_ACCOUNTKEYS lstAccountKeys;
	Lock();
	if (false == pDBHandle->Query_UserDeviceRoom(tTag.dwTagID1, tVisitor.tKey.dwDeviceID, lstRoomID))
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s Query_UserDeviceRoom failed\n", __FUNCTION__);
		UnLock();
		return -1;
	}
	if (false == pSDBHandle->Query_Visitor(tVisitor,lstRoomID,lstAccountKeys))
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s Query_Visitor failed\n", __FUNCTION__);
		UnLock();
		return -1;
	}
	UnLock();
	return SendCmd_Qiniu_StorageKeys(tTag, lstAccountKeys);
}

//上传访客留影信息
int CAppServerCmd::OnQiniu_ReportUploadResult(PUCHAR pData, int nLen)
{
//	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = 2*sizeof(BYTE) + 4*sizeof(DWORD) + LENGTH_TIMESTAMP2;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	StoreKey_t tKey; memset(&tKey, 0, sizeof(StoreKey_t));
	bufferGet >> tKey.dwDeviceID >> tKey.dwRoomID >> tKey.dwSize >> tKey.dwStoreID >> tKey.bType >> tKey.bRecReason;
	bufferGet >> CByteArrayBuffer((PUCHAR)tKey.szTimeStamp, LENGTH_TIMESTAMP2);
	LIST_STOREKEY listInfo;
	listInfo.push_back(tKey);

	ISDBHandle* pSDBHandle = DBHandle_GetSDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSDBHandle, -1);
	pSDBHandle->StoregeVisitorList(listInfo);
	return 0;
}
//3.2.14 室内机绑定门口机
int CAppServerCmd::OnIndoorBindDevice(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= 0, -1);

	int nNeedLen = LENGTH_SERIALNO + sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_SERVER, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	BYTE szIndoorSN[LENGTH_SERIALNO + 1] = {0};
	DWORD dwCount = 0;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> CByteArrayBuffer(szIndoorSN, LENGTH_SERIALNO);
	bufferGet >> dwCount;
	nNeedLen += dwCount * sizeof(DWORD);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_MAIN, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	BindInfo_t tBindInfo;
	LIST_BIND_INFO listBindInfo;
	memset(&tBindInfo, 0, sizeof(BindInfo_t));
	for(int i = 0; i < dwCount; ++i)
	{
		bufferGet >> tBindInfo.dwDeviceID;
		bufferGet >> tBindInfo.dwRoomID;
		listBindInfo.push_back(tBindInfo);
	}

	BindInfoRep_t tBindRep;
	memset(&tBindRep, 0, sizeof(tBindRep));
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	bool ret = pDBHandle->Insert_IndoorInfo(szIndoorSN ,listBindInfo,tBindRep);
	LOG_DEBUG(LOG_DB_SERVER,"CAppServerCmd::%s IndoorID=%d,Result=%d,ret=%d\n",__FUNCTION__,tBindRep.dwIndoorID, tBindRep.dwResult,ret);
	if((ret == true) && (tBindRep.dwResult == 0) )
	{
		pDBHandle->UpdateIndoorIndex(listBindInfo);
		//正确返回
		LOG_DEBUG(LOG_DB_SERVER,"CAppServerCmd::%s BindDev Sucess\n",__FUNCTION__);
		SendCmd_BindIndoorRep(tBindRep);
	}
	else
	{
		LOG_DEBUG(LOG_DB_SERVER,"CAppServerCmd::%s BindDev Failed\n",__FUNCTION__);
		SendCmd_BindIndoorRep(tBindRep);
	}
	return 0;//
}
//3.1.20  获取室内机绑定门口机在线离线状态
int CAppServerCmd::OnGetBindDevStatus(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CAppServerCmd::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= sizeof(DWORD), -1);
	DWORD dwIndoorID;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwIndoorID;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	
	LIST_DEVICEINFO lstDevInfo;
	pDBHandle->QueryIndoorBindDev(dwIndoorID,lstDevInfo);
	SendCmd_DevStatusRep(lstDevInfo);
	return 0;
}
//
int CAppServerCmd::OnGetIndoorID(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CAppServerCmd::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= sizeof(DWORD), -1);
	DWORD dwDeviceID;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwDeviceID;

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	LIST_DWORD	lstIndoorID;
	pDBHandle->Query_DeviceIndoorID(dwDeviceID, lstIndoorID);
	DevStatus tDevStat;
	tDevStat.dwDeviceID = dwDeviceID;
	SendCmd_DeviceIndoorID(tDevStat,lstIndoorID);
	return 0;
}
//特殊账号获取设备列表
int CAppServerCmd::OnGetIndoorInfo(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER,"[1004] CAppServerCmd::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_SERVER, nLen >= LENGTH_SERIALNO, -1);

	BYTE szIndoorSN[LENGTH_SERIALNO + 1] = {0};
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> CByteArrayBuffer(szIndoorSN, LENGTH_SERIALNO);

	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	DWORD dwIndoorID = 0;

	DWORD len = sizeof((PUCHAR)szIndoorSN);
	LOG_DEBUG(LOG_DB_SERVER,"[1005] len = %d\n", len);
	if(len != 20) 
	{
		dwIndoorID = atoi((const char*)szIndoorSN);
		LOG_DEBUG(LOG_DB_SERVER,"CAppServerCmd::%s dwIndoorID:%d\n",__FUNCTION__,dwIndoorID);
	}
	else
	{
		pDBHandle->Query_InDoorIDBySN(szIndoorSN, dwIndoorID);
		LOG_DEBUG(LOG_DB_SERVER,"CAppServerCmd::%s dwIndoorID:%d\n",__FUNCTION__,dwIndoorID);
	}

	LIST_DEVICEINFO lstDevInfo;
	pDBHandle->QueryIndoorBindDev(dwIndoorID,lstDevInfo);
	LOG_DEBUG(LOG_DB_SERVER,"[1006] CAppServerCmd::%s\n",__FUNCTION__);
	pDBHandle->QueryDevInfo(lstDevInfo);
	SendCmd_DevListRep(lstDevInfo);
	LOG_DEBUG(LOG_DB_SERVER,"[1007] CAppServerCmd::%s\n",__FUNCTION__);
	return 0;
}

//StorageBusiness
//////////////////////////////////////////////////////////////////////////
int CAppServerCmd::OnGetAdvertUrl(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_DB_SERVER, "CAppServerCmd::%s\n", __FUNCTION__);
#if defined(DBSERVER_SB)
	DWORD dwDeviceID = 0;
	DWORD dwCount = 0;
	LIST_DWORD lstAdvertID;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwDeviceID >> dwCount;
	for (int i = 0; i < dwCount; i++)
	{
		DWORD dwAdvertID = 0;
		bufferGet >> dwAdvertID;
		lstAdvertID.push_back(dwAdvertID);
	}
	//1 - 将视频广告和非视频广告分开，非视频广告放到lstAdvertInfo，视频广告ID放入lstVideoAdvertID
	LIST_ADVERTINFO lstAdvertInfo;
	LIST_DWORD lstVideoAdvertID;
	DWORD dwStoreID = 0;
	IOperationDBHandle* pSBHandle = DBHandle_GetODBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSBHandle, -1);
	pSBHandle->AdvertInfoMgr(dwStoreID, lstAdvertID, lstAdvertInfo, lstVideoAdvertID);
	
	//非视频处理
	if (lstAdvertInfo.size() != 0)
	{		
		//2 - 从数据库中查找到生成七牛token的各项key
		StorageAccount_t tAccount;
		ISDBHandle* pSDBHandle = DBHandle_GetSDBHandle();
		LOG_ASSERT_RET(LOG_DB_SERVER, pSDBHandle, -1);
		pSDBHandle->Query_StorageKeys(dwStoreID, tAccount);
		//3 - 非视频广告生成七牛url
		LIST_ADVERTINFO_REP lstAdvertInfoRep;
		pSBHandle->GetQiniuDownloadUrl(lstAdvertInfo, tAccount, lstAdvertInfoRep);
		//4 - 回调非视频广告给设备
		SendCmd_AdvertUrl_Rep(lstAdvertInfoRep);
	}
	//5 - 视频广告处理, 将要进行的视频广告放到待处理列表中
	if (lstVideoAdvertID.size() != 0)	pSBHandle->AddTaskList(dwDeviceID, lstVideoAdvertID);
#endif
	return 0;
}

int CAppServerCmd::OnReportProgress(PUCHAR pData, int nLen)
{
#if defined(DBSERVER_SB)
	LOG_DEBUG(LOG_DB_SERVER, "CAppServerCmd::%s\n", __FUNCTION__);
	DWORD dwDeviceID = 0;
	DWORD dwProgress = 0;
	DWORD dwSuccess = 0;
	DWORD dwCount = 0;
	Progress_t tInfo;
	LIST_PROGRESS lstProgress;
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> dwDeviceID >> dwCount;
	for (int i = 0; i < dwCount; i++)
	{
		bufferGet >> tInfo.dwAdID >> tInfo.dwProgress >> tInfo.cFinish;
//		LOG_DEBUG(LOG_DB_SERVER, "%s DeviceID %d, AdID = %d, Progress %d, dwSuccess %d\n", __FUNCTION__, dwDeviceID, tInfo.dwAdID, tInfo.dwProgress, tInfo.cFinish);
		lstProgress.push_back(tInfo);
	}
	IOperationDBHandle* pSBHandle = DBHandle_GetODBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSBHandle, -1);
	pSBHandle->AddLocalProgress(dwDeviceID, lstProgress);
#endif
}

int CAppServerCmd::SendCmd_AdvertUrl_Rep(LIST_ADVERTINFO_REP& lstAdvertInfoRep)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
#if defined(DBSERVER_SB)
	DWORD dwDeviceID = 0;
	DWORD dwCount = lstAdvertInfoRep.size();
	if(dwCount == 0) return 0;
	LIST_ADVERTINFO_REP::iterator iter = lstAdvertInfoRep.begin();
	for (; lstAdvertInfoRep.end() != iter; ++iter)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID;
		buffer << iter->tInfo.dwAdID << iter->tInfo.dwAdType << iter->tInfo.dwApplyID << iter->tInfo.dwApplyType;
		buffer << iter->tInfo.dwSize << iter->tInfo.dwStoreID << iter->tInfo.dwStoreType << iter->tInfo.dwTimeStamp;
		buffer << iter->tInfo.dwUsePosition << iter->tInfo.dwUseType;
		PutVariableStr(buffer, iter->tInfo.szFormat);
		PutVariableStr(buffer, iter->szUrl);
		SendPacket(buffer,  CMD_GET_ADVERTURLS_REP, ERROR_NO);
	}
#endif
	return 0;
}
//////////////////////////////////////////////////////////////////////////
//3.2.14 室内机绑定门口机
int CAppServerCmd::SendCmd_BindIndoorRep(BindInfoRep_t& tBindRep)
{
	LOG_DEBUG(LOG_DB_SERVER, "CAppServerCmd::%s\n", __FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << tBindRep.dwResult << tBindRep.dwIndoorID;
	SendPacket(buffer,CMD_INDOOR_BIND_DEVICE_REP,ERROR_NO);
	return 0;
}

int CAppServerCmd::SendCmd_DevListRep(LIST_DEVICEINFO& lstDevInfo)
{
	LOG_DEBUG(LOG_DB_SERVER, "[1008] CAppServerCmd::%s\n", __FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = lstDevInfo.size();
	buffer << dwCount;
	LOG_DEBUG(LOG_DB_SERVER,"CAppServerCmd::%s Count=%d\n",__FUNCTION__, dwCount);
	LIST_DEVICEINFO::iterator iter = lstDevInfo.begin();
	for(; iter != lstDevInfo.end(); iter++)
	{
		buffer << iter->dwDeviceID << iter->dwVendorID << iter->dwGroupID;
		buffer << CByteArrayBuffer(iter->szSerialNO, LENGTH_SERIALNO);
		PutVariableStr(buffer, (PUCHAR)iter->szDeviceName);
		LOG_DEBUG(LOG_MAIN,"CAppServerCmd::%s DeviceID=%d, VendorID=%d, GroupID=%d\n",__FUNCTION__, iter->dwDeviceID,iter->dwVendorID, iter->dwGroupID);
	}
	SendPacket(buffer, CMD_GET_BIND_DEV_STATE_REP, ERROR_NO);
	return 0;
}
//3.1.20 通知室内机其绑定设备的在线离线状态
int CAppServerCmd::SendCmd_DeviceIndoorID(DevStatus& tDevStat,LIST_DWORD& lstIndoorID)
{
	LOG_DEBUG(LOG_DB_SERVER, "CAppServerCmd::%s\n", __FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = lstIndoorID.size();
	buffer << tDevStat.dwDeviceID << tDevStat.dwStatus << dwCount;
	LIST_DWORD::iterator iter = lstIndoorID.begin();
	for(; iter != lstIndoorID.end(); iter++)
	{
		DWORD dwIndoorID = *iter;
		buffer << dwIndoorID;
	}
	SendPacket(buffer, CMD_GET_DEV_BIND_IDNOOR_REP, ERROR_NO);
	return 0;
}
//3.1.20 室内机获取绑定设备列表
int CAppServerCmd::SendCmd_DevStatusRep(LIST_DEVICEINFO& lstDevInfo)
{
	LOG_DEBUG(LOG_DB_SERVER, "CAppServerCmd::%s\n", __FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	DWORD dwCount = lstDevInfo.size();
	buffer << dwCount;
	LIST_DEVICEINFO::iterator iter = lstDevInfo.begin();
	for(; iter != lstDevInfo.end(); iter++)
	{
		buffer << iter->dwDeviceID;
		LOG_DEBUG(LOG_MAIN,"CAppServerCmd::%s DeviceID=%d\n",__FUNCTION__, iter->dwDeviceID);
	}
	SendPacket(buffer, CMD_GET_LIST_DEV_STAT_REP, ERROR_NO);
}
//////////////////////////////////////////////////////////////////////////
int CAppServerCmd::SendCmd_ServerInfo(ServerInfo_t& tInfo)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << (DWORD)tInfo.dwServerID << (DWORD)tInfo.dwVendorID << (BYTE)tInfo.bServerType;
	buffer << CByteArrayBuffer((PUCHAR)tInfo.szSerialNO, LENGTH_SERIALNO);
	PutVariableStr(buffer, (PUCHAR)tInfo.szUserName);
	buffer << CByteArrayBuffer((PUCHAR)tInfo.szPassword, LENGTH_PASSWORD);
	buffer << (DWORD)tInfo.dwIP << (UINT)tInfo.nNetID;
	PutVariableStr(buffer, (PUCHAR)tInfo.szPosition);
	return SendPacket(buffer, OTHERCMD_SERVER_AUTH, ERROR_NO);
}

int CAppServerCmd::SendCmd_ServerInfo(LIST_SERVERINFO& listInfo)
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwCount;
		return SendPacket(buffer, CMD_GET_SERVER_INFO_REP, ERROR_NO);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_ServerInfo(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << (DWORD)iter->dwServerID << (DWORD)iter->dwVendorID << (BYTE)iter->bServerType;
			buffer << CByteArrayBuffer((PUCHAR)iter->szSerialNO, LENGTH_SERIALNO);
			PutVariableStr(buffer, (PUCHAR)iter->szUserName);
			buffer << CByteArrayBuffer((PUCHAR)iter->szPassword, LENGTH_PASSWORD);
			buffer << (DWORD)iter->dwIP << (UINT)iter->nNetID;
			PutVariableStr(buffer, (PUCHAR)iter->szPosition);
			iter++;
		}
		SendPacket(buffer, CMD_GET_SERVER_INFO_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}
//获取设备列表3
int CAppServerCmd::SendCmd_UserDevice( DWORD dwUserID, DWORD dwIndex, LIST_DEVICEINFO& listInfo, MAP_DEVROOMINFO& mapDevRoomInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServerCmd::%s\n",__FUNCTION__);
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwIndex << dwCount;
		return SendPacket(buffer, CMD_GET_USERDEVICE_INFO_REP, ERROR_NO);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_UserDevice(mapDevRoomInfo, listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_DEVICEINFO::iterator iter = listInfo.begin();
	MAP_DEVROOMINFO::iterator iter2 = mapDevRoomInfo.begin();

	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwIndex << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << iter->dwDeviceID << iter->dwVendorID << iter->dwGroupID;
			buffer << CByteArrayBuffer((PUCHAR)iter->szSerialNO, LENGTH_SERIALNO);
			PutVariableStr(buffer, (PUCHAR)iter->szDeviceName);
			buffer << CByteArrayBuffer((PUCHAR)iter->szPassword, LENGTH_PASSWORD);
			buffer << CByteArrayBuffer((PUCHAR)iter->szRoom, LENGTH_USERROOM);
			LOG_DEBUG(LOG_DB_CLIENT,"UserID:%d,DeviceID:%d,Room:%s\n",dwUserID,iter->dwDeviceID,iter->szRoom);
			iter++;
			//////////////////////////////////////////////////////////////////////////
			DWORD dwDeviceID = iter2->first;
			DWORD dwRoomCount = iter2->second.size();
			buffer << dwDeviceID << dwRoomCount;
			LOG_DEBUG(LOG_DB_SERVER,"DeviceID:%d,Count:%d\n",dwDeviceID, dwRoomCount);
			LIST_ROOMINFO2::iterator iter3 = iter2->second.begin();
			for(; iter3 != iter2->second.end(); iter3++)
			{
				LOG_DEBUG(LOG_DB_SERVER,"RoomID:%d,Room:%s\n",iter3->dwRoomID, iter3->szRoom);
				buffer << iter3->dwRoomID;
				buffer << CByteArrayBuffer(iter3->szRoom, LENGTH_USERROOM);
			}
			iter2++;
		}
		SendPacket(buffer, CMD_GET_USERDEVICE_INFO_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CAppServerCmd::SendCmd_UserGroup( DWORD dwUserID, DWORD dwIndex, LIST_GROUPINFO& listInfo )
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwIndex << dwCount;
		return SendPacket(buffer, CMD_GET_USERGROUP_INFO_REP, ERROR_NO);
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
		SendPacket(buffer, CMD_GET_USERGROUP_INFO_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CAppServerCmd::SendCmd_UserRoom(DWORD dwUserID, DWORD dwIndex, LIST_ROOMINFO& listInfo)
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwIndex << dwCount;
		return SendPacket(buffer, CMD_GET_USERROOM_INFO_REP, ERROR_NO);
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
			iter++;
		}
		SendPacket(buffer, CMD_GET_USERROOM_INFO_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}
//6
int CAppServerCmd::SendCmd_DeviceInfo( DeviceInfo_t& tInfo, WORD wError )
{
	DECLARE_PUTBUFFER( buffer )
	buffer << (DWORD)tInfo.dwDeviceID << (DWORD)tInfo.dwConfigureIndex << (DWORD)tInfo.dwConfigureIndex2
		<< (DWORD)tInfo.dwVendorID << (DWORD)tInfo.dwAutoRelay;
	buffer << CByteArrayBuffer((PUCHAR)tInfo.szSerialNO, LENGTH_SERIALNO);
	PutVariableStr(buffer, (PUCHAR)tInfo.szDeviceName);
	buffer << tInfo.dwStoreID;
	PutVariableStr(buffer, (PUCHAR)tInfo.szDeadLine);//
	buffer << tInfo.dwGroupID;
	return SendPacket(buffer, CMD_GET_DEVICE_INFO_REP, wError);
}

int CAppServerCmd::SendCmd_DeviceUser( DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo )
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwIndex << dwCount;
		return SendPacket(buffer, CMD_GET_DEVICEUSER_INFO_REP, ERROR_NO);
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

int CAppServerCmd::SendCmd_DevicePush( DWORD dwDeviceID, DWORD dwIndex, LIST_PUSHINFO& listInfo )
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwIndex << dwCount;
		return SendPacket(buffer, CMD_GET_DEVICEPUSH_INFO_REP, ERROR_NO);
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

int CAppServerCmd::SendCmd_DeviceRoomSum(DWORD dwDeviceID, LIST_ROOMSUM& listInfo)
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		// PacketHeader(buffer, CMD_GET_DEVICE_ROOM_SUM_REP, ERROR_NO);
		// AddMulPkt(dwDeviceID, MP_Sum, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		SendPacket(buffer, CMD_GET_DEVICE_ROOM_SUM_REP, ERROR_NO);
		m_dwCmdFlag++;
		m_wSegFlag++;
		// StartSendMulPkt(dwDeviceID, MP_Sum, this);
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

// 		PacketHeader(buffer, CMD_GET_DEVICE_ROOM_SUM_REP, ERROR_NO, wTotalSeg, wSubSeg);
// 		AddMulPkt(dwDeviceID, MP_Sum, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		
		LOG_DEBUG(LOG_MAIN, "%s did=%d t=%d s=%d cut=%d\n", __FUNCTION__, dwDeviceID, wTotalSeg, wSubSeg, dwPacketCount);
		SendPacket(buffer, CMD_GET_DEVICE_ROOM_SUM_REP, ERROR_NO, wTotalSeg, wSubSeg);
		m_dwCmdFlag++;
	}
	m_wSegFlag++;
	// StartSendMulPkt(dwDeviceID, MP_Sum, this);
	return 0;
}

int CAppServerCmd::SendCmd_DeviceRoomUser(DWORD dwDeviceID, LIST_ROOMUSER& listInfo)
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		// PacketHeader(buffer, CMD_GET_DEVICE_ROOMUSER_REP, ERROR_NO);
		// AddMulPkt(dwDeviceID, MP_UserIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		SendPacket(buffer, CMD_GET_DEVICE_ROOMUSER_REP, ERROR_NO);
		m_dwCmdFlag++;
		m_wSegFlag++;
		// StartSendMulPkt(dwDeviceID, MP_UserIndex, this);
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
			// PacketHeader(buffer, CMD_GET_DEVICE_ROOMUSER_REP, ERROR_NO, wTotalSeg, wSubSeg);
			// AddMulPkt(dwDeviceID, MP_UserIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			SendPacket(buffer, CMD_GET_DEVICE_ROOMUSER_REP, ERROR_NO, wTotalSeg, wSubSeg);
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
			// PacketHeader(buffer, CMD_GET_DEVICE_ROOMUSER_REP, ERROR_NO, wTotalSeg, wSubSeg);
			// AddMulPkt(dwDeviceID, MP_UserIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			SendPacket(buffer, CMD_GET_DEVICE_ROOMUSER_REP, ERROR_NO, wTotalSeg, wSubSeg);
			m_dwCmdFlag++;
		}
		posCount++;
	}
	m_wSegFlag++;
	// StartSendMulPkt(dwDeviceID, MP_UserIndex, this);
	return 0;
}

int CAppServerCmd::SendCmd_DeviceRoomPush( DWORD dwDeviceID, LIST_ROOMPUSH& listInfo )
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		// PacketHeader(buffer, CMD_GET_DEVICE_ROOMPUSH_REP, ERROR_NO);
		// AddMulPkt(dwDeviceID, MP_PushIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		SendPacket(buffer, CMD_GET_DEVICE_ROOMPUSH_REP, ERROR_NO);
		m_dwCmdFlag++;
		m_wSegFlag++;
		// StartSendMulPkt(dwDeviceID, MP_PushIndex, this);
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
			// PacketHeader(buffer, CMD_GET_DEVICE_ROOMPUSH_REP, ERROR_NO, wTotalSeg, wSubSeg);
			// AddMulPkt(dwDeviceID, MP_PushIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			SendPacket(buffer, CMD_GET_DEVICE_ROOMPUSH_REP, ERROR_NO, wTotalSeg, wSubSeg);
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
			// PacketHeader(buffer, CMD_GET_DEVICE_ROOMPUSH_REP, ERROR_NO, wTotalSeg, wSubSeg);
			// AddMulPkt(dwDeviceID, MP_PushIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			SendPacket(buffer, CMD_GET_DEVICE_ROOMPUSH_REP, ERROR_NO, wTotalSeg, wSubSeg);
			m_dwCmdFlag++;
		}
		posCount++;
	}
	m_wSegFlag++;
	// StartSendMulPkt(dwDeviceID, MP_PushIndex, this);
	return 0;
}

int CAppServerCmd::SendCmd_DeviceRoomCard(DWORD dwDeviceID, LIST_ROOMCARD& listInfo)
{
	LOG_DEBUG(LOG_DB_SERVER," CAppServerCmd::%s\n",__FUNCTION__);
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		// PacketHeader(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO);
		// AddMulPkt(dwDeviceID, MP_CardIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		SendPacket(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO);
		m_dwCmdFlag++;
		m_wSegFlag++;
		// StartSendMulPkt(dwDeviceID, MP_CardIndex, this);
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
			// PacketHeader(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO, wTotalSeg, wSubSeg);
			// AddMulPkt(dwDeviceID, MP_CardIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			SendPacket(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO, wTotalSeg, wSubSeg);
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
				//	iter->dwRoomID, iter->dwCardIndex, pos->szCard, pos->bCardType);
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
			// PacketHeader(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO, wTotalSeg, wSubSeg);
			// AddMulPkt(dwDeviceID, MP_CardIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			SendPacket(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO, wTotalSeg, wSubSeg);
			m_dwCmdFlag++;
		}
		posCount++;
	}
	m_wSegFlag++;
	// StartSendMulPkt(dwDeviceID, MP_CardIndex, this);
	return 0;
}

//2.2.17  获取室内机信息
int CAppServerCmd::SendCmd_DeviceRoomIndoor(DWORD dwDeviceID, LIST_ROOMINDOOR2& listInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"=====================start===============================\n");
	LOG_DEBUG(LOG_DB_SERVER,"CAppServerCmd::%s Count=%d\n",__FUNCTION__, listInfo.size());

	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		// PacketHeader(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO);
		// AddMulPkt(dwDeviceID, MP_CardIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
		SendPacket(buffer, CMD_GET_DEVICE_ROOMINDOOR_REP, ERROR_NO);
		m_dwCmdFlag++;
		m_wSegFlag++;
		// StartSendMulPkt(dwDeviceID, MP_CardIndex, this);
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
			DECLARE_PUTBUFFER( buffer )
			buffer << dwDeviceID << dwPacketCount;
			LOG_DEBUG(LOG_DB_SERVER,"DeviceID=%d, dwPacketCount=%d\n",dwDeviceID, dwPacketCount);
			for (int i = 0; i < dwPacketCount; ++i)
			{
				Packet_RoomIndoorInfo(buffer, *iter);
				iter++;
				bChange = true;
			}

			m_tHeader.commandflag = m_dwCmdFlag;
			m_tHeader.segmentflag = m_wSegFlag;
			// PacketHeader(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO, wTotalSeg, wSubSeg);
			// AddMulPkt(dwDeviceID, MP_CardIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			SendPacket(buffer, CMD_GET_DEVICE_ROOMINDOOR_REP, ERROR_NO, wTotalSeg, wSubSeg);
			m_dwCmdFlag++;
		}
		else
		{
			bChange = false;

			DECLARE_PUTBUFFER( buffer )
			buffer << dwDeviceID << (int)1 << iter->dwRoomID << iter->dwIndoorIndex << dwPacketCount;

			for (int i = 0; i < dwPacketCount; ++i)
			{
//				PutVariableStr(buffer, (PUCHAR)pos->szSerialNO);
				buffer << CByteArrayBuffer(pos->szSerialNO, LENGTH_SERIALNO);
				buffer << pos->dwInDoorID;
				LOG_DEBUG(LOG_DB_SERVER,"IndoorID=%d, szSerialNO=%s\n", pos->dwInDoorID, pos->szSerialNO);
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
			// PacketHeader(buffer, CMD_GET_DEVICE_ROOMCARD_REP, ERROR_NO, wTotalSeg, wSubSeg);
			// AddMulPkt(dwDeviceID, MP_CardIndex, (PUCHAR)buffer, buffer.GetFilledSize(), m_dwCmdFlag, m_wSegFlag);
			SendPacket(buffer, CMD_GET_DEVICE_ROOMINDOOR_REP, ERROR_NO, wTotalSeg, wSubSeg);
			m_dwCmdFlag++;
		}
		posCount++;
	}
	m_wSegFlag++;
	LOG_DEBUG(LOG_DB_SERVER,"========================end============================\n");
	return 0;
}

int CAppServerCmd::SendCmd_DeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& listInfo)
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwDeviceID << dwCount;

		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		SendPacket(buffer, CMD_GET_DEVICE_ROOMOTHER_REP, ERROR_NO);
		m_dwCmdFlag++;
		m_wSegFlag++;
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
			posAttr++;
		}
		m_tHeader.commandflag = m_dwCmdFlag;
		m_tHeader.segmentflag = m_wSegFlag;
		SendPacket(buffer, CMD_GET_DEVICE_ROOMOTHER_REP, ERROR_NO, wTotalSeg, wSubSeg);
		m_dwCmdFlag++;
	}
	m_wSegFlag++;
	return 0;
}

int CAppServerCmd::SendCmd_PieceOfSerialNO( LIST_PIECEOFSERIALNO& listInfo )
{
	UINT nCount = listInfo.size();
	if (nCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << nCount;
		return SendPacket(buffer, CMD_PIECE_OF_SERIALNO, ERROR_NO);
	}
	WORD wTotalSeg = nCount / MAX_SEND_PIECE_OF_SERIALNO;
	if(nCount % MAX_SEND_PIECE_OF_SERIALNO) wTotalSeg++;

	LIST_PIECEOFSERIALNO::iterator iter = listInfo.begin();
	UINT nPacketCount, nRemainCount = nCount, dwCount = MAX_SEND_PIECE_OF_SERIALNO;
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		nPacketCount = 0;
		if (dwCount > nRemainCount) dwCount = nRemainCount;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwCount;

		for(; listInfo.end() != iter; ++iter)
		{
			buffer << iter->dwBegin << iter->dwEnd << iter->dwVendorID;

			nRemainCount--;
			if(++nPacketCount < dwCount) continue;

			SendPacket(buffer, CMD_PIECE_OF_SERIALNO, ERROR_NO, wTotalSeg, wSubSeg);	
			nPacketCount = 0; ++iter; break;
		}
	}
	return 0;
}

int CAppServerCmd::SendCmd_DserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo)
{
	DWORD dwCount = listInfo.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwVendorID << dwConfigureIndex << dwCount;
		return SendPacket(buffer, CMD_DSERVER_CONFIGUREINDEX, ERROR_NO);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DserverConfigureIndex(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwVendorID << dwConfigureIndex << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << (DWORD)iter->dwServerID << (DWORD)iter->dwVendorID << (BYTE)iter->bServerType;
			buffer << CByteArrayBuffer((PUCHAR)iter->szSerialNO, LENGTH_SERIALNO);
			PutVariableStr(buffer, (PUCHAR)iter->szUserName);
			buffer << CByteArrayBuffer((PUCHAR)iter->szPassword, LENGTH_PASSWORD);
			buffer << (DWORD)iter->dwIP << (UINT)iter->nNetID;
			PutVariableStr(buffer, (PUCHAR)iter->szPosition);
			iter++;
		}
		SendPacket(buffer, CMD_DSERVER_CONFIGUREINDEX, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CAppServerCmd::SendCmd_UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << dwVendorID << dwUserID;
	return SendPacket(buffer, CMD_USER_CONFIGUREINDEX, ERROR_NO);
}

int CAppServerCmd::SendCmd_DeviceConfigureIndex( DWORD dwVendorID, DWORD dwDeviceID, DWORD dwRoomID, BYTE bType )
{
	DECLARE_PUTBUFFER( buffer )
	buffer << dwVendorID << dwDeviceID << dwRoomID << bType;
	return SendPacket(buffer, CMD_DEVICE_CONFIGUREINDEX, ERROR_NO);
}

int CAppServerCmd::SendCmd_UpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, int nType, LIST_DWORD& lstRoomID)
{
	DWORD dwCount = lstRoomID.size();
	if (dwCount == 0) return 0;

	BYTE bType = nType;
	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_RoomIndex(lstRoomID, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_DWORD::iterator iter = lstRoomID.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwVendorID << dwDeviceID << bType << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			DWORD dwRoomID = *iter;
			buffer << dwRoomID;
			iter++;
		}
		SendPacket(buffer, CMD_UPDATE_DEVICE_ROOMINFO, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CAppServerCmd::SendCmd_ClearRooms(MAP_DWORD& mapDeviceID)
{
	DWORD dwCount = mapDeviceID.size();
	if (dwCount == 0) return 0;

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_ClearRooms(mapDeviceID, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	MAP_DWORD::iterator iter = mapDeviceID.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			DWORD dwDeviceID = iter->first;
			buffer << dwDeviceID;
			iter++;
		}
		SendPacket(buffer, CMD_CLEAR_ROOMS, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CAppServerCmd::SendCmd_UpdateDevice(MAP_DWORD& mapDeviceID)
{
	DWORD dwCount = mapDeviceID.size();
	if (dwCount == 0) return 0;

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_UpdateDevice(mapDeviceID, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	MAP_DWORD::iterator iter = mapDeviceID.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			DWORD dwDeviceID = iter->first;
			buffer << dwDeviceID;
			iter++;
		}
		SendPacket(buffer, CMD_UPDATE_DEVICE, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CAppServerCmd::SendCmd_SetPushInfoEx(WORD wError, BYTE bOpr, ClientTokenArray_t& tInfo)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << bOpr << tInfo.dwUserID << tInfo.dwVendorID << tInfo.bLanguage << tInfo.nCount;
	for (int i = 0; i < tInfo.nCount; i++)
	{
		buffer << (BYTE)tInfo.tToken[i].bMainFlag << (BYTE)tInfo.tToken[i].bPushType;
		PutVariableStr(buffer, (PUCHAR)tInfo.tToken[i].szToken);
	}
	buffer << tInfo.bLoginOtherPlaceFlag;
	PutVariableStr(buffer, tInfo.szCreated);
	return SendPacket(buffer, CMD_ADD_DEL_PUSH_INFO_EX_REP, wError);
}

int CAppServerCmd::SendCmd_Qiniu_StorageAccount(StorageTag_t& tTag, StorageAccount_t& tAccount)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID;
	buffer << tAccount.dwStoreID;
	PutVariableStr(buffer, (PUCHAR)tAccount.szAccessKey);
	PutVariableStr(buffer, (PUCHAR)tAccount.szSecretKey);
	PutVariableStr(buffer, (PUCHAR)tAccount.szBucket);
	PutVariableStr(buffer, (PUCHAR)tAccount.szDomain);
	return SendPacket(buffer, CMD_GET_STORAGEACCOUNT_REP, ERROR_NO);
}
//开门记录2004
int CAppServerCmd::SendCmd_UnlockRecordsRep(UnlockRep_t& rInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"2004:%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << rInfo.dwDeviceID << rInfo.dwResult << rInfo.dwCount;
	for(int i = 0; i < rInfo.dwCount; i++)
	{
		buffer << rInfo.UnlockIndex[i];
		LOG_DEBUG(LOG_DB_SERVER,"Index:%d\n",rInfo.UnlockIndex[i]);
	}
	return SendPacket(buffer,DD_PUSH_UNLOCK_RECORD_REP,ERROR_NO);
}

//1.查询特殊人群列表
int CAppServerCmd::Query_SpecialCrowdList(LIST_SPECIALCROWD& lstSpecialCrowd,  LIST_SMSINFO2& lstSmsInfo)
{
	LOG_DEBUG(LOG_DB_SERVER," CAppServerCmd::%s\n",__FUNCTION__);
	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pDBHandle, -1);
	ISDBHandle* pSDBHandle = DBHandle_GetSDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSDBHandle, -1);

	pDBHandle->Query_SpecialUsers(lstSpecialCrowd);
	pSDBHandle->Query_SpecialCrowdInfo(lstSpecialCrowd, lstSmsInfo);
	return 0;
}

int CAppServerCmd::SendCmd_Qiniu_StorageKeys(StorageTag_t& tTag, LIST_STORE_ACCOUNTKEYS& lstAccountKeys)
{
	LOG_DEBUG(LOG_DB_SERVER, "CAppServerCmd::%s\n", __FUNCTION__);

	DWORD dwCount = lstAccountKeys.size();
	if (dwCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID << dwCount;
		return SendPacket(buffer, CMD_GET_STORAGEKEYS_REP, ERROR_NO);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_ListStoreAccountKeys(lstAccountKeys, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_STORE_ACCOUNTKEYS::iterator iter = lstAccountKeys.begin();
	LIST_STOREKEY::iterator pos;
	bool bChange = true;
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		if (lstAccountKeys.end() == iter) break;

		if (bChange) pos = iter->lstKey.begin();

		DWORD dwHighBit16 = ( (*posCount) & 0xffff0000 ) >> 16; // 高16位 0表示不同的RoomInfo，其他表示同一个RoomInfo
		DWORD dwPacketCount = ( (*posCount) & 0x0000ffff ); // 低16位 高16位为0时RoomInfo个数 / 高16位不为0时PushInfo个数

		if (dwHighBit16 == 0)
		{
			DECLARE_PUTBUFFER( buffer )
			buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID << dwPacketCount;

			for (int i = 0; i < dwPacketCount; ++i)
			{
				Packet_StoreAccountKeys(buffer, *iter);
				iter++;
				bChange = true;
			}

			SendPacket(buffer, CMD_GET_STORAGEKEYS_REP, ERROR_NO, wTotalSeg, wSubSeg);
		}
		else
		{
			bChange = false;

			DECLARE_PUTBUFFER( buffer )
			buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID << (int)1;
			buffer << iter->tAccount.dwStoreID;
			PutVariableStr(buffer, (PUCHAR)iter->tAccount.szAccessKey);
			PutVariableStr(buffer, (PUCHAR)iter->tAccount.szSecretKey);
			PutVariableStr(buffer, (PUCHAR)iter->tAccount.szBucket);
			PutVariableStr(buffer, (PUCHAR)iter->tAccount.szDomain);
			buffer << dwPacketCount;

			for (int i = 0; i < dwPacketCount; ++i)
			{
				buffer << pos->dwDeviceID << pos->dwRoomID << pos->dwSize << pos->dwStoreID << pos->bType << pos->bRecReason;
				buffer << CByteArrayBuffer((PUCHAR)pos->szTimeStamp, LENGTH_TIMESTAMP2);

				pos++;
				if (pos == iter->lstKey.end())
				{
					iter++;
					bChange = true;
					if (lstAccountKeys.end() != iter) pos = iter->lstKey.begin();
					break;
				}
			}

			SendPacket(buffer, CMD_GET_STORAGEKEYS_REP, ERROR_NO, wTotalSeg, wSubSeg);
		}
		posCount++;
	}
	return 0;
}

IMPLEMENT_SINGLETON(CAppServerMgr)
CAppServerMgr::CAppServerMgr()
{
	m_dwTimeout = 0;
#if defined(DSERVER_SDB)
	AddTimer(TIMER_SPECIALCROWD,600,this);
#endif
//#if defined(DSERVER_SB)
	AddTimer(TIMER_ADVERT,TIMER_QNADVERTCHECK,this);
//#endif
}

CAppServerMgr::~CAppServerMgr()
{
	ThreadStop();
	DBHandle_SetSink(NULL);
	SBHandle_SetSink(NULL);

	DelTimer(TIMER_NORMAL, this);
#if defined(DSERVER_SDB)
	DelTimer(TIMER_SPECIALCROWD,this);
#endif
#if defined(DSERVER_SB)
	DelTimer(TIMER_ADVERT, this);
#endif
}

bool CAppServerMgr::Start(WORD wRawUdpPort)
{
	DBHandle_SetSink(this);
	SBHandle_SetSink(this);
	AddTimer(TIMER_NORMAL, DATABASE_KEEPALIVE_TIMER, this);

	if (false == OpenSocket(wRawUdpPort))
	{
		return false;
	}

	if (false == ThreadStart())
	{
		return false;
	}
	return true;
}

int CAppServerMgr::AddServer(ServerInfo_t& tInfo, INetConnection* pCon)
{
	LOG_DEBUG(LOG_DB_SERVER, "CAppServerMgr::AddServer pCon %p type: %d svrid: %d\n", pCon, tInfo.bServerType, tInfo.dwServerID);
	CAppServer* p = GetElem(tInfo.dwServerID);
	if (NULL == p)
	{
		try
		{
			p = new CAppServer();
		}
		catch(std::bad_alloc &memExp)
		{
			if (pCon) { pCon->Disconnect(); NetworkDestroyConnection(pCon); }
			return -1;
		}
		AddElem(tInfo.dwServerID, p);
		LOG_DEBUG(LOG_DB_SERVER, "CAppServerMgr::AddServer m_mapElement.size() %d\n", m_mapElement.size());
	}
	if (pCon)
	{
		p->SetNetConnection(pCon);
		p->SetServerInfo(tInfo);
	}
	return 0;
}

void CAppServerMgr::PieceOfSerialNO()
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tInfo;
		p->GetServerInfo(tInfo);
		//if ((tInfo.bServerType == SERVER_TYPE_D) && (tInfo.dwVendorID == dwVendorID))
		if (tInfo.bServerType == SERVER_TYPE_LOGIN)
		{
			p->InnerCmd_Pieceofsn();
		}
	}
}

void CAppServerMgr::DserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tInfo;
		p->GetServerInfo(tInfo);
		//if ((tInfo.bServerType == SERVER_TYPE_D) && (tInfo.dwVendorID == dwVendorID))
		if ( (tInfo.bServerType == SERVER_TYPE_LOGIN) || (tInfo.bServerType == SERVER_TYPE_D) )
		{
			p->InnerCmd_Dsvrcfgindex(dwVendorID, dwConfigureIndex);
		}
	}
}

void CAppServerMgr::UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwVendorID %d dwUserID %d\n", __FUNCTION__, dwVendorID, dwUserID);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tInfo;
		p->GetServerInfo(tInfo);  
		//if ((tInfo.bServerType == SERVER_TYPE_D) && (tInfo.dwVendorID == dwVendorID))
		if (tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_UserConfigureIndex(dwVendorID, dwUserID);
		}
	}
}
//02
void CAppServerMgr::UpdateDeviceDeadLine(LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_DB_SERVER, "102:%s, DeviceCount:%d \n", __FUNCTION__, lstDevID.size());
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tInfo;
		p->GetServerInfo(tInfo);
		if (tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_DeviceDeadLine(lstDevID);
		}
	}
}

//22.更新物業公告
void CAppServerMgr::UpdatePropertyAnnounce(DWORD dwVillageId, DWORD dwNoticeIndex)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServerMgr::%s\n", __FUNCTION__);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for(; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if(NULL == p) continue;
		ServerInfo_t tInfo;										
		p->GetServerInfo(tInfo);
		if(tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_PropertyAnnounce(dwVillageId, dwNoticeIndex);
		}
	}
}
//更新广告信息
void CAppServerMgr::UpdateAdvertInfo(DWORD dwVillageId, DWORD dwAdvertIndex)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServerMgr::%s\n", __FUNCTION__);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for(; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if(NULL == p) continue;
		ServerInfo_t tInfo;										
		p->GetServerInfo(tInfo);
		if(tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_UpdateAdvertInfo(dwVillageId, dwAdvertIndex);
		}
	}
}

void CAppServerMgr::DownLoadAdvertInfo(DWORD dwDeviceID, AdvertInfo_t& tAdInfo, PUCHAR pUrl)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServerMgr::%s\n", __FUNCTION__);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for(; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if(NULL == p) continue;
		ServerInfo_t tInfo;										
		p->GetServerInfo(tInfo);
		if(tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_DownLoadAdvertInfo(dwDeviceID, tAdInfo, pUrl);
		}
	}
}

void CAppServerMgr::UpdateFirmwareRequest(PCHAR strVersion, LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s\n", __FUNCTION__);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for(; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if(NULL == p) continue;
		ServerInfo_t tInfo;										
		p->GetServerInfo(tInfo);
		if(tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_UpdateFirmwareRequest(strVersion,lstDevID);
		}
	}
}

void CAppServerMgr::DeviceConfigureIndex(DWORD dwVendorID, DWORD dwDeviceID, DWORD dwRoomID, BYTE bType)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwVendorID %d dwDeviceID %d dwRoomID %d bType %d\n", __FUNCTION__, dwVendorID, dwDeviceID, dwRoomID, bType);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tInfo;
		p->GetServerInfo(tInfo);
		//if ((tInfo.bServerType == SERVER_TYPE_D) && (tInfo.dwVendorID == dwVendorID))
		if (tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_DeviceConfigureIndex(dwVendorID, dwDeviceID, dwRoomID, bType);
		}
	}
}

void CAppServerMgr::UpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, int nType, LIST_DWORD& lstRoomID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwVendorID %d dwDeviceID %d nType %d RoomCount %d\n",
		__FUNCTION__, dwVendorID, dwDeviceID, nType, lstRoomID.size());
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tInfo;
		p->GetServerInfo(tInfo);
		//if ((tInfo.bServerType == SERVER_TYPE_D) && (tInfo.dwVendorID == dwVendorID))
		if (tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_UpdateDeviceRoom(dwVendorID, dwDeviceID, nType, lstRoomID);
		}
	}
}

void CAppServerMgr::ClearRooms(MAP_DWORD& mapDeviceID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s DeviceCount %d\n", __FUNCTION__, mapDeviceID.size());
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tInfo;
		p->GetServerInfo(tInfo);
		//if ((tInfo.bServerType == SERVER_TYPE_D) && (tInfo.dwVendorID == dwVendorID))
		if (tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_ClearRooms(mapDeviceID);
		}
	}
}

void CAppServerMgr::UpdateDevice(MAP_DWORD& mapDeviceID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s DeviceCount %d\n", __FUNCTION__, mapDeviceID.size());
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tInfo;
		p->GetServerInfo(tInfo);
		//if ((tInfo.bServerType == SERVER_TYPE_D) && (tInfo.dwVendorID == dwVendorID))
		if (tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_UpdateDevice(mapDeviceID);
		}
	}
}

void CAppServerMgr::UpdateDeviceEx(int nType, LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s nType %d DeviceCount %d\n", __FUNCTION__, nType, lstDevID.size());
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tInfo;
		p->GetServerInfo(tInfo);
		if (tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_UpdateDeviceEx(nType, lstDevID);
		}
	}
	m_bOpr_db = true;
	ActivateThread();
}

void CAppServerMgr::LoginOtherPlace(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo)
{
	//LOG_DEBUG(LOG_DB_SERVER, "%s Cur Server Count %d\n", __FUNCTION__, m_mapElement.size());
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if (NULL == p) continue;
		ServerInfo_t tServerInfo;
		p->GetServerInfo(tServerInfo);
		if (tServerInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_SetPushInfoEx((WORD)nReason, bOpr, tInfo);
		}
	}
}

int CAppServerMgr::OnCmdResponse(LIST_MSG& listResponse)
{
	//LOG_DEBUG(LOG_DB_SERVER, "CAppServerCmd::%s listResponse.size %d\n", __FUNCTION__, listResponse.size());
	LIST_MSG::iterator iter = listResponse.begin();
	for (; iter != listResponse.end(); iter++)
	{
		PUCHAR pData = (PUCHAR)iter->strMsg.c_str();
		int nLen = iter->strMsg.size();
		MsgType_e eMsgType = iter->tMsgTag.eMsgType;
		DWORD dwServerID = iter->tMsgTag.dwServerID;

		if (eMsgType == MsgType_dbcallback)
		{
			//LOG_DEBUG(LOG_DB_SERVER, "MsgType_dbcallback\n");
			ELEM_MAP::iterator iter = m_mapElement.begin();
			for (; iter != m_mapElement.end(); iter++)
			{
				CAppServer* p = iter->second;
				if (NULL == p) continue;
				ServerInfo_t tInfo;
				p->GetServerInfo(tInfo);
				//if ((tInfo.bServerType == SERVER_TYPE_D) && (tInfo.dwVendorID == dwVendorID))
				if (tInfo.bServerType == SERVER_TYPE_D)
				{
					p->SendPacket(pData, nLen);
				}
			}
		}
		else if (eMsgType == MsgType_auth && dwServerID)
		{
			//LOG_DEBUG(LOG_DB_SERVER, "MsgType_auth CServerAuth 0x%08x\n", dwServerID);
			ServerInfo_t tInfo;
			memset(&tInfo, 0, sizeof(ServerInfo_t));
			if (-1 == ParseServerAuth(pData, nLen, tInfo)) continue;
			CServerAuth* pAuth = (CServerAuth*)dwServerID;
			if (CServerAuthMgr::Instance()->IsAlive(pAuth))
			{
				LIST_SERVERINFO listInfo;
				listInfo.push_back(tInfo);
				pAuth->OnQuery_ServerInfo(listInfo);
			}
		}
		else if ((eMsgType == MsgType_cmd) && dwServerID)
		{
			//LOG_DEBUG(LOG_DB_SERVER, "MsgType_cmd dwServerID %d\n", dwServerID);
			CAppServer* p = GetElem(dwServerID);
			if (p)
			{
				p->SendPacket(pData, nLen);
			}			
		}
	}
	return 0;
}

void CAppServerMgr::ServerAuth(DWORD dwID, PUCHAR pSN)
{
	BYTE szBuffer[128];
	CPutBuffer buffer( szBuffer, 128 );

	// Header
	buffer << (BYTE)m_tHeader.groupcode << OTHERCMD_SERVER_AUTH/*Command ID*/ << (BYTE)m_tHeader.reserved0/*Reserved0*/
		<< (WORD)m_tHeader.version/*Version*/ << (WORD)m_tHeader.reserved1/*Reserved1*/
		<< (DWORD)m_tHeader.destinationid/*Source ID*/
		<< (DWORD)m_tHeader.sourceid/*Destination ID*/
		<< (DWORD)m_tHeader.commandflag/*Command Flag*/
		<< (WORD)1/*Total Segment*/ << (WORD)1/*Sub Segment*/
		<< (WORD)m_tHeader.segmentflag/*Segment Flag*/ << (WORD)m_tHeader.reserved2/*Reserved2*/
		<< (DWORD)m_tHeader.reserved3/*Reserved3*/;
	// Payload
	buffer << (WORD)0/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;

	buffer << CByteArrayBuffer((PUCHAR)pSN, LENGTH_SERIALNO);
	int nLen = buffer.GetFilledSize();
	LOG_DEBUG(LOG_DB_SERVER, "ServerAuth len:%d\n", nLen);
	
	MsgTag_t tMsgTag;
	tMsgTag.dwServerID = dwID;
	tMsgTag.bServerType = 0;
	tMsgTag.eMsgType = MsgType_auth;
	AddCommand(tMsgTag, (PUCHAR)buffer, nLen);
}

void CAppServerMgr::OnTimer(TimerReason_e eReason, ITimerSink* pSink)
{
#if defined(DBSERVER_DB)
	if (TIMER_NORMAL == eReason)
	{	
		PieceOfSerialNO();
		DserverConfigureIndex(0, 0);
		m_bTimer_db = true;
		ActivateThread();
	}
#endif
	if (TIMER_ADVERT == eReason)
	{
		VideoProgressMgr();
		VideoDownloadMgr();
	}
#if defined(DBSERVER_SDB)
	if(TIMER_SPECIALCROWD == eReason)
	{
		LIST_SPECIALCROWD lstSpecialCrowd;
		LIST_SMSINFO2 lstSmsInfo;
		Query_SpecialCrowdList(lstSpecialCrowd, lstSmsInfo);
		LOG_DEBUG(LOG_DB_SERVER,"lstSpecialCrowd.size = %d, lstNotifySmsInfo.size = %d\n", lstSpecialCrowd.size(), lstSmsInfo.size());
		//////////////////////////////////////////////////////////////////////////
		LIST_SPECIALCROWD::iterator it = lstSpecialCrowd.begin();
		for(it = lstSpecialCrowd.begin(); lstSpecialCrowd.end() != it;  ++it)
		{
			LOG_DEBUG(LOG_DB_SERVER, "VendorID %d GroupID %d DeviceID %d RoomID %d UserID %d Phone %s PUserID %d PPhone %s Time %d, DevName %s\n",
				it->dwVendorID, it->dwGroupID,  it->dwDevcieID,  it->dwRoomID, it->dwUserID, it->szUserPhone,  it->dwPropertyUserID,  it->szPropertyPhone, 
				it->dwUnlockTime, it->szDeviceName);
		}

		LIST_SMSINFO2::iterator iter = lstSmsInfo.begin();
		for(; lstSmsInfo.end() != iter; ++iter)
		{
			for(it = lstSpecialCrowd.begin(); lstSpecialCrowd.end() != it;  ++it)
			{
				if(it->dwRoomID == iter->dwRoomID && it->dwUserID != iter->dwUserID)		
				{
					Phone_t tPhone;
					memcpy(tPhone.szMobilePhone, it->szUserPhone, sizeof(it->szUserPhone));
					iter->lstPhone.push_back(tPhone);
				}
			}
			LOG_DEBUG(LOG_DB_SERVER,"UserID %d, RoomID %d, VendorID %d, Language %d, Phone %s, ProPhone %s\n",iter->dwUserID, iter->dwRoomID, iter->dwVendorID, iter->bLanguage, iter->szMobilePhone, iter->szPropertyPhone);
			LIST_PHONE::iterator iphone = iter->lstPhone.begin();
			for(; iter->lstPhone.end() != iphone; iphone++)
			{
				LOG_DEBUG(LOG_DB_CLIENT,"Phone==>%s\n", iphone->szMobilePhone);
			}
			SpecialCrowdSms(*iter);
		}
	}
#endif
}

int CAppServerMgr::ParseServerAuth(PUCHAR pData, int nLen, ServerInfo_t& tInfo)
{
	CGetBuffer bufferGet(pData, nLen);
	int nNeedLen = PACKET_HEADER_SIZE + 2*sizeof(DWORD) + sizeof(BYTE) + LENGTH_SERIALNO;
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "2 wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	bufferGet.Skip(PACKET_HEADER_SIZE);
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
	return 0;
}

void CAppServerMgr::OnUserConfigureIndex(DWORD dwVendorID, DWORD dwUserID)
{
	MsgTag_t tMsgTag;
	memcpy(&tMsgTag, &m_tMsgTag, sizeof(MsgTag_t));
	m_tMsgTag.dwServerID = 0;
	m_tMsgTag.bServerType = 0;
	m_tMsgTag.eMsgType = MsgType_dbcallback;
	SendCmd_UserConfigureIndex(dwVendorID, dwUserID);

	memcpy(&m_tMsgTag, &tMsgTag, sizeof(MsgTag_t));
}

void CAppServerMgr::OnUpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, LIST_DWORD& lstRoomID, BYTE bType)
{
	MsgTag_t tMsgTag;
	memcpy(&tMsgTag, &m_tMsgTag, sizeof(MsgTag_t));
	m_tMsgTag.dwServerID = 0;
	m_tMsgTag.bServerType = 0;
	m_tMsgTag.eMsgType = MsgType_dbcallback;
	SendCmd_UpdateDeviceRoom(dwVendorID, dwDeviceID, (int)bType, lstRoomID);

	memcpy(&m_tMsgTag, &tMsgTag, sizeof(MsgTag_t));
}

void CAppServerMgr::OnClearRooms(DWORD dwVendorID, DWORD dwDeviceID)
{
	MsgTag_t tMsgTag;
	memcpy(&tMsgTag, &m_tMsgTag, sizeof(MsgTag_t));
	m_tMsgTag.dwServerID = 0;
	m_tMsgTag.bServerType = 0;
	m_tMsgTag.eMsgType = MsgType_dbcallback;
	MAP_DWORD mapDeviceID; mapDeviceID.insert(std::make_pair(dwDeviceID, dwVendorID));
	SendCmd_ClearRooms(mapDeviceID);

	memcpy(&m_tMsgTag, &tMsgTag, sizeof(MsgTag_t));
}

void CAppServerMgr::OnUpdateDevice(DWORD dwVendorID, DWORD dwDeviceID)
{
	MsgTag_t tMsgTag;
	memcpy(&tMsgTag, &m_tMsgTag, sizeof(MsgTag_t));
	m_tMsgTag.dwServerID = 0;
	m_tMsgTag.bServerType = 0;
	m_tMsgTag.eMsgType = MsgType_dbcallback;
	MAP_DWORD mapDeviceID; mapDeviceID.insert(std::make_pair(dwDeviceID, dwVendorID));
	SendCmd_UpdateDevice(mapDeviceID);

	memcpy(&m_tMsgTag, &tMsgTag, sizeof(MsgTag_t));
}

void CAppServerMgr::DeleteDeviceOnline(LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServerMgr::%s\n", __FUNCTION__);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for(; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if(NULL == p) continue;
		ServerInfo_t tInfo;										
		p->GetServerInfo(tInfo);
		if(tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_DeleteDeviceOnline(lstDevID);
		}
	}
}

void CAppServerMgr::SpecialCrowdSms(SmsInfo2_t& tSmsInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"CAppServerMgr::%s UserID %d\n", __FUNCTION__, tSmsInfo.dwUserID);
	ELEM_MAP::iterator iter = m_mapElement.begin();
	for(; iter != m_mapElement.end(); iter++)
	{
		CAppServer* p = iter->second;
		if(NULL == p) continue;
		ServerInfo_t tInfo;										
		p->GetServerInfo(tInfo);
		if(tInfo.bServerType == SERVER_TYPE_D)
		{
			p->SendCmd_SpecialCrowdSms(tSmsInfo);
		}
	}
}

// 6 - 七牛广告视频下载管理
bool CAppServerMgr::VideoDownloadMgr()
{
//	LOG_DEBUG(LOG_DB_SERVER, "CAppServerMgr::%s m_lstAdID.size() = %d\n", __FUNCTION__, m_lstAdID.size());
#if defined(DBSERVER_SB)
	DWORD dwCount = m_lstAdID.size();
	if (dwCount == 0) 	
	{
		//检查磁盘空间
//		if (false == CheckDiskSpace())	return true;
		GetQiniuTaskList();
		return true;
	}
	if (m_dwVideoFlag == 1)		return true;
	m_dwVideoFlag = 1;

	LIST_DWORD::iterator iter_ad = m_lstAdID.begin();
	m_dwAdID = *iter_ad;
	IOperationDBHandle* pSBHandle = DBHandle_GetODBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSBHandle, -1);
	AdvertInfo_t tInfo;
	if(false == pSBHandle->Query_Advert(m_dwAdID, tInfo))
	{
		LOG_DEBUG(LOG_DB_SERVER, "Not Find %d\n", m_dwAdID);
		m_lstAdID.erase(iter_ad);
		m_dwVideoFlag = 0;
		return true;
	}

	DWORD dwStoreID = tInfo.dwStoreID;
	StorageAccount_t tAccount;
	ISDBHandle* pSDBHandle = DBHandle_GetSDBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSDBHandle, -1);
	if(false == pSDBHandle->Query_StorageKeys(dwStoreID, tAccount))
	{
		LOG_DEBUG(LOG_DB_SERVER, "SDB Not Find StorageKey, StoreID = %d\n", tInfo.dwStoreID);
		m_lstAdID.erase(iter_ad);
		m_dwVideoFlag = 0;
		return true;
	}

	CSTRING strAdUrl;
	pSBHandle->Qiniu_GetAdvertUrl(tInfo, tAccount, strAdUrl);
	
	char szName[LENGTH_NAME] = {0};
	sprintf(szName, "%d.%s", tInfo.dwAdID, tInfo.szFormat);
	char szPathName[LENGTH_NAME] = {0};
	sprintf(szPathName, "/home/www/default/VResource_d/%s", szName);

	IHttpDownload* pHttpHandle = Reg_HttpDownload();
	LOG_ASSERT_RET(LOG_DB_SERVER, pHttpHandle, -1);
	pHttpHandle->Download((char*)strAdUrl.c_str(), (const char*)szPathName, this);

	m_lstAdID.erase(iter_ad);
	m_strAdName.assign(szName);
#endif
	return true;
}

// 7 - 获取视频广告下载进度管理, 上报七牛视频广告下载完成，并删除m_lstAdID中AdID
bool CAppServerMgr::VideoProgressMgr()
{
//	LOG_DEBUG(LOG_DB_SERVER, "CAppServerMgr::%s m_dwTimeout = %d\n", __FUNCTION__, m_dwTimeout);
#if defined(DBSERVER_SB)
	if (m_dwVideoFlag == 1)
	{	
		LOG_DEBUG(LOG_DB_SERVER,"%s %d Progress [%d%] Finish = %d, Timeout = %d\n",__FUNCTION__, m_dwAdID, m_dwProgress, m_bFinish, m_dwTimeout);
		m_dwTimeout += TIMER_QNADVERTCHECK;
	}
	if (m_bFinish == 1)
	{
		LOG_DEBUG(LOG_DB_SERVER, "QiniuVideoAdvert Download Finish\n");
		IOperationDBHandle* pSBHandle = DBHandle_GetODBHandle();
		LOG_ASSERT_RET(LOG_DB_SERVER, pSBHandle, -1);
		pSBHandle->ClearQiniuList(m_dwAdID);
		m_dwTimeout = 0;
		m_bFinish = 0;
		m_dwAdID = 0;
		m_dwProgress = 0;
		m_dwVideoFlag = 0;
	}
	if (m_dwTimeout > 60)//1分钟超时
	{
		m_dwTimeout = 0;
		m_bFinish = 0;
		m_dwAdID = 0;
		m_dwProgress = 0;
		m_dwVideoFlag = 0;
	}
#endif
	return true;
}

bool CAppServerMgr::GetQiniuTaskList()
{
//	LOG_DEBUG(LOG_DB_SERVER, "CAppServerMgr::%s\n", __FUNCTION__);
#if defined(DBSERVER_SB)
	if (m_lstAdID.size() != 0) return false;
	IOperationDBHandle* pSBHandle = DBHandle_GetODBHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pSBHandle, -1);
	if(false == pSBHandle->GetQiniuList(m_lstAdID)) return false;
#endif
	return true;
}

void CAppServerMgr::OnDownloadStatus(char* pUrl, int nPesent, bool bFinish)
{
#if defined(DBSERVER_SB)
	LOG_DEBUG(LOG_DB_SERVER,"%s Progress [%d%] Finish = %d\n", __FUNCTION__, nPesent, bFinish);
	if (m_dwProgress < nPesent)
	{
		m_dwProgress = nPesent;
		m_dwTimeout = 0;
	}
	m_bFinish = bFinish;
#endif
}