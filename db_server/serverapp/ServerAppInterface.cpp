#include "ServerAppInterface.h"
#include "ServerAuth.h"
#include "ServerApp.h"

bool ServerAppInit(WORD wRawUdpPort)
{
	if (false == CServerAuthMgr::Instance()->Start()) return false;
	if (false == CAppServerMgr::Instance()->Start(wRawUdpPort)) return false;
	return true;
}

void ServerAppFinish()
{
	CServerAuthMgr::DeleteInstance();
	CAppServerMgr::DeleteInstance();
}

IServerAppHandle* ServerApp_GetHandle()
{
	CAppServerMgr::Instance();
}
