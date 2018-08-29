#include "MainDBClient.h"

IMPLEMENT_SINGLETON(CMainDBClient)
CMainDBClient::CMainDBClient()
{
}
CMainDBClient::~CMainDBClient()
{
}
int CMainDBClient::GetServerInfo( PUCHAR pServerType )
{
	DECLARE_PUTBUFFER(bufferPut)
	int nLen = strlen((const char*)pServerType);
	bufferPut << CByteArrayBuffer(pServerType, nLen);
	return SendMsg(bufferPut, CMD_GET_SERVER_INFO, ERROR_NO);
}
int CMainDBClient::QueryMobilePhone( PUCHAR pMobilePhone )
{
	DECLARE_PUTBUFFER(bufferPut)
	PutBase64Str(bufferPut, pMobilePhone);
	return SendMsg(bufferPut, CMD_QUERY_USER, ERROR_NO);
}
int CMainDBClient::SetSecret( DWORD dwVendorID, BYTE bLanguage, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pMobilePhone )
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwVendorID << bLanguage;
	PutVariableStr(bufferPut, pUserName);
	bufferPut << CByteArrayBuffer(pPassword, LENGTH_PASSWORD);
	PutBase64Str(bufferPut, pMobilePhone);
	return SendMsg(bufferPut, CMD_SET_SECRET, ERROR_NO);
}
int CMainDBClient::GetUserInfo( PUCHAR pUserName, DWORD dwVendorID )
{
	DECLARE_PUTBUFFER(bufferPut)
	PutVariableStr(bufferPut, pUserName);
	bufferPut << dwVendorID;
	return SendMsg(bufferPut, CMD_GET_USER_INFO, ERROR_NO);
}
int CMainDBClient::GetUserDeviceInfo( DWORD dwUserID )
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwUserID;
	return SendMsg(bufferPut, CMD_GET_USERDEVICE_INFO, ERROR_NO);
}

int CMainDBClient::GetUserGroupInfo( DWORD dwUserID )
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwUserID;
	return SendMsg(bufferPut, CMD_GET_USERGROUP_INFO, ERROR_NO);
}
int CMainDBClient::GetUserRoomInfo( DWORD dwUserID )
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwUserID;
	return SendMsg(bufferPut, CMD_GET_USERROOM_INFO, ERROR_NO);
}
int CMainDBClient::GetDeviceInfo( PUCHAR pSN )
{
	LOG_DEBUG(LOG_DB_CLIENT, "Registered | %s  \n", __FUNCTION__);
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << CByteArrayBuffer(pSN, LENGTH_SERIALNO);
	return SendMsg(bufferPut, CMD_GET_DEVICE_INFO, ERROR_NO);
}
int CMainDBClient::GetDeviceUserInfo( DWORD dwDeviceID )
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwDeviceID;
	return SendMsg(bufferPut, CMD_GET_DEVICEUSER_INFO, ERROR_NO);
}
int CMainDBClient::GetDevicePushInfo(DWORD dwDeviceID)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwDeviceID;
	return SendMsg(bufferPut, CMD_GET_DEVICEPUSH_INFO, ERROR_NO);
}

int CMainDBClient::AddDevice( DWORD dwUserID, PUCHAR pSerialNO, PUCHAR pDevName, PUCHAR pRoom )
{
	DECLARE_PUTBUFFER( bufferPut )
	bufferPut << dwUserID;
	bufferPut << CByteArrayBuffer(pSerialNO, LENGTH_SERIALNO);
	PutVariableStr(bufferPut, pDevName);
	PutVariableStr(bufferPut, pRoom);
	return SendMsg(bufferPut, CMD_ADD_DEVICE, ERROR_NO);
}
int CMainDBClient::AddDelPushInfoEx(BYTE bOpr, ClientTokenArray_t& tInfo)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << bOpr << tInfo.dwUserID << tInfo.dwVendorID << tInfo.bLanguage << tInfo.nCount;
	for (int i = 0; i < tInfo.nCount; i++)
	{
		bufferPut << (BYTE)tInfo.tToken[i].bMainFlag << (BYTE)tInfo.tToken[i].bPushType;
		PutVariableStr(bufferPut, (PUCHAR)tInfo.tToken[i].szToken);
	}
	bufferPut << tInfo.dwView;
	return SendMsg(bufferPut, CMD_ADD_DEL_PUSH_INFO_EX, 0);
}
int CMainDBClient::SetDeviceName( DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName )
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwVendorID << dwUserID << dwDeviceID;
	PutVariableStr(bufferPut, pDeviceName);
	return SendMsg(bufferPut, CMD_SET_DEVICE_NAME, 0);
}
int CMainDBClient::DelDevice(DWORD dwUserID, DWORD dwDeviceID)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwUserID << dwDeviceID;
	return SendMsg(bufferPut, CMD_DEL_DEVICE, 0);
}
int CMainDBClient::Authorize(DWORD dwOwnerID, PUCHAR pUser, DWORD dwDeviceID)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwOwnerID;
	PutVariableStr(bufferPut, pUser);
	bufferPut << dwDeviceID;
	return SendMsg(bufferPut, CMD_AUTHORIZE, 0);
}
int CMainDBClient::Authorize2(DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName, PUCHAR pRoom)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwUserID << dwDeviceID;
	PutVariableStr(bufferPut, pDeviceName);
	PutVariableStr(bufferPut, pRoom);
	return SendMsg(bufferPut, CMD_AUTHORIZE2, 0);
}

int CMainDBClient::GetDeviceRoomSum( DWORD dwDeviceID )
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwDeviceID;
	return SendMsg(bufferPut, CMD_GET_DEVICE_ROOM_SUM, 0);
}
int CMainDBClient::GetDeviceRoomInfo( DWORD dwDeviceID, BYTE bType, LIST_DWORD& listInfo )
{
	LOG_DEBUG(LOG_MAIN,"CMainDBClient::%s\n",__FUNCTION__);
	DWORD dwCount = listInfo.size();
	DECLARE_PUTBUFFER( buffer )
	buffer << dwDeviceID << bType << dwCount;
	LIST_DWORD::iterator iter = listInfo.begin();
	for (; iter != listInfo.end(); iter++)
	{
		DWORD dwRoomID = *iter;
		buffer << dwRoomID;
	}
	return SendMsg(buffer, CMD_GET_DEVICE_ROOMINFO, 0);
}

int CMainDBClient::GetDServerInfo( PUCHAR pSN )
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << CByteArrayBuffer(pSN, LENGTH_SERIALNO);
	return SendMsg(bufferPut, CMD_GET_DSERVER_INFO, 0);
}
//公告
int CMainDBClient::GetNoticeIndex(DWORD dwGroupID,DWORD dwDeviceID)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwGroupID << dwDeviceID;
	return SendMsg(bufferPut, DD_UPDATE_BULLETIN, 0);
}
//广告
int CMainDBClient::GetAdvertIndex(DWORD dwGroupID,DWORD dwDeviceID)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwGroupID << dwDeviceID;
	return SendMsg(bufferPut, CMD_GET_ADVERT_INFO, 0);
}
//访客配置
int CMainDBClient::GetVisitorCfg(DWORD dwGroupID,DWORD dwDeviceID)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwGroupID << dwDeviceID;
	return SendMsg(bufferPut, CMD_UPDATE_VISITORCFG, 0);
}

//获取室内机绑定门口机在线状态
int CMainDBClient::GetBindDevStatus(DWORD dwIndoorID)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwIndoorID;
	return SendMsg(bufferPut, CMD_GET_BIND_DEV_STATE, 0);
}
int CMainDBClient::GetBindIndoorID(DWORD dwDeviceID)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwDeviceID;
	return SendMsg(bufferPut, CMD_GET_DEV_BIND_IDNOOR, 0);
}

int CMainDBClient::RepoartAlarmStatus(AlarmStatus_t& alarmStatus)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << alarmStatus.dwDeviceID << alarmStatus.dwRoomID << alarmStatus.dwType << alarmStatus.dwSubType;
	bufferPut << CByteArrayBuffer(alarmStatus.szTimeStamp, LENGTH_TIMESTAMP);
	return SendMsg(bufferPut, CMD_REPORT_ALARMSTATUS, 0);
}

int CMainDBClient::Qiniu_GetStorageAccount(StorageTag_t& tTag)
{
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID;
	return SendMsg(bufferPut, CMD_GET_STORAGEACCOUNT, 0);
}
int CMainDBClient::Qiniu_GetStorageKeys(StorageTag_t& tTag, StoreKey_t& tKey)
{
	LOG_DEBUG(LOG_MAIN,"CMainDBClient::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER(buffer)
	buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID;
	buffer << tKey.dwDeviceID << tKey.dwRoomID << tKey.dwSize << tKey.dwStoreID << tKey.bType << tKey.bRecReason;
	buffer << CByteArrayBuffer((PUCHAR)tKey.szTimeStamp, LENGTH_TIMESTAMP2);
	return SendMsg(buffer, CMD_GET_STORAGEKEYS, 0);
}
int CMainDBClient::Qiniu_GetStorageKeys2(StorageTag_t& tTag, StoreVisitor_t& tVisitor)
{
	LOG_DEBUG(LOG_MAIN,"CMainDBClient::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER(buffer)
	buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID;
	buffer << tVisitor.tKey.dwDeviceID << tVisitor.tKey.dwRoomID << tVisitor.tKey.dwSize << tVisitor.tKey.dwStoreID 
		<< tVisitor.tKey.bType << tVisitor.tKey.bRecReason;
	buffer << CByteArrayBuffer((PUCHAR)tVisitor.tKey.szTimeStamp, LENGTH_TIMESTAMP2);
	buffer << tVisitor.startIndex << tVisitor.dwCount;
	return SendMsg(buffer, CMD_GET_STORAGEKEYS2, 0);
}
int CMainDBClient::Qiniu_ReportUploadResult( StoreKey_t& tKey )
{
	DECLARE_PUTBUFFER(buffer)
	buffer << tKey.dwDeviceID << tKey.dwRoomID << tKey.dwSize << tKey.dwStoreID << tKey.bType << tKey.bRecReason;
	buffer << CByteArrayBuffer((PUCHAR)tKey.szTimeStamp, LENGTH_TIMESTAMP2);
	return SendMsg(buffer, CMD_REPORT_UPLOAD_RESULT, 0);
}

int CMainDBClient::GetDeviceCfg(DWORD dwID, int nType)
{
	DECLARE_PUTBUFFER(buffer)
	buffer << dwID << nType;
	return SendMsg(buffer, CMD_GET_DEVICE_CFG, 0);
}
int CMainDBClient::SetPushSwitch(DWORD dwUserID, int nSwitch)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << dwUserID << nSwitch;
	return SendMsg(buffer, CMD_PUSH_SWITCH, ERROR_NO);
}

//室内机
int CMainDBClient::IndoorBindDevice(PUCHAR szIndoorSN, LIST_BIND_INFO& listBindInfo)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << CByteArrayBuffer(szIndoorSN, LENGTH_SERIALNO);
	DWORD dwCount = listBindInfo.size();
	buffer << dwCount;
	LIST_BIND_INFO::iterator iter = listBindInfo.begin();
	for(; iter != listBindInfo.end(); iter++)
	{
		buffer << iter->dwDeviceID << iter->dwRoomID;
	}
	return SendMsg(buffer, CMD_INDOOR_BIND_DEVICE, ERROR_NO);
}

int CMainDBClient::GetIndoorDevList(PUCHAR pSN)
{
	LOG_DEBUG(LOG_MAIN,"CMainDBClient::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << CByteArrayBuffer(pSN, LENGTH_SERIALNO);
	return SendMsg(buffer, CMD_GET_INDOOR_INFO, ERROR_NO);
}
