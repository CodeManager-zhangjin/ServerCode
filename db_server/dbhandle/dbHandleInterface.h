#ifndef _DBHANDLE_INTERFACE_H_
#define _DBHANDLE_INTERFACE_H_

#include "DataStruct.h"

#define DATABASE_HANDLE_ERROR_NOT_EXIST		1
#define DATABASE_HANDLE_ERROR_WRONG_PWD		2
#define DATABASE_HANDLE_ERROR_ADD_AGAIN		3 // �ظ����
#define DATABASE_HANDLE_ERROR_SYSTEM		4
#define DATABASE_HANDLE_EXCEPTION			5 // �쳣
#define DATABASE_HANDLE_LOGIN_OTHER_PLACE	6 // �����ط���¼

bool DBHandleInit(PUCHAR pHost, PUCHAR pSrc, PUCHAR pUserName, PUCHAR pPassword);
bool DBHandleInit_storage(PUCHAR pHost, PUCHAR pSrc, PUCHAR pUserName, PUCHAR pPassword);
bool DBHandleInit_operation(PUCHAR pHost, PUCHAR pSrc, PUCHAR pUserName, PUCHAR pPassword);

void DBHandleFinish();

void DBHandle_Timer();

int DBHandle_GetError();
int DBHandle_storage_GetError();
int DBHandleInit_operation_GetError();

class IDBHandle
{
public:
	virtual ~IDBHandle(){}

	////////////////////////////���������///////////////////////////////////
	// ��ѯ��������Ϣ
	// ���pServerType			����������
	// ���dwVendorID			����ID
	// ���listInfo				��������Ϣ
	virtual bool Query_ServerInfo	(PUCHAR pServerType, DWORD dwVendorID, LIST_SERVERINFO& listInfo) = 0;
	// ��ѯע�������Index
	virtual bool Query_DserverConfigureIndex(DWORD dwVendorID, MAP_DWORD& mapInfo) = 0;
	// ��ѯ���кŶ�
	virtual bool Query_PieceOfSerialNO(LIST_PIECEOFSERIALNO& listInfo) = 0;

	virtual bool Query_ServerInfo	(ServerInfo_t& tInfo) = 0;

	virtual bool Query_DeviceDeadLine(LIST_DWORD& lstDevID,LIST_DEVICE_DEADLINE& devDeadLine) = 0;
	////////////////////////////ע���˺����///////////////////////////////////
	// ��ѯ�û��Ƿ����
	// ���pUser				�û���
	virtual bool Query_User			(PUCHAR pUser) = 0;
	// �����û�
	virtual bool Insert_User		(DWORD dwVendorID, BYTE bLanguage, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pUser) = 0;

	////////////////////////////�˺����///////////////////////////////////
	// ��ѯ�û���Ϣ
	virtual bool Query_UserInfo		(PUCHAR pUserName, DWORD dwAppVendorID, UserInfo_t& tInfo, ClientTokenArray_t& tArray) = 0;
	// ��ѯ�˺����豸��Ϣ
	virtual bool Query_UserDevice	(DWORD dwUserID, LIST_DEVICEINFO& listInfo) = 0;
	// ��ѯ�˺�������Ϣ:
	virtual bool Query_UserGroup	(DWORD dwUserID, LIST_GROUPINFO& listInfo) = 0;
	// ��ѯ�˺����豸�ķ�����Ϣ
	virtual bool Query_UserRoom		(DWORD dwUserID, LIST_ROOMINFO& listInfo) = 0;
	// ��ѯ�û��������豸�ķ�����Ϣ
	virtual bool Query_UserDeviceRoomInfo(DWORD dwUserID, LIST_DEVICEINFO& lstDevInfo, MAP_DEVROOMINFO& mapDevRoomInfo) = 0;
	////////////////////////////�豸���///////////////////////////////////
	// ��ѯ�豸��Ϣ
	virtual bool Query_DeviceInfo	(PUCHAR pSN, DeviceInfo_t& tInfo) = 0;
	// ��ѯ�豸�����˺���Ϣ
	virtual bool Query_DeviceUser	(DWORD dwDeviceID, DWORD& dwConfigureIndex, LIST_SMSINFO& listInfo) = 0;
	// ��ѯ�豸����������Ϣ
	virtual bool Query_DevicePush	(DWORD dwDeviceID, DWORD& dwConfigureIndex2, LIST_PUSHINFO& listInfo) = 0;

	////////////////////////////�˺����豸����///////////////////////////////////
	// �˺�����豸
	virtual bool Insert_UserDevice	(DWORD dwUserID, PUCHAR pSerialNO, PUCHAR pDevName, PUCHAR pUserName, PUCHAR pRoom) = 0;
	// ��Ȩ
	virtual bool Insert_UserDevice	(DWORD dwOwnerID, PUCHAR pUser, DWORD dwDeviceID, DWORD& dwUserID) = 0;
	// ��Ȩ������SDK��
	virtual bool Insert_UserDevice	(DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDevName, PUCHAR pRoom) = 0;
	// ȡ����Ȩ / ɾ��
	virtual bool Delete_DeviceUser	(DWORD dwUserID, DWORD dwDeviceID) = 0;
	// �����˺����豸���豸����
	virtual bool Update_DeviceName	(DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName) = 0;

	////////////////////////////�������///////////////////////////////////
	// ����/ɾ��������Ϣ
	virtual bool Insert_PushInfo	(ClientTokenArray_t& tInfo, bool bTry) = 0;
	virtual bool Delete_PushInfo	(ClientTokenArray_t& tInfo) = 0;

	////////////////////////////�豸�������///////////////////////////////////
	// ��ѯ��Ԫ�ſڻ�����ժҪ/��Ϣ
	virtual bool Query_DeviceRoomSum	(DWORD dwDeviceID, LIST_ROOMSUM& listInfo) = 0;
	virtual bool Query_DeviceRoomPush	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMPUSH& listInfo) = 0;
	virtual bool Query_DeviceRoomUser	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMUSER& listInfo) = 0;
	virtual bool Query_DeviceRoomCard	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMCARD& listInfo) = 0;
	virtual bool Query_DeviceRoomOther	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMOTHER& listInfo) = 0;
	virtual	bool Query_DeviceRoomIndoor(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMINDOOR2& listInfo) = 0;
	virtual bool Query_DeviceRoomPushSwitch(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMPUSHSWITCH& listInfo) = 0;
//	virtual bool GetDeviceRoomIndoor	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, RoomIndoor_t& tRoomIndoor) = 0;


	virtual bool Query_DServerInfo(DServerInfo_t& tInfo) = 0;

	// ��ѯ�˺��豸�ķ���
	virtual bool Query_UserDeviceRoom(DWORD dwUserID, DWORD dwDeviceID, LIST_DWORD& lstRoomID) = 0;

	// ��ѯ�ƴ洢��������
	virtual bool Query_StoreLimit(DWORD dwDeviceID, int& nStoreLimit) = 0;

	virtual bool Query_UpdateDeviceCfg() = 0;
	virtual bool Query_DeviceCfg(DWORD dwDeviceID, int nType, UcpaasInfo_t& tUcpaas) = 0;
	virtual bool Query_VendorPhone(DWORD dwID, int nType, SystemCfg_t& tPhone) = 0;
	virtual bool Query_UpdatePushSwitch(DWORD dwUserID, int nSwitch) = 0;
	//������ҵ����
	virtual bool Query_GroupDevice(DWORD dwVillageId, LIST_DWORD& lstDevID) = 0;
	virtual bool Query_NoticeIndex(DWORD dwVillageId, DWORD& dwNoticeIndex) = 0;	
	virtual bool Query_AdvertIndex(DWORD dwVillageId, DWORD& dwAdvertIndex) = 0;
	virtual bool Query_VisitorCfg(DWORD dwGroupId, DWORD dwVisitorCfg) = 0;
	//���ڻ�
	virtual bool Insert_IndoorInfo(PUCHAR pIndoorSN , LIST_BIND_INFO& listBindInfo, BindInfoRep_t& tBindRep) = 0;
	virtual bool UpdateIndoorIndex(LIST_BIND_INFO& listBindInfo) = 0;
	virtual bool QueryIndoorBindDev(DWORD dwIndoorID, LIST_DEVICEINFO& lstDevInfo) = 0;
	virtual bool Query_DeviceIndoorID(DWORD dwDeviceID, LIST_DWORD& lstIndoorID) = 0;
	virtual bool Query_InDoorIDBySN(PUCHAR pIndoorSN, DWORD dwIndoorID) = 0;
	virtual bool QueryDevInfo(LIST_DEVICEINFO& lstDevInfo) = 0;

	virtual 	bool Query_SpecialUsers(LIST_SPECIALCROWD& m_lstSpecialCrowd) = 0;
};
IDBHandle* DBHandle_GetDBHandle();

class IDBHandleSink
{
public:
	virtual ~IDBHandleSink(){}
	virtual void OnUserConfigureIndex(DWORD dwVendorID, DWORD dwUserID) = 0;

	virtual void OnUpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, LIST_DWORD& lstRoomID, BYTE bType) = 0;
	virtual void OnClearRooms(DWORD dwVendorID, DWORD dwDeviceID) = 0;
	virtual void OnUpdateDevice(DWORD dwVendorID, DWORD dwDeviceID) = 0;
};
void DBHandle_SetSink(IDBHandleSink* pSink);


class ISDBHandle
{
public:
	virtual ~ISDBHandle(){}
	
	virtual bool Query_StorageInfo(StorageAccount_t& tAccount) = 0;
	virtual bool Query_StorageKeys(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_STORE_ACCOUNTKEYS& lstAccountKeys) = 0;
	virtual bool Query_StorageKeys(DWORD dwStoreID, StorageAccount_t& tAccount) = 0;
	//����״̬
	virtual bool StorageAlarmRecordList(LIST_ALARMSTATUS& listInfo) = 0;
	//���ż�¼
	virtual bool StorageUnlockList(LIST_DEVICE_UNLOCK& listInfo) = 0;
	virtual bool Delete_OpenDoorRecords() = 0;
	virtual bool Query_QiniuInfo(LIST_DWORD& lstStoreID) = 0;
	//�ÿ���Ӱ
	virtual bool StoregeVisitorList(LIST_STOREKEY& listInfo) = 0;//�����ÿ���Ӱ����
	virtual bool Insert_StoreKey(LIST_STOREKEY& listInfo) = 0;//�����ÿ���Ӱ����
//	virtual bool Insert_StoreKey(int nStoreLimit, StoreKey_t& tKey) = 0;
	virtual bool Query_Visitor(StoreVisitor_t& tVisitor, LIST_DWORD& lstRoomID,LIST_STORE_ACCOUNTKEYS& lstAccountKeys) = 0;

	virtual bool Query_SpecialCrowdInfo(LIST_SPECIALCROWD& lstSpecialCrowd,  LIST_SMSINFO2& lstSmsInfo) = 0;

};
ISDBHandle* DBHandle_GetSDBHandle();

class IOperationDBHandle
{
public:
	virtual ~IOperationDBHandle(){}

	virtual bool GetQiniuDownloadUrl(LIST_ADVERTINFO& lstAdvertInfo, StorageAccount_t& tAccount, LIST_ADVERTINFO_REP& lstAdvertInfoRep) = 0;
	virtual bool AdvertInfoMgr(DWORD& dwID, LIST_DWORD& lstAdvertID, LIST_ADVERTINFO& lstAdvertInfo, LIST_DWORD& lstVideoAdvertID) = 0;
	virtual bool Query_Advert(DWORD dwAdID, AdvertInfo_t& tInfo) = 0;
	virtual bool Query_StorageKeys(DWORD dwStoreID, StorageAccount_t& tAccount) = 0;

	virtual bool AddTaskList(DWORD dwDeviceID, LIST_DWORD& lstAdID) = 0;
	virtual bool Qiniu_GetAdvertUrl(AdvertInfo_t& tInfo, StorageAccount_t&tAccount, CSTRING& AdUrl) = 0;
	virtual bool AddLocalProgress(DWORD dwDeviceID, LIST_PROGRESS& lstProgress) = 0;
	virtual bool GetQiniuList(LIST_DWORD& lstAdID) = 0;
	virtual bool ClearQiniuList(DWORD& dwAdID) = 0;
};
IOperationDBHandle* DBHandle_GetODBHandle();

class IOperationDBHandleSink
{
public:	
	virtual ~IOperationDBHandleSink(){}
	virtual void DownLoadAdvertInfo(DWORD dwDeviceID, AdvertInfo_t& tInfo, PUCHAR pUrl) = 0;
};
void SBHandle_SetSink(IOperationDBHandleSink* pSink);

#endif
