#ifndef _NET_LISTEN_H_
#define _NET_LISTEN_H_

#include "DataStruct.h"

class IConDispatcherSink
{
public:
	virtual int OnDispatchConnection(INetConnection* pCon, int nNetType, PUCHAR pData, int nLen) = 0;
	virtual ~IConDispatcherSink() {}
};

bool NetListenInit(DWORD dwWorkIP, WORD wWorkPort, bool bRelay = false);
void NetListenFinish();

bool NetListenInitRawTcp(DWORD dwWorkIP, WORD wWorkPort);

void RegisterNetListen(BYTE bGroupCode, IConDispatcherSink* pSink);
void UnRegisterNetListen(BYTE bGroupCode, IConDispatcherSink* pSink);

#endif
