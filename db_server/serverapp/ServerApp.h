#pragma once

#include "ServerAppInterface.h"
#include "UtilityInterface.h"
#include "ElemMap_ServerApp.h"
#include "singleton.h"
#include "NetListenInterface.h"
#include "Thread.h"
#include "Protocol.h"
#include "putbuffer.h"
#include "dbHandleInterface.h"
#include "DataStruct.h"
#include "qiniuInterface.h"

enum MsgType_e
{
	MsgType_cmd, // cmd or innercmd
	MsgType_auth, // server auth
	MsgType_dbcallback, // db callback
};
struct MsgTag_t
{
	MsgType_e eMsgType;
	DWORD dwServerID; // CAppServerID or CServerAuth
	BYTE bServerType;
};
struct Msg_t
{
	MsgTag_t tMsgTag;
	CSTRING strMsg;
};
typedef std::list<Msg_t>	LIST_MSG;

const BYTE UpdateDevRoomType_UserIndex = 1;
const BYTE UpdateDevRoomType_PushIndex = 2;
const BYTE UpdateDevRoomType_CardIndex = 4;
const BYTE UpdateDevRoomType_Other = 8;
const BYTE UpdateDevRoomType_Delete = 16;
const BYTE UpdateDevRoomType_PushSwitchIndex = 32;
const DWORD TIMER_QNADVERTCHECK = 5;

class CAppServer : public INetConnectionSink, public ITimerSink
{
public:
	CAppServer();
	~CAppServer();

	void SetServerInfo(ServerInfo_t& tInfo);
	void GetServerInfo(ServerInfo_t& tInfo);
	void SetNetConnection(INetConnection* pCon);

	int InnerCmd(CPutBuffer& buffer, WORD wCommand);
	int InnerCmd_Pieceofsn();
	int InnerCmd_Dsvrcfgindex(DWORD dwVendorID, DWORD dwConfigureIndex);

	int SendPacket(PUCHAR pData, int nLen);
	int SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg = 1, WORD wSubSeg = 1);

	int SendCmd_PieceOfSerialNO(LIST_PIECEOFSERIALNO& listInfo);
	int SendCmd_DserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo);
	int SendCmd_UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID);
	int SendCmd_DeviceConfigureIndex(DWORD dwVendorID, DWORD dwDeviceID, DWORD dwRoomID, BYTE bType);
	int SendCmd_UpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, int nType, LIST_DWORD& lstRoomID);
	int SendCmd_ClearRooms(MAP_DWORD& mapDeviceID);
	int SendCmd_UpdateDevice(MAP_DWORD& mapDeviceID);
	int SendCmd_UpdateDeviceEx(int nType, LIST_DWORD& lstDevID);
	int SendCmd_SetPushInfoEx(WORD wError, BYTE bOpr, ClientTokenArray_t& tInfo);
	int SendCmd_DeviceDeadLine(LIST_DWORD& lstDevID);
	int SendCmd_PropertyAnnounce(DWORD dwVillageId, DWORD dwNoticeIndex);
	int SendCmd_UpdateAdvertInfo(DWORD dwVillageId, DWORD dwAdvertIndex);
	int SendCmd_UpdateFirmwareRequest(PCHAR strVersion, LIST_DWORD& lstDevID);
	int SendCmd_DeleteDeviceOnline(LIST_DWORD& lstDevID);
	int SendCmd_SpecialCrowdSms(SmsInfo2_t& tSmsInfo);

	int SendCmd_DownLoadAdvertInfo(DWORD dwDeviceID, AdvertInfo_t& tInfo, PUCHAR pUrl);
	int SendError(WORD wError);

	//同步1
	int SendCmd_DeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER lstRoomOther);

	// INetConnectionSink
	int OnConnect		(int nReason, INetConnection* pCon){ return 0; }
	int OnDisconnect	(int nReason, INetConnection* pCon);
	int OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSend			(INetConnection* pCon){ return 0; }
	int OnCommand		(PUCHAR pData, int nLen, INetConnection* pCon);

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
private:
	WORD ProcessCommand		(PUCHAR pData, int nLen);

	ServerInfo_t m_tBaseInfo;
	PacketHeader_t m_tHeader;
	BYTE m_bGroupCode;
	INetConnection* m_pCon;
	static BYTE m_szBuffer[MAX_PACKET_LEN];
};

class CAppServerCmd : public INetConnectionSink, public CThread
{
public:
	CAppServerCmd();
	virtual ~CAppServerCmd();

	virtual int OnCmdResponse(LIST_MSG& listResponse) = 0;

	bool OpenSocket(WORD wRawUdpPort);

	WORD AddCommand(MsgTag_t& tMsgTag, PUCHAR pData, int nLen);

	// INetConnectionSink
	int OnConnect		(int nReason, INetConnection* pCon){ return 0; }
	int OnDisconnect	(int nReason, INetConnection* pCon){ return 0; }
	int OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSend			(INetConnection* pCon){ return 0; }
	int OnCommand		(PUCHAR pData, int nLen, INetConnection* pCon);

	// CThread
	void ThreadLoop();

	int SendCmd_PieceOfSerialNO(LIST_PIECEOFSERIALNO& listInfo);
	int SendCmd_DserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo);
	int SendCmd_UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID);
	int SendCmd_DeviceConfigureIndex(DWORD dwVendorID, DWORD dwDeviceID, DWORD dwRoomID, BYTE bType);
	int SendCmd_UpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, int nType, LIST_DWORD& lstRoomID);
	int SendCmd_ClearRooms(MAP_DWORD& mapDeviceID);
	int SendCmd_UpdateDevice(MAP_DWORD& mapDeviceID);
	//室内机
	int SendCmd_BindIndoorRep(BindInfoRep_t& tBindRep);
	int SendCmd_DevListRep(LIST_DEVICEINFO& lstDevInfo);
	int SendCmd_DeviceIndoorID(DevStatus& tDevStat,LIST_DWORD& lstIndoorID);
	int SendCmd_DevStatusRep(LIST_DEVICEINFO& lstDevInfo);
	//
	int Query_SpecialCrowdList(LIST_SPECIALCROWD& lstSpecialCrowd, LIST_SMSINFO2& lstSmsInfo);
	//for sb
	int SendCmd_AdvertUrl_Rep(LIST_ADVERTINFO_REP& lstAdvertInfoRep);
private:
	// for db
	int OnGetServerInfo		(PUCHAR pData, int nLen);
	int OnQueryUser			(PUCHAR pData, int nLen);
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
	int OnGetDeviceRoomSum			(PUCHAR pData, int nLen);
	int OnGetDeviceRoomSumRepAck	(PUCHAR pData, int nLen);
	int OnGetDeviceRoomInfo			(PUCHAR pData, int nLen);
	int OnGetDeviceRoomInfoRepAck	(PUCHAR pData, int nLen);
	int OnGetDServerInfo			(PUCHAR pData, int nLen);
	int OnGetNoticeIndex	(PUCHAR pData, int nLen);
	int OnGetAdvertIndex	(PUCHAR pData, int nLen);
	int OnGetVisitorCfg		(PUCHAR pData, int nLen);
	int OnGetDeviceCfg		(PUCHAR pData, int nLen);
	int OnReportUnlockLog	(PUCHAR pData, int nLen);
	int OnSetPushSwitch		(PUCHAR pData, int nLen);
	//for db 室内机
	int OnIndoorBindDevice(PUCHAR pData, int nLen);
	int OnGetBindDevStatus(PUCHAR pData, int nLen);
	int OnGetIndoorID(PUCHAR pData, int nLen);
	int OnGetIndoorInfo(PUCHAR pData, int nLen);
	//for sb
	int OnGetAdvertUrl(PUCHAR pData, int nLen);
	int OnReportProgress(PUCHAR pData, int nLen);

	// for sdb
	int OnQiniu_GetStorageAccount(PUCHAR pData, int nLen);
	int OnQiniu_GetStorageKeys(PUCHAR pData, int nLen);
	int OnQiniu_GetStorageKeys2(PUCHAR pData, int nLen);
	int OnQiniu_ReportUploadResult(PUCHAR pData, int nLen);
	int OnStorageDBAlarmRecords(PUCHAR pData, int nLen);
	// for inner
	int OnInnerPieceOfSN(PUCHAR pData, int nLen);
	int OnInnerDsvrCfgIndex(PUCHAR pData, int nLen);

	int OnServerAuth(PUCHAR pData, int nLen);

	void AddCommandResponse(PUCHAR pData, int nLen);

	int OnStorageDBUnlockRecords(PUCHAR pData, int nLen);

	int SendCmd_ServerInfo	(ServerInfo_t& tInfo);
	int SendCmd_ServerInfo	(LIST_SERVERINFO& listInfo);
	int SendCmd_UserDevice	(DWORD dwUserID, DWORD dwIndex, LIST_DEVICEINFO& listInfo, MAP_DEVROOMINFO& mapDevRoomInfo);
	int SendCmd_UserGroup	(DWORD dwUserID, DWORD dwIndex, LIST_GROUPINFO& listInfo);
	int SendCmd_UserRoom	(DWORD dwUserID, DWORD dwIndex, LIST_ROOMINFO& listInfo);
	int SendCmd_DeviceInfo	(DeviceInfo_t& tInfo, WORD wError);
	int SendCmd_DeviceUser	(DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo);
	int SendCmd_DevicePush	(DWORD dwDeviceID, DWORD dwIndex, LIST_PUSHINFO& listInfo);
	int SendCmd_DeadLine	(LIST_DEVICE_DEADLINE& listInfo);
	
	int SendCmd_SetPushInfoEx(WORD wError, BYTE bOpr, ClientTokenArray_t& tInfo);

	int SendCmd_Qiniu_StorageAccount(StorageTag_t& tTag, StorageAccount_t& tAccount);
	int SendCmd_Qiniu_StorageKeys	(StorageTag_t& tTag, LIST_STORE_ACCOUNTKEYS& lstAccountKeys);

	//开门记录
	int SendCmd_UnlockRecordsRep(UnlockRep_t& rInfo);

	// ·¿¼äÐÅÏ¢
	int SendCmd_DeviceRoomSum(DWORD dwDeviceID, LIST_ROOMSUM& listInfo);
	int SendCmd_DeviceRoomUser(DWORD dwDeviceID, LIST_ROOMUSER& listInfo);
	int SendCmd_DeviceRoomPush(DWORD dwDeviceID, LIST_ROOMPUSH& listInfo);
	int SendCmd_DeviceRoomCard(DWORD dwDeviceID, LIST_ROOMCARD& listInfo);
	int SendCmd_DeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& listInfo);
	int SendCmd_DeviceRoomIndoor(DWORD dwDeviceID, LIST_ROOMINDOOR2& listInfo);

	int SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg = 1, WORD wSubSeg = 1);

	void NotifyMainThread();

	void ProcessThreadCommand();

	int OnOprDB();
	int OnTestCommand();
public:
	bool m_bTimer_db;
	bool m_bOpr_db;
	PacketHeader_t m_tHeader;
	MsgTag_t m_tMsgTag;

private:
	LIST_MSG m_listRequest;
	LIST_MSG m_listResponse;

	INetConnection* m_pLocalCon;
	int m_sock;
	WORD m_wRawUdpPort;
	DWORD m_dwCmdFlag;
	WORD m_wSegFlag;

	//////////////////////////////////////////////////////////////////////////
	typedef int (CAppServerCmd::*PMFHANDLER)(PUCHAR, int);
	struct HandlerEntry
	{
		WORD wCommand;
		PMFHANDLER pmfHandler;
	};

	//////////////////////////////////////////////////////////////////////////
	static const HandlerEntry mHandlers[];
	static BYTE m_szBuffer[MAX_PACKET_LEN];
};

class CAppServerMgr : public IServerAppHandle, public CAppServerCmd, public IDBHandleSink, public CElemMap_ServerApp<CAppServer> ,public ITimerSink, public IHttpDownloadSink, public IOperationDBHandleSink
{
	DECLARE_SINGLETON(CAppServerMgr)
public:
	CAppServerMgr();
	~CAppServerMgr();

	bool Start(WORD wRawUdpPort);
	int AddServer(ServerInfo_t& tInfo, INetConnection* pCon);
	void ServerAuth(DWORD dwID, PUCHAR pSN);
	void LoginOtherPlace(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo);
	
	// IServerAppHandle
	void PieceOfSerialNO();
	void DserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex);
	void UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID);
	void DeviceConfigureIndex(DWORD dwVendorID, DWORD dwDeviceID, DWORD dwRoomID, BYTE bType);
	void UpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, int nType, LIST_DWORD& lstRoomID);
	void ClearRooms(MAP_DWORD& mapDeviceID);
	void UpdateDevice(MAP_DWORD& mapDeviceID);
	void UpdateDeviceEx(int nType, LIST_DWORD& lstDevID);
	void UpdateDeviceDeadLine(LIST_DWORD& lstDevID);
	void UpdatePropertyAnnounce(DWORD dwVillageId, DWORD dwNoticeIndex);
	void UpdateFirmwareRequest(PCHAR strVersion, LIST_DWORD& lstDevID);
	void DeleteDeviceOnline(LIST_DWORD& lstDevID);
	void UpdateAdvertInfo(DWORD dwVillageId, DWORD dwAdvertIndex);
	void SpecialCrowdSms(SmsInfo2_t& tSmsInfo);
	
	// CAppServerCmd
	int OnCmdResponse(LIST_MSG& listResponse);

	// IDBHandleSink--childthread
	void OnUserConfigureIndex(DWORD dwVendorID, DWORD dwUserID);
	void OnUpdateDeviceRoom(DWORD dwVendorID, DWORD dwDeviceID, LIST_DWORD& lstRoomID, BYTE bType);
	void OnClearRooms(DWORD dwVendorID, DWORD dwDeviceID);
	void OnUpdateDevice(DWORD dwVendorID, DWORD dwDeviceID);

	//storagebusiness
	void OnDownloadStatus(char* pUrl, int nPesent, bool bFinish);

	void DownLoadAdvertInfo(DWORD dwDeviceID, AdvertInfo_t& tInfo, PUCHAR pUrl);
	//七牛广告视频下载管理
	bool VideoDownloadMgr();
	//获取视频广告下载进度管理
	bool VideoProgressMgr();
	//获取下载列表
	bool GetQiniuTaskList();

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

private:
	int ParseServerAuth(PUCHAR pData, int nLen, ServerInfo_t& tInfo);

	// 所有需要从七牛下载的视频广告ID列表
	LIST_DWORD m_lstAdID;
	// 七牛下载视频广告进度的回调
	IHttpDownloadSink* m_pSink;
	// 下载视频广告标识 0 - 无下载或无任务进行  1 - 有下载任务正在进行
	DWORD  m_dwVideoFlag; 
	// 保存正在下载的视频广告ID和广告名称，没有为0
	DWORD m_dwAdID;
	CSTRING m_strAdName;
	// 视频广告下载完成标识 true - 下载完成或没有下载 ， false - 下载没有完成
	bool m_bFinish;
	// 七牛下载进度标识
	DWORD m_dwProgress;
	// 超时标准
	DWORD m_dwTimeout;
};

#define DECLARE_PUTBUFFER( bufferPut ) \
	CPutBuffer bufferPut( m_szBuffer, MAX_PACKET_LEN ); \
	bufferPut.Skip ( PACKET_HEADER_SIZE );
