#ifndef _DEVICE_INTERFACE_H_
#define _DEVICE_INTERFACE_H_

#include "DataStruct.h"

bool DeviceInit();
void DeviceFinish();

class IDeviceHandle
{
public:
	virtual ~IDeviceHandle(){}
	// 获取设备状态
	virtual void Dev_GetDeviceStatus(DWORD dwUserID, LIST_DWORD& listDeviceID, LIST_DEVICESTATUS& listInfo) = 0;
	// 获取设备连接信息
	virtual bool Dev_GetDeviceConnectInfo(TransClientInfo_t& tClientInfo, TransServerInfo_t& tServerInfo, bool bTransClientInfo, DWORD& dwAutoRelay) = 0;
	// 获取设备列表
	virtual void Dev_GetDeviceList(LIST_DEVICESTATUS& listInfo) = 0;
	// 向设备发送对端地址信息
	virtual void Dev_TransClientInfo(TransClientInfo_t& tClientInfo) = 0;
	virtual DWORD  Dev_GetDeviceType(DWORD dwDeviceID) = 0;
	virtual PUCHAR Dev_GetDeviceImgVer(DWORD dwDeviceID) = 0;

	virtual void Dev_UpdateDeviceRoomInfo(DWORD dwVendorID, DWORD dwDeviceID, BYTE bType, LIST_DWORD& lstRoomID) = 0;
	virtual void Dev_SendDeviceDeadLine(DeviceDeadLineInfo_t& tInfo) = 0;
	virtual void Dev_SendPropertyAnnounce(DWORD dwNoticeIndex, LIST_DWORD& lstDevID) = 0;
	virtual void Dev_SendAdvertInfo(LIST_ADVERT& lstDevAdvert) = 0;
	virtual void Dev_SendFirmwareRequest(DevUpgrad_t& tInfo) = 0;
	virtual void Dev_SendDeleteDeviceOnline(LIST_DWORD& lstDevID) = 0;
	virtual void Dev_ClearRooms(LIST_DWORD& lstDeviceID) = 0;
	virtual void Dev_UpdateDevice(LIST_DWORD& lstDeviceID) = 0;
	virtual void Dev_UpdateDeviceEx(int nType, LIST_DWORD& lstDeviceID) = 0;
	virtual void Dev_GetDevBindIndoorID(DevStatus& tDevStat,LIST_DWORD& lstIndoorID) = 0;
	virtual void Dev_CallIndoor(LIST_DWORD& lstIndoorID, DWORD dwStatus, DWORD dwDeviceID) = 0;
	virtual void Dev_SmsSpecialCrowd(SmsInfo2_t& tInfo) = 0;
	virtual void Dev_VideoAdvertUrl(DWORD dwDeviceID, AdvertInfoRep_t& tAdvertRep) = 0;

	//同步1
	virtual void Dev_SendDeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& listDeviceRoomOther) = 0;

	virtual void Dev_Capacity(int nCapacity) = 0;

	// 设备模块保存报警状态
	virtual void Dev_CacheDeviceStatus(DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg) = 0;
	// 透传消息到设备
	virtual bool Dev_SdkTunnel(SdkTunnel_t& tInfo) = 0;

	// 上传凭证
	virtual void Dev_Qiniu_UploadToken(StorageTag_t& tTag, PUCHAR pUploadToken) = 0;
	// 下载外链
	virtual void Dev_Qiniu_DownloadUrls(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo) = 0;

	virtual void Dev_GetDeviceStatus(LIST_DEVSTATUS& lstDevStatus) = 0;

};
IDeviceHandle* Device_GetHandle();

class IDeviceHandleSink
{
public:
	virtual ~IDeviceHandleSink(){}
	// 设备上报状态
	virtual void Dev_OnReportDeviceStatus(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, LIST_SMSINFO& list1, LIST_PUSHINFO& list2, ReportDevStatus& devStat) = 0;
	virtual void Dev_OnSmsSpecialCrowd(SmsInfo2_t& tInfo) = 0;

	// 透传消息回应
	virtual void Dev_OnSdkTunnel(SdkTunnel_t& tInfo) = 0;
	//
	virtual void Dev_OnGetIndoorBindDevStatus(LIST_DEVSTATUS& lstDevStatus)= 0;

	// 云存储
	virtual void Dev_OnQiniu_GetUploadToken(StorageTag_t& tTag) = 0;
	virtual void Dev_OnQiniu_GetDownloadUrls(StorageTag_t& tTag, StoreKey_t& tKey) = 0;
	virtual void Dev_OnQiniu_ReportUploadResult(StoreKey_t& tKey) = 0;
	virtual void Dev_ReportAlarmStatus(AlarmStatus_t& alarmStatus) = 0;
};
void Device_SetSink(IDeviceHandleSink* pSink);

void TEST_YSC_ONOFF_LINE(char cOnOff);

#endif
