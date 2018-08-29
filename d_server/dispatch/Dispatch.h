#pragma once

#include "DeviceInterface.h"
#include "ViewInterface.h"
#include "ServerAppInterface.h"
#include "singleton.h"

class CDispatch : public IDeviceHandleSink, public IViewHandleSink, public IServerAppHandleSink
{
	DECLARE_SINGLETON(CDispatch)
public:
	CDispatch();
	~CDispatch();

	bool Start();

	// IDeviceHandleSink
	// 设备上报状态
	void Dev_OnReportDeviceStatus(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, LIST_SMSINFO& list1, LIST_PUSHINFO& list2, ReportDevStatus& devStatus);
	void Dev_OnSmsSpecialCrowd(SmsInfo2_t& tInfo);
	// 透传消息回应
	void Dev_OnSdkTunnel(SdkTunnel_t& tInfo);
	// 云存储
	void Dev_ReportAlarmStatus(AlarmStatus_t& alarmStatus);
	void Dev_OnQiniu_GetUploadToken(StorageTag_t& tTag);
	void Dev_OnQiniu_GetDownloadUrls(StorageTag_t& tTag, StoreKey_t& tKey);
	void Dev_OnQiniu_ReportUploadResult(StoreKey_t& tKey);
	//
	void Dev_OnGetIndoorBindDevStatus(LIST_DEVSTATUS& lstDevStatus);

	// IViewHandleSink
	// 收到客户端观看请求
	void View_OnTransClientInfo(TransClientInfo_t& tInfo, bool bRelay, TransServerInfo_t& tServerInfo);
	// 收到客户端获取设备状态请求
	void View_OnGetDeviceStatus(GetDeviceStatus_t& tInfo);
	// 上报状态
	void View_OnReportUserStatus(UserStatus_t& tInfo);
	// 设备模块保存报警状态
	void View_CacheDeviceStatus(DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg);
	// 收到客户端SDK透传请求
	void View_OnSdkTunnel(SdkTunnel_t& tInfo);
	// 云存储
	void View_OnQiniu_GetUploadToken(StorageTag_t& tTag);
	void View_OnQiniu_GetDownloadUrls(StorageTag_t& tTag, StoreKey_t& tKey);
	void View_OnQiniu_GetDownloadUrls2( StorageTag_t& tTag, StoreVisitor_t& tVisitor );
	// IServerAppHandleSink
	// 收到状态服务器上报设备状态
	void SA_OnReportDeviceStatus_Status(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2);
	// 收到状态服务器获取设备状态请求
	void SA_OnGetDeviceStatus_Status(DWORD dwServerID, GetDeviceStatus_t& tInfo);
	// 收到状态服务器获取设备状态回应
	void SA_OnGetDeviceStatusRep_Status(GetDeviceStatusRep_t& tInfo);
	// 收到状态服务器观看请求
	void SA_OnTransClientInfo_Status(TransClientInfo_t& tInfo, bool bRelay);
	// 收到状态服务器观看请求回应
	void SA_OnTransServerInfo_Status(TransServerInfo_t& tInfo);
	// 收到转发服务器回应
	void SA_OnAddTempUser_Relay(TransClientInfo_t& tInfo, TransServerInfo_t& tServerInfo);
	// 收到转发服务器观看请求
	void SA_OnTransClientInfo_Relay(TransClientInfo_t& tInfo, DWORD dwServerID);
	// 与状态服务器建立连接
	void SA_Online_Status();
	// 收到通知服务器发送短信回应
	void SA_OnSmsRepsonse_Notify(DWORD dwClientID, int nReason){}
	// 收到注册服务器增加临时用户命令
	void SA_OnAddTempUser_D(TempUser_t& tInfo){}

	void SA_OnSdkTunnel_Status(SdkTunnel_t& tInfo);
	void SA_OnSdkTunnelRep_Status(SdkTunnel_t& tInfo);

	// 云存储
	void SA_OnQiniu_GetUploadToken_Storage(StorageTag_t& tTag, PUCHAR pUploadToken);
	void SA_OnQiniu_GetDownloadUrls_Storage(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo);
};
