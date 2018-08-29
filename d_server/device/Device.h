#pragma once

#include "DeviceInterface.h"
#include "ElemMapDevice.h"
#include "singleton.h"
#include "NetListenInterface.h"
#include "DataBaseInterface.h"
#include "Protocol.h"
#include "putbuffer.h"
#include "UtilityInterface.h"
#include "Thread.h"
#include <map>

class CDevice : public INetConnectionSink, public IDataBaseSink, public IStorageDBSink, IStorageBusinessSink, public IDealMulPktSink
#ifdef CACHE_DEVICE_STATUS
	, public ITimerSink
#endif
{
public:
	CDevice(DWORD dwClientID);
	~CDevice();

	bool IsYunShangCheng();

	DWORD GetElemID(){ return m_dwClientID; }
	DWORD GetDeviceID(){ return m_tDeviceInfo.dwDeviceID; }
	void  GetDeviceStatus(DeviceStatus_t& tInfo);
	void  GetDeviceStatus(DWORD dwUserID, DeviceStatus_t& tInfo);
	void  GetDeviceNetInfo(NetInfo_t& tNetInfo, DWORD& dwAutoRelay);
	void  GetDevicePassword(PUCHAR pPassword);
	DWORD GetDeviceType()    { return m_tDeviceInfo.dwDeviceType; }
	PUCHAR GetDeviceImgVer() { return (PUCHAR)m_tDeviceInfo.szImageVer; }

	int SendCmd_UnlockInfo(UnlockInfo_t& tInfo);
	int SendCmd_TransClientInfo(TransClientInfo_t& tInfo);
	int SendCmd_SdlTunnel(SdkTunnel_t& tInfo);
	int SendCmd_Qiniu_UploadTokenRep(StorageTag_t& tTag, PUCHAR pUploadToken, WORD wError);

	//同步1
	int SendCmd_DeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& listInfo);


	void SetNetConnection(INetConnection* pCon);
	void UpdateDeviceRoomInfo(BYTE bType, LIST_DWORD& lstRoomID);
	void SendDeviceDeadLine(DeviceDeadLineInfo_t& tInfo);
	void SendDevicePropertyAnnounce(DWORD dwNoticeIndex);
	void SendDeviceAdvertInfo(DWORD dwAdvertIndex);
	void SendFirmwareRequest(DevUpgrad_t& tInfo);
	void SendCallIndoor(DWORD dwDeviceID, DWORD dwStatus);

	void ClearRooms(LIST_DWORD& lstRoomID);
	void UpdateDevice();
	void UpdateDeviceEx(int nType);
	void UpdateDevStat(DevStatus& tDevStat);

	void CacheDeviceStatus(DWORD dwUserID, DWORD dwStatus, PUCHAR pStatusMsg);
	void ClearDeviceStatus(DWORD dwUserID);

	// INetConnectionSink
	int OnConnect		(int nReason, INetConnection* pCon){ return 0; }
	int OnDisconnect	(int nReason, INetConnection* pCon);
	int OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSend			(INetConnection* pCon){ return 0; }
	int OnCommand		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnPeerIPChange	(DWORD dwPeerAddr, WORD wPort, INetConnection *pCon);

	// IDataBaseSink
	int OnGetDeviceInfo		(DeviceInfo_t& tInfo);
	int OnGetDeviceUserInfo	(DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo);
	int OnGetDevicePushInfo	(DWORD dwDeviceID, DWORD dwIndex, LIST_PUSHINFO& listInfo);
	int OnGetDeviceRoomSum	(DWORD dwDeviceID, LIST_ROOMSUM& lstInfo, WORD totalsegment, WORD subsegment);
	int OnGetDeviceRoomUser	(DWORD dwDeviceID, LIST_ROOMUSER& lstInfo, WORD totalsegment, WORD subsegment);
	int OnGetDeviceRoomPush	(DWORD dwDeviceID, LIST_ROOMPUSH& lstInfo, WORD totalsegment, WORD subsegment);
	int OnGetDeviceRoomCard	(DWORD dwDeviceID, LIST_ROOMCARD& lstInfo, WORD totalsegment, WORD subsegment);
	int OnGetDeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& lstInfo, WORD totalsegment, WORD subsegment);
	int OnGetDeviceRoomIndoor(DWORD dwDeviceID, LIST_ROOMINDOOR2& lstInfo, WORD totalsegment, WORD subsegment);

	int OnGetBulletinIndex	(DWORD dwNoticeIndex);
	int OnGetAdvertIndex	(DWORD dwAdvertIndex, DWORD dwAdvertType);
	int OnGetVisitorCfg(DWORD dwVisitorCfg);

	
	int OnGetDeviceCfg		(DWORD dwDeviceID, int nType, UcpaasInfo_t& tUcpaas);
	int OnGetSystemCfg		(DWORD dwVendorID, int nType, SystemCfg_t& tCfgInfo);
	int OnGetIndoorBindDevStatus(LIST_DEVSTATUS& lstDevStatus);

	bool ReportDeviceStatusRep(ReportDevStatus& devStatus);
	//IStorageDBSink
	int OnAddUnlockItem_Rep(UnlockRep_t& tInfo);

	int OnGetAdvertUrls_Rep(AdvertInfoRep_t& tAdvertRep);
	
	// IDealMulPktSink
	int OnSendNext(DWORD dwCmdFlag, PUCHAR pData, int nLen);

	int ReportAlarmStatus(AlarmStatus_t& alarmStatus);
#ifdef CACHE_DEVICE_STATUS
	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
#endif
private:
	int ProcessCommand		(PUCHAR pData, int nLen);
	int OnRegister			(PUCHAR pData, int nLen);
	int OnReportNetwork		(PUCHAR pData, int nLen);
	int OnReportDeviceStatus(PUCHAR pData, int nLen);
	int OnGetDeviceUserInfo	(PUCHAR pData, int nLen);
	int OnGetDevicePushInfo	(PUCHAR pData, int nLen);
	int OnGetRegisterInfo	(PUCHAR pData, int nLen);

	int OnGetDeviceRoomSum			(PUCHAR pData, int nLen);
	int OnGetDeviceRoomSumRepAck	(PUCHAR pData, int nLen);
	int OnGetDeviceRoomInfo			(PUCHAR pData, int nLen);
	int OnGetDeviceRoomInfoRepAck	(PUCHAR pData, int nLen);

	int OnSdkTunnel			(PUCHAR pData, int nLen);
	
	int OnQiniu_GetUploadToken	(PUCHAR pData, int nLen);
	int OnQiniu_GetDownloadUrls	(PUCHAR pData, int nLen);
	int OnQiniu_ReportUploadResult(PUCHAR pData, int nLen);
	int OnQiniu_ReportUploadResult2(PUCHAR pData, int nLen);//d_sdb
	int OnGetDeviceCfg(PUCHAR pData, int nLen);

	int OnGetServerTime(PUCHAR pData, int nLen);
	int OnReportUnlockLog(PUCHAR pData, int nLen);//开门记录
	int OnGetNoticeIndex(PUCHAR pData, int nLen); //公告
	
	int OnGetAdvertByID2(PUCHAR pData, int nLen);
	int OnReportDownloadProgress2(PUCHAR pData, int nLen);

	int OnGetAdvertIndex(PUCHAR pData, int nLen);//广告
	int OnGetVisitorCfg(PUCHAR pData, int nLen);//访客配置
	int OnGetBindDevStatus(PUCHAR pData, int nLen);//获取绑定设备状态
	int OnCallIndoorDev(PUCHAR pData, int nLen);

	int SendCmd_RegisterRep	(DWORD dwDserverConfigureIndex, WORD wError);
	int SendCmd_RegisterInfo(DWORD dwDeviceID, DWORD dwIndex, LIST_SERVERINFO& listInfo);
	int SendCmd_ServerTimeRep(DWORD wError);
	int SendCmd_DeviceUser	(DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo, WORD wError = 0);
	int SendCmd_DevicePush	(DWORD dwDeviceID, DWORD dwIndex, LIST_PUSHINFO& listInfo, WORD wError = 0);
	int SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg = 1, WORD wSubSeg = 1);

	void PacketHeader(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg = 1, WORD wSubSeg = 1);
	// 房间信息
	int SendCmd_DeviceRoomSum(DWORD dwDeviceID, LIST_ROOMSUM& listInfo);
	int SendCmd_DeviceRoomUser(DWORD dwDeviceID, LIST_ROOMUSER& listInfo);
	int SendCmd_DeviceRoomPush(DWORD dwDeviceID, LIST_ROOMPUSH& listInfo);
	int SendCmd_DeviceRoomCard(DWORD dwDeviceID, LIST_ROOMCARD& listInfo);
	int SendCmd_DeviceRoomIndoor(DWORD dwDeviceID, LIST_ROOMINDOOR2& listInfo);
	int SendCmd_DeviceRoomPushSwitch(DWORD dwDeviceID, LIST_ROOMPUSHSWITCH& listInfo);

	int SendCmd_DeleteRoom(DWORD dwDeviceID, LIST_DWORD& lstRoomID);
	int SendCmd_DeviceCfg(DWORD dwDeviceID, int nType, UcpaasInfo_t& tUcpaas, WORD wError = 0);
	int SendCmd_SystemCfg(DWORD dwVendorID, int nType, SystemCfg_t& tCfgInfo, WORD wError = 0);
	int SendCmd_IndoorBindDevStatus(LIST_DEVSTATUS& lstDevStatus);

	//门口机离线
	int GetBindIndoorID(DWORD dwDeviceID);

	int SendCmd_UnlockRecords(UnlockRep_t& tInfo);
	//////////////////////////////////////////////////////////////////////////
	typedef int (CDevice::*PMFHANDLER)(PUCHAR, int);
	struct HandlerEntry
	{
		BYTE bCommand;
		PMFHANDLER pmfHandler;
	};

	INetConnection* m_pCon;
	DWORD m_dwClientID;
	IDataBase* m_pDataBase;
	IStorageDB* m_pStorageDB;
	IStorageBusiness *m_pStorageB;
	PacketHeader_t m_tHeader;
	NetInfo_t m_tNetInfo;

	DeviceInfo_t m_tDeviceInfo;
	SystemCfg_t  m_tSystemInfo;
	WORD m_wHeaderVersion;

	static BYTE m_szBuffer[MAX_PACKET_LEN];
	static const HandlerEntry mHandlers[];

	DWORD m_dwCmdFlag; // 每个分片加1
	WORD m_wSegFlag;   // 每个完整的包加1

	// 房号信息列表
	typedef std::map<WORD, LIST_ROOMSUM>		MAP_ROOMSUM;
	typedef std::map<WORD, LIST_ROOMUSER>		MAP_ROOMUSER;
	typedef std::map<WORD, LIST_ROOMPUSH>		MAP_ROOMPUSH;
	typedef std::map<WORD, LIST_ROOMCARD>		MAP_ROOMCARD;
	typedef std::map<WORD, LIST_ROOMOTHER>		MAP_ROOMOTHER;
	typedef std::map<WORD, LIST_ROOMINDOOR2>	MAP_ROOMINDOOR;
	typedef std::map<WORD, LIST_ROOMPUSHSWITCH>	MAP_ROOMPUSHSWITCH;
	MAP_ROOMSUM m_mapDeviceRoomSum;
	MAP_ROOMUSER m_mapDeviceRoomUser;
	MAP_ROOMPUSH m_mapDeviceRoomPush;
	MAP_ROOMCARD m_mapDeviceRoomCard;
	MAP_ROOMOTHER m_mapDeviceRoomOther;
	MAP_ROOMINDOOR m_mapDeviceRoomIndoor;
	MAP_ROOMPUSHSWITCH m_mapRoomPushSwitch;

#ifdef CACHE_DEVICE_STATUS
	// 对不同的用户缓存设备状态1分钟
	struct CacheDeviceStatus_t
	{
		BYTE bTTL;
		DWORD dwStatus;
		BYTE szStatusMsg[LENGTH_MSGCONTENT+1];
	};
	typedef std::map<DWORD, CacheDeviceStatus_t>		MAP_DEVST;
	MAP_DEVST m_mapDevSt;// <dwUserID, CacheDeviceStatus_t>
#endif
};

#define DECLARE_PUTBUFFER( bufferPut ) \
	CPutBuffer bufferPut( m_szBuffer, MAX_PACKET_LEN ); \
	bufferPut.Skip ( PACKET_HEADER_SIZE );

struct OnOff_t
{
	CSTRING strSN;
	char cOnOff;
};
typedef std::list<OnOff_t> LIST_ONOFF;

#define PATH_NOTIFY_ONOFF_YSC "/home/dong_server/d/YunSC/YunSC_Notify.php"

class CDeviceMgr : public IDeviceHandle, 
				   public IConDispatcherSink,
				   public CThread,
				   public CElemMapDevice<CDevice>
#ifdef _TEST_STORAGE_
	, public ITimerSink
#endif
{
	DECLARE_SINGLETON(CDeviceMgr)
public:
	CDeviceMgr();
	~CDeviceMgr();

	bool Start();

	bool AddDevice(DWORD dwDeviceID, CDevice* pDevice);
	void DelDevice(DWORD dwDeviceID);

	void SetSink(IDeviceHandleSink* pSink){m_pSink = pSink;}
	IDeviceHandleSink* GetSink(){return m_pSink;}

	bool AddNotify_OnOff(PUCHAR pSN, char cOnOff);
	// CThread
	void ThreadLoop();

	// IConDispatcherSink
	int OnDispatchConnection(INetConnection* pCon, int nNetType, PUCHAR pData, int nLen);
	// IDeviceHandle
	// 获取设备状态
	void Dev_GetDeviceStatus(DWORD dwUserID, LIST_DWORD& listDeviceID, LIST_DEVICESTATUS& listInfo);
	//获取室内机绑定设备在线离线
	void Dev_GetDeviceStatus(LIST_DEVSTATUS& lstDevStatus);
	// 获取设备连接信息
	bool Dev_GetDeviceConnectInfo(TransClientInfo_t& tClientInfo, TransServerInfo_t& tServerInfo, bool bTransClientInfo, DWORD& dwAutoRelay);
	// 获取设备列表
	void Dev_GetDeviceList(LIST_DEVICESTATUS& listInfo);
	// 向设备发送对端地址信息
	void Dev_TransClientInfo(TransClientInfo_t& tClientInfo);
	virtual DWORD  Dev_GetDeviceType(DWORD dwDeviceID);
	virtual PUCHAR Dev_GetDeviceImgVer(DWORD dwDeviceID);

	void Dev_UpdateDeviceRoomInfo(DWORD dwVendorID, DWORD dwDeviceID, BYTE bType, LIST_DWORD& lstRoomID);
	void Dev_SendDeviceDeadLine(DeviceDeadLineInfo_t& tInfo);
	void Dev_SendPropertyAnnounce(DWORD dwNoticeIndex, LIST_DWORD& lstDevID);
	void Dev_SendAdvertInfo(LIST_ADVERT& lstDevAdvert);
	void Dev_SendFirmwareRequest(DevUpgrad_t& tInfo);
	void Dev_SendDeleteDeviceOnline(LIST_DWORD& lstDevID);
	void Dev_ClearRooms(LIST_DWORD& lstDeviceID);
	void Dev_UpdateDevice(LIST_DWORD& lstDeviceID);
	void Dev_UpdateDeviceEx(int nType, LIST_DWORD& lstDeviceID);
	void Dev_GetNoticeIndex(DWORD dwNoticeIndex);
	void Dev_GetDevBindIndoorID(DevStatus &tDevStat, LIST_DWORD& lstIndoorID);
	void Dev_CallIndoor(LIST_DWORD& lstIndoorID, DWORD dwStatus, DWORD dwDeviceID);
	void Dev_SmsSpecialCrowd(SmsInfo2_t& tInfo);
	void Dev_VideoAdvertUrl(DWORD dwDeviceID, AdvertInfoRep_t& tAdvertRep);
	void Dev_Capacity(int nCapacity);

	//同步1
	void Dev_SendDeviceRoomOther(DWORD dwDeviceID, LIST_ROOMOTHER& listDeviceRoomOther);
	
	// 设备模块保存报警状态
	void Dev_CacheDeviceStatus(DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg);
	// 透传消息到设备
	bool Dev_SdkTunnel(SdkTunnel_t& tInfo);
	// 上传凭证
	void Dev_Qiniu_UploadToken(StorageTag_t& tTag, PUCHAR pUploadToken);
	// 下载外链
	void Dev_Qiniu_DownloadUrls(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo);
#ifdef _TEST_STORAGE_
	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
	time_t m_tTime;
	int m_nSize;
#endif

private:
	void ToServerCapacity();	// 将在线设备数降到能力值以下
	void OfflineDevice();		// 强制设备下线
	void ProcessThreadCommand();
	bool ExecCommand(const char* pCommand);
	void InitPlatformVID();

private:
	PlatformVID_e m_ePVID;
	LIST_ONOFF m_listOnOffLine;

	DWORD m_dwClientID;
	IDeviceHandleSink* m_pSink;
	int m_nCapacity;
};