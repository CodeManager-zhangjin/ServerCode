#include "ServerAppInterface.h"
#include "ServerAuth.h"
#include "ServerApp.h"
#include "Log.h"

bool ServerAppInit(PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword)
{
	if (false == CServerAuthMgr::Instance()->Start(pSN, pUserName, pPassword)) return false;
	return true;
}

void ServerAppFinish()
{
	CServerAuthMgr::DeleteInstance();
	CAppServerMgr::DeleteInstance();
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
}

IServerAppHandle* ServerApp_GetHandle()
{
	CAppServerMgr::Instance();
}

void ServerApp_SetSink( IServerAppHandleSink* pSink )
{
	CAppServerMgr::Instance()->SetSink(pSink);
}


