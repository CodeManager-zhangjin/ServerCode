#include "ddLogin.h"
#include "getbuffer.h"
#include "Log.h"


//#define DEBUG_DDLGN 1

const int STEP_ZERO = 0;
const int STEP_UDP_CONNECTING = 1;
const int STEP_UDP_CONNECTED = 2;
const int STEP_TCP_CONNECTING = 3;
const int STEP_TCP_CONNECTED = 4;

const int TIMER_LGN = 60;

const int TIMER_LGNCHECK = 24*3456;

const CDDLogin::HandleEntry_t CDDLogin::m_Handles[] =
{
	{ CMD_GET_DSERVER_INFO_REP,		&CDDLogin::OnGetDServerInfo	},
};

BYTE CDDLogin::m_szTempData[MAX_PACKET_LEN] = {0};
CDDLogin::CDDLogin()
{
	m_pCon = NULL;
	m_dwIP = 0;
	m_wPort = 0;
	m_nStep = STEP_ZERO;
	memset(&m_tHeader, 0, sizeof(PacketHeader_t));
	AddTimer(TIMER_NORMAL, TIMER_LGN, this);
}

CDDLogin::~CDDLogin()
{
	if (m_pCon)
	{
		NetworkDestroyConnection(m_pCon); m_pCon = NULL;
	}
	DelTimer(TIMER_NORMAL, this);
}

int CDDLogin::GetDServerInfo(PUCHAR pSN)
{
	DEFINE_PUTBUFFER(bufferPut)
	bufferPut << CByteArrayBuffer(pSN, LENGTH_SERIALNO);
	return SendPacket(bufferPut, CMD_GET_DSERVER_INFO);
}

int CDDLogin::SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError /* = 0 */, WORD wTotalSeg /* = 1 */, WORD wSubSeg /* = 1 */)
{
	if (NULL == m_pCon) return -1;
	int nLen = buffer.GetFilledSize();
	buffer.SetOffset(0);
	// Header
	buffer << (BYTE)GROUPCODE_LOGIN_CLIENT << (WORD)wCommand/*Command ID*/ << (BYTE)0/*Reserved0*/
		<< (WORD)0/*Version*/ << (WORD)0/*Reserved1*/
		<< (DWORD)m_tHeader.destinationid/*Source ID*/
		<< (DWORD)m_tHeader.sourceid/*Destination ID*/
		<< (DWORD)0/*Command Flag*/
		<< (WORD)wTotalSeg/*Total Segment*/ << (WORD)wSubSeg/*Sub Segment*/
		<< (WORD)0/*Segment Flag*/ << (WORD)0/*Reserved2*/
		<< (DWORD)0/*Reserved3*/;
	// Payload
	buffer << (WORD)wError/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;
	buffer.SetOffset(nLen);
	LOG_DEBUG(LOG_MAIN, "pCon %p SendData cmd:0x%04x err:0x%04x len:%d\n", m_pCon, wCommand, wError, nLen);
	return m_pCon->SendCommand(buffer, buffer.GetFilledSize());
}

int CDDLogin::OnDisconnect(int nReason, INetConnection* pCon)
{
	if (NULL == pCon) return -1;
	if (pCon != m_pCon) return -1;
	if (m_pCon)
	{
		NetworkDestroyConnection(m_pCon); m_pCon = NULL;
	}
	return 0;
}

int CDDLogin::OnReceive(PUCHAR pData, int nLen, INetConnection* pCon)
{
	return OnCommand(pData, nLen, pCon);
}

int CDDLogin::OnCommand(PUCHAR pData, int nLen, INetConnection* pCon)
{
//	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	if (false == ParsePacketHeader(pData, nLen, m_tHeader)) return -1;
	LOG_DEBUG(LOG_MAIN, "%s CommandID 0x%04x\n", __FUNCTION__, m_tHeader.commandid);
	int nHandlerCount = sizeof(m_Handles) / sizeof(m_Handles[0]);
	for (int i = 0; i < nHandlerCount; ++i)
	{
		if (m_tHeader.commandid == m_Handles[i].wCommand)
		{
			LgnServerHandler pHandler = m_Handles[i].pHandler;
			return (this->*pHandler)(pData + PACKET_HEADER_SIZE, nLen - PACKET_HEADER_SIZE, pCon);
		}
	}
//	LOG_DEBUG(LOG_MAIN, "%s Wrong CommandID 0x%04x\n", __FUNCTION__, m_tHeader.commandid);
	return -1;
}

int CDDLogin::OnGetDServerInfo( PUCHAR pData, int nLen, INetConnection* pCon )
{
//	LOG_DEBUG(LOG_MAIN, "%s nLen %d\n", __FUNCTION__, nLen);
	LOG_ASSERT_RET(LOG_MAIN, nLen >= 0, -1);

	int nNeedLen = LENGTH_SERIALNO + 2*sizeof(UINT);
	if (nLen < nNeedLen)
	{
		LOG_DEBUG(LOG_DB_CLIENT, "wrong packet len:%d needlen:%d\n", nLen, nNeedLen); return -1;
	}
	CGetBuffer bufferGet(pData, nLen);
	DServerInfo_t tInfo; memset(&tInfo, 0, sizeof(DServerInfo_t));
	bufferGet >> CByteArrayBuffer((PUCHAR)tInfo.szSerialNO, LENGTH_SERIALNO);
	bufferGet >> tInfo.nPermission >> tInfo.nCapacity;
#ifdef DEBUG_DDLGN
	LOG_DEBUG(LOG_MAIN, "DServerInfo sn:%s,premission:%d,capacity:%d\n", tInfo.szSerialNO, tInfo.nPermission, tInfo.nCapacity);
#endif
	
	IDDLoginSink* pSink = CDDLoginMgr::Instance()->GetSink();
	if (pSink) pSink->OnGetDServerInfo(tInfo);
}

int CDDLogin::OnConnect( int nReason, INetConnection* pCon )
{
#ifdef DEBUG_DDLGN
	LOG_DEBUG(LOG_MAIN, "%s nReason %d\n", __FUNCTION__, nReason);
#endif

	if (nReason == NETWORK_REASONSUCCESSFUL)
	{
		if (m_nStep == STEP_UDP_CONNECTING) m_nStep = STEP_UDP_CONNECTED;
		else if (m_nStep == STEP_TCP_CONNECTING) m_nStep = STEP_TCP_CONNECTED;

		return GetDServerInfo(CDDLoginMgr::Instance()->GetSerialNo());
	}

	if (m_pCon)
	{
		NetworkDestroyConnection(m_pCon); m_pCon = NULL;
	}
	if (m_nStep == STEP_UDP_CONNECTING) TcpConnect();
	return 0;
}

int CDDLogin::UdpConnect()
{
	if (m_pCon) return 0;
	m_pCon = CreateUdpMediaCon(this, NULL);
	if (NULL == m_pCon) return -1;
	m_pCon->Connect(m_dwIP, m_wPort, NETWORK_CONNECT_TYPE_UDP);
	m_nStep = STEP_UDP_CONNECTING;
#ifdef DEBUG_DDLGN
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
#endif
	return 0;
}

int CDDLogin::TcpConnect()
{
	if (m_pCon) return 0;
	m_pCon = CreateTcpCon(this);
	if (NULL == m_pCon) return -1;
	m_pCon->Connect(m_dwIP, m_wPort, NETWORK_CONNECT_TYPE_TCP);
	m_nStep = STEP_TCP_CONNECTING;
#ifdef DEBUG_DDLGN
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
#endif

	return 0;
}

int CDDLogin::ConnectLgn( DWORD dwIP, WORD wPort )
{
	m_dwIP = dwIP; m_wPort = wPort;
	return UdpConnect();
}

void CDDLogin::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
//	LOG_DEBUG(LOG_MAIN, "CDDLogin::%s\n", __FUNCTION__);
	CDDLoginMgr::Instance()->DelElem(this);
}

IMPLEMENT_SINGLETON(CDDLoginMgr)
CDDLoginMgr::CDDLoginMgr()
{
	m_pLoginSink = NULL;
	m_dwDD121IP = 0;
}

void CDDLoginMgr::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	if ( (m_dwDD121IP != 0) && (m_dwDD121IP != 0x7f000001) )
	{
		CDDLogin* pLogin = NULL;
		try
		{
			pLogin = new CDDLogin();
		}
		catch(std::bad_alloc &memExp)
		{
			LOG_DEBUG(LOG_MAIN, "new CDDLogin failed\n"); return;
		}
		CDDLoginMgr::Instance()->AddElem(pLogin);
#ifdef DEBUG_DDLGN
		LOG_DEBUG(LOG_MAIN, "CDDLogin cur count:%d\n", m_setElement.size());
#endif
		pLogin->ConnectLgn(m_dwDD121IP, SERVER_PORT_LOGIN);
	
		AddTimer(TIMER_NORMAL, CalcRandomInterval(), this);
	}
	ActivateThread();
}

int  CDDLoginMgr::CalcRandomInterval()
{
	srand( (int)time(0) );
	int nTimerLgnCheck = (int) (1.0*7*rand()/(RAND_MAX+1.0));
	nTimerLgnCheck = nTimerLgnCheck * TIMER_LGNCHECK;
	if(nTimerLgnCheck <= 0) nTimerLgnCheck = TIMER_LGNCHECK;
	return nTimerLgnCheck;
}
void CDDLoginMgr::SetSerialNo( PUCHAR pSN )
{
	memset(m_szDServerSN, 0, LENGTH_SERIALNO+1);
	memcpy(m_szDServerSN, pSN, LENGTH_SERIALNO);
}

int CDDLoginMgr::OnDispatchConnection( INetConnection* pCon, int nNetType, PUCHAR pData, int nLen )
{
	
	return 0;
}

bool CDDLoginMgr::Start(PUCHAR pSN)
{
	AddTimer(TIMER_NORMAL, CalcRandomInterval(), this);
	SetSerialNo(pSN);
	return ThreadStart();
}

void CDDLoginMgr::Stop()
{
	DelTimer(TIMER_NORMAL, this);
	ThreadStop();
}

void CDDLoginMgr::ThreadLoop()
{
	while (m_bRunning)
	{
		HangUpThread();

		DWORD dwDD121IP = GetHostIP("www.dd121.com");
		if ( (dwDD121IP != 0) && (dwDD121IP != 0x7f000001) )
		{
			m_dwDD121IP = dwDD121IP;
#ifdef DEBUG_DDLGN
			m_dwDD121IP = IpStr2Dword((char*)"192.168.68.55");
#endif
		}
#ifdef DEBUG_DDLGN
		LOG_DEBUG(LOG_MAIN, "CDDLoginMgr m_dwDD121IP %s\n", IpDword2Str(dwDD121IP));
#endif
	}
}
