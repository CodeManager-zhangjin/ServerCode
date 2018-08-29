#ifndef STORAGE_BUSINESS_CLIENT_H
#define STORAGE_BUSINESS_CLIENT_H
#include "DataBaseInterface.h"
#include "DBClient.h"
#include "singleton.h"

class CStorageBusinessClient : public IStorageBusiness, public CDBClient
{
	DECLARE_SINGLETON(CStorageBusinessClient)
public:
	CStorageBusinessClient();
	virtual ~CStorageBusinessClient();

	void SetSink(IStorageBusinessStatusSink* pSink) { m_pSbSink = pSink; }

	// IStorageBusiness
	int GetAdvertInfo		(DWORD dwDeviceID,LIST_DWORD& lstAdvertID);
	int ReportAdvertProgress(DWORD dwDeviceID, LIST_PROGRESS& lstProgress);

};
#endif