#pragma once

#include "UtilityInterface.h"
#include "singleton.h"
#include "putbuffer.h"
#include "ElemSet_Webcmd.h"
#include "NetListenInterface.h"

class CWebCmd : public INetConnectionSink, public ITimerSink
{
public:
	CWebCmd();
	~CWebCmd();

	void SetNetConnection(INetConnection *pCon);

	// INetConnectionSink
	int OnConnect		(int nReason, INetConnection* pCon){ return 0; }
	int OnDisconnect	(int nReason, INetConnection* pCon);
	int OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSend			(INetConnection* pCon){ return 0; }
	int OnCommand		(PUCHAR pData, int nLen, INetConnection* pCon);

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
private:
	int OnPieceOfSerialNO		(CSTRING& strParam);
	int OnDserverConfigureIndex	(CSTRING& strParam);
	int OnUserConfigureIndex	(CSTRING& strParam);
	int OnUpdateDeviceRoom		(CSTRING& strParam);// 更新房间信息
	int OnClearRooms			(CSTRING& strParam);// 删除设备下所有房号
	int OnUpdateDevice			(CSTRING& strParam);// 更新设备下房间信息
	int OnUpdateDeviceEx		(CSTRING& strParam);
	int OnQueryDevice			(CSTRING& strParam);// 查询设备信息
	int OnUpdateDeviceDeadLine  (CSTRING& strParam);// 查询设备使用截至日期
	int OnGetQiNiuDownloadToken (CSTRING& strParam);// 获取七牛下载凭证
	int OnUpdatePropertyAnnounce(CSTRING& strParam);// 更新物業公告
	int OnUpdateFirmwareRequest (CSTRING& strParam);// 设备固件升级
	int OnDeleteDeviceOnline	(CSTRING& strParam);// 两台设备交换后让设备重新注册
	int OnUpdateAdvertInfo		(CSTRING& strParam);// 更新广告信息

	int AppendData(PUCHAR pData, int nLen);

	int ProcessCommand(CSTRING& strData);
	int SendResponse(bool bSuccessful);

	INetConnection* m_pCon;

	typedef int (CWebCmd::*PMFHANDLER)(CSTRING&);
	typedef struct tagHandlerEntry
	{
		PCHAR		pCmd;
		PMFHANDLER  pmfHandler;
	} HandlerEntry;
	static const HandlerEntry m_handlers[];
	static BYTE m_szBuffer[MAX_PACKET_LEN];

	CSTRING m_strBuffer;
	//
	typedef struct tagQiNiuToken
	{
		DWORD storeid;
		BYTE  downloadToken[32];
	}QiNiuToken;
};

class CCGICenter : public IConDispatcherSink, public CElemSet_Webcmd<CWebCmd>
{
	DECLARE_SINGLETON(CCGICenter)
public:
	CCGICenter();
	~CCGICenter();

	bool Start();

	// IConDispatcherSink
	int OnDispatchConnection(INetConnection* pCon, int nNetType, PUCHAR pData, int nLen);

};
