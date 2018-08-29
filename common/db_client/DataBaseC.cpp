#include "DataBaseC.h"
#include "StorageBusinessClient.h"

IMPLEMENT_SINGLETON(CDataBaseCMgr)
IStorageBusinessSink* CDataBaseCMgr::GetDataBaseSink(DWORD dwID)
{
	CDataBaseC* p = CDataBaseCMgr::Instance()->GetElem(dwID);
	if (NULL == p) return NULL;
	return p->GetDataBaseSink();
}

CDataBaseC::CDataBaseC(DWORD dwID, IStorageBusinessSink* pSink)
{
	m_dwID = dwID;
	m_pSink = pSink;
}
CDataBaseC::~CDataBaseC()
{
	m_pSink = NULL;
}
//请求广告信息
int  CDataBaseC::GetAdvertInfo(DWORD dwDeviceID,LIST_DWORD& lstAdvertID)
{
	LOG_DEBUG(LOG_MAIN, "CDataBaseC::%s m_dwID = %d\n", __FUNCTION__, m_dwID);
	CStorageBusinessClient::Instance()->SetDataBaseAID(m_dwID);
	return CStorageBusinessClient::Instance()->GetAdvertInfo(dwDeviceID, lstAdvertID);
}

int CDataBaseC::ReportAdvertProgress(DWORD dwDeviceID, LIST_PROGRESS& lstProgress)

{
	LOG_DEBUG(LOG_MAIN, "CDataBaseC::%s m_dwID = %d\n", __FUNCTION__, m_dwID);
	CStorageBusinessClient::Instance()->SetDataBaseAID(m_dwID);
	return CStorageBusinessClient::Instance()->ReportAdvertProgress(dwDeviceID, lstProgress);
}
