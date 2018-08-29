#include "Server.h"
#include "NetListenInterface.h"
#include "UtilityInterface.h"
#include "ServerAppInterface.h"
#include "DeviceInterface.h"
#include "ViewInterface.h"
#include "DispatchInterface.h"
#include "DataBaseInterface.h"
#include "Log.h"
#include "SvrCheck.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

IMPLEMENT_SINGLETON(CServer)
CServer::CServer()
{
	memset(&m_tCfgInfo, 0, sizeof(ServerCfg_t));
	m_dwWorkIP = 0;
	m_dwDBIP = 0;
	m_bOnce = false;
}

CServer::~CServer()
{
	Stop();
}

bool CServer::Run()
{
	NetworkInit();
	VigoLogInit((char*)LOG_DONG_D_FILE);
	UtilityInit();

	if (false == GetDServerCfg(m_tCfgInfo)) return false;
	if (false == GetSystemCfg(m_tSysCfgInfo)) return false;
	if (false == CheckExchange((char*)m_tCfgInfo.szExchange))
	{
		LOG_ERR(LOG_MAIN, "Wrong Exchange!!!\n");
		return false;
	}
	m_dwDBIP = IpStr2Dword((char*)m_tCfgInfo.szDBIP);
	// 1、启动数据库客户端
	if (false == DataBaseModuleInit(m_dwDBIP, SERVER_PORT_DB, GROUPCODE_DB_D,
		(PUCHAR)m_tCfgInfo.szSerialNO, (PUCHAR)m_tCfgInfo.szUserName, (PUCHAR)m_tCfgInfo.szPassword, 
		this))
	{
		return false;
	}
	// 2、启动数据库客户端
	if (false == StorageDBInit(m_dwDBIP, SERVER_PORT_SDB, GROUPCODE_DB_D,
		(PUCHAR)m_tCfgInfo.szSerialNO, (PUCHAR)m_tCfgInfo.szUserName, (PUCHAR)m_tCfgInfo.szPassword, 
		this))
	{
		return false;
	}
	// 3、启动数据库客户端sb
	if (false == StorageBusinessInit(m_dwDBIP, SERVER_PORT_SB, GROUPCODE_DB_D,
		(PUCHAR)m_tCfgInfo.szSerialNO, (PUCHAR)m_tCfgInfo.szUserName, (PUCHAR)m_tCfgInfo.szPassword, 
		this))
	{
		return false;
	}

	try {
		NetworkRunEventLoop();   
	} catch(...) {
		LOG_ERR(LOG_MAIN, "Exception in NetworkRunEventLoop!!!\n"); 
	}

	Stop();
	return true;
}


void CServer::Stop()
{
	ViewFinish();
	DeviceFinish();
	DispatchFinish();
	ServerAppFinish();
#ifdef _LGNCHECK_
	DDLgnFinish();
#endif
	NetListenFinish();
	DataBaseModuleFinish();
	UtilityFinish();
	NetworkFini();
	VigoLogFinish();
}

const int DATABASE_STATUS_CONNECTED = 0;
const int DATABASE_STATUS_AUTH = 1;
const int DATABASE_STATUS_DISCONNECT = 2;
int CServer::OnDBConStatus(int nResult, char cType)
{
	const char* pTag = "MainDB";
	if(2 == cType) pTag = "StorageDB";
	if(3 == cType) pTag = "StorageBusiness";
	if (nResult == DATABASE_STATUS_CONNECTED)
	{
		LOG_DEBUG(LOG_MAIN, "Connect %s Success!!!\n", pTag); return 0;
	}
	if (nResult == DATABASE_STATUS_DISCONNECT)
	{
		LOG_DEBUG(LOG_MAIN, "DisConnect %s\n", pTag); return -1;
	}
	LOG_DEBUG(LOG_MAIN, "%s Auth Success!!!\n", pTag);
	return 0;
}

int CServer::OnDserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo)
{
	// 2、启动侦听
	if (false == m_bOnce)
	{
		m_bOnce = true;
		m_dwWorkIP = IpStr2Dword((char*)m_tCfgInfo.szWorkIP);
		if (false == NetListenInit(m_dwWorkIP, SERVER_PORT_D)) return -1;
		if (false == ServerAppInit((PUCHAR)m_tCfgInfo.szSerialNO, (PUCHAR)m_tCfgInfo.szUserName, (PUCHAR)m_tCfgInfo.szPassword)) return -1;
		if (false == ViewInit()) return -1;
		if (false == DeviceInit()) return -1;
		if (false == DispatchInit()) return -1;
#ifdef _LGNCHECK_
		if (false == DDLgnInit((PUCHAR)m_tCfgInfo.szSerialNO)) return -1;
		DDLogin_SetSink(this);
#endif
	}

	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->SA_DserverConfigureIndex(dwVendorID, dwConfigureIndex, listInfo);
	return 0;

}

int CServer::OnUserConfigureIndex(DWORD dwVendorID, DWORD dwUserID)
{
	// 通知到view模块
	IViewHandle* pHandle = View_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->View_UserConfigureIndex(dwVendorID, dwUserID, 0);
	return 0;
}

int CServer::OnDeviceConfigureIndex(DWORD dwVendorID, DWORD dwDeviceID, DWORD dwRoomID, BYTE bType)
{
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	LIST_DWORD lstRoomID; lstRoomID.push_back(dwRoomID);
	pHandle->Dev_UpdateDeviceRoomInfo(dwVendorID, dwDeviceID, bType, lstRoomID);
	return 0;
}
//06
int CServer::DeviceDeadLine(DeviceDeadLineInfo_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN,"106:%s\n",__FUNCTION__);
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_SendDeviceDeadLine(tInfo);
	return 0;
}

//55.更新物業公告
int CServer::DevUpdatePropertyAnnounce(DWORD dwNoticeIndex, LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_MAIN,"CServer::%s NoticeIndex = %d\n",__FUNCTION__, dwNoticeIndex);
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_SendPropertyAnnounce(dwNoticeIndex, lstDevID);
	return 0;
}

int CServer::DevUpdateAdvertInfo(LIST_ADVERT& lstDevAdvert) 
{
	LOG_DEBUG(LOG_MAIN,"CServer::%s\n",__FUNCTION__);
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_SendAdvertInfo(lstDevAdvert);
	return 0;
}

int CServer::DevUpdateFirmwareRequest(DevUpgrad_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN,"%s\n",__FUNCTION__);
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_SendFirmwareRequest(tInfo);
	return 0;
}

int CServer::DevDeleteDeviceOnline(LIST_DWORD& lstDevID)
{
	LOG_DEBUG(LOG_MAIN,"%s\n",__FUNCTION__);
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_SendDeleteDeviceOnline(lstDevID);
	return 0;
}
//
int CServer::Dev_GetDeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& listDeviceRoomOther)
{
	LOG_DEBUG(LOG_MAIN,"%s\n",__FUNCTION__);
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_SendDeviceRoomOther(dwDeviceID, listDeviceRoomOther);
	return 0;
}

int CServer::OnUpdateDeviceRoomInfo( DWORD dwVendorID, DWORD dwDeviceID, BYTE bType, LIST_DWORD& lstRoomID )
{
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_UpdateDeviceRoomInfo(dwVendorID, dwDeviceID, bType, lstRoomID);
	return 0;
}

int CServer::OnClearRooms( LIST_DWORD& lstDeviceID )
{
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_ClearRooms(lstDeviceID);
	return 0;
}

int CServer::OnUpdateDevice( LIST_DWORD& lstDeviceID )
{
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_UpdateDevice(lstDeviceID);
	return 0;
}

int CServer::OnUpdateDeviceEx(int nType, LIST_DWORD& lstDeviceID)
{
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_UpdateDeviceEx(nType, lstDeviceID);
	return 0;
}
//3.1.20 通知室内机其绑定设备的在线离线状态
int CServer::OnGetDevBindIndoorID(DevStatus& tDevStat,LIST_DWORD& lstIndoorID)
{
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_GetDevBindIndoorID(tDevStat, lstIndoorID);
	return 0;
}
int CServer::DevSmsSpecialCrowd(SmsInfo2_t& tInfo)
{
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_SmsSpecialCrowd(tInfo);
	return 0;
}

int CServer::Dev_VideoAdvertUrl(DWORD dwDeviceID, AdvertInfoRep_t& tAdvertRep)
{
	IDeviceHandle* pHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->Dev_VideoAdvertUrl(dwDeviceID, tAdvertRep);
	return 0;
}

int CServer::OnGetServerInfo( LIST_SERVERINFO& listInfo )
{
	// 4、连接相关服务器
	RemoveSameServer(listInfo);
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	for (; iter != listInfo.end(); iter++)
	{
		IServerAppHandle* pHandle = ServerApp_GetHandle();
		LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
		pHandle->SA_AddServer(*iter, NULL);
	}
	return 0;
}

int CServer::OnDataBaseError( int nResult )
{
	LOG_DEBUG(LOG_MAIN, "DataBase Error %d\n", nResult); return 0;
}

int CServer::OnLoginOtherPlace( int nReason, BYTE bOpr, ClientTokenArray_t& tInfo )
{
	// 通知到view模块
	IViewHandle* pHandle = View_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->View_LoginOtherPlace(nReason, bOpr, tInfo);
	return 0;
}

#ifdef _LGNCHECK_
int CServer::OnGetDServerInfo( DServerInfo_t& tInfo )
{
//	LOG_DEBUG(LOG_MAIN, "OnGetDServerInfo sn:%s,premission:%d,capacity:%d\n", tInfo.szSerialNO, tInfo.nPermission, tInfo.nCapacity);
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pDevHandle, -1);
	IViewHandle* pViewHandle = View_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pViewHandle, -1);
	pDevHandle->Dev_Capacity(tInfo.nCapacity);
	pViewHandle->View_Permission(tInfo.nPermission);
	return 0;
}
#endif
