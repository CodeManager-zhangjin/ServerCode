#include "NetListenInterface.h"
#include "NetListen.h"

bool NetListenInit( DWORD dwWorkIP, WORD wWorkPort, bool bRelay /* = false */ )
{
	return CNetListen::Instance()->Init(dwWorkIP, wWorkPort, bRelay);
}

void NetListenFinish()
{
	CNetListen::DeleteInstance();
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
}

void RegisterNetListen(BYTE bGroupCode, IConDispatcherSink* pSink)
{
	CNetListen::Instance()->RegisterNetListen(bGroupCode, pSink);
}

void UnRegisterNetListen(BYTE bGroupCode, IConDispatcherSink* pSink)
{
	CNetListen::Instance()->UnRegisterNetListen(bGroupCode, pSink);
}

bool NetListenInitRawTcp( DWORD dwWorkIP, WORD wWorkPort )
{
	return CNetListen::Instance()->InitRawTcp(dwWorkIP, wWorkPort);
}
