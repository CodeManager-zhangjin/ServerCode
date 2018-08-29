#include "ClientAppInterface.h"
#include "Client.h"

bool ClientAppInit()
{
	return CClientMgr::Instance()->Start();
}

void ClientAppFinish()
{
	CClientMgr::DeleteInstance();
}

IClientAppHandle* ClientApp_GetHandle()
{
	CClientMgr::Instance();
}
