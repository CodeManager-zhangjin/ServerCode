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
	
	////////////////////////////���������///////////////////////////////////
	// ��ѯ��������Ϣ
	// ���pServerType			����������
	// ���dwVendorID			����ID
	// ���listInfo				��������Ϣ
	bool Query_ServerInfo	(PUCHAR pServerType, DWORD dwVendorID, LIST_SERVERINFO& listInfo);
	bool Query_ServerInfo	(ServerInfo_t& tInfo);
	// ��ѯע�������Index
	bool Query_DserverConfigureIndex(DWORD dwVendorID, MAP_DWORD& mapInfo);
	// ��ѯ���кŶ�
	bool Query_PieceOfSerialNO(LIST_PIECEOFSERIALNO& listInfo);	
	
	////////////////////////////ע���˺����///////////////////////////////////
	// ��ѯ�û��Ƿ����
	// ���pUser				�û���
	bool Query_User			(PUCHAR pUser);
	// �����û�
	bool Insert_User		(DWORD dwVendorID, BYTE bLanguage, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pUser);

	////////////////////////////�˺����///////////////////////////////////
	// ��ѯ�û���Ϣ
	bool Query_UserInfo		(PUCHAR pUserName, DWORD dwAppVendorID, UserInfo_t& tInfo, ClientTokenArray_t& tArray);
	// ��ѯ�˺����豸��Ϣ
	bool Query_UserDevice	(DWORD dwUserID, LIST_DEVICEINFO& listInfo);
	// ��ѯ�˺�������Ϣ
	bool Query_UserGroup	(DWORD dwUserID, LIST_GROUPINFO& listInfo);
	// ��ѯ�˺����豸�ķ�����Ϣ
	bool Query_UserRoom		(DWORD dwUserID, LIST_ROOMINFO& listInfo);
	// ��ѯ�û����豸������Ϣ
	bool Query_UserDeviceRoomInfo(DWORD dwUserID, LIST_DEVICEINFO& lstDevInfo, MAP_DEVROOMINFO& mapDevRoomInfo);


	////////////////////////////�豸���///////////////////////////////////
	// ��ѯ�豸��Ϣ
	bool Query_DeviceInfo	(PUCHAR pSN, DeviceInfo_t& tInfo);
	// ��ѯ�豸�����˺���Ϣ
	bool Query_DeviceUser	(DWORD dwDeviceID, DWORD& dwConfigureIndex, LIST_SMSINFO& listInfo);
	// ��ѯ�豸����������Ϣ
	bool Query_DevicePush	(DWORD dwDeviceID, DWORD& dwConfigureIndex2, LIST_PUSHINFO& listInfo);
	//�����ݿ��в�ѯ�豸��������
	bool Query_DeviceDeadLine(LIST_DWORD& lstDevID, LIST_DEVICE_DEADLINE& devDeadLine);
	////////////////////////////�˺����豸����///////////////////////////////////
	// �˺�����豸
	bool Insert_UserDevice	(DWORD dwUserID, PUCHAR pSerialNO, PUCHAR pDevName, PUCHAR pUserName, PUCHAR pRoom);
	// ��Ȩ
	bool Insert_UserDevice	(DWORD dwOwnerID, PUCHAR pUser, DWORD dwDeviceID, DWORD& dwUserID);
	// ��Ȩ������SDK��
	bool Insert_UserDevice	(DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDevName, PUCHAR pRoom);
	// ȡ����Ȩ / ɾ��
	bool Delete_DeviceUser	(DWORD dwUserID, DWORD dwDeviceID);
	// �����˺����豸���豸����
	bool Update_DeviceName	(DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName);

	////////////////////////////�������///////////////////////////////////
	// ����/ɾ��������Ϣ
	bool Insert_PushInfo	(ClientTokenArray_t& tInfo, bool bTry);
	bool Delete_PushInfo	(ClientTokenArray_t& tInfo);

	////////////////////////////�豸�������///////////////////////////////////
	// ��ѯ��Ԫ�ſڻ�����ժҪ/��Ϣ
	bool Query_DeviceRoomSum	(DWORD dwDeviceID, LIST_ROOMSUM& listInfo);
	bool Query_DeviceRoomPush	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMPUSH& listInfo);
	bool Query_DeviceRoomUser	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMUSER& listInfo);
	bool Query_DeviceRoomCard	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMCARD& listInfo);
	//��ѯ�󶨵����ڻ�
	bool Query_DeviceRoomIndoor(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMINDOOR2& listInfo);

	bool Query_DeviceRoomOther	(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMOTHER& listInfo);
	bool Query_DeviceRoomPushSwitch(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMPUSHSWITCH& listInfo);

	bool Query_DServerInfo(DServerInfo_t& tInfo);
	bool Query_NoticeIndex(DWORD dwVillageId, DWORD& dwNoticeIndex);
	bool Query_AdvertIndex(DWORD dwVillageId, DWORD& dwAdvertIndex);
	bool Query_VisitorCfg(DWORD dwGroupId, DWORD dwVisitorCfg);

	// ��ѯ�˺��豸�ķ���
	bool Query_UserDeviceRoom(DWORD dwUserID, DWORD dwDeviceID, LIST_DWORD& lstRoomID);

	// ��ѯ�ƴ洢��������
	bool Query_StoreLimit(DWORD dwDeviceID, int& nStoreLimit);

	bool Query_UpdateDeviceCfg();
	bool Query_DeviceCfg(DWORD dwDeviceID, int nType, UcpaasInfo_t& tUcpaas);
	bool Query_VendorPhone(DWORD dwID, int nType, SystemCfg_t& tPhone);
	bool Query_UpdatePushSwitch(DWORD dwUserID, int nSwitch);
	// ��ѯС���豸
	bool Query_GroupDevice(DWORD dwVillageId, LIST_DWORD& lstDevID);
	//���ڻ�
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

	//������Ⱥ
	bool Query_SpecialUsers(LIST_SPECIALCROWD& m_lstSpecialCrowd);
	bool Query_PropertyPhone(DWORD dwDeviceID, SpecialCrowd_t& tInfo);
private:
	// �������ݿ����ӣ���ֹ���ӳ�ʱ 
	void Keepalive();

	// ִ�����ݿ�������������쳣
	int CatchException(Query& query);
	int CatchException(Query& query, StoreQueryResult& res);

	// ��ѯ������ժҪ
	bool Query_DeviceMainRoomSum(DWORD dwDeviceID, RoomSum_t& tRoomSum);

	// ��ȡ��Ч���û����������ֻ��ţ�������䣬����û���
	void GetVaildUserName(std::string& strMobilePhone, std::string& strEmail, std::string& strUserName, PUCHAR pUser);
	
	// ����deviceroom��roomid�ַ���
	void MkRoomIDStr(LIST_DWORD& lstRoomID, CSTRING& strRoomID, bool bTableFlag = true);
	
	// ��ѯ����ժҪ
	bool Query_DeviceRoomSum(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMSUM& lstRoomSum);

	// ���뷿��
	bool Insert_Room(DWORD dwDeviceID, PUCHAR pRoom);

	// ��ѯ�˺����Ƿ��и��豸
	bool Query_UserHasDevice(DWORD dwUserID, DWORD dwDeviceID, DWORD& dwRoomID, PUCHAR pDeviceName);
	bool Query_UserHasDevice(DWORD dwUserID, DWORD dwDeviceID, LIST_DWORD& lstRoomID);

	// ȡ����Ȩ
	bool CancelAuthorize	(DWORD dwUserID, DWORD dwDeviceID);
	// ɾ���豸
	bool DeleteDeviceInUser	(DWORD dwUserID, DWORD dwDeviceID);

	// ��Ȩ�˺��޸��豸����
	bool Update_DeviceNameSub(DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName);
	// ���˺��޸��豸����
	bool Update_DeviceNameMain(DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName);

	// ��ѯ�˺��������豸ID
	bool Query_DeviceIDByUserID(DWORD dwUserID, LIST_DWORD& lstDeviceID);	

	// �����豸OwnerID��GroupID
	bool Update_DeviceOwnerIDGroupID(DWORD dwDeviceID, DWORD dwOwnerID);

	// ɾ����ͬUserID+VendorID+PushType(OS)��������ͬ��token
//	bool Delete_OtherToken(PushInfo_t& tInfo, bool bTry);

	// ����userid�ַ���
	void MkUserIDStr(LIST_DWORD& lstUserID, CSTRING& strRoomID);
	void MkPushUserIDStr(LIST_DWORD& lstUserID, CSTRING& strRoomID);
	
	// �����˺����豸��PushIndex
	bool Update_UserPush(DWORD dwUserID);
	bool Update_UserPush(LIST_DWORD& lstUserID);
	bool Update_UserPushSwitch(LIST_DWORD& lstUserID);

	bool Update_UserConfigureIndex(SET_DWORD& lstUserID);
	bool Update_RoomPushIndex(LIST_DWORD& lstRoomID);
	bool Update_RoomUserIndex(LIST_DWORD& lstRoomID);
	bool Update_RoomUserIndexPushIndex(LIST_DWORD& lstRoomID);
	bool Update_RoomPushSwitchIndex(LIST_DWORD& lstRoomID);

	// ֪ͨ
	void Notify_UserConfigureIndex(DWORD dwUserID);
	// 1-UserIndex��2-PushIndex��4-CardIndex��8-Other(Room & Password & Reserve) 16-Delete
	void Notify_UpdateDeviceRoom(DWORD dwDeviceID, LIST_DWORD& lstRoomID, BYTE bType);
	void Notify_ClearRooms(DWORD dwDeviceID);
	void Notify_UpdateDevice(DWORD dwDeviceID);

	void GetToken(ClientTokenArray_t& tInfo, PushTokenType_t& tTokenType, int& nOS);
	void MkTokenStr(ClientTokenArray_t& tInfo, CSTRING& strToken);
	void MkPushIDStr(LIST_DWORD& lstID, CSTRING& strID);
	void AppendRoomPush(CSTRING strToken, PushInfo_t& tPushInfo, RoomPush_t& tRoomPush, LIST_ROOMPUSH& listInfo);

	//�ж��豸�Ƿ���Դ�绰
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

	// ���˺ŵ�storeid�ֶ�Ϊ�ջ���Ϊ0,0��ʾδ�����ƴ洢�˺ţ���ʹ�ø��ʺŵ���һ�����ƴ洢�˺�
	// ��ҵ�˺�/��ͨ�˺�-->�������˺�/�����˺�
	DWORD QueryStoreIDByUserID(DWORD dwUserID);

	IDBHandleSink* m_pSink; // ��Sink����CAppServerMgr
	Connection m_clsSrcDBCon;
	int m_nError;
	bool m_bConnectSuccess;
	DWORD m_dwView;//������ʾ�Ƿ���ͬһ��APP��¼����ΪCView����ָ��
};
