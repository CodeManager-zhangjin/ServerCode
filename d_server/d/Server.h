#pragma once

#include "DataBaseInterface.h"
#include "singleton.h"
#ifdef _LGNCHECK_
#include "ddlgnInterface.h"
#endif

// 1、View收到观看和获取状态消息时需要依赖Camera和ServerApp
// 2、Camera收到告警消息时需要依赖View和ServerApp
// 3、ServerApp收到观看和告警消息时需要依赖View和Camera
class CServer : public IDataBaseStatusSink, public IStorageDBStatusSink, public IStorageBusinessStatusSink, public IDataBaseSink
#ifdef _LGNCHECK_
	, public IDDLoginSink
#endif
{
	DECLARE_SINGLETON(CServer)
public:
	CServer();
	~CServer();

	bool Run();
	// IStorageBusinessStatusSink
	int OnStorageBusinessStatus(int nResult) {return OnDBConStatus(nResult, 3); }
	// IStorageDBStatusSink
	int OnStorageDBStatus(int nResult){ return OnDBConStatus(nResult, 2); }
	// IDataBaseStatusSink
	int OnDataBaseStatus(int nResult) { return OnDBConStatus(nResult, 1); }
	int OnDserverConfigureIndex	(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo);
	int OnUserConfigureIndex	(DWORD dwVendorID, DWORD dwUserID);
	int OnDeviceConfigureIndex	(DWORD dwVendorID, DWORD dwDeviceID, DWORD dwRoomID, BYTE bType);
	int DeviceDeadLine			(DeviceDeadLineInfo_t& tInfo);

	int OnUpdateDeviceRoomInfo	(DWORD dwVendorID, DWORD dwDeviceID, BYTE bType, LIST_DWORD& lstRoomID);
	int OnClearRooms			(LIST_DWORD& lstDeviceID);
	int OnUpdateDevice			(LIST_DWORD& lstDeviceID);
	int OnUpdateDeviceEx		(int nType, LIST_DWORD& lstDeviceID);

	int OnGetDevBindIndoorID(DevStatus& tDevStat,LIST_DWORD& lstIndoorID);
	int DevSmsSpecialCrowd(SmsInfo2_t& tInfo);

	// 提示其他Token用户账号在其他位置登录
	int OnLoginOtherPlace(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo);

	// CDataBaseSink
	int OnGetServerInfo(LIST_SERVERINFO& listInfo);
	int OnDataBaseError(int nResult);
	//更新物业公告
	int DevUpdatePropertyAnnounce(DWORD dwNoticeIndex, LIST_DWORD& lstDevID);
	int OnGetNoticeIndex(DWORD dwNoticeIndex);
	//更新公告
	int DevUpdateAdvertInfo(LIST_ADVERT& lstDevAdvert);

	//设备升级
	int DevUpdateFirmwareRequest(DevUpgrad_t& tInfo);
	//删除在线设备
	int DevDeleteDeviceOnline(LIST_DWORD& lstDevID);

	//同步房号
	int Dev_GetDeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& listDeviceRoomOther);

	// CStoragebusiness
	int Dev_VideoAdvertUrl(DWORD dwDeviceID, AdvertInfoRep_t& tAdvertRep);

#ifdef _LGNCHECK_
	// IDDLoginSink
	int OnGetDServerInfo(DServerInfo_t& tInfo);
#endif

private:
	void Stop();
	int  OnDBConStatus(int nResult, char cType);

private:
	ServerCfg_t m_tCfgInfo;
	SystemCfg_t m_tSysCfgInfo;
	DWORD m_dwWorkIP, m_dwDBIP;

	bool m_bOnce;
};
