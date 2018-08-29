#pragma once

#include "DataBaseInterface.h"
#include "Protocol.h"
#include "putbuffer.h"
#include "getbuffer.h"
#include "Log.h"

// 内部消息机制打包解析类
class CDBClientHandle
{
public:
	CDBClientHandle();
	virtual ~CDBClientHandle();

	void SetDataBaseAID(DWORD dwID) { m_dwDataBaseAID = dwID; }

	//////////////////////////////////////////////////////////////////////////
	int ProcessCommand	(PUCHAR pData, int nLen);
	int GetChallenge	();
	int Auth			(PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pChallenge);
	int SendMsg			(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg = 1, WORD wSubSeg = 1);
private:
	int SendDeviceRoomSumRepAck(DWORD dwDeviceID);
	int SendDeviceRoomInfoRepAck(DWORD dwDeviceID, BYTE bType);
	int SendCmd_DeviceDeadLineInfo(DeviceDeadLineInfo_t &listInfo);
	int SendCmd_PropertyAnnounce(LIST_DWORD& lstDevID);
	int SendCmd_DeviceUnlockRecords(UnlockInfo_t& tInfo);

//	int SendMsg			(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg = 1, WORD wSubSeg = 1);

	int ParseOnUserInfoEx(CGetBuffer& bufferGet, int nNeedLen, int nDataLen, ClientTokenArray_t& tArray);
	//////////////////////////////////////////////////////////////////////////
	// 解析
	int OnChallenge			(PUCHAR pData, int nLen);
	int OnAuth				(PUCHAR pData, int nLen);
	int OnGetServerInfo		(PUCHAR pData, int nLen);
	int OnQueryMobilePhone	(PUCHAR pData, int nLen);
	int OnSetSecret			(PUCHAR pData, int nLen);
	int OnGetUserInfo		(PUCHAR pData, int nLen);
	int OnGetUserDeviceInfo	(PUCHAR pData, int nLen);
	int OnGetUserGroupInfo	(PUCHAR pData, int nLen);
	int OnGetUserRoomInfo	(PUCHAR pData, int nLen);
	int OnGetDeviceInfo		(PUCHAR pData, int nLen);
	int OnGetDeviceUserInfo	(PUCHAR pData, int nLen);
	int OnGetDevicePushInfo	(PUCHAR pData, int nLen);
	int OnAddDevice			(PUCHAR pData, int nLen);
	int OnAddDelPushInfoEx	(PUCHAR pData, int nLen);
	int OnSetDeviceName		(PUCHAR pData, int nLen);
	int OnDelDevice			(PUCHAR pData, int nLen);
	int OnAuthorize			(PUCHAR pData, int nLen);
	int OnAuthorize2		(PUCHAR pData, int nLen);
	int OnPieceOfSerialNO		(PUCHAR pData, int nLen);
	int OnDserverConfigureIndex	(PUCHAR pData, int nLen);
	int OnUserConfigureIndex	(PUCHAR pData, int nLen);
	int OnDeviceConfigureIndex	(PUCHAR pData, int nLen);
	int OnError				(PUCHAR pData, int nLen);

	int OnGetDeviceRoomSum	(PUCHAR pData, int nLen);
	int OnGetDeviceRoomUser	(PUCHAR pData, int nLen);
	int OnGetDeviceRoomPush	(PUCHAR pData, int nLen);
	int OnGetDeviceRoomCard	(PUCHAR pData, int nLen);
	int OnGetDeviceRoomOther(PUCHAR pData, int nLen);
	int OnGetDeviceRoomIndoor(PUCHAR pData, int nLen);

	int OnUpdateDeviceRoomInfo	(PUCHAR pData, int nLen);
	int OnClearRooms			(PUCHAR pData, int nLen);
	int OnUpdateDevice			(PUCHAR pData, int nLen);
	int OnUpdateDeviceEx		(PUCHAR pData, int nLen);

	int OnGetDServerInfo		(PUCHAR pData, int nLen);
	int OnGetNoticeIndex		(PUCHAR pData, int nLen);

	int OnQiniu_GetStorageAccount	(PUCHAR pData, int nLen);
	int OnQiniu_GetStorageKeys		(PUCHAR pData, int nLen);

	int OnGetDeviceCfg			(PUCHAR pData, int nLen);

	int OnSetPushSwitch			(PUCHAR pData, int nLen);
	int OnGetDeviceDeadLine		(PUCHAR pData, int nLen); 
	int OnUnlockRecords			(PUCHAR pData, int nLen);
	int OnUpdatePropertyAnnounce(PUCHAR pData, int nLen);
	int OnUpdateAdvertInfo		(PUCHAR pData, int nLen);
	int OnUpdateBulletin		(PUCHAR pData, int nLen);
	int OnGetAdvertInfo			(PUCHAR pData, int nLen);
	int OnGetVisitorCfg			(PUCHAR pData, int nLen);

	int OnUpdateFirmwareRequest (PUCHAR pData, int nLen);
	int OnDeleteDeviceOnline	(PUCHAR pData, int nLen);
	//室内机
	int OnIndoorBindDeviceRep(PUCHAR pData, int nLen);
	int OnGetIndoorBindDevList(PUCHAR pData, int nLen);
	int OnGetListDevStat(PUCHAR pData, int nLen);
	int OnGetDevBindIndoorID(PUCHAR pData, int nLen);

	int OnSmsSpecialCrowd(PUCHAR pData, int nLen);
	//StorageBusiness
	int OnGetAdvertUrls_Rep(PUCHAR pData, int nLen);
public:
	INetConnection*	m_pCon;
	BYTE m_bGroupCode;
	
	IDataBaseStatusSink* m_pSink;
	IStorageDBStatusSink* m_pSdbSink;
	IStorageBusinessStatusSink* m_pSbSink;

	DWORD m_dwDataBaseAID; // 作为协议头的sourceid字段填入，回应包的时候根据这个字段找到对应的对象

	BYTE m_szSerialNO[LENGTH_SERIALNO+1];
	BYTE m_szUserName[LENGTH_NAME+1];
	BYTE m_szPassword[LENGTH_PASSWORD+1];
private:
	LIST_PIECEOFSERIALNO m_listPieceOfSerialNO;
	LIST_SERVERINFO m_listDserverInfo;

	LIST_DEVICEINFO m_listUserDeviceInfo;
	MAP_DEVROOMINFO mapDevRoomInfo;
	LIST_ROOMINFO2 lstRoomInfo;
	LIST_GROUPINFO m_listUserGroupInfo;
	LIST_ROOMINFO m_listUserRoomInfo;
	LIST_SMSINFO m_listDeviceUserInfo;
	LIST_PUSHINFO m_listDevicePushInfo;
	LIST_DEVICE_DEADLINE m_listDeviceDeadLineInfo; 

	PacketHeader_t m_tHeader;
	typedef int (CDBClientHandle::*PMFHANDLER)(PUCHAR, int);
	typedef struct _tagHandlerEntry
	{
		WORD wCommand;
		PMFHANDLER pmfHandler;
	} HandlerEntry;
	static const HandlerEntry m_handlers[];
public:
	BYTE m_szBuffer[MAX_PACKET_LEN];
};

#define DECLARE_PUTBUFFER( bufferPut ) \
	CPutBuffer bufferPut( CDBClientHandle::m_szBuffer, MAX_PACKET_LEN ); \
	bufferPut.Skip ( PACKET_HEADER_SIZE );
