#pragma once

#include "DataBaseInterface.h"
#include "singleton.h"
#include "ElemMap_DataBase.h"
#include "UtilityInterface.h"

class CGetUserGroup : public ITimerSink
{
public:
	CGetUserGroup(DWORD dwUserID);
	virtual ~CGetUserGroup();

	void CallbackUserGroup(DWORD dwUserID, DWORD dwIndex, LIST_GROUPINFO& lstUserDeviceInfo);
	void SetCallbackID(DWORD dwCallbackID);
	int GetUserGroup();

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
private:
	DWORD m_dwUserID;
	DWORD m_dwIndex;
	LIST_GROUPINFO m_listUserGroupInfo;

	SET_DWORD m_setCallbackID;
	bool m_bCallback;
};

class CGetUserGroupMgr : public CElemMapDataBase<CGetUserGroup>
{
	DECLARE_SINGLETON(CGetUserGroupMgr)
public:
	CGetUserGroupMgr(){}
	~CGetUserGroupMgr(){}

	int GetUserGroup(DWORD dwCallbackID, DWORD dwUserID);

};
