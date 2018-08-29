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

	// ���·�����Ϣ
	virtual void UpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, int nType, LIST_DWORD& lstRoomID) = 0;
	// ɾ���豸�����з���
	virtual void ClearRooms(MAP_DWORD& mapDeviceID) = 0;
	// �����豸�·�����Ϣ
	virtual void UpdateDevice(MAP_DWORD& mapDeviceID) = 0;
	virtual void UpdateDeviceEx(int nType, LIST_DWORD& lstDevID) = 0;
	// ���½�ֹ����
	virtual void UpdateDeviceDeadLine(LIST_DWORD& lstDevID) = 0;
	// ������I����
	virtual void UpdatePropertyAnnounce(DWORD dwVillage, DWORD dwNoticeIndex) = 0;
	// �̼�����
	virtual void UpdateFirmwareRequest(PCHAR strVersion, LIST_DWORD& lstDevID) = 0;
	// ɾ�������豸
	virtual void DeleteDeviceOnline(LIST_DWORD& lstDevID) = 0;
	virtual void UpdateAdvertInfo(DWORD dwVillageId, DWORD dwAdvertIndex) = 0;
	// ���ع�� sb
	virtual void DownLoadAdvertInfo(DWORD dwDeviceID, AdvertInfo_t& tInfo, PUCHAR pUrl) = 0;
};
IServerAppHandle* ServerApp_GetHandle();

#endif
