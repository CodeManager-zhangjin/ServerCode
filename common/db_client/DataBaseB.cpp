#include "DataBaseB.h"
#include "StorageDBClient.h"

IMPLEMENT_SINGLETON(CDataBaseBMgr)
IStorageDBSink* CDataBaseBMgr::GetDataBaseSink(DWORD dwID)
{
	CDataBaseB* p = CDataBaseBMgr::Instance()->GetElem(dwID);
	if (NULL == p) return NULL;
	return p->GetDataBaseSink();
}

CDataBaseB::CDataBaseB(DWORD dwID, IStorageDBSink* pSink)
{
	m_dwID = dwID;
	m_pSink = pSink;
}
CDataBaseB::~CDataBaseB()
{
	m_pSink = NULL;
}
/*
int CDataBaseB::SDB_AddUnlockItem(LIST_DEVICE_UNLOCK& listInfo)
{
	CStorageDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CStorageDBClient::Instance()->SDB_AddUnlockItem(listInfo);
}
*/

int CDataBaseB::SDB_UnlockItemTunel(PUCHAR pData, int nLen)
{
	CStorageDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CStorageDBClient::Instance()->SDB_UnlockItemTunel(pData,nLen);
}
int CDataBaseB::SDB_AlarmStatus(PUCHAR pData, int nLen)
{
	CStorageDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CStorageDBClient::Instance()->SDB_AlarmStatus(pData,nLen);
}
//·Ã¿ÍÁôÓ°d_sdb
int CDataBaseB::SDB_ReportUploadResult(PUCHAR pData, int nLen)
{
	CStorageDBClient::Instance()->SetDataBaseAID(m_dwID);
	return CStorageDBClient::Instance()->SDB_ReportUploadResult(pData,nLen);
}
