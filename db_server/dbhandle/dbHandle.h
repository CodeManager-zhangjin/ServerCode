#pragma once

#include <mysql++.h>
#include "singleton.h"
#include "DataStruct.h"
#include "dbHandleInterface.h"
#include "UtilityInterface.h"
#include "dbdef.h"

using namespace mysqlpp;

extern DWORD g_dwGroupID;
class FindGroupInfoByID
{
public:
	bool operator() (LIST_GROUPINFO::value_type& pos)
	{
		if (g_dwGroupID == pos.dwGroupID) return true;
		return false;
	}
};

extern DWORD g_dwRoomID;
class FindRoomUserByRoomID
{
public:
	bool operator() (LIST_ROOMUSER::value_type& pos)
	{
		if (g_dwRoomID == pos.dwRoomID) return true;
		return false;
	}
};
class FindRoomPushByRoomID
{
public:
	bool operator() (LIST_ROOMPUSH::value_type& pos)
	{
		if (g_dwRoomID == pos.dwRoomID) return true;
		return false;
	}
};
class FindRoomPushSwitchByRoomID
{
public:
	bool operator() (LIST_ROOMPUSHSWITCH::value_type& pos)
	{
		if (g_dwRoomID == pos.dwRoomID) return true;
		return false;
	}
};
class FindRoomCardByRoomID
{
public:
	bool operator() (LIST_ROOMCARD::value_type& pos)
	{
		if (g_dwRoomID == pos.dwRoomID) return true;
		return false;
	}
};

class FindRoomIndoorByRoomID
{
public:
	bool operator() (LIST_ROOMINDOOR2::value_type& pos)
	{
		if(g_dwRoomID == pos.dwRoomID) return true;
		return false;
	}
};

class FindRoomSumByRoomID
{
public:
	bool operator() (LIST_ROOMSUM::value_type& pos)
	{
		if (g_dwRoomID == pos.dwRoomID) return true;
		return false;
	}
};

class CDataBaseHandle : public IDBHandle, public ITimerSink
{
	DECLARE_SINGLETON(CDataBaseHandle)
public:
	CDataBaseHandle();
	virtual ~CDataBaseHandle();

	bool Connect(PUCHAR pHost, PUCHAR pDatabase, PUCHAR pUserName, PUCHAR pPassword);
	int  GetError();
	void SetSink(IDBHandleSink* pSink) {m_pSink = pSink;}
	bool IsConnect(){ return m_bConnectSuccess; }
	void OnTimer();
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
	
	////////////////////////////服务器相关///////////////////////////////////
	// 查询服务器信息
	// 入参pServerType			服务器类型
	// 入参dwVendorID			厂商ID
	// 入参listInfo				服务器信息
	bool Query_ServerInfo	(PUCHAR pServerType, DWORD dwVendorID, LIST_SERVERINFO& listInfo);
	bool Query_ServerInfo	(ServerInfo_t& tInfo);
	// 查询注册服务器Index
	bool Query_DserverConfigureIndex(DWORD dwVendorID, MAP_DWORD& mapInfo);
	// 查询序列号段
	bool Query_PieceOfSerialNO(LIST_PIECEOFSERIALNO& listInfo);	
	
	////////////////////////////注册账号相关///////////////////////////////////
	// 查询用户是否存在
	// 入参pUser				用户名
	bool Query_User			(PUCHAR pUser);
	// 插入用户
	bool Insert_User		(DWORD dwVendorID, BYTE bLanguage, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pUser);

	////////////////////////////账号相关///////////////////////////////////
	// 查询用户信息
	bool Query_UserInfo		(PUCHAR pUserName, DWORD dwAppVendorID, UserInfo_t& tInfo, ClientTokenArray_t& tArray);
	// 查询账号下设备信息
	bool Query_UserDevice	(DWORD dwUserID, LIST_DEVICEINFO& listInfo);
	// 查询账号下组信息
	bool Query_UserGroup	(DWORD dwUserID, LIST_GROUPINFO& listInfo);
	// 查询账号下设备的房号信息
	bool Query_UserRoom		(DWORD dwUserID, LIST_ROOMINFO& listInfo);
	// 查询用户下设备房号信息
	bool Query_UserDeviceRoomInfo(DWORD dwUserID, LIST_DEVICEINFO& lstDevInfo, MAP_DEVROOMINFO& mapDevRoomInfo);


	////////////////////////////设备相关///////////////////////////////////
	// 查询设备信息
	bool Query_DeviceInfo	(PUCHAR pSN, DeviceInfo_t& tInfo);
	// 查询设备所属账号信息
	bool Query_DeviceUser	(DWORD dwDeviceID, DWORD& dwConfigureIndex, LIST_SMSINFO& listInfo);
	// 查询设备所需推送信息
	bool Query_DevicePush	(DWORD dwDeviceID, DWORD& dwConfigureIndex2, LIST_PUSHINFO& listInfo);
	//从数据库中查询设备截至日期
	bool Query_DeviceDeadLine(LIST_DWORD& lstDevID, LIST_DEVICE_DEADLINE& devDeadLine);
	////////////////////////////账号与设备关联///////////////////////////////////
	// 账号添加设备
	bool Insert_UserDevice	(DWORD dwUserID, PUCHAR pSerialNO, PUCHAR pDevName, PUCHAR pUserName, PUCHAR pRoom);
	// 授权
	bool Insert_UserDevice	(DWORD dwOwnerID, PUCHAR pUser, DWORD dwDeviceID, DWORD& dwUserID);
	// 授权（冠林SDK）
	bool Insert_UserDevice	(DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDevName, PUCHAR pRoom);
	// 取消授权 / 删除
	bool Delete_DeviceUser	(DWORD dwUserID, DWORD dwDeviceID);
	// 更新账号下设备的设备名称
	bool Update_DeviceName	(DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName);

	////////////////////////////推送相关///////////////////////////////////
	// 插入/删除推送信息
	bool Insert_PushInfo	(ClientTokenArray_t& tInfo, bool bTry);
	bool Delete_PushInfo	(ClientTokenArray_t& tInfo);

	////////////////////////////设备房号相关///////////////////////////////////
	// 查询单元门口机房号摘要/信息
	bool Query_DeviceRoomSum	(DWORD dwDeviceID, LIST_ROOMSUM& listInfo);
	bool Query_DeviceRoomPush	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMPUSH& listInfo);
	bool Query_DeviceRoomUser	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMUSER& listInfo);
	bool Query_DeviceRoomCard	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMCARD& listInfo);
	//查询绑定的室内机
	bool Query_DeviceRoomIndoor(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMINDOOR2& listInfo);

	bool Query_DeviceRoomOther	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMOTHER& listInfo);
	bool Query_DeviceRoomPushSwitch(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMPUSHSWITCH& listInfo);

	bool Query_DServerInfo(DServerInfo_t& tInfo);
	bool Query_NoticeIndex(DWORD dwVillageId, DWORD& dwNoticeIndex);
	bool Query_AdvertIndex(DWORD dwVillageId, DWORD& dwAdvertIndex);
	bool Query_VisitorCfg(DWORD dwGroupId, DWORD dwVisitorCfg);

	// 查询账号设备的房号
	bool Query_UserDeviceRoom(DWORD dwUserID, DWORD dwDeviceID, LIST_DWORD& lstRoomID);

	// 查询云存储数量限制
	bool Query_StoreLimit(DWORD dwDeviceID, int& nStoreLimit);

	bool Query_UpdateDeviceCfg();
	bool Query_DeviceCfg(DWORD dwDeviceID, int nType, UcpaasInfo_t& tUcpaas);
	bool Query_VendorPhone(DWORD dwID, int nType, SystemCfg_t& tPhone);
	bool Query_UpdatePushSwitch(DWORD dwUserID, int nSwitch);
	// 查询小区设备
	bool Query_GroupDevice(DWORD dwVillageId, LIST_DWORD& lstDevID);
	//室内机
	bool Insert_IndoorInfo(PUCHAR pIndoorSN , LIST_BIND_INFO& listBindInfo, BindInfoRep_t& tBindRep);
	bool UpdateIndoorIndex(LIST_BIND_INFO& listBindInfo);
	bool MkInDoorStr( DWORD dwIndoorID, LIST_BIND_INFO& listBindInfo, CSTRING& strInDoor);
	bool CheckBindInfo(PUCHAR pIndoorSN , LIST_BIND_INFO& listBindInfo, BindInfoRep_t& tBindRep);
	int  Query_DevRoom(LIST_BIND_INFO& listBindInfo);
	bool Query_InDoorInfo(DWORD dwDeviceID,LIST_TABINDOORINFO& lstInDoorInfo);
	bool Query_InDoorIndex(DWORD dwDeviceID,DWORD dwRoomID,DWORD& dwIndoorIndex);
	bool QueryRoomIndoorCount(DWORD dwDeviceID, DWORD dwRoomID, LIST_INDOORINFO& lstIndoorInfo);
	bool QueryIndoorBindDev(DWORD dwIndoorID, LIST_DEVICEINFO& lstDevInfo);
	bool QueryDevInfo(LIST_DEVICEINFO& lstDevInfo);
	bool Query_DeviceIndoorID(DWORD dwDeviceID, LIST_DWORD& lstIndoorID);
	bool Query_InDoorID(PUCHAR pIndoorSN, DWORD& dwIndoorID);
	bool Query_InDoorSN(DWORD dwIndoorID, PUCHAR pIndoorSN);
	bool Query_InDoorIDBySN(PUCHAR pIndoorSN, DWORD dwIndoorID);

	//特殊人群
	bool Query_SpecialUsers(LIST_SPECIALCROWD& m_lstSpecialCrowd);
	bool Query_PropertyPhone(DWORD dwDeviceID, SpecialCrowd_t& tInfo);
private:
	// 保持数据库连接，防止连接超时 
	void Keepalive();

	// 执行数据库操作，并捕获异常
	int CatchException(Query& query);
	int CatchException(Query& query, StoreQueryResult& res);

	// 查询主房号摘要
	bool Query_DeviceMainRoomSum(DWORD dwDeviceID, RoomSum_t& tRoomSum);

	// 获取有效的用户名：优先手机号，其次邮箱，最后用户名
	void GetVaildUserName(std::string& strMobilePhone, std::string& strEmail, std::string& strUserName, PUCHAR pUser);
	
	// 产生deviceroom下roomid字符串
	void MkRoomIDStr(LIST_DWORD& lstRoomID, CSTRING& strRoomID, bool bTableFlag = true);
	
	// 查询房号摘要
	bool Query_DeviceRoomSum(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMSUM& lstRoomSum);

	// 插入房号
	bool Insert_Room(DWORD dwDeviceID, PUCHAR pRoom);

	// 查询账号下是否有该设备
	bool Query_UserHasDevice(DWORD dwUserID, DWORD dwDeviceID, DWORD& dwRoomID, PUCHAR pDeviceName);
	bool Query_UserHasDevice(DWORD dwUserID, DWORD dwDeviceID, LIST_DWORD& lstRoomID);

	// 取消授权
	bool CancelAuthorize	(DWORD dwUserID, DWORD dwDeviceID);
	// 删除设备
	bool DeleteDeviceInUser	(DWORD dwUserID, DWORD dwDeviceID);

	// 授权账号修改设备名称
	bool Update_DeviceNameSub(DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName);
	// 主账号修改设备名称
	bool Update_DeviceNameMain(DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName);

	// 查询账号下所有设备ID
	bool Query_DeviceIDByUserID(DWORD dwUserID, LIST_DWORD& lstDeviceID);	

	// 更新设备OwnerID和GroupID
	bool Update_DeviceOwnerIDGroupID(DWORD dwDeviceID, DWORD dwOwnerID);

	// 删除相同UserID+VendorID+PushType(OS)的其他不同的token
//	bool Delete_OtherToken(PushInfo_t& tInfo, bool bTry);

	// 产生userid字符串
	void MkUserIDStr(LIST_DWORD& lstUserID, CSTRING& strRoomID);
	void MkPushUserIDStr(LIST_DWORD& lstUserID, CSTRING& strRoomID);
	
	// 更新账号下设备的PushIndex
	bool Update_UserPush(DWORD dwUserID);
	bool Update_UserPush(LIST_DWORD& lstUserID);
	bool Update_UserPushSwitch(LIST_DWORD& lstUserID);

	bool Update_UserConfigureIndex(SET_DWORD& lstUserID);
	bool Update_RoomPushIndex(LIST_DWORD& lstRoomID);
	bool Update_RoomUserIndex(LIST_DWORD& lstRoomID);
	bool Update_RoomUserIndexPushIndex(LIST_DWORD& lstRoomID);
	bool Update_RoomPushSwitchIndex(LIST_DWORD& lstRoomID);

	// 通知
	void Notify_UserConfigureIndex(DWORD dwUserID);
	// 1-UserIndex，2-PushIndex，4-CardIndex，8-Other(Room & Password & Reserve) 16-Delete
	void Notify_UpdateDeviceRoom(DWORD dwDeviceID, LIST_DWORD& lstRoomID, BYTE bType);
	void Notify_ClearRooms(DWORD dwDeviceID);
	void Notify_UpdateDevice(DWORD dwDeviceID);

	void GetToken(ClientTokenArray_t& tInfo, PushTokenType_t& tTokenType, int& nOS);
	void MkTokenStr(ClientTokenArray_t& tInfo, CSTRING& strToken);
	void MkPushIDStr(LIST_DWORD& lstID, CSTRING& strID);
	void AppendRoomPush(CSTRING strToken, PushInfo_t& tPushInfo, RoomPush_t& tRoomPush, LIST_ROOMPUSH& listInfo);

	//判断设备是否可以打电话
	char SNCharToVal(char ch);
	DWORD GetDeviceTID(char* pSerialNum);
	DWORD PhoneCall(DWORD tid);

	struct SamePushInfoItem_t
	{
		bool bUpdateFlag;
		bool bSameFlag;
		DWORD dwPushInfoID;
	};
	bool Insert_PushInfoA(ClientTokenArray_t& tInfo, SamePushInfoItem_t& tItem);
	bool IsSameDeviceToken(CSTRING& curToken, CSTRING& setToken);

	//
	bool Delete_PushInfoByIDs(LIST_DWORD& lstID);

	// 该账号的storeid字段为空或者为0,0表示未设置云存储账号，则使用该帐号的上一级的云存储账号
	// 物业账号/普通账号-->代理商账号/厂商账号
	DWORD QueryStoreIDByUserID(DWORD dwUserID);

	IDBHandleSink* m_pSink; // 该Sink就是CAppServerMgr
	Connection m_clsSrcDBCon;
	int m_nError;
	bool m_bConnectSuccess;
	DWORD m_dwView;//用来表示是否是同一个APP登录，此为CView对象指针
};
