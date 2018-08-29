#include "CGetUserDeviceMgr.h"
#include "MainDBClient.h"
#include "DataBaseA.h"

IMPLEMENT_SINGLETON(CGetUserDeviceMgr)
int CGetUserDeviceMgr::GetUserDevice(DWORD dwCallbackID, DWORD dwUserID)
{
	CGetUserDevice* p = GetElem(dwUserID);
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
			p = new CGetUserDevice(dwUserID);
		}
		catch(std::bad_alloc &memExp)
		{
			printf("new CGetUserDevice failed\n"); return -1;
		}
		AddElem(dwUserID, p);
		p->SetCallbackID(dwCallbackID);
		p->GetUserDevice();
	}
	return 0;
}

CGetUserDevice::CGetUserDevice(DWORD dwUserID)
{
	m_dwUserID = dwUserID;
	m_dwIndex = 0;
	m_listUserDeviceInfo.clear();
	m_bCallback = false;
	AddTimer(TIMER_NORMAL, 60, this);
}

CGetUserDevice::~CGetUserDevice()
{
	DelTimer(TIMER_NORMAL, this);
}

void CGetUserDevice::SetCallbackID(DWORD dwCallbackID)
{
	m_setCallbackID.insert(dwCallbackID);
	MAP_DEVROOMINFO mapDevRoomInfo;
	if (m_bCallback) CallbackUserDevice(m_dwUserID, m_dwIndex, m_listUserDeviceInfo, mapDevRoomInfo);
}

int CGetUserDevice::GetUserDevice()
{	
	CMainDBClient::Instance()->SetDataBaseAID(m_dwUserID);
	return CMainDBClient::Instance()->GetUserDeviceInfo(m_dwUserID);
}

void CGetUserDevice::CallbackUserDevice(DWORD dwUserID, DWORD dwIndex, LIST_DEVICEINFO& lstUserDeviceInfo, MAP_DEVROOMINFO& mapDevRoomInfo)
{
	LOG_DEBUG(LOG_MAIN,"CGetUserDevice::%s\n",__FUNCTION__);
	SET_DWORD::iterator iter = m_setCallbackID.begin();
	for (; iter != m_setCallbackID.end(); iter++)
	{
		IDataBaseSink* pSink =  CDataBaseAMgr::Instance()->GetDataBaseSink(*iter);
		if (NULL == pSink) continue;
		LOG_DEBUG(LOG_DB_SERVER,"===>mapCount %d\n",mapDevRoomInfo.size());
		LOG_DEBUG(LOG_MAIN,"CallbackUserDevice UserID %d Index %d list %d map %d\n", dwUserID, dwIndex, lstUserDeviceInfo.size(), mapDevRoomInfo.size());
		pSink->OnGetUserDeviceInfo(dwUserID, dwIndex, lstUserDeviceInfo, mapDevRoomInfo);
	}
	m_setCallbackID.clear();

	if (m_bCallback == false)
	{
		AddTimer(TIMER_NORMAL, 3, this);
		m_dwIndex = dwIndex;
		m_listUserDeviceInfo.insert(m_listUserDeviceInfo.end(), lstUserDeviceInfo.begin(), lstUserDeviceInfo.end());
		m_bCallback = true;
	}
}

void CGetUserDevice::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	CGetUserDeviceMgr::Instance()->DelElem(m_dwUserID);
}
