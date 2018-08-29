#pragma once

#include "DataBaseInterface.h"
#include "singleton.h"
#include "ElemMap_DataBase.h"

class CDataBaseA : public IDataBase
{
public:
	CDataBaseA(DWORD dwID, IDataBaseSink* pSink);
	virtual ~CDataBaseA();

	DWORD GetID(){return m_dwID;}
	IDataBaseSink* GetDataBaseSink() { return m_pSink; }
	//////////////////////////////////////////////////////////////////////////
	// IDataBase interface
	int GetServerInfo		(PUCHAR pServerType);
	int QueryMobilePhone	(PUCHAR pMobilePhone);
	int SetSecret			(DWORD dwVendorID, BYTE bLanguage, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pMobilePhone);
	int GetUserInfo			(PUCHAR pUserName, DWORD dwVendorID);
	int GetUserDeviceInfo	(DWORD dwUserID);
	int GetUserGroupInfo	(DWORD dwUserID);
	int GetUserRoomInfo		(DWORD dwUserID);
	int GetDeviceInfo		(PUCHAR pSN);
	int GetDeviceUserInfo	(DWORD dwDeviceID);
	int GetDevicePushInfo	(DWORD dwDeviceID);
	int AddDevice			(DWORD dwUserID, PUCHAR pSerialNO, PUCHAR pDevName, PUCHAR pRoom);
	int AddDelPushInfoEx	(BYTE bOpr, ClientTokenArray_t& tInfo);
	int SetDeviceName		(DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName);
	int DelDevice			(DWORD dwUserID, DWORD dwDeviceID);
	int Authorize			(DWORD dwOwnerID, PUCHAR pUser, DWORD dwDeviceID);
	int Authorize2			(DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName, PUCHAR pRoom);

	int GetDeviceRoomSum	(DWORD dwDeviceID);
	int GetDeviceRoomInfo	(DWORD dwDeviceID, BYTE bType, LIST_DWORD& listInfo);

	int GetDServerInfo		(PUCHAR pSN);
	int GetNoticeIndex		(DWORD dwGroupID,DWORD dwDeviceID);
	int GetAdvertIndex		(DWORD dwGroupID,DWORD dwDeviceID);

	int GetVisitorCfg			(DWORD dwGroupID,DWORD dwDeviceID);
	int GetBindDevStatus	(DWORD dwIndoorID);
	int GetBindIndoorID		(DWORD dwDeviceID);
	
	int RepoartAlarmStatus(AlarmStatus_t& alarmStatus);
	int Qiniu_GetStorageAccount(StorageTag_t& tTag);
	int Qiniu_GetStorageKeys(StorageTag_t& tTag, StoreKey_t& tKey);
	int Qiniu_GetStorageKeys2(StorageTag_t& tTag, StoreVisitor_t& tVisitor);
	int Qiniu_ReportUploadResult(StoreKey_t& tKey);

	int GetDeviceCfg		(DWORD dwDeviceID, int nType);
	int SetPushSwitch		(DWORD dwUserID, int nSwitch);

	//ÊÒÄÚ»ú
	int IndoorBindDevice	(PUCHAR szIndoorSN, LIST_BIND_INFO& listBindInfo);
	int GetIndoorDevList(PUCHAR pSN);
private:
	DWORD m_dwID;
	IDataBaseSink* m_pSink;
};

class CDataBaseAMgr : public CElemMapDataBase<CDataBaseA>
{
	DECLARE_SINGLETON(CDataBaseAMgr)
public:
	CDataBaseAMgr(){}
	~CDataBaseAMgr(){}

	IDataBaseSink* GetDataBaseSink(DWORD dwID);
};
