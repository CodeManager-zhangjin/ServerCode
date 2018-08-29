#pragma once

#include "ElemSet_ServerApp.h"
#include "singleton.h"
#include "NetListenInterface.h"
#include "UtilityInterface.h"

// 预认证
class CServerAuth : public INetConnectionSink, public ITimerSink
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
	// 收到对方认证的摘要之后，向数据库进行查询，查询完成之后的回调
	int OnQuery_ServerInfo(LIST_SERVERINFO& listInfo);
private:
	int ProcessCommand(PUCHAR pData, int nLen, INetConnection* pCon);
	void SetAuthInfo(BYTE bServerType, PUCHAR pSN, PUCHAR pUserName, PUCHAR pDigist);
	int  SendChallengeRep();

	BYTE m_bServerType;
	INetConnection* m_pCon;
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

	bool Start();
	bool IsAlive(CServerAuth* pElem);
	
	// IConDispatcherSink
	int OnDispatchConnection(INetConnection* pCon, int nNetType, PUCHAR pData, int nLen);
private:
	int DestroyConnection(INetConnection* pCon);

};
