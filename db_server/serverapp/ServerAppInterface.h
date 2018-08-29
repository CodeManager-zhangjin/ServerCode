#ifndef _SERVERAPP_INTERFACE_H_
#define _SERVERAPP_INTERFACE_H_

#include "DataStruct.h"

bool ServerAppInit(WORD wRawUdpPort);
void ServerAppFinish();

class IServerAppHandle
{
public:
	virtual ~IServerAppHandle(){}
	virtual void PieceOfSerialNO() = 0;
	virtual void DserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex) = 0;
	virtual void UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID) = 0;
	virtual void DeviceConfigureIndex(DWORD dwVendorID, DWORD dwDeviceID, DWORD dwRoomID, BYTE bType) = 0;

	// 更新房间信息
	virtual void UpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, int nType, LIST_DWORD& lstRoomID) = 0;
	// 删除设备下所有房号
	virtual void ClearRooms(MAP_DWORD& mapDeviceID) = 0;
	// 更新设备下房间信息
	virtual void UpdateDevice(MAP_DWORD& mapDeviceID) = 0;
	virtual void UpdateDeviceEx(int nType, LIST_DWORD& lstDevID) = 0;
	// 更新截止日期
	virtual void UpdateDeviceDeadLine(LIST_DWORD& lstDevID) = 0;
	// 更新物業公告
	virtual void UpdatePropertyAnnounce(DWORD dwVillage, DWORD dwNoticeIndex) = 0;
	// 固件升级
	virtual void UpdateFirmwareRequest(PCHAR strVersion, LIST_DWORD& lstDevID) = 0;
	// 删除在线设备
	virtual void DeleteDeviceOnline(LIST_DWORD& lstDevID) = 0;
	virtual void UpdateAdvertInfo(DWORD dwVillageId, DWORD dwAdvertIndex) = 0;
	// 下载广告 sb
	virtual void DownLoadAdvertInfo(DWORD dwDeviceID, AdvertInfo_t& tInfo, PUCHAR pUrl) = 0;
};
IServerAppHandle* ServerApp_GetHandle();

#endif
