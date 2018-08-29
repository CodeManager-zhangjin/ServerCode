#pragma once

#include "ServerAppInterface.h"
#include "ServerSend.h"
#include "ElemMap_ServerApp.h"
#include "singleton.h"
#include "UtilityInterface.h"
#if defined(SERVERAPP_NOTIFY)
#include "NotifyInterface.h"
#elif defined(SERVERAPP_STORAGE_BUSINESS)
#include "DataBaseInterface.h"
#endif

class CAppServer : public CServerSend, public INetConnectionSink, public ITimerSink
//#if defined(SERVERAPP_STORAGE_BUSINESS)
//	, public IDataBaseSink
//#endif
{
public:
	CAppServer();
	~CAppServer();

	BYTE GetServerType(){ return m_tBaseInfo.bServerType; }
	DWORD GetServerVendorID(){ return m_tBaseInfo.dwVendorID; }
	bool IsServerOnline(){ return m_bAuth; }
	void GetServerInfo(ServerInfo_t& tInfo);

	void SetWorkPort(WORD wPort) { m_wWorkPort = wPort; }
	void SetServerInfo( ServerInfo_t& tInfo );

	// 主动连接：启动定时器，定时重连
	int Start(ServerInfo_t& tInfo);

	// 被动连接
	void SetNetConnection(INetConnection* pCon);

	// INetConnectionSink
	int OnConnect		(int nReason, INetConnection* pCon);
	int OnDisconnect	(int nReason, INetConnection* pCon);
	int OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSend			(INetConnection* pCon){ return 0; }
	int OnCommand		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnPeerIPChange	(DWORD dwPeerAddr, WORD wPort, INetConnection *pCon);

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

#if defined(SERVERAPP_STORAGE_BUSINESS)
	int OnQiniu_GetStorageAccount(StorageTag_t& tTag, StorageAccount_t& tInfo);
	int OnQiniu_GetStorageKeys(StorageTag_t& tTag, LIST_STORE_ACCOUNTKEYS& lstAccountKeys);
#endif

private:
	int ProcessCommand		(PUCHAR pData, int nLen);
	int OnChallenge			(PUCHAR pData, int nLen);
	int OnAuth				(PUCHAR pData, int nLen);
	int OnSms				(PUCHAR pData, int nLen);
	int OnPush				(PUCHAR pData, int nLen);
	int OnReportDeviceStatus(PUCHAR pData, int nLen);
	int OnReportUserStatus	(PUCHAR pData, int nLen);
	int OnGetDeviceStatus	(PUCHAR pData, int nLen);
	int OnGetDeviceStatusRep(PUCHAR pData, int nLen);
	int OnTransClientInfo	(PUCHAR pData, int nLen);
	int OnTransServerInfo	(PUCHAR pData, int nLen);
	int OnAddTempUser		(PUCHAR pData, int nLen);
	int OnReportNetwork		(PUCHAR pData, int nLen);
	int OnSdkTunnel			(PUCHAR pData, int nLen);
	int OnSdkTunnelRep		(PUCHAR pData, int nLen);
	int OnRepoartAlarmStatus(PUCHAR pData, int nLen);
	int OnQiniu_GetUploadToken		(PUCHAR pData, int nLen);
	int OnQiniu_GetDownloadUrls		(PUCHAR pData, int nLen);
	int OnQiniu_GetDownloadUrls2	(PUCHAR pData, int nLen);
	int OnQiniu_GetUploadTokenRep	(PUCHAR pData, int nLen);
	int OnQiniu_GetDownloadUrlsRep	(PUCHAR pData, int nLen);
	int OnQiniu_ReportUploadResult	(PUCHAR pData, int nLen);
	int OnReportUnlockLog	(PUCHAR pData, int nLen);

	//////////////////////////////////////////////////////////////////////////
	typedef int (CAppServer::*PMFHANDLER)(PUCHAR, int);
	struct HandlerEntry
	{
		BYTE bCommand;
		PMFHANDLER pmfHandler;
	};
	//////////////////////////////////////////////////////////////////////////
	WORD m_wWorkPort;
	bool m_bAuth;
	NetInfo_t m_tNetInfo;
	int m_nTimeout;
	static const HandlerEntry mHandlers[];

//#if defined(SERVERAPP_STORAGE_BUSINESS)
//	IDataBase* m_pDataBase;
//#endif
};

class CAppServerMgr : public IServerAppHandle, public CElemMap_ServerApp<CAppServer>
{
	DECLARE_SINGLETON( CAppServerMgr )
public:
	CAppServerMgr();
	~CAppServerMgr(){}

	void SetSink(IServerAppHandleSink* pSink){ m_pSink = pSink; }
	IServerAppHandleSink* GetSink(){ return m_pSink; }
#if defined(SERVERAPP_STATUS)
	// 上报设备状态到DServer
	bool ReportDeviceStatus_D	(DWORD dwServerID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2);
	// 向Dserver请求设备状态
	bool GetDeviceStatus_D		(DWORD dwSrcServerID, DWORD dwDstServerID, DWORD dwUserID, DWORD dwSessionID, LIST_DWORD& lstDeviceID);
	// 向Dserver上报设备状态回应
	bool GetDeviceStatusRep_D	(DWORD dwDstServerID, DWORD dwUserID, DWORD dwSessionID, LIST_DEVICESTATUS& lstInfo);
#endif
	bool SA_AddServer				(ServerInfo_t& tInfo, INetConnection* pCon);
	bool SA_GetDServers				(DWORD& dwVendorID, DWORD& dwConfigureIndex, LIST_SERVERINFO& listInfo);
	bool SA_DserverConfigureIndex	(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo);

	bool SA_Sms_Notification		(PUCHAR pMsg, LIST_SMSINFO& listSmsInfo, DWORD dwClientID);
	bool SA_Push_Notification		(DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, DWORD dwDeviceID, LIST_PUSHINFO& listToken);

	// 向状态服务器上报设备状态
	bool SA_ReportDeviceStatus_Status	(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2);
	// 向状态服务器上报客户端状态
	bool SA_ReportUserStatus_Status		(UserStatus_t& tInfo);
	// 回应状态服务器观看信息
	bool SA_TransServerInfo_Status		(TransServerInfo_t& tInfo);
	// 回应转发服务器观看信息
	bool SA_TransServerInfo_Relay		(TransServerInfo_t& tInfo, DWORD dwServerID);
	// 向状态服务器获取设备状态
	bool SA_GetDeviceStatus_Status		(GetDeviceStatus_t& tInfo);
	// 向状态服务器上报获取设备状态回应
	bool SA_GetDeviceStatusRep_Status	(DWORD dwServerID, GetDeviceStatusRep_t& tInfo);
	// 设备在当前注册服务器上注册，通过转发服务器观看
	bool SA_AddTempUser_Relay			(TransClientInfo_t& tInfo, bool bRelay, DWORD dwAutoRelay);
	// 设备未在当前注册服务器上注册，通过状态服务器透传消息
	bool SA_TransClientInfo_Status		(TransClientInfo_t& tInfo, bool bRelay);
	
	bool SA_SdkTunnel_Status(SdkTunnel_t& tInfo);
	bool SA_SdkTunnelRep_Status(SdkTunnel_t& tInfo);

	bool SA_ReportAlarmStatus(AlarmStatus_t& alarmStatus);
	bool SA_ReportUploadResult_Storage(StoreKey_t& tKey);
	bool SA_GetUploadToken_Storage(StorageTag_t& tTag);
	bool SA_GetDownloadUrls_Storage(StorageTag_t& tTag, StoreKey_t& tKey);
	bool SA_GetDownloadUrls_Storage2(StorageTag_t& tTag, StoreVisitor_t& tVisitor);

//	int Qiniu_GetStorageAccount(StorageTag_t& tTag){return 0;}
//	int RepoartAlarmStatus(AlarmStatus_t& alarmStatus);
//	int Qiniu_GetStorageKeys(StorageTag_t& tTag, StoreKey_t& tKey){return 0;}
//	int Qiniu_GetStorageKeys2(StorageTag_t& tTag, StoreVisitor_t& tVisitor){return 0;}
//	int Qiniu_ReportUploadResult(StoreKey_t& tKey){return 0;}

	
private:
	CAppServer* GetServer(BYTE bType);
	CAppServer* GetServer(DWORD dwServerID);

	IServerAppHandleSink* m_pSink;

	MAP_DWORD m_mapServerIndex;
	typedef std::map<DWORD, LIST_SERVERINFO>	MAP_SERVERINFO;
	MAP_SERVERINFO m_mapServerInfo;
};
