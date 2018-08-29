#ifndef _SERVERAPP_INTERFACE_H_
#define _SERVERAPP_INTERFACE_H_

#include "DataStruct.h"

bool ServerAppInit(PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword);
void ServerAppFinish();

class IServerAppHandle
{
public:
	virtual ~IServerAppHandle(){}
	virtual bool SA_AddServer(ServerInfo_t& tInfo, INetConnection* pCon) = 0;
	virtual bool SA_GetDServers(DWORD& dwVendorID, DWORD& dwConfigureIndex, LIST_SERVERINFO& listInfo) = 0;
	virtual bool SA_DserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo) = 0;

	virtual bool SA_Sms_Notification(PUCHAR pMsg, LIST_SMSINFO& listSmsInfo, DWORD dwClientID) = 0;
	virtual bool SA_Push_Notification(DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, DWORD dwDeviceID, LIST_PUSHINFO& listToken) = 0;

	// 向状态服务器上报设备状态
	virtual bool SA_ReportDeviceStatus_Status(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2) = 0;
	// 向状态服务器上报客户端状态
	virtual bool SA_ReportUserStatus_Status(UserStatus_t& tInfo) = 0;
	// 回应状态服务器观看信息
	virtual bool SA_TransServerInfo_Status(TransServerInfo_t& tInfo) = 0;
	// 回应转发服务器观看信息
	virtual bool SA_TransServerInfo_Relay(TransServerInfo_t& tInfo, DWORD dwServerID) = 0;
	// 向状态服务器获取设备状态
	virtual bool SA_GetDeviceStatus_Status(GetDeviceStatus_t& tInfo) = 0;
	// 向状态服务器上报获取设备状态回应
	virtual bool SA_GetDeviceStatusRep_Status(DWORD dwServerID, GetDeviceStatusRep_t& tInfo) = 0;
	// 设备在当前注册服务器上注册，通过转发服务器观看
	virtual bool SA_AddTempUser_Relay(TransClientInfo_t& tInfo, bool bRelay, DWORD dwAutoRelay) = 0;
	// 设备未在当前注册服务器上注册，通过状态服务器透传消息
	virtual bool SA_TransClientInfo_Status(TransClientInfo_t& tInfo, bool bRelay) = 0;

	virtual bool SA_SdkTunnel_Status(SdkTunnel_t& tInfo) = 0;
	virtual bool SA_SdkTunnelRep_Status(SdkTunnel_t& tInfo) = 0;

	virtual bool SA_ReportAlarmStatus(AlarmStatus_t& alarmStatus) = 0;
	virtual bool SA_GetUploadToken_Storage(StorageTag_t& tTag) = 0;
	virtual bool SA_ReportUploadResult_Storage(StoreKey_t& tKey) = 0;
	virtual bool SA_GetDownloadUrls_Storage(StorageTag_t& tTag, StoreKey_t& tKey) = 0;
	virtual bool SA_GetDownloadUrls_Storage2(StorageTag_t& tTag, StoreVisitor_t& tVisitor) = 0;
};
IServerAppHandle* ServerApp_GetHandle();

class IServerAppHandleSink
{
public:
	virtual ~IServerAppHandleSink(){}
	// 收到状态服务器上报设备状态
	virtual void SA_OnReportDeviceStatus_Status(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2) {}
	// 收到状态服务器获取设备状态请求
	virtual void SA_OnGetDeviceStatus_Status(DWORD dwServerID, GetDeviceStatus_t& tInfo) {}
	// 收到状态服务器获取设备状态回应
	virtual void SA_OnGetDeviceStatusRep_Status(GetDeviceStatusRep_t& tInfo) {}
	// 收到状态服务器观看请求
	virtual void SA_OnTransClientInfo_Status(TransClientInfo_t& tInfo, bool bRelay) {}
	// 收到状态服务器观看请求回应
	virtual void SA_OnTransServerInfo_Status(TransServerInfo_t& tInfo) {}
	// 收到转发服务器回应
	virtual void SA_OnAddTempUser_Relay(TransClientInfo_t& tInfo, TransServerInfo_t& tServerInfo) {}
	// 收到转发服务器观看请求
	virtual void SA_OnTransClientInfo_Relay(TransClientInfo_t& tInfo, DWORD dwServerID) {}
	// 与状态服务器建立连接
	virtual void SA_Online_Status() {}
	// 收到通知服务器发送短信回应
	virtual void SA_OnSmsRepsonse_Notify(DWORD dwClientID, int nReason) {}
	// 收到注册服务器增加临时用户命令
	virtual void SA_OnAddTempUser_D(TempUser_t& tInfo) {}

	virtual void SA_OnSdkTunnel_Status(SdkTunnel_t& tInfo) {}
	virtual void SA_OnSdkTunnelRep_Status(SdkTunnel_t& tInfo) {}

	// 云存储
	virtual void SA_OnQiniu_GetUploadToken_Storage(StorageTag_t& tTag, PUCHAR pUploadToken) {}
	virtual void SA_OnQiniu_GetDownloadUrls_Storage(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo) {}
};
void ServerApp_SetSink(IServerAppHandleSink* pSink);


#endif
