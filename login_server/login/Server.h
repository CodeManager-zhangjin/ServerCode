#pragma once

#include "DataBaseInterface.h"
#include "singleton.h"

class CServer : public IDataBaseStatusSink, public IDataBaseSink
{
	DECLARE_SINGLETON(CServer)
public:
	CServer();
	~CServer();

	bool Run();

	// IDataBaseStatusSink
	int OnDataBaseStatus(int nResult);
	int OnPieceOfSerialNO		(LIST_PIECEOFSERIALNO& listInfo);
	int OnDserverConfigureIndex	(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo);

	// CDataBaseSink
	int OnGetServerInfo(LIST_SERVERINFO& listInfo);
	int OnDataBaseError(int nResult);
private:
	void Stop();
	
	IDataBase* m_pDataBase;
	ServerCfg_t m_tCfgInfo;
	DWORD m_dwWorkIP, m_dwDBIP;
	bool m_bOnce;
};
