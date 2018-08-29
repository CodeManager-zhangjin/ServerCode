#include "StorageBusinessClient.h"

IMPLEMENT_SINGLETON(CStorageBusinessClient);
CStorageBusinessClient::CStorageBusinessClient()
{
}
CStorageBusinessClient::~CStorageBusinessClient()
{
}
int CStorageBusinessClient::GetAdvertInfo(DWORD dwDeviceID,LIST_DWORD& lstAdvertID)
{
	LOG_DEBUG(LOG_MAIN, "CStorageBusinessClient::%s\n", __FUNCTION__);
	DWORD dwCount = lstAdvertID.size();
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwDeviceID << dwCount;
	LOG_DEBUG(LOG_MAIN, "DevID = %d, Count = %d\n", dwDeviceID, dwCount);
	LIST_DWORD::iterator iter = lstAdvertID.begin();
	for( ; lstAdvertID.end() != iter; ++iter)
	{
		DWORD dwAdID = *iter;
		bufferPut << dwAdID;
		LOG_DEBUG(LOG_MAIN, "AdID = %d\n", dwAdID);
	}
	return SendMsg(bufferPut, CMD_GET_ADVERTURLS, 0);
}
int CStorageBusinessClient::ReportAdvertProgress(DWORD dwDeviceID, LIST_PROGRESS& lstProgress)
{
	LOG_DEBUG(LOG_MAIN, "%s Count = %d\n", __FUNCTION__, dwDeviceID, lstProgress.size());
	DWORD dwCount =lstProgress.size();
	DECLARE_PUTBUFFER(bufferPut)
	bufferPut << dwDeviceID << dwCount;
	LIST_PROGRESS::iterator iter = lstProgress.begin();
	for (; lstProgress.end() != iter; ++iter)
	{
		bufferPut << iter->dwAdID << iter->dwProgress << iter->cFinish;
		LOG_DEBUG(LOG_MAIN, "%s DeviceID %d, AdID = %d, Progress = %d, Finish = %d\n", __FUNCTION__, dwDeviceID, iter->dwAdID, iter->dwProgress, iter->cFinish);
	}
	return SendMsg(bufferPut, CMD_REPORT_PROGRESS, 0);
}
