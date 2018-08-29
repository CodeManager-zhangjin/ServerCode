#include "dbHandleInterface.h"
#include "dbHandle.h"
#include "dbHandle_storage.h"
#include "StorageEngine.h"
#if defined(DBSERVER_SB)
#include "dbHandle_operation.h"
#endif

bool DBHandleInit( PUCHAR pHost, PUCHAR pSrc, PUCHAR pUserName, PUCHAR pPassword )
{
	if (false == CDataBaseHandle::Instance()->Connect(pHost, pSrc, pUserName, pPassword)) return false;
	return true;
}

bool DBHandleInit_storage( PUCHAR pHost, PUCHAR pSrc, PUCHAR pUserName, PUCHAR pPassword )
{
	if (false == CStorageDataBaseHandle::Instance()->Connect(pHost, pSrc, pUserName, pPassword)) return false;
#if defined(DBSERVER_SDB)
	CStorageEngine::Instance()->EngineStart();
#endif
	return true;
}
#if defined(DBSERVER_SB)
bool DBHandleInit_operation(PUCHAR pHost, PUCHAR pSrc, PUCHAR pUserName, PUCHAR pPassword)
{
	if (false == COperationDataBaseHandle::Instance()->Connect(pHost, pSrc, pUserName, pPassword)) return false;
	return true;
}
#endif
void DBHandle_Timer()
{
	if (CDataBaseHandle::Instance()->IsConnect())
	{
		CDataBaseHandle::Instance()->OnTimer();
	}
	if (CStorageDataBaseHandle::Instance()->IsConnect())
	{
		CStorageDataBaseHandle::Instance()->OnTimer();
	}
#if defined(DBSERVER_SB)
	if (COperationDataBaseHandle::Instance()->IsConnect())
	{
		COperationDataBaseHandle::Instance()->OnTimer();
	}
#endif
}

void DBHandleFinish()
{
	CDataBaseHandle::DeleteInstance();
	CStorageEngine::DeleteInstance();
	CStorageDataBaseHandle::DeleteInstance();
#if defined(DBSERVER_SB)
	COperationDataBaseHandle::DeleteInstance();
#endif
}

IDBHandle* DBHandle_GetDBHandle()
{
	return CDataBaseHandle::Instance();
}

void DBHandle_SetSink( IDBHandleSink* pSink )
{
	CDataBaseHandle::Instance()->SetSink(pSink);
}

int DBHandle_GetError()
{
	return CDataBaseHandle::Instance()->GetError();
}

int DBHandle_storage_GetError()
{
	return CStorageDataBaseHandle::Instance()->GetError();
}

ISDBHandle* DBHandle_GetSDBHandle()
{
	return CStorageDataBaseHandle::Instance();
}
#if defined(DBSERVER_SB)
IOperationDBHandle* DBHandle_GetODBHandle()
{
	return COperationDataBaseHandle::Instance();
}
#endif
void SBHandle_SetSink( IOperationDBHandleSink* pSink )
{
#if defined(DBSERVER_SB)
	COperationDataBaseHandle::Instance()->SetSink(pSink);
#endif
}
