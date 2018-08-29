#pragma once

#include "DataBaseInterface.h"
#include "singleton.h"
#include "ElemMap_DataBase.h"
#include "UtilityInterface.h"

extern DWORD g_dwGetUserInfo_VendorID;
extern CSTRING g_strGetUserInfo_UserName;

class CGetUserInfo : public ITimerSink
{
public:
	CGetUserInfo(DWORD dwID);
	virtual ~CGetUserInfo();

	void CallbackUserInfo(UserInfo_t& tInfo, ClientTokenArray_t& tArray);
	void SetCallbackID(DWORD dwCallbackID);
	int GetUserInfo(PUCHAR pUserName, DWORD dwVendorID);

	DWORD GetVendorID(){ return m_dwVendorID; }
	PUCHAR GetUserName(){ return (PUCHAR)m_szUserName; }

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
private:
	DWORD m_dwID;
	DWORD m_dwVendorID;
	BYTE m_szUserName[LENGTH_NAME+1];
	UserInfo_t m_tUserInfo;
	ClientTokenArray_t m_tArray;
	SET_DWORD m_setCallbackID;
	bool m_bCallback;
};

class CGetUserInfoMgr : public CElemMapDataBase<CGetUserInfo>
{
	DECLARE_SINGLETON(CGetUserInfoMgr)
public:
	CGetUserInfoMgr(){}
	~CGetUserInfoMgr(){}

	int GetUserInfo(DWORD dwCallbackID, PUCHAR pUserName, DWORD dwVendorID);

	class FindCGetUserInfo
	{
	public:
		bool operator() (std::map<DWORD, CGetUserInfo*>::value_type& pos)
		{
			if (g_dwGetUserInfo_VendorID != pos.second->GetVendorID()) return false;
			if (g_strGetUserInfo_UserName.compare((const char*)pos.second->GetUserName())) return false;
			return true;
		}
	};

};
