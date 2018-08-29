#pragma once

#include "ClientAppInterface.h"
#include "ServerAppInterface.h"
#include "ElemMap_ClientApp.h"
#include "singleton.h"
#include "NetListenInterface.h"
#include "DataBaseInterface.h"
#include "Protocol.h"
#include "putbuffer.h"
#include "UtilityInterface.h"

//////////////////////////////////////////////////////////////////////////
struct  SmsLimit_t
{
	DWORD dwTTL; 
	DWORD dwCount; 
	DWORD dwTimestamp;
};
typedef std::map<CSTRING, SmsLimit_t>	MAP_SMS_LIMIT;

class  SmsLimit : public ITimerSink
{
	DECLARE_SINGLETON(SmsLimit)
public:
	SmsLimit();
	~SmsLimit();
	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
	MAP_SMS_LIMIT m_mapSmsLimit;
};
SmsLimit* GetSmsLimitHandle();
//////////////////////////////////////////////////////////////////////////

extern DWORD g_dwCameraID;
class FindVendorIDByCameraID
{
public:
	bool operator() (LIST_PIECEOFSERIALNO::value_type& pos)
	{
		if ( (g_dwCameraID >= pos.dwBegin) && (g_dwCameraID <= pos.dwEnd) ) return true;
		return false;
	}
};

class CClient : public INetConnectionSink, public IDataBaseSink
{
public:
	CClient(DWORD dwClientID);
	~CClient();

	void SetNetConnection(INetConnection* pCon);

	void OnSmsRepsonse(int nReason);

	// INetConnectionSink
	int OnConnect		(int nReason, INetConnection* pCon){ return 0; }
	int OnDisconnect	(int nReason, INetConnection* pCon);
	int OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSend			(INetConnection* pCon){ return 0; }
	int OnCommand		(PUCHAR pData, int nLen, INetConnection* pCon);
	
	// CDataBaseSink
	int OnQueryMobilePhone	(PUCHAR pMobilePhone, int nReason);
	int OnSetSecret			(int nReason);
	int OnGetDServerInfo	(DServerInfo_t& tInfo);

private:
	int ProcessCommand(PUCHAR pData, int nLen);
	int OnGetRegisterInfo	(PUCHAR pData, int nLen);	
	int OnQueryUser			(PUCHAR pData, int nLen);
	int OnSmsAuth			(PUCHAR pData, int nLen);
	int OnSetSecret			(PUCHAR pData, int nLen);
	int OnGetDServerInfo	(PUCHAR pData, int nLen);	

	int SendCmd_RegisterInfo(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo);
	int SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg = 1, WORD wSubSeg = 1);

	//////////////////////////////////////////////////////////////////////////
	typedef int (CClient::*PMFHANDLER)(PUCHAR, int);
	struct HandlerEntry
	{
		BYTE bCommand;
		PMFHANDLER pmfHandler;
	};

	INetConnection* m_pCon;
	DWORD m_dwClientID;
	IDataBase* m_pDataBase;
	PacketHeader_t m_tHeader;
	static BYTE m_szBuffer[MAX_PACKET_LEN];
	static const HandlerEntry mHandlers[];
};

class CClientMgr : public IClientAppHandle, public CElemMap_ClientApp<CClient>, public IConDispatcherSink, public IServerAppHandleSink
{
	DECLARE_SINGLETON(CClientMgr)
public:
	CClientMgr();
	~CClientMgr();

	// IClientAppHandle
	int SetPieceOfSerialNO(LIST_PIECEOFSERIALNO& listInfo);

	// IConDispatcherSink
	int OnDispatchConnection(INetConnection* pCon, int nNetType, PUCHAR pData, int nLen);
	
	void SA_OnSmsRepsonse_Notify(DWORD dwClientID, int nReason);

	bool Start();
	DWORD CalcVendorID(DWORD dwCameraID);
private:
	DWORD m_dwClientID;
	LIST_PIECEOFSERIALNO m_listPieceOfSerialNO;
};

#define DECLARE_PUTBUFFER( bufferPut ) \
	CPutBuffer bufferPut( m_szBuffer, MAX_PACKET_LEN ); \
	bufferPut.Skip ( PACKET_HEADER_SIZE );
