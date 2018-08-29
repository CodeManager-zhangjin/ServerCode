#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Http.h"
#include "Log.h"

const int HTTP_TIMER = 3;
const int HTTP_TIMER_TIMEOUT = 20;

const int HTTP_CONNECT_FAILED = 1;
const int HTTP_CONNECTED = 2;
const int HTTP_ONRECEIVE = 3;

CHttp::CHttp(IHttpSink* pSink)
{
	m_dwHost = 0;
	m_wPort = 0;
	m_pHttpSink = pSink;
	m_pCon = NULL;
	m_nStep = 0;
	m_nTTL = 0;
}

CHttp::~CHttp()
{
	m_pHttpSink = NULL;
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	DelTimer(TIMER_NORMAL, this);
}

int CHttp::SetHttp(const char* pHost)
{
	m_dwHost = GetHostIP(pHost);
	m_wPort = 80;
	if (m_dwHost == 0) return -1;
	return 0;
}

int CHttp::SetHttp(const char* pHost, WORD wPort)
{
	m_dwHost = GetHostIP(pHost);
	m_wPort = wPort;
	if (m_dwHost == 0) return -1;
	return 0;
}

int CHttp::SetHttp(DWORD dwHost, WORD wPort)
{
	m_dwHost = dwHost;
	m_wPort = wPort;
	if (m_dwHost == 0) return -1;
	return 0;
}

int CHttp::HttpRequest( CSTRING& request )
{
	m_httpStr = request;
	AddTimer(TIMER_NORMAL, HTTP_TIMER, this);
	ConnectPeer();
	return 0;
}

bool CHttp::ConnectPeer()
{
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
	m_pCon = CreateRawTcpCon(this);
	LOG_ASSERT_RET(LOG_UTIL, m_pCon, false)
	LOG_ASSERT_RET(LOG_UTIL, m_dwHost, false)
	
	struct in_addr sin_addr;
	sin_addr.s_addr = htonl(m_dwHost);
	LOG_DEBUG(LOG_UTIL, "http Connecting %s:%d\n", inet_ntoa(sin_addr), m_wPort);
	m_pCon->Connect(m_dwHost, m_wPort, NETWORK_CONNECT_TYPE_TCP);
	return true;
}

int CHttp::OnConnect( int nReason, INetConnection* pCon )
{
	LOG_DEBUG(LOG_UTIL, "CHttp::OnConnect nReason = %d\n", nReason);
	if ( (pCon == NULL) || (m_pCon != pCon) ) return -1;
	if (nReason)
	{
		m_nStep = HTTP_CONNECT_FAILED;
		m_pHttpSink->OnHttpError(this, HTTP_ERROR_CONNECT);
		return -1;
	}
	if (m_httpStr.size() <= 0) return -1;
	m_nStep = HTTP_CONNECTED;
	int nWlen = m_pCon->SendData((PUCHAR)m_httpStr.c_str(), (int)m_httpStr.size());
	LOG_DEBUG(LOG_UTIL, "CHttp SendData nWlen %d RealLen %d\n", nWlen, m_httpStr.size());
	if (nWlen != 0)
	{
		LOG_DEBUG(LOG_UTIL, "CHttp SendData Failed\n");
	}
	return 0;
}

int CHttp::OnDisconnect( int nReason, INetConnection* pCon )
{
	return 0;
}

int CHttp::OnReceive( PUCHAR pData, int nLen, INetConnection* pCon )
{
	LOG_ASSERT_RET(LOG_UTIL, pData, -1)
	LOG_ASSERT_RET(LOG_UTIL, nLen > 0, -1)
	LOG_ASSERT_RET(LOG_UTIL, pCon == m_pCon, -1)
	LOG_ASSERT_RET(LOG_UTIL, m_pHttpSink, -1)
	m_pHttpSink->OnHttpResponse(this, pData, nLen);
	m_nStep = HTTP_ONRECEIVE;
	return 0;
}

void CHttp::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	LOG_ASSERT_RETVOID(LOG_UTIL, m_pHttpSink);
	m_nTTL += 3;
	if ( (m_nStep == HTTP_CONNECT_FAILED)	||	// 连接失败
		 ((m_nTTL >= 3) && (m_nStep < HTTP_CONNECT_FAILED)) ||	// 3秒连接段对端
		 ((m_nTTL >= 12) && (m_nStep < HTTP_CONNECTED))		// 12秒收到对端回应
		 )
	{
		ConnectPeer();
	}
	if (m_nTTL >= HTTP_TIMER_TIMEOUT)
	{
		m_pHttpSink->OnHttpError(this, HTTP_ERROR_TIMEOUT);
		DelTimer(TIMER_NORMAL, this);
	}
}

IMPLEMENT_SINGLETON(CHttpMgr)
