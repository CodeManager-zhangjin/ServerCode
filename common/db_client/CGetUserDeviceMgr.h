#pragma once

#include "DataBaseInterface.h"
#include "singleton.h"
#include "ElemMap_DataBase.h"
#include "UtilityInterface.h"

class CGetUserDevice : public ITimerSink
{
public:
	CGetUserDevice(DWORD dwUserID);
	virtual ~CGetUserDevice();

	void CallbackUserDevice(DWORD dwUserID, DWORD dwIndex, LIST_DEVICEINFO& lstUserDeviceInfo,MAP_DEVROOMINFO& mapDevRoomInfo);
	void SetCallbackID(DWORD dwCallbackID);
	int GetUserDevice();

	DWORD GetUserID(){ return m_dwUserID; }

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
private:
	DWORD m_dwUserID;
	DWORD m_dwIndex;
	LIST_DEVICEINFO m_listUserDeviceInfo;

	SET_DWORD m_setCallbackID;
	bool m_bCallback;
};

class CGetUserDeviceMgr : public CElemMapDataBase<CGetUserDevice>
{
	DECLARE_SINGLETON(CGetUserDeviceMgr)
public:
	CGetUserDeviceMgr(){}
	~CGetUserDeviceMgr(){}

	int GetUserDevice(DWORD dwCallbackID, DWORD dwUserID);
};
