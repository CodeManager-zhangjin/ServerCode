#ifndef _CLIENTAPP_INTERFACE_H_
#define _CLIENTAPP_INTERFACE_H_

#include "DataStruct.h"

bool ClientAppInit();
void ClientAppFinish();

class IClientAppHandle
{
public:
	virtual int SetPieceOfSerialNO(LIST_PIECEOFSERIALNO& listInfo) = 0;
	virtual ~IClientAppHandle() {}
};

IClientAppHandle* ClientApp_GetHandle();

#endif
