#pragma once

#include "ElemSet_ServerApp.h"
#include "singleton.h"
#include "NetListenInterface.h"
#include "UtilityInterface.h"
#include "DataBaseInterface.h"

class CServerAuth : public INetConnectionSink, public ITimerSink, public IDataBaseSink
{
public:
	CServerAuth();
	~CServerAuth();

	void SetNetConnection(INetConnection *pCon);
	void SetPacketHeader(PacketHeader_t& tHeader);

	// INetConnectionSink
	int OnConnect(int nReason, INetConnection *pCon){ return 0; }
	int OnDisconnect(int nReason, INetConnection *pCon);
	int OnReceive(unsigned char *pData, int nLen, INetConnection *pCon){ return 0; }
	int OnCommand(unsigned char *pData, int nLen, INetConnection *pCon);
	int OnSend(INetConnection *pCon){ return 0; }

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

	// CDataBaseSink
	int OnGetServerInfo(LIST_SERVERINFO& listInfo);
	int OnDataBaseError(int nResult);
private:
	void SetAuthInfo(BYTE bServerType, PUCHAR pSN, PUCHAR pUserName, PUCHAR pDigist);
	int  SendChallengeRep();

	BYTE m_bServerType;
	INetConnection* m_pCon;
	IDataBase* m_pDataBase;
	BYTE m_szSerialNO[LENGTH_SERIALNO+1];
	BYTE m_szUserName[LENGTH_NAME+1];
	BYTE m_szChallenge[LENGTH_CHALLENGE+1];
	BYTE m_szDigist[LENGTH_CHALLENGE+1];
	PacketHeader_t m_tHeader;
	static BYTE m_szBuffer[MAX_PACKET_LEN];
};

class CServerAuthMgr : public CElemSet_ServerApp<CServerAuth>, public IConDispatcherSink
{
	DECLARE_SINGLETON(CServerAuthMgr)
public:
	CServerAuthMgr();
	~CServerAuthMgr();

	bool Start(PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword); // 向对端服务器认证
	PUCHAR GetSN(){ return m_szSerialNO; }
	PUCHAR GetUserName(){ return m_szUserName; }
	PUCHAR GetPassword(){ return m_szPassword; }

	// IConDispatcherSink
	int OnDispatchConnection(INetConnection* pCon, int nNetType, PUCHAR pData, int nLen);
private:
	int DestroyConnection(INetConnection* pCon);
	BYTE m_szSerialNO[LENGTH_SERIALNO+1];
	BYTE m_szUserName[LENGTH_NAME+1];
	BYTE m_szPassword[LENGTH_PASSWORD+1];
};

