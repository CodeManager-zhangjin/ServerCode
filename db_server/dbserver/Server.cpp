#include "Server.h"
#include "NetListenInterface.h"
#include "UtilityInterface.h"
#include "ServerAppInterface.h"
#include "dbHandleInterface.h"
#include "Log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(DBSERVER_DB)
#include "WebcmdInterface.h"
#endif

IMPLEMENT_SINGLETON(CServer)
CServer::CServer()
{
	memset(&m_tCfgInfo, 0, sizeof(ServerCfg_t));
	m_dwWorkIP = 0;
}

CServer::~CServer()
{
	Stop();
}

bool CServer::Run()
{
	NetworkInit();
#if defined(DBSERVER_DB)
	VigoLogInit((char*)LOG_DONG_DB_FILE);
#elif defined(DBSERVER_SDB)
	VigoLogInit((char*)LOG_DONG_STORAGE_DB_FILE);
#endif
	UtilityInit();

#if defined(DBSERVER_DB)
	if (false == GetDBServerCfg(m_tCfgInfo)) return false;
#elif defined(DBSERVER_SDB)
	if (false == GetSDBServerCfg(m_tCfgInfo)) return false;
#endif
	LOG_DEBUG(LOG_DB_CLIENT,"%s DBIP %d, DSvrIP %d, Host %s, Passwd %s SerNo %s, SrcDB %s SrcDB2 %s Switch %s UserName %s WorkIP %d\n",
		__FUNCTION__, m_tCfgInfo.szDBIP, m_tCfgInfo.szDServerIP, m_tCfgInfo.szHost, m_tCfgInfo.szPassword, m_tCfgInfo.szSerialNO,m_tCfgInfo.szSrcDB, m_tCfgInfo.szSrcDB2, m_tCfgInfo.szSwitch, m_tCfgInfo.szUserName, m_tCfgInfo.szWorkIP);

	if( strlen(m_tCfgInfo.szWorkIP) > 0 )
	{
		m_dwWorkIP = inet_addr(m_tCfgInfo.szWorkIP);
		if(INADDR_NONE == m_dwWorkIP) {
			LOG_WARN(LOG_DB_SERVER, "Invalid WorkIP in config\n");
			m_dwWorkIP = 0;
		} else {
			m_dwWorkIP = ntohl(m_dwWorkIP);
		}
	}

	if (false == DBHandleInit((PUCHAR)m_tCfgInfo.szHost, (PUCHAR)m_tCfgInfo.szSrcDB, (PUCHAR)m_tCfgInfo.szUserName, (PUCHAR)m_tCfgInfo.szPassword)) return false;
#if defined(DBSERVER_SDB)
	if (false == DBHandleInit_storage((PUCHAR)m_tCfgInfo.szHost, (PUCHAR)m_tCfgInfo.szSrcDB2, (PUCHAR)m_tCfgInfo.szUserName, (PUCHAR)m_tCfgInfo.szPassword)) return false;
	if (false == NetListenInit(m_dwWorkIP, SERVER_PORT_SDB)) return false;
#elif defined(DBSERVER_DB)
	if (false == NetListenInit(m_dwWorkIP, SERVER_PORT_DB)) return false;
	if (false == NetListenInitRawTcp(m_dwWorkIP, SERVER_PORT_WEB_DB)) return false;
#endif


#if defined(DBSERVER_SDB)
	if (false == ServerAppInit(SERVER_PORT_SDB_LOCAL)) return false;
#elif defined(DBSERVER_DB)
	if (false == ServerAppInit(SERVER_PORT_DB_LOCAL)) return false;
#endif

#if defined(DBSERVER_DB)
	if (false == WebcmdInit()) return false;
#endif

	try {
		NetworkRunEventLoop();   
	} catch(...) {
		LOG_ERR(LOG_DB_SERVER, "Exception in NetworkRunEventLoop!!!\n");
	}

	Stop();
	return true;
}

void CServer::Stop()
{
#if defined(DBSERVER_DB)
	WebcmdFinish();
#endif
	DBHandleFinish();
	ServerAppFinish();
	NetListenFinish();
	UtilityFinish();
	NetworkFini();
	VigoLogFinish();
}
