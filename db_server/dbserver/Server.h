#pragma once

#include "DataStruct.h"
#include "singleton.h"

class CServer
{
	DECLARE_SINGLETON(CServer)
public:
	CServer();
	~CServer();

	bool Run();
private:
	void Stop();
	
	ServerCfg_t m_tCfgInfo;
	DWORD m_dwWorkIP;
};
