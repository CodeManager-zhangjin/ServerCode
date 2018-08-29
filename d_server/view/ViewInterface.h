#ifndef _VIEW_INTERFACE_H_
#define _VIEW_INTERFACE_H_

#include "DataStruct.h"

bool ViewInit();
void ViewFinish();

class IViewHandle
{
public:
	virtual ~IViewHandle(){}
	// 主动上报最新列表
	virtual void View_UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID, DWORD dwConfigureIndex) = 0;
	// 主动上报设备状态
	virtual void View_ReportDeviceStatus(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2) = 0;
	// 设备状态回应
	virtual void View_GetDeviceStatusRep(GetDeviceStatusRep_t& tInfo) = 0;
	// 观看请求回应
	virtual void View_TransServerInfo(TransServerInfo_t& tInfo) = 0;
	// 获取客户端列表
	virtual void View_GetUserList(LIST_USERSTATUS& listInfo) = 0;

	// 提示其他Token用户账号在其他位置登录
	virtual void View_LoginOtherPlace(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo) = 0;

	virtual void View_Permission(int nPermission) = 0;

	virtual void View_SdkTunnelRep(SdkTunnel_t& tInfo) = 0;

	// 上传凭证
	virtual void View_Qiniu_UploadToken(StorageTag_t& tTag, PUCHAR pUploadToken) = 0;
	// 下载外链
	virtual void View_Qiniu_DownloadUrls(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo) = 0;

	//室内机
	virtual void View_ReportIndoorStatus(DWORD dwUserID, ReportDevStatus& devStatus) = 0;

};
IViewHandle* View_GetHandle();

class IViewHandleSink
{
public:
	virtual ~IViewHandleSink(){}
	// 收到客户端观看请求
	virtual void View_OnTransClientInfo(TransClientInfo_t& tInfo, bool bRelay,TransServerInfo_t& tServerInfo) = 0;
	// 收到客户端获取设备状态请求
	virtual void View_OnGetDeviceStatus(GetDeviceStatus_t& tInfo) = 0;
	// 上报状态
	virtual void View_OnReportUserStatus(UserStatus_t& tInfo) = 0;

	// 设备模块保存报警状态
	virtual void View_CacheDeviceStatus(DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg) = 0;
	// 收到客户端SDK透传请求
	virtual void View_OnSdkTunnel(SdkTunnel_t& tInfo) = 0;

	// 云存储
	virtual void View_OnQiniu_GetUploadToken(StorageTag_t& tTag) = 0;
	virtual void View_OnQiniu_GetDownloadUrls(StorageTag_t& tTag, StoreKey_t& tKey) = 0;
	virtual void View_OnQiniu_GetDownloadUrls2(StorageTag_t& tTag, StoreVisitor_t& tVisitor) = 0;
};
void View_SetSink(IViewHandleSink* pSink);

#endif
