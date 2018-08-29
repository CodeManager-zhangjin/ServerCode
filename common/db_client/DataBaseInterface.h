#ifndef _DATABASE_INTERFACE_H_
#define _DATABASE_INTERFACE_H_

#include "DataStruct.h"

// 以下接口处理的都是跟dd_db相关的功能
class IDataBase
{
public:
	virtual ~IDataBase(){}

	virtual int GetServerInfo		(PUCHAR pServerType) = 0;
	virtual int QueryMobilePhone	(PUCHAR pMobilePhone) = 0;
	virtual int SetSecret			(DWORD dwVendorID, BYTE bLanguage, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pMobilePhone) = 0;
	virtual int GetUserInfo			(PUCHAR pUserName, DWORD dwVendorID) = 0;
	virtual int GetUserDeviceInfo	(DWORD dwUserID) = 0;
	virtual int GetIndoorDevList	(PUCHAR pSN) = 0;
	virtual int GetUserGroupInfo	(DWORD dwUserID) = 0;
	virtual int GetUserRoomInfo		(DWORD dwUserID) = 0;
	virtual int GetDeviceInfo		(PUCHAR pSN) = 0;
	virtual int GetDeviceUserInfo	(DWORD dwDeviceID) = 0;
	virtual int GetDevicePushInfo	(DWORD dwDeviceID) = 0;
	virtual int AddDevice			(DWORD dwUserID, PUCHAR pSerialNO, PUCHAR pDevName, PUCHAR pRoom) = 0;
	virtual int AddDelPushInfoEx	(BYTE bOpr, ClientTokenArray_t& tInfo) = 0;
	virtual int SetDeviceName		(DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName) = 0;

	virtual int DelDevice			(DWORD dwUserID, DWORD dwDeviceID) = 0;
	virtual int Authorize			(DWORD dwOwnerID, PUCHAR pUser, DWORD dwDeviceID) = 0;
	virtual int Authorize2			(DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName, PUCHAR pRoom) = 0;

	virtual int GetDeviceRoomSum	(DWORD dwDeviceID) = 0;
	virtual int GetDeviceRoomInfo	(DWORD dwDeviceID, BYTE bType, LIST_DWORD& listInfo) = 0;

	virtual int GetDServerInfo		(PUCHAR pSN) = 0;
	virtual int GetNoticeIndex		(DWORD dwGroupID, DWORD dwDeviceID) = 0;//公告
	virtual int GetAdvertIndex		(DWORD dwGroupID, DWORD dwDeviceID) = 0;//广告


	virtual int GetVisitorCfg			(DWORD dwGroupID,DWORD dwDeviceID) = 0;//访客配置

	virtual int GetBindDevStatus	(DWORD dwIndoorID) = 0;//获取室内机绑定门口机在线离线状态
	virtual int GetBindIndoorID		(DWORD dwDeviceID) = 0;//获取室内机ID

	virtual int RepoartAlarmStatus(AlarmStatus_t& alarmStatus) = 0;
	// 获取云存储账号信息
	virtual int Qiniu_GetStorageAccount(StorageTag_t& tTag) = 0;
	// 获取云存储Key列表
	virtual int Qiniu_GetStorageKeys(StorageTag_t& tTag, StoreKey_t& tKey) = 0;
	virtual int Qiniu_GetStorageKeys2(StorageTag_t& tTag, StoreVisitor_t& tVisitor) = 0;
	// 上报上传结果
	virtual int Qiniu_ReportUploadResult(StoreKey_t& tKey) = 0;

	virtual int GetDeviceCfg		(DWORD dwDeviceID, int nType) = 0;
	virtual int SetPushSwitch		(DWORD dwUserID, int nSwitch) = 0;
	//室内机
	virtual int IndoorBindDevice(PUCHAR pIndoorSN, LIST_BIND_INFO& listBindInfo) = 0;

};
class IDataBaseSink
{
public:
	virtual ~IDataBaseSink(){}

	virtual int OnGetServerInfo		(LIST_SERVERINFO& listInfo) {return 0;}
	virtual int OnQueryMobilePhone	(PUCHAR pMobilePhone, int nReason) {return 0;}
	virtual int OnSetSecret			(int nReason) {return 0;}
	virtual int OnGetUserInfo		(UserInfo_t& tInfo, ClientTokenArray_t& tArray) {return 0;}
	virtual int OnGetUserDeviceInfo	(DWORD dwUserID, DWORD dwIndex, LIST_DEVICEINFO& listInfo, MAP_DEVROOMINFO& mapDevRoomInfo) {return 0;}
	virtual int OnGetUserGroupInfo	(DWORD dwUserID, DWORD dwIndex, LIST_GROUPINFO& listInfo) {return 0;}
	virtual int OnGetUserRoomInfo	(DWORD dwUserID, DWORD dwIndex, LIST_ROOMINFO& listInfo) {return 0;}
	virtual int OnGetIndoorBindDev	(LIST_DEVICEINFO& lstDevInfo){ return 0;}
	virtual int OnGetIndoorBindDevStatus(LIST_DEVSTATUS& lstDevStatus) { return 0;}
	virtual int OnGetDeviceInfo		(DeviceInfo_t& tInfo) {return 0;}
	virtual int OnGetDeviceUserInfo	(DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo) {return 0;}
	virtual int OnGetDevicePushInfo	(DWORD dwDeviceID, DWORD dwIndex, LIST_PUSHINFO& listInfo) {return 0;}
	virtual int OnAddDevice			(int nReason, PUCHAR pUser) {return 0;}
	virtual int OnAddDelPushInfoEx	(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo) {return 0;}
	virtual int OnSetDeviceName		(int nReason) {return 0;}
	virtual int OnDelDevice			(int nReason, DWORD dwUserID, DWORD dwDeviceID) {return 0;}
	virtual int OnAuthorize			(int nReason, DWORD dwUserID) {return 0;}
	virtual int OnAuthorize2		(int nReason) {return 0;}

	//
	virtual int OnGetBulletinIndex	(DWORD dwNoticeIndex) {return 0;}
	virtual int OnGetAdvertIndex	(DWORD dwAdvertIndex, DWORD dwAdvertType) {return 0;}
	virtual int OnGetVisitorCfg(DWORD dwVisitorCfg) {return 0;}

	virtual int OnGetDeviceRoomSum	(DWORD dwDeviceID, LIST_ROOMSUM& lstInfo, WORD totalsegment, WORD subsegment) {return 0;}
	virtual int OnGetDeviceRoomUser	(DWORD dwDeviceID, LIST_ROOMUSER& lstInfo, WORD totalsegment, WORD subsegment) {return 0;}
	virtual int OnGetDeviceRoomPush	(DWORD dwDeviceID, LIST_ROOMPUSH& lstInfo, WORD totalsegment, WORD subsegment) {return 0;}
	virtual int OnGetDeviceRoomCard	(DWORD dwDeviceID, LIST_ROOMCARD& lstInfo, WORD totalsegment, WORD subsegment) {return 0;}
	virtual int OnGetDeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& lstInfo, WORD totalsegment, WORD subsegment) {return 0;}
	virtual int OnGetDeviceRoomIndoor(DWORD dwDeviceID, LIST_ROOMINDOOR2& lstInfo, WORD totalsegment, WORD subsegment) {return 0;}
	
	virtual int OnGetDServerInfo	(DServerInfo_t& tInfo) { return 0; }
	virtual int OnQiniu_GetStorageAccount(StorageTag_t& tTag, StorageAccount_t& tInfo) { return 0; }
	virtual int OnQiniu_GetStorageKeys(StorageTag_t& tTag, LIST_STORE_ACCOUNTKEYS& lstAccountKeys) { return 0; }

	virtual int OnGetDeviceCfg(DWORD dwID, int nType, UcpaasInfo_t& tUcpaas) { return 0; }
	virtual int OnGetSystemCfg(DWORD dwID, int nType, SystemCfg_t& tCfgInfo) {return 0;}
	virtual int OnSetPushSwitch(DWORD dwUserID, int nSwitch) {return 0;}
	virtual int OnDataBaseError(int nResult) {return 0;}
	virtual int OnIndoorBindDevice_Rep(BindInfoRep_t& tBindInfo){return 0;}
	virtual int OnGetIndoorBindDevList(LIST_DEVSTATUS& lstDevStatus){return 0;}
};
IDataBase*	RegisterDataBase(IDataBaseSink* pSink);
void		UnRegisterDataBase(IDataBase* pDataBase);

class IDataBaseStatusSink
{
public:
	virtual int OnDataBaseStatus(int nResult) = 0; // 0-数据库连接成功, 1-认证成功 2-连接断开

	virtual int OnPieceOfSerialNO		(LIST_PIECEOFSERIALNO& listInfo) { return 0; }
	virtual int OnDserverConfigureIndex	(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo) { return 0; }
	virtual int OnUserConfigureIndex	(DWORD dwVendorID, DWORD dwUserID) { return 0; }
	virtual int OnDeviceConfigureIndex	(DWORD dwVendorID, DWORD dwDeviceID, DWORD dwRoomID, BYTE bType) { return 0; }
	virtual int DeviceDeadLine			(DeviceDeadLineInfo_t& listInfo) {return 0;}
	virtual int DevUpdatePropertyAnnounce(DWORD dwNoticeIndex, LIST_DWORD& lstDevID){return 0;};
	virtual int DevUpdateAdvertInfo(LIST_ADVERT& lstDevAdvert) {return 0;}
	virtual int DevUpdateFirmwareRequest(DevUpgrad_t& tInfo) {return 0;}
	virtual int DevDeleteDeviceOnline(LIST_DWORD& lstDevID)	{return 0;};
	virtual int OnUpdateDeviceRoomInfo	(DWORD dwVendorID, DWORD dwDeviceID, BYTE bType, LIST_DWORD& lstRoomID) { return 0; }
	virtual int OnClearRooms			(LIST_DWORD& lstDeviceID) { return 0; }
	virtual int OnUpdateDevice			(LIST_DWORD& lstDeviceID) { return 0; }
	virtual int OnUpdateDeviceEx		(int nType, LIST_DWORD& lstDeviceID) { return 0; }
	virtual int OnGetDevBindIndoorID(DevStatus& tDevStat, LIST_DWORD& lstIndoorID) {return 0;}
	

	// 提示其他Token用户账号在其他位置登录
	virtual int OnLoginOtherPlace(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo) {return 0;}
	//
	virtual int Dev_GetDeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& listDeviceRoomOther){return 0;}

	virtual ~IDataBaseStatusSink(){}
};

bool		DataBaseModuleInit(DWORD dwIP, WORD wPort,							// 数据库服务 地址
							   BYTE bGroupCode,									// 
							   PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword,	// 认证序列号 用户名 密码
							   IDataBaseStatusSink* pSink							//
							   );
void		DataBaseModuleFinish();

//////////////////////////////////////////////////////////////////////////
// 以下接口处理的都是跟storage_db相关的功能
class IStorageDB
{
public:
	virtual ~IStorageDB(){}
//	virtual int SDB_AddUnlockItem(LIST_DEVICE_UNLOCK& listInfo) = 0;
	virtual int SDB_UnlockItemTunel(PUCHAR pData, int nLen) = 0;
	virtual int SDB_ReportUploadResult(PUCHAR pData, int nLen) = 0;
	virtual int SDB_AlarmStatus(PUCHAR pData, int nLen) = 0;
};
class IStorageDBSink
{
public:
	virtual ~IStorageDBSink(){}
	virtual int OnAddUnlockItem_Rep(UnlockRep_t& rInfo) {return 0;}
};
IStorageDB*	RegStorageDB(IStorageDBSink* pSink);
void		UnRegStorageDB(IStorageDB* pDataBase);

class IStorageDBStatusSink
{
public:
	virtual ~IStorageDBStatusSink(){}
	virtual int OnStorageDBStatus(int nResult) = 0; // 0-数据库连接成功, 1-认证成功 2-连接断开
	virtual int DevSmsSpecialCrowd(SmsInfo2_t& tInfo) {return 0;}
};
bool		StorageDBInit(DWORD dwIP, WORD wPort, BYTE bGroupCode,  // 数据库服务 地址
						  PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword,	// 认证序列号 用户名 密码
						  IStorageDBStatusSink* pSink );

//////////////////////////////////////////////////////////////////////////
// 以下接口处理的都是跟storage_business相关的功能
class IStorageBusiness
{
public:
	virtual ~IStorageBusiness() {}
	virtual int GetAdvertInfo		(DWORD dwDeviceID,LIST_DWORD& lstAdvertID) = 0;
	virtual 	int ReportAdvertProgress(DWORD dwDeviceID, LIST_PROGRESS& lstProgress) = 0;

};
 
class IStorageBusinessSink
{
public:
	virtual ~IStorageBusinessSink() {}
	virtual int OnGetAdvertUrls_Rep(AdvertInfoRep_t& tAdvertRep) = 0;

};
IStorageBusiness* RegStorageBusiness(IStorageBusinessSink* pSink);
void		UnRegStorageBusiness(IStorageBusiness* pDataBase);

class IStorageBusinessStatusSink
{
public:
	virtual ~IStorageBusinessStatusSink() {}
	virtual int OnStorageBusinessStatus(int nResult) = 0; // 0-数据库连接成功, 1-认证成功 2-连接断开
	virtual int Dev_VideoAdvertUrl(DWORD dwDeviceID, AdvertInfoRep_t& tAdvertRep) {return 0;};
};
bool		StorageBusinessInit(DWORD dwIP, WORD wPort, BYTE bGroupCode,  // 数据库服务 地址
	PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword,	// 认证序列号 用户名 密码
	IStorageBusinessStatusSink* pSink );
#endif // _DATABASE_INTERFACE_H_
