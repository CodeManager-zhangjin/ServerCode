#ifndef STORAGE_DB_CLIENT_H
#define STORAGE_DB_CLIENT_H
#include "DataBaseInterface.h"
#include "DBClient.h"
#include "singleton.h"

class CStorageDBClient : public IStorageDB, public CDBClient
{
	DECLARE_SINGLETON(CStorageDBClient)
public:
	CStorageDBClient();
	virtual ~CStorageDBClient();

	void SetSink(IStorageDBStatusSink* pSink) { m_pSdbSink = pSink; }

	// IStorageDB
//	int SDB_AddUnlockItem(LIST_DEVICE_UNLOCK& listInfo);
	int SDB_UnlockItemTunel(PUCHAR pData, int nLen);
	int SDB_ReportUploadResult(PUCHAR pData, int nLen);
	int SDB_AlarmStatus(PUCHAR pData, int nLen);
};
#endif
