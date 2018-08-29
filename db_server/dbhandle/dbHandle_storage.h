#pragma once

#include <mysql++.h>
#include "singleton.h"
#include "DataStruct.h"
#include "dbHandleInterface.h"
#include "UtilityInterface.h"
#include "Thread.h"
#include <string>
#include "DBSDK.h"

using namespace mysqlpp;

class FindStoreAcutByStoreID
{
public:
	FindStoreAcutByStoreID(DWORD dwID) { m_dwStoreID = dwID; }
	bool operator() (LIST_STORE_ACCOUNTKEYS::value_type& pos)
	{
		if (m_dwStoreID == pos.tAccount.dwStoreID) return true;
		return false;
	}
private:
	DWORD m_dwStoreID;
};

class CStorageDataBaseHandle : public ISDBHandle, 
							   public CThread, 
							   public ITimerSink,
							   public IVisitor, 
							   public IUnlock,
							   public IAlarm
{
	DECLARE_SINGLETON( CStorageDataBaseHandle )
public:
	CStorageDataBaseHandle();
	virtual ~CStorageDataBaseHandle();

	bool Connect(PUCHAR pHost, PUCHAR pDatabase, PUCHAR pUserName, PUCHAR pPassword);
	int  GetError();
	bool IsConnect(){ return m_bConnectSuccess; }
	void OnTimer();  
	void ThreadLoop();
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

	bool Query_TotalCount();
	//UInt64 GetTotalCount_StorageKey() { return m_nStoreKeyCount; }
	//UInt64 GetTotalCount_StorageKeybak() { return m_nStoreKeybakCount; }

	// ISDBHandle
	bool Query_StorageInfo(StorageAccount_t& tAccount);
	bool Query_StorageKeys(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_STORE_ACCOUNTKEYS& lstAccountKeys);
	bool Query_StorageKeys(DWORD dwStoreID, StorageAccount_t& tAccount);

	//查询用户最新的开门记录信息
	bool Query_UnlockInfo(DWORD dwUserID, char tabName[LENGTH_TABLENAME]);
	//特殊人群
	bool Query_SpecialCrowdInfo(LIST_SPECIALCROWD& lstSpecialCrowd,  LIST_SMSINFO2& lstSmsInfo);
	bool Query_OnSpecialCrowdInfo(char* pName, SpecialCrowd_t& tInfo);

	bool StorageAlarmRecordList(LIST_ALARMSTATUS& listInfo);
	bool StorageUnlockList(LIST_DEVICE_UNLOCK& listInfo);
	int  InsertAlarmRecords(DWORD Flag);
	int  InsertUnlockRecords(DWORD Flag);
	bool Insert_AlarmStatusRecords(LIST_ALARMSTATUS& listInfo);
	bool Insert_OpenDoorRecords(LIST_DEVICE_UNLOCK& listInfo);
	bool Delete_OpenDoorRecords();
	bool Query_QiniuInfo(LIST_DWORD& lstStoreID);

	//访客留影
	bool StoregeVisitorList(LIST_STOREKEY& listInfo);
	bool Insert_StoreKey(DWORD Flag);
	bool Insert_StoreKey(LIST_STOREKEY& listInfo);

	bool Query_Visitor(StoreVisitor_t& tVisitor, LIST_DWORD& lstRoomID,LIST_STORE_ACCOUNTKEYS& lstAccountKeys);
	bool Query_StorageKeys2(PCHAR pName,PCHAR pYM, DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_STOREKEY& listKey);
	bool Query_StorageAcount2(StorageAccount_t& tAccount);
	bool SpecialCrowds(LIST_SPECIALCROWD& m_lstSpecialCrowd);

private:
	// 保持数据库连接，防止连接超时 
	void Keepalive();

	void Timer_Delete_StoreKey(); // storekey and storekeybak

	bool Query_RoomStoreCount(DWORD dwDeviceID, DWORD dwRoomID, int& nStoreCount);
	bool Delete_StoreKey(DWORD dwDeviceID, DWORD dwRoomID, int nStoreCount);

	void CheckTotalCount_StoreKey(); // storekey and storekeybak
	void CheckOvertime_StoreKey();
	
	bool Delete_StoreKeyByCount();
	bool Delete_StoreKeybakByCount();
	bool Delete_StoreKeyByTime();
	bool Delete_StoreKeybakByTime();

	// 执行数据库操作，并捕获异常
	int CatchException(Query& query);
	int CatchException(Query& query, StoreQueryResult& res);

	void MkRoomIDStr( PCHAR pName, LIST_DWORD& lstRoomID, CSTRING& strRoomID );
	void MkStoreIDStr(SET_DWORD& setStoreID, CSTRING& strStoreID);

	bool ToStorageEngine(LIST_STOREKEY& lstKey);

	Connection m_clsSrcDBCon;
	int m_nError;
	bool m_bConnectSuccess;

	DWORD m_nStoreKeyCount;
	DWORD m_nStoreKeybakCount;

	LIST_DEVICE_UNLOCK m_listUnlockRecords;//存开门记录
	LIST_STOREKEY m_listVisitorRecords;//存访客留影记录
	LIST_ALARMSTATUS m_listAlarmStatusRecords;//报警状态
};

