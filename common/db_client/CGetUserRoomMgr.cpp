#include "CGetUserRoomMgr.h"
#include "MainDBClient.h"
#include "DataBaseA.h"

IMPLEMENT_SINGLETON(CGetUserRoomMgr)
int CGetUserRoomMgr::GetUserRoom(DWORD dwCallbackID, DWORD dwUserID)
{
	CGetUserRoom* p = GetElem(dwUserID);
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
			p = new CGetUserRoom(dwUserID);
		}
		catch(std::bad_alloc &memExp)
		{
			printf("new CGetUserRoom failed\n"); return -1;
		}
		AddElem(dwUserID, p);
		p->SetCallbackID(dwCallbackID);
		p->GetUserRoom();
	}
	return 0;
}

CGetUserRoom::CGetUserRoom(DWORD dwUserID)
{
	m_dwUserID = dwUserID;
	m_dwIndex = 0;
	m_listInfo.clear();
	m_bCallback = false;
	AddTimer(TIMER_NORMAL, 60, this);
}

CGetUserRoom::~CGetUserRoom()
{
	DelTimer(TIMER_NORMAL, this);
}

void CGetUserRoom::SetCallbackID(DWORD dwCallbackID)
{
	m_setCallbackID.insert(dwCallbackID);

	if (m_bCallback) CallbackUserRoom(m_dwUserID, m_dwIndex, m_listInfo);
}

int CGetUserRoom::GetUserRoom()
{	
	CMainDBClient::Instance()->SetDataBaseAID(m_dwUserID);
	return CMainDBClient::Instance()->GetUserRoomInfo(m_dwUserID);
}

void CGetUserRoom::CallbackUserRoom(DWORD dwUserID, DWORD dwIndex, LIST_ROOMINFO& lstInfo)
{
	SET_DWORD::iterator iter = m_setCallbackID.begin();
	for (; iter != m_setCallbackID.end(); iter++)
	{
		IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(*iter);
		if (NULL == pSink) continue;
		pSink->OnGetUserRoomInfo(dwUserID, dwIndex, lstInfo);
	}
	m_setCallbackID.clear();

	if (m_bCallback == false)
	{
		AddTimer(TIMER_NORMAL, 3, this);
		m_dwIndex = dwIndex;
		m_listInfo.insert(m_listInfo.end(), lstInfo.begin(), lstInfo.end());
		m_bCallback = true;
	}
}
/*
void CGetUserRoom::CallbackIndoorBindDev(LIST_DEVSTATUS& lstDevStatus)
{
	LOG_DEBUG(LOG_MAIN,"[1021] CGetUserRoom::%s\n",__FUNCTION__);
	SET_DWORD::iterator iter = m_setCallbackID.begin();
	for (; iter != m_setCallbackID.end(); iter++)
	{
		IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(*iter);
		if (NULL == pSink) continue;
		LOG_DEBUG(LOG_MAIN,"[1022] CGetUserRoom::%s\n",__FUNCTION__);
		pSink->OnGetIndoorBindDev(lstDevStatus);
	}
	m_setCallbackID.clear();
}
*/
void CGetUserRoom::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	CGetUserRoomMgr::Instance()->DelElem(m_dwUserID);
}
