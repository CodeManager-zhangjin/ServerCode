#include "CGetUserGroupMgr.h"
#include "MainDBClient.h"
#include "DataBaseA.h"

IMPLEMENT_SINGLETON(CGetUserGroupMgr)
int CGetUserGroupMgr::GetUserGroup(DWORD dwCallbackID, DWORD dwUserID)
{
	CGetUserGroup* p = GetElem(dwUserID);
	if (p)
	{
		//		LOG_DEBUG(LOG_DB_CLIENT, "1 %s CallbackID:%d UserName:%s VendorID:%d\n", __FUNCTION__, dwCallbackID, pUserName, dwVendorID);
		p->SetCallbackID(dwCallbackID);
	}
	else
	{
		//		LOG_DEBUG(LOG_DB_CLIENT, "2 %s CallbackID:%d UserName:%s VendorID:%d\n", __FUNCTION__, dwCallbackID, pUserName, dwVendorID);
		try
		{
			p = new CGetUserGroup(dwUserID);
		}
		catch(std::bad_alloc &memExp)
		{
			printf("new CGetUserGroup failed\n"); return -1;
		}
		AddElem(dwUserID, p);
		p->SetCallbackID(dwCallbackID);
		p->GetUserGroup();
	}
	return 0;
}

CGetUserGroup::CGetUserGroup(DWORD dwUserID)
{
	m_dwUserID = dwUserID;
	m_dwIndex = 0;
	m_listUserGroupInfo.clear();
	m_bCallback = false;
	AddTimer(TIMER_NORMAL, 60, this);
}

CGetUserGroup::~CGetUserGroup()
{
	DelTimer(TIMER_NORMAL, this);
}

void CGetUserGroup::SetCallbackID(DWORD dwCallbackID)
{
	m_setCallbackID.insert(dwCallbackID);

	if (m_bCallback) CallbackUserGroup(m_dwUserID, m_dwIndex, m_listUserGroupInfo);
}

int CGetUserGroup::GetUserGroup()
{	
	CMainDBClient::Instance()->SetDataBaseAID(m_dwUserID);
	return CMainDBClient::Instance()->GetUserGroupInfo(m_dwUserID);
}

void CGetUserGroup::CallbackUserGroup(DWORD dwUserID, DWORD dwIndex, LIST_GROUPINFO& lstUserGroupInfo)
{
	SET_DWORD::iterator iter = m_setCallbackID.begin();
	for (; iter != m_setCallbackID.end(); iter++)
	{
		IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(*iter);
		if (NULL == pSink) continue;
		printf("CallbackUserGroup UserID %d Index %d list %d\n", dwUserID, dwIndex, lstUserGroupInfo.size());
		pSink->OnGetUserGroupInfo(dwUserID, dwIndex, lstUserGroupInfo);
	}
	m_setCallbackID.clear();

	if (m_bCallback == false)
	{
		AddTimer(TIMER_NORMAL, 3, this);
		m_dwIndex = dwIndex;
		m_listUserGroupInfo.insert(m_listUserGroupInfo.end(), lstUserGroupInfo.begin(), lstUserGroupInfo.end());
		m_bCallback = true;
	}
}

void CGetUserGroup::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	CGetUserGroupMgr::Instance()->DelElem(m_dwUserID);
}
