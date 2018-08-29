#include "StorageDBClient.h"

IMPLEMENT_SINGLETON(CStorageDBClient);
CStorageDBClient::CStorageDBClient()
{
}
CStorageDBClient::~CStorageDBClient()
{
}
int CStorageDBClient::SDB_UnlockItemTunel(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CStorageDBClient::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, pData && (nLen >= 12), -1);

	pData[0] = m_bGroupCode;
	DWORD dwSrcID = m_dwDataBaseAID;
	dwSrcID = htonl(dwSrcID);
	memcpy(pData+8, &dwSrcID, sizeof(DWORD));
	return m_pCon->SendCommand(pData, nLen);
}
int CStorageDBClient::SDB_AlarmStatus(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CStorageDBClient::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, pData && (nLen >= 12), -1);

	pData[0] = m_bGroupCode;
	DWORD dwSrcID = m_dwDataBaseAID;
	dwSrcID = htonl(dwSrcID);
	memcpy(pData+8, &dwSrcID, sizeof(DWORD));
	return m_pCon->SendCommand(pData, nLen);
}
//test d_sdb
int CStorageDBClient::SDB_ReportUploadResult(PUCHAR pData, int nLen)
{
	LOG_DEBUG(LOG_MAIN,"CStorageDBClient::%s\n",__FUNCTION__);
	LOG_ASSERT_RET(LOG_DB_CLIENT, pData && (nLen >= 12), -1);
	pData[0] = m_bGroupCode;
	DWORD dwSrcID = m_dwDataBaseAID;
	dwSrcID = htonl(dwSrcID);
	memcpy(pData+8, &dwSrcID, sizeof(DWORD));
	return m_pCon->SendCommand(pData, nLen);
}
