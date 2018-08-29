#pragma once

#include "ViewInterface.h"
#include "ElemMapView.h"
#include "singleton.h"
#include "NetListenInterface.h"
#include "DataBaseInterface.h"
#include "Protocol.h"
#include "putbuffer.h"

class CView : public INetConnectionSink, public IDataBaseSink
{
public:
	CView(DWORD dwSessionID);
	~CView();

	DWORD GetUserID() { return m_tUserInfo.dwUserID; }
	DWORD GetSessionID() { return m_dwSessionID; }
	PUCHAR GetToken() { return m_tGtPushInfo.szToken; }
	BYTE GetPushType() { return m_tGtPushInfo.bPushType; }
	void GetUserStatus(UserStatus_t& tStatus);
	void SetNetConnection(INetConnection* pCon, int nNetType);
	INetConnection* GetNetConnection();
	bool CompareToken(ClientTokenArray_t& tInfo);
	void SetPermissionPass(bool bPermissionPass) { m_bPermissionPass = bPermissionPass; }

	// INetConnectionSink
	int OnConnect		(int nReason, INetConnection* pCon){ return 0; }
	int OnDisconnect	(int nReason, INetConnection* pCon);
	int OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSend			(INetConnection* pCon){ return 0; }
	int OnCommand		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnPeerIPChange	(DWORD dwPeerAddr, WORD wPort, INetConnection *pCon);

	// IDataBaseSink
	int OnGetUserInfo		(UserInfo_t& tInfo, ClientTokenArray_t& tArray);
	int OnGetUserDeviceInfo	(DWORD dwUserID, DWORD dwIndex, LIST_DEVICEINFO& listInfo, MAP_DEVROOMINFO& mapDevRoomInfo);
	int OnGetUserGroupInfo	(DWORD dwUserID, DWORD dwIndex, LIST_GROUPINFO& listInfo);
	int OnGetUserRoomInfo	(DWORD dwUserID, DWORD dwIndex, LIST_ROOMINFO& listInfo);
	int OnGetIndoorBindDev	(LIST_DEVICEINFO& lstDevInfo);
	int OnGetDeviceUserInfo	(DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo);
	int OnAddDevice			(int nReason, PUCHAR pUser);
	int OnAddDelPushInfoEx	(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo);
	int OnSetDeviceName		(int nReason);
	int OnDelDevice			(int nReason, DWORD dwUserID, DWORD dwDeviceID);
	int OnAuthorize			(int nReason, DWORD dwUserID);
	int OnAuthorize2		(int nReason);
	int OnSetPushSwitch		(DWORD dwUserID, int nSwitch);
	//室内机
	int OnIndoorBindDevice_Rep(BindInfoRep_t& tBindInfo);
	int LoginGetDevList(PUCHAR pSN);

	int SendCmd_UserConfigureIndex	(DWORD dwVendorID, DWORD dwUserID, DWORD dwConfigureIndex);
	int SendCmd_DeviceStatus		(DWORD dwUserID, DWORD dwSessionID, LIST_DEVICESTATUS& listInfo);
	int SendCmd_DeviceStatus		(DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg);
	int SendCmd_TransServerInfo		(TransServerInfo_t& tInfo);
	int SendCmd_SetPushInfoEx	(WORD wError, BYTE bOpr, ClientTokenArray_t& tInfo);
	int SendCmd_SdlTunnelRep(SdkTunnel_t& tInfo);
	int SendCmd_Qiniu_DownloadUrlsRep(StorageTag_t& tTag, LIST_STORE_KEYURL& listInfo, WORD wError);
	int SendCmd_SetPushSwitch	(DWORD dwUserID, int nSwitch, WORD wError);
	//室内机
	int SendCmd_IndoorStatus(ReportDevStatus& devStatus);
	int SendCmd_DevStatus(LIST_DEVSTATUS& lstDevStatus);
	//室内机
	void SendCmd_IndoorBindDevice(BindInfoRep_t& tBindInfo);
private:
	int ProcessCommand		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnGetChallenge		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnLogin				(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnReportNetwork		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnGetUserDeviceInfo	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnGetUserGroupInfo	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnGetUserRoomInfo	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnAddDevice			(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnGetDeviceStatus	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnTransClientInfo	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnGetRegisterInfo	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSetDeviceName		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnDelDevice			(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnAuthorize			(PUCHAR pData, int nLen, INetConnection* pCon); // 授权
	int OnAuthorize2		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnGetDeviceUserInfo	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnAddDelPushInfo	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnAddDelPushInfoEx	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSdkTunnel			(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnQiniu_GetUploadToken	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnQiniu_GetDownloadUrls	(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnQiniu_GetDownloadUrls2(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSetPushSwitch		(PUCHAR pData, int nLen, INetConnection* pCon);
	// 室内机
	int OnIndoorBindDevice(PUCHAR pData, int nLen, INetConnection* pCon);


	int SendCmd_Challenge		();
	int SendCmd_AuthRep			(DWORD dwDserverConfigureIndex, WORD wError, PUCHAR pUrl, ClientTokenArray_t& tArray);
	int SendCmd_UserDevice		(DWORD dwUserID, DWORD dwIndex, LIST_DEVICEINFO& listInfo,MAP_DEVROOMINFO& mapDevRoomInfo, WORD wError = 0);
	int SendCmd_UserGroup		(DWORD dwUserID, DWORD dwIndex, LIST_GROUPINFO& listInfo, WORD wError = 0);
	int SendCmd_UserRoom		(DWORD dwUserID, DWORD dwIndex, LIST_ROOMINFO& listInfo, WORD wError = 0);
	int SendCmd_IndoorDev		(LIST_DEVICEINFO& lstDevInfo);
	int SendCmd_RegisterInfo	(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo, INetConnection* pCon);
	int SendCmd_DeviceUser		(DWORD dwDeviceID, DWORD dwIndex, LIST_SMSINFO& listInfo, WORD wError = 0);
	int SendCmd_Error			(WORD wCommand, WORD wError);
	int SendCmd_SetPushInfo		(WORD wError, BYTE bOpr, ClientTokenArray_t& tInfo);
	int SendPacket				(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg = 1, WORD wSubSeg = 1);

	void SetThirdPartyPushInfo(ClientTokenArray_t& tInfo);

	bool IsValidVendorID(DWORD dwVendorID);

private:
	bool IsTestAccount(PUCHAR pPhone);

	//////////////////////////////////////////////////////////////////////////
	typedef int (CView::*PMFHANDLER)(PUCHAR, int, INetConnection*);
	struct HandlerEntry
	{
		BYTE bCommand;
		PMFHANDLER pmfHandler;
	};

	DWORD m_dwSessionID;
	DWORD m_dwAppVendorID;
	INetConnection* m_pCon;
	IDataBase* m_pDataBase;
	PacketHeader_t m_tHeader;
	bool m_bAuth;
	NetInfo_t m_tNetInfo;
	DWORD m_dwClientVersion;
	BYTE m_cUserType;

	UserInfo_t m_tUserInfo;
	PushInfo_t m_tGtPushInfo;
	PushInfo_t m_tBaiduPushInfo;
	PushInfo_t m_tHWPushInfo;
	PushInfo_t m_tMIPushInfo;
	PushInfo_t m_tJGPushInfo;
	PushInfo_t m_tMZPushInfo;
	BYTE m_szChallenge[LENGTH_CHALLENGE+1];
	BYTE m_szDigist[LENGTH_CHALLENGE+1];
	bool m_bPermissionPass;
	static BYTE m_szBuffer[MAX_PACKET_LEN];
	static const HandlerEntry mHandlers[];

	BYTE szIndoorSN[LENGTH_NAME+1];
};

#define DECLARE_PUTBUFFER( bufferPut ) \
	CPutBuffer bufferPut( m_szBuffer, MAX_PACKET_LEN ); \
	bufferPut.Skip ( PACKET_HEADER_SIZE );

extern DWORD g_dwUserID;
extern CSTRING g_token;
extern BYTE g_bPushType;

class FindViewByToken
{
public:
	bool operator() (LIST_PUSHINFO::value_type& pos)
	{
		if ( (0 == g_token.compare((const char*)pos.szToken)) && (g_dwUserID == pos.dwUserID) ) return true;
		return false;
	}
};

class FindViewByToken2
{
public:
	bool operator() (LIST_PUSHINFO::value_type& pos)
	{
		if ( g_bPushType != pos.bPushType ) return false;
		if ( g_token.compare((const char*)pos.szToken) ) return false;
		return true;
	}
};

class CViewMgr : public IViewHandle, public IConDispatcherSink, public CElemMapView<CView>
{
	DECLARE_SINGLETON(CViewMgr)
public:
	CViewMgr();
	~CViewMgr();

	bool Start();

	IViewHandleSink* GetSink() { return m_pSink; }
	void SetSink(IViewHandleSink* pSink) { m_pSink = pSink; }

	void AddViewSession(DWORD dwUserID, DWORD dwSessionID, CView* pView);
	void DelViewSession(DWORD dwUserID, DWORD dwSessionID, CView* pView);

	// IConDispatcherSink
	int OnDispatchConnection(INetConnection* pCon, int nNetType, PUCHAR pData, int nLen);
	// IViewHandle
	// 主动上报最新列表
	void View_UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID, DWORD dwConfigureIndex);
	// 主动上报设备状态
	void View_ReportDeviceStatus(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2);
	// 设备状态回应
	void View_GetDeviceStatusRep(GetDeviceStatusRep_t& tInfo);
	// 观看请求回应
	void View_TransServerInfo(TransServerInfo_t& tInfo);
	// 获取客户端列表
	void View_GetUserList(LIST_USERSTATUS& listInfo);

	// 提示其他Token用户账号在其他位置登录
	void View_LoginOtherPlace(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo);

	void View_Permission(int nPermission);
	void View_SdkTunnelRep(SdkTunnel_t& tInfo);
	// 上传凭证
	void View_Qiniu_UploadToken(StorageTag_t& tTag, PUCHAR pUploadToken);
	// 下载外链
	void View_Qiniu_DownloadUrls(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo);
	//室内机
	void View_ReportIndoorStatus(DWORD dwUserID, ReportDevStatus& devStatus);

private:
	void ReportDeviceStatus(DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg);

	//bool CheckSameView(CView* pView, CView* pPreView);
	//CView* GetSameView(DWORD dwUserID, DWORD dwPreSession, CView* pView);

	DWORD m_dwSessionID;
	IViewHandleSink* m_pSink;

	int m_nPermission;
};
