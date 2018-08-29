#include "Server.h"
#include "NetListenInterface.h"
#include "UtilityInterface.h"
#include "ServerAppInterface.h"
#include "ClientAppInterface.h"
#include "Log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../clientapp/Client.h"

IMPLEMENT_SINGLETON(CServer)
CServer::CServer()
{
	m_pDataBase = NULL;
	memset(&m_tCfgInfo, 0, sizeof(ServerCfg_t));
	m_dwWorkIP = 0;
	m_dwDBIP = 0;
	m_bOnce = false;
}

CServer::~CServer()
{
	Stop();
}

bool CServer::Run()
{
	NetworkInit();
	VigoLogInit((char*)LOG_DONG_LGN_FILE);
	UtilityInit();

	if (false == GetLgnServerCfg(m_tCfgInfo)) return false;
	m_dwDBIP = IpStr2Dword((char*)m_tCfgInfo.szDBIP);

	// 1、启动数据库客户端
	if (false == DataBaseModuleInit(m_dwDBIP, SERVER_PORT_DB, GROUPCODE_DB_LOGIN,
									(PUCHAR)m_tCfgInfo.szSerialNO, (PUCHAR)m_tCfgInfo.szUserName, (PUCHAR)m_tCfgInfo.szPassword, 
									this))
	{
		return false;
	}
	
	GetSmsLimitHandle();
	try {
		NetworkRunEventLoop();   
	} catch(...) {
		LOG_ERR(LOG_MAIN, "Exception in NetworkRunEventLoop!!!\n");
	}

	Stop();
	return true;
}

void CServer::Stop()
{
	ServerAppFinish();
	ClientAppFinish();
	if (m_pDataBase) { UnRegisterDataBase(m_pDataBase); m_pDataBase = NULL; }
	NetListenFinish();
	DataBaseModuleFinish();
	UtilityFinish();
	NetworkFini();
	VigoLogFinish();
}

const int DATABASE_STATUS_CONNECTED = 0;
const int DATABASE_STATUS_AUTH = 1;
const int DATABASE_STATUS_DISCONNECT = 2;
int CServer::OnDataBaseStatus( int nResult )
{
	if (nResult == DATABASE_STATUS_CONNECTED)
	{
		LOG_DEBUG(LOG_MAIN, "Connect DataBase Success!!!\n"); return 0;
	}
	if (nResult == DATABASE_STATUS_DISCONNECT)
	{
		LOG_ERR(LOG_MAIN, "DisConnect DataBase\n"); return -1;
	}
	LOG_DEBUG(LOG_MAIN, "DataBase Auth Success!!!\n");
	return 0;
}

int CServer::OnPieceOfSerialNO(LIST_PIECEOFSERIALNO& listInfo)
{
	IClientAppHandle* pHandle = ClientApp_GetHandle();
	LOG_DEBUG(LOG_MAIN, "%s listInfo size %d pHandle %p\n", __FUNCTION__, listInfo.size(), pHandle);
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->SetPieceOfSerialNO(listInfo);
	return 0;
}

int CServer::OnDserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo)
{
	LOG_DEBUG(LOG_MAIN, "%s dwVendorID %d dwConfigureIndex %d listserver size %d\n", __FUNCTION__, dwVendorID, dwConfigureIndex, listInfo.size());
	// 2、启动侦听
	if (false == m_bOnce)
	{
		m_bOnce = true;
		m_dwWorkIP = IpStr2Dword((char*)m_tCfgInfo.szWorkIP);
		if (false == NetListenInit(m_dwWorkIP, SERVER_PORT_LOGIN)) return -1;
		if (false == ServerAppInit((PUCHAR)m_tCfgInfo.szSerialNO, (PUCHAR)m_tCfgInfo.szUserName, (PUCHAR)m_tCfgInfo.szPassword)) return -1;
		if (false == ClientAppInit()) return -1;
		
		// 3、获取相关的服务器信息
		if (m_pDataBase == NULL) m_pDataBase = RegisterDataBase(this);
		LOG_ASSERT_RET(LOG_MAIN, m_pDataBase, -1);
		BYTE szServerType[2] = {0};
		szServerType[0] = SERVER_TYPE_NOTIFICATION;
//		szServerType[1] = SERVER_TYPE_STATUS;
		m_pDataBase->GetServerInfo((PUCHAR)szServerType);
	}

	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
	pHandle->SA_DserverConfigureIndex(dwVendorID, dwConfigureIndex, listInfo);
	return 0;
}

int CServer::OnGetServerInfo( LIST_SERVERINFO& listInfo )
{
	// 4、连接相关服务器
	RemoveSameServer(listInfo);
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	for (; iter != listInfo.end(); iter++)
	{
		IServerAppHandle* pHandle = ServerApp_GetHandle();
		LOG_ASSERT_RET(LOG_MAIN, pHandle, -1);
		pHandle->SA_AddServer(*iter, NULL);
	}
	return 0;
}

int CServer::OnDataBaseError( int nResult )
{
	LOG_DEBUG(LOG_MAIN, "DataBase Error %d\n", nResult); return 0;
}
