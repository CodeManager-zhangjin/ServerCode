#include "MainDBClient.h"
#include "StorageDBClient.h"
#include "StorageBusinessClient.h"
#include "DataBaseA.h"
#include "DataBaseB.h"
#include "DataBaseC.h"
#include "CGetUserInfoMgr.h"
#include "CGetUserGroupMgr.h"
#include "CGetUserDeviceMgr.h"

DWORD g_dwDataBaseID = 0;
IDataBase* RegisterDataBase(IDataBaseSink* pSink)
{
	CDataBaseA* p = NULL;
	try
	{
		p = new CDataBaseA(++g_dwDataBaseID, pSink);
	}
	catch(std::bad_alloc &memExp)
	{
		printf("new CDataBaseA failed\n"); return NULL;
	}
	CDataBaseAMgr::Instance()->AddElem(g_dwDataBaseID, p);
	return p;
}

void UnRegisterDataBase(IDataBase* pDataBase)
{
	if (NULL == pDataBase) return;
	CDataBaseA* p = (CDataBaseA*)pDataBase;
	CDataBaseAMgr::Instance()->DelElem(p->GetID());
}

bool DataBaseModuleInit(DWORD dwIP, WORD wPort,
						BYTE bGroupCode,
						PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword,
						IDataBaseStatusSink* pSink
						)
{
	CMainDBClient::Instance()->SetSink(pSink);
	CMainDBClient::Instance()->ClientInit(dwIP, wPort, bGroupCode, pSN, pUserName, pPassword);
	return true;
}

//////////////////////////////////////////////////////////////////////////
IStorageDB*	RegStorageDB(IStorageDBSink* pSink)
{
	CDataBaseB* p = NULL;
	try
	{
		p = new CDataBaseB(++g_dwDataBaseID, pSink);
	}
	catch(std::bad_alloc &memExp)
	{
		printf("new CDataBaseB failed\n"); return NULL;
	}
	CDataBaseBMgr::Instance()->AddElem(g_dwDataBaseID, p);
	return p;
}
void		UnRegStorageDB(IStorageDB* pDataBase)
{
	if (NULL == pDataBase) return;
	CDataBaseB* p = (CDataBaseB*)pDataBase;
	CDataBaseBMgr::Instance()->DelElem(p->GetID());
}
bool StorageDBInit(DWORD dwIP, WORD wPort, BYTE bGroupCode, 
				   PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword, 
				   IStorageDBStatusSink* pSink)
{
	CStorageDBClient::Instance()->SetSink(pSink);
	CStorageDBClient::Instance()->ClientInit(dwIP, wPort, bGroupCode, pSN, pUserName, pPassword);
	return true;
}
//////////////////////////////////////////////////////////////////////////sb
IStorageBusiness* RegStorageBusiness(IStorageBusinessSink* pSink)
{
	CDataBaseC* p = NULL;
	try
	{
		p = new CDataBaseC(++g_dwDataBaseID, pSink);
	}
	catch(std::bad_alloc &memExp)
	{
		printf("new CDataBaseA failed\n"); return NULL;
	}
	CDataBaseCMgr::Instance()->AddElem(g_dwDataBaseID, p);
	return p;
}

void UnRegStorageBusiness(IStorageBusiness* pDataBase)
{
	if (NULL == pDataBase) return;
	CDataBaseC* p = (CDataBaseC*)pDataBase;
	CDataBaseCMgr::Instance()->DelElem(p->GetID());
}

bool StorageBusinessInit(DWORD dwIP, WORD wPort,
	BYTE bGroupCode,
	PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword,
	IStorageBusinessStatusSink* pSink
	)
{
	CStorageBusinessClient::Instance()->SetSink(pSink);
	CStorageBusinessClient::Instance()->ClientInit(dwIP, wPort, bGroupCode, pSN, pUserName, pPassword);
	return true;
}

//////////////////////////////////////////////////////////////////////////
void DataBaseModuleFinish()
{
	CDataBaseAMgr::DeleteInstance();
	CDataBaseCMgr::DeleteInstance();

	CMainDBClient::DeleteInstance();
	CStorageDBClient::DeleteInstance();
	CStorageBusinessClient::DeleteInstance();

	CGetUserInfoMgr::DeleteInstance();
	CGetUserGroupMgr::DeleteInstance();
	CGetUserDeviceMgr::DeleteInstance();
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
}
