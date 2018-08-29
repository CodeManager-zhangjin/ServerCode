#pragma once

#include "DataBaseInterface.h"
#include "singleton.h"
#include "ElemMap_DataBase.h"
#include "UtilityInterface.h"

class CGetUserRoom : public ITimerSink
{
public:
	CGetUserRoom(DWORD dwUserID);
	virtual ~CGetUserRoom();

	void CallbackUserRoom(DWORD dwUserID, DWORD dwIndex, LIST_ROOMINFO& lstInfo);
//	void CallbackIndoorBindDev(LIST_DEVSTATUS& lstDevStatus);
	void SetCallbackID(DWORD dwCallbackID);
	int GetUserRoom();

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
private:
	DWORD m_dwUserID;
	DWORD m_dwIndex;
	LIST_ROOMINFO m_listInfo;

	SET_DWORD m_setCallbackID;
	bool m_bCallback;
};

class CGetUserRoomMgr : public CElemMapDataBase<CGetUserRoom>
{
	DECLARE_SINGLETON(CGetUserRoomMgr)
public:
	CGetUserRoomMgr(){}
	~CGetUserRoomMgr(){}

	int GetUserRoom(DWORD dwCallbackID, DWORD dwUserID);

};
