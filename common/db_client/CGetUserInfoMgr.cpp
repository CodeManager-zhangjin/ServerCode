#include "CGetUserInfoMgr.h"
#include "MainDBClient.h"
#include "DataBaseA.h"
#include <algorithm>

DWORD g_dwGetUserInfo_ID = 0;
DWORD g_dwGetUserInfo_VendorID = 0;
CSTRING g_strGetUserInfo_UserName;


IMPLEMENT_SINGLETON(CGetUserInfoMgr)
CGetUserInfo::CGetUserInfo(DWORD dwID)
{
	m_dwID = dwID;
	memset(&m_tUserInfo, 0, sizeof(UserInfo_t));
	memset(&m_tArray, 0, sizeof(ClientTokenArray_t));
	m_bCallback = false;
	AddTimer(TIMER_NORMAL, 60, this);
}

CGetUserInfo::~CGetUserInfo()
{
	DelTimer(TIMER_NORMAL, this);
}

int CGetUserInfoMgr::GetUserInfo( DWORD dwCallbackID, PUCHAR pUserName, DWORD dwVendorID )
{
	g_dwGetUserInfo_VendorID = dwVendorID;
	g_strGetUserInfo_UserName.assign((const char*)pUserName);
	ELEM_MAP::iterator iter = std::find_if(m_mapElement.begin(), m_mapElement.end(), FindCGetUserInfo());
	if (iter != m_mapElement.end())
	{
//		LOG_DEBUG(LOG_DB_CLIENT, "1 %s CallbackID:%d UserName:%s VendorID:%d\n", __FUNCTION__, dwCallbackID, pUserName, dwVendorID);
		iter->second->SetCallbackID(dwCallbackID);
	}
	else
	{
//		LOG_DEBUG(LOG_DB_CLIENT, "2 %s CallbackID:%d UserName:%s VendorID:%d\n", __FUNCTION__, dwCallbackID, pUserName, dwVendorID);
		CGetUserInfo* p = NULL;
		try
		{
			p = new CGetUserInfo(++g_dwGetUserInfo_ID);
		}
		catch(std::bad_alloc &memExp)
		{
			printf("new CGetUserInfo failed\n"); return -1;
		}
		AddElem(g_dwGetUserInfo_ID, p);
		p->SetCallbackID(dwCallbackID);
		p->GetUserInfo(pUserName, dwVendorID);
	}
	return 0;
}

void CGetUserInfo::SetCallbackID(DWORD dwCallbackID)
{
	m_setCallbackID.insert(dwCallbackID);
	if (m_bCallback) CallbackUserInfo(m_tUserInfo, m_tArray);
}

int CGetUserInfo::GetUserInfo(PUCHAR pUserName, DWORD dwVendorID)
{
	m_dwVendorID = dwVendorID;
	int nCpyLen = strlen((const char*)pUserName) > LENGTH_NAME ? LENGTH_NAME : strlen((const char*)pUserName);
	memset(m_szUserName, 0, LENGTH_NAME+1);
	memcpy(m_szUserName, pUserName, nCpyLen);
	CMainDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CMainDBClient::Instance()->GetUserInfo(pUserName, dwVendorID);
}

void CGetUserInfo::CallbackUserInfo( UserInfo_t& tInfo, ClientTokenArray_t& tArray )
{
	SET_DWORD::iterator iter = m_setCallbackID.begin();
	for (; iter != m_setCallbackID.end(); iter++)
	{
		IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(*iter);
		LOG_DEBUG(LOG_DB_CLIENT, "%s CallbackID:%d UserName:%s VendorID:%d pSink %p\n", __FUNCTION__, *iter, m_szUserName, m_dwVendorID, pSink);
		if (NULL == pSink) continue;
		pSink->OnGetUserInfo(tInfo, tArray);
	}
	m_setCallbackID.clear();

	if (m_bCallback == false)
	{
		AddTimer(TIMER_NORMAL, 3, this);
		memcpy(&m_tUserInfo, &tInfo, sizeof(UserInfo_t));
		memcpy(&m_tArray, &tArray, sizeof(ClientTokenArray_t));
		m_bCallback = true;
	}
}

void CGetUserInfo::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	LOG_DEBUG(LOG_DB_CLIENT, "%s CallbackID:%d UserName:%s VendorID:%d\n", __FUNCTION__, m_dwID, m_szUserName, m_dwVendorID);
	CGetUserInfoMgr::Instance()->DelElem(m_dwID);
}
