#include "DataBaseA.h"
#include "MainDBClient.h"
#include "CGetUserInfoMgr.h"
#include "CGetUserDeviceMgr.h"
#include "CGetUserGroupMgr.h"
#include "CGetUserRoomMgr.h"

IMPLEMENT_SINGLETON(CDataBaseAMgr)
CDataBaseA::CDataBaseA(DWORD dwID, IDataBaseSink* pSink)
{
	m_dwID = dwID;
	m_pSink = pSink;
}

CDataBaseA::~CDataBaseA()
{
	m_pSink = NULL;
}

IDataBaseSink* CDataBaseAMgr::GetDataBaseSink(DWORD dwID)
{
	CDataBaseA* p = CDataBaseAMgr::Instance()->GetElem(dwID);
	if (NULL == p) return NULL;
	return p->GetDataBaseSink();
}

int CDataBaseA::GetServerInfo( PUCHAR pServerType )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetServerInfo(pServerType);
}

int CDataBaseA::QueryMobilePhone( PUCHAR pMobilePhone )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->QueryMobilePhone(pMobilePhone);
}

int CDataBaseA::SetSecret( DWORD dwVendorID, BYTE bLanguage, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pMobilePhone )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->SetSecret(dwVendorID, bLanguage, pUserName, pPassword, pMobilePhone);
}

int CDataBaseA::GetUserInfo( PUCHAR pUserName, DWORD dwVendorID )
{
	return CGetUserInfoMgr::Instance()->GetUserInfo(m_dwID, pUserName, dwVendorID);
}

int CDataBaseA::GetUserDeviceInfo( DWORD dwUserID )
{
	return CGetUserDeviceMgr::Instance()->GetUserDevice(m_dwID, dwUserID);
}

int CDataBaseA::GetUserGroupInfo( DWORD dwUserID )
{
	return CGetUserGroupMgr::Instance()->GetUserGroup(m_dwID, dwUserID);
}

int CDataBaseA::GetUserRoomInfo( DWORD dwUserID )
{
	return CGetUserRoomMgr::Instance()->GetUserRoom(m_dwID, dwUserID);
}
//2
int CDataBaseA::GetDeviceInfo( PUCHAR pSN )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetDeviceInfo(pSN);
}

int CDataBaseA::GetDeviceUserInfo( DWORD dwDeviceID )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetDeviceUserInfo(dwDeviceID);
}

int CDataBaseA::GetDevicePushInfo(DWORD dwDeviceID)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetDevicePushInfo(dwDeviceID);
}

int CDataBaseA::AddDevice( DWORD dwUserID, PUCHAR pSerialNO, PUCHAR pDevName, PUCHAR pRoom )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->AddDevice(dwUserID, pSerialNO, pDevName, pRoom);
}

int CDataBaseA::AddDelPushInfoEx(BYTE bOpr, ClientTokenArray_t& tInfo)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->AddDelPushInfoEx(bOpr, tInfo);
}

int CDataBaseA::SetDeviceName( DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->SetDeviceName(dwVendorID, dwUserID, dwDeviceID, pDeviceName);
}

int CDataBaseA::DelDevice(DWORD dwUserID, DWORD dwDeviceID)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->DelDevice(dwUserID, dwDeviceID);
}

int CDataBaseA::Authorize(DWORD dwOwnerID, PUCHAR pUser, DWORD dwDeviceID)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->Authorize(dwOwnerID, pUser, dwDeviceID);
}

int CDataBaseA::Authorize2(DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName, PUCHAR pRoom)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->Authorize2(dwUserID, dwDeviceID, pDeviceName, pRoom);
}

int CDataBaseA::GetDeviceRoomSum( DWORD dwDeviceID )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetDeviceRoomSum(dwDeviceID);
}

int CDataBaseA::GetDeviceRoomInfo( DWORD dwDeviceID, BYTE bType, LIST_DWORD& listInfo )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetDeviceRoomInfo(dwDeviceID, bType, listInfo);
}

int CDataBaseA::GetDServerInfo( PUCHAR pSN )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetDServerInfo(pSN);
}

int CDataBaseA::GetNoticeIndex(DWORD dwGroupID, DWORD dwDeviceID)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetNoticeIndex(dwGroupID, dwDeviceID);
}
//广告
int CDataBaseA::GetAdvertIndex(DWORD dwGroupID,DWORD dwDeviceID)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetAdvertIndex(dwGroupID, dwDeviceID);
}

//访客配置
int CDataBaseA::GetVisitorCfg(DWORD dwGroupID,DWORD dwDeviceID)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetVisitorCfg(dwGroupID, dwDeviceID);
}


//获取室内机绑定门口机在线状态
int CDataBaseA::GetBindDevStatus(DWORD dwIndoorID)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetBindDevStatus(dwIndoorID);
}
int CDataBaseA::GetBindIndoorID(DWORD dwDeviceID)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetBindIndoorID(dwDeviceID);
}

int CDataBaseA::RepoartAlarmStatus(AlarmStatus_t& alarmStatus)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->RepoartAlarmStatus(alarmStatus);
}

int CDataBaseA::Qiniu_GetStorageAccount(StorageTag_t& tTag)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->Qiniu_GetStorageAccount(tTag);
}

int CDataBaseA::Qiniu_GetStorageKeys(StorageTag_t& tTag, StoreKey_t& tKey)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->Qiniu_GetStorageKeys(tTag, tKey);
}

int CDataBaseA::Qiniu_GetStorageKeys2(StorageTag_t& tTag, StoreVisitor_t& tVisitor)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->Qiniu_GetStorageKeys2(tTag, tVisitor);
}

int CDataBaseA::Qiniu_ReportUploadResult( StoreKey_t& tKey )
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->Qiniu_ReportUploadResult(tKey);
}

int CDataBaseA::GetDeviceCfg(DWORD dwDeviceID, int nType)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetDeviceCfg(dwDeviceID, nType);
}

int CDataBaseA::SetPushSwitch(DWORD dwUserID, int nSwitch)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->SetPushSwitch(dwUserID, nSwitch);
}

//室内机
int CDataBaseA::IndoorBindDevice(PUCHAR pIndoorSN, LIST_BIND_INFO& listBindInfo)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->IndoorBindDevice(pIndoorSN, listBindInfo);
}

int CDataBaseA::GetIndoorDevList(PUCHAR pSN)
{
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetIndoorDevList(pSN);
}

