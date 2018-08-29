#include "Log.h"
#include "dbTag.h"
#include "Protocol.h"
#include "putbuffer.h"
#include "UtilityInterface.h"
#include "dbHandle_operation.h"
#include <algorithm>

using namespace std;
const WORD TIMER_ADVERTCHECK = 5;

IMPLEMENT_SINGLETON( COperationDataBaseHandle )
COperationDataBaseHandle::COperationDataBaseHandle()
{
	m_dwProgress = 0;
	m_dwAdFlag = 0;
	m_dwTimeout = 0;
	m_bFinish = false;
	m_bConnectSuccess = false;
	LocalDiskAdvertList();
	AddTimer(TIMER_ADVERT,TIMER_ADVERTCHECK,this);
}
COperationDataBaseHandle::~COperationDataBaseHandle()
{
	m_clsSrcDBCon.disconnect();
	DelTimer(TIMER_ADVERT,this);
}
int COperationDataBaseHandle::GetError()
{
	int nError = m_nError;
	m_nError = 0;
	return nError;
}
bool COperationDataBaseHandle::Connect(PUCHAR pHost, PUCHAR pDatabase, PUCHAR pUserName, PUCHAR pPassword)
{
	m_bConnectSuccess = false;
	try
	{
		m_bConnectSuccess = m_clsSrcDBCon.connect((const char*)pDatabase, (const char*)pHost, (const char*)pUserName, (const char*)pPassword);
		Query query = m_clsSrcDBCon.query();
		query.exec("SET NAMES 'utf8'");
	}
	catch ( BadQuery er )
	{
		LOG_ERR(LOG_DB_SERVER, "Connection database(%s) with error: %s\n", pDatabase, er.what());
	}
	LOG_DETAIL(LOG_DB_SERVER, "Connection database(%s) successful\n", pDatabase);
	return m_bConnectSuccess;
}

void COperationDataBaseHandle::OnTimer()
{

}
void COperationDataBaseHandle::OnTimer(TimerReason_e eReason, ITimerSink* pSink)
{
//	LOG_DEBUG(LOG_DB_SERVER,"COperationDataBaseHandle::%s\n",__FUNCTION__);
	if(eReason == TIMER_ADVERT)
	{
		AdvertProgressMgr();		
		AdvertDownloadMgr();
	}
}


bool COperationDataBaseHandle::AdvertDownloadMgr()
{
//	LOG_DEBUG(LOG_DB_SERVER, "COperationDataBaseHandle::%s\n", __FUNCTION__);
	DWORD dwCount = m_lstDevAdID.size();
	if (dwCount == 0)		return true;	
	LOG_DEBUG(LOG_DB_SERVER, "%s Count = %d\n", __FUNCTION__, dwCount);
	if (m_dwAdFlag == 1)	{
		LOG_DEBUG(LOG_DB_SERVER, "DevID %d AdID %d Is DownLoading ... \n", m_tInfo.dwDeviceID, m_tInfo.dwAdvertID);
		return true;
	}

	LIST_DEV_ADVERT_ID::iterator iter_ad = m_lstDevAdID.begin();
	DWORD dwDeviceID = iter_ad->dwDeviceID;
	DWORD dwAdvertID = iter_ad->dwAdvertID;

	LOG_DEBUG(LOG_DB_SERVER, "%s 1\n", __FUNCTION__);
	AdvertInfo_t tInfo;
	if(false == Query_Advert(dwAdvertID, tInfo))
	{
		LOG_DEBUG(LOG_DB_SERVER, "Not Find %d\n", dwAdvertID);
		m_lstDevAdID.erase(iter_ad);
		dwDeviceID = 0;
		dwAdvertID = 0;
		m_dwAdFlag = 0;
		return true;
	}

	LOG_DEBUG(LOG_DB_SERVER, "%s 2\n", __FUNCTION__);
	CSTRING strName;
	char szName[LENGTH_NAME] = {0};
	sprintf(szName, "%d.%s", tInfo.dwAdID, tInfo.szFormat);
	strName.assign(szName);
	if (false == CheckLstAdName(strName))
	{
		LOG_DEBUG(LOG_DB_SERVER, "Qiniu Not Download %s, Please Waiting ...\n", strName.c_str());
		m_lstDevAdID.erase(iter_ad);
		DevAdvertID_t tInfo;
		tInfo.dwDeviceID = dwDeviceID;
		tInfo.dwAdvertID = dwAdvertID;
		m_lstDevAdID.push_back(tInfo);
		//没有从七牛下载过
		CheckQiniuList(dwAdvertID);
		return true;
	}
	
	LOG_DEBUG(LOG_DB_SERVER, "%s 3\n", __FUNCTION__);
	CSTRING strLocalUrl;
	Local_GetAdvertUrl(strName,strLocalUrl);
	char szUrl[LENGTH_URL+1] = {0};
	memcpy(szUrl, strLocalUrl.c_str(), strLocalUrl.size());

	LOG_DEBUG(LOG_DB_SERVER, "%s  szUrl = %s\n", __FUNCTION__, szUrl);
	if (m_pSink)	m_pSink->DownLoadAdvertInfo(dwDeviceID, tInfo, (PUCHAR)szUrl);

	m_dwAdFlag = 1;
	m_tInfo.dwDeviceID = dwDeviceID;
	m_tInfo.dwAdvertID = dwAdvertID;
	return true;
}
bool COperationDataBaseHandle::AdvertProgressMgr()
{
	if (m_dwTimeout != 0)	LOG_DEBUG(LOG_DB_SERVER, "COperationDataBaseHandle::%s Timeout = %d\n", __FUNCTION__, m_dwTimeout);
	if (m_tInfo.dwDeviceID == 0 || m_tInfo.dwAdvertID == 0) return true;
	LOG_DEBUG(LOG_DB_SERVER, "%s Total %d, DevID %d, AdID %d, Progress %d, Timeout %d\n", 
		__FUNCTION__, m_lstDevAdID.size(), m_tInfo.dwDeviceID, m_tInfo.dwAdvertID, m_dwProgress, m_dwTimeout);
	m_dwTimeout += TIMER_ADVERTCHECK;
	if (m_dwTimeout > 120)//2分钟
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s Download the timeout\n", __FUNCTION__);
		m_dwAdFlag = 0;
		DeleteLstDevAdID(m_tInfo);
		m_tInfo.dwDeviceID = 0;
		m_tInfo.dwAdvertID = 0;
		m_dwTimeout = 0;
		return true;
	}
	if (m_tInfo.dwDeviceID != 0 && m_tInfo.dwAdvertID != 0 && m_bFinish == true)
	{
		m_dwAdFlag = 0;
		DeleteLstDevAdID(m_tInfo);
		m_tInfo.dwDeviceID = 0;
		m_tInfo.dwAdvertID = 0;
		m_dwTimeout = 0;
		return true;
	}
	return true;
}

bool COperationDataBaseHandle::AdvertInfoMgr(DWORD& dwID, LIST_DWORD& lstAdvertID, LIST_ADVERTINFO& lstAdvertInfo, LIST_DWORD& lstVideoAdvertID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	AdvertInfo_t tInfo;
	memset(&tInfo, 0, sizeof(AdvertInfo_t));
	LIST_DWORD::iterator iter = lstAdvertID.begin();
	for( ; lstAdvertID.end() != iter; ++iter)
	{
		DWORD dwAdvertID = *iter;
		LOG_DEBUG(LOG_DB_SERVER, "AdID = %d\n", dwAdvertID);
		Query_Advert(dwAdvertID, tInfo);
		if(tInfo.dwAdType == 1)  lstVideoAdvertID.push_back(dwAdvertID);
		else	lstAdvertInfo.push_back(tInfo);
	}
	dwID = tInfo.dwStoreID;
	return true;
}
bool COperationDataBaseHandle::GetQiniuDownloadUrl(LIST_ADVERTINFO& lstAdvertInfo, StorageAccount_t& tAccount, LIST_ADVERTINFO_REP& lstAdvertInfoRep)		
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	DWORD dwCount = lstAdvertInfo.size();
	if(dwCount == 0) return true;

	AdvertInfoRep_t tAdvertRep;
	memset(&tAdvertRep, 0, sizeof(tAdvertRep));

	LIST_ADVERTINFO::iterator iter = lstAdvertInfo.begin();
	for (; lstAdvertInfo.end() != iter; ++iter)
	{
		CSTRING strUrl;
		Qiniu_GetAdvertUrl(*iter, tAccount, strUrl);
		tAdvertRep.tInfo.dwAdID = iter->dwAdID;
		tAdvertRep.tInfo.dwAdType = iter->dwAdType;
		tAdvertRep.tInfo.dwApplyID = iter->dwApplyID;
		tAdvertRep.tInfo.dwApplyType = iter->dwApplyType;
		tAdvertRep.tInfo.dwSize = iter->dwSize;
		tAdvertRep.tInfo.dwStoreID = iter->dwStoreID;
		tAdvertRep.tInfo.dwStoreType = iter->dwStoreType;
		tAdvertRep.tInfo.dwTimeStamp = iter->dwTimeStamp;
		tAdvertRep.tInfo.dwUsePosition = iter->dwUsePosition;
		tAdvertRep.tInfo.dwUseType = iter->dwUseType;
		memcpy(tAdvertRep.tInfo.szFormat, iter->szFormat, sizeof(iter->szFormat));
		memcpy(tAdvertRep.szUrl, strUrl.c_str(), strUrl.size());
		lstAdvertInfoRep.push_back(tAdvertRep);
	}
	return true;
}
bool COperationDataBaseHandle::Qiniu_GetAdvertUrl(AdvertInfo_t& tInfo, StorageAccount_t&tAccount, CSTRING& AdUrl)
{
	char szToken[100] = {"\0"};
	Qiniu_GetDownloadToken((const char*)tAccount.szAccessKey,(const char*)tAccount.szSecretKey, (char*)szToken);
	char szURL[250] = {"\0"};
	sprintf(szURL,"http://advert.dd121.com/%d/%d/%d/%d-%d-%d-1-%d-%d.%s%s",
		tInfo.dwApplyType, tInfo.dwApplyID, tInfo.dwAdType, tInfo.dwTimeStamp, 
		tInfo.dwSize, tInfo.dwStoreID, tInfo.dwUseType, tInfo.dwUsePosition, tInfo.szFormat, szToken);
	LOG_DEBUG(LOG_DB_SERVER, "szUrl = %s\n", szURL);
	AdUrl.clear();		AdUrl.assign(szURL);
	LOG_DEBUG(LOG_DB_SERVER, "AdUrl = %s\n", AdUrl.c_str());
	return true;
}
bool COperationDataBaseHandle::Local_GetAdvertUrl(CSTRING& strName, CSTRING& AdUrl)
{
	LOG_DEBUG(LOG_DB_SERVER,"COperationDataBaseHandle::%s\n", __FUNCTION__);
	//url格式:http://www.dd121.com/VResource_d/advertId.format
	char szCommand[256] = {0};
	sprintf((char*)szCommand,"http://www.dd121.com/VResource_d/%s", strName.c_str());
	AdUrl.assign(szCommand);
	LOG_DEBUG(LOG_DB_SERVER, "AdUrl = %s\n", AdUrl.c_str());
	return true;
}
// query
bool COperationDataBaseHandle::Query_Advert(DWORD dwAdID, AdvertInfo_t& tInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"COperationDataBaseHandle::%s dwAdID = %d\n", __FUNCTION__, dwAdID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	// select applytype, applyid, adtype, timestamp, size, storeid, storetype, format, usetype, useposition form advert where adid = dwAdID;
	query << "SELECT " << T_ADVERT_APPLYTYPE << ", " 
			<< T_ADVERT_APPLYID << ", " 
			<< T_ADVERT_ADTYPE << ", " 
			<< T_ADVERT_TIMESTAMP << ", "
			<< T_ADVERT_SIZE << ", " 
			<< T_ADVERT_STOREID << ", " 
			<< T_ADVERT_STORETYPE << ", " 
			<< T_ADVERT_FORMAT << ", " 
			<< T_ADVERT_USETYPE << ", "
			<< T_ADVERT_USEPOSITION 
		<< " FROM " << TABLE_ADVERT 
		<< " WHERE " << T_ADVERT_ADID << "=" << dwAdID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0) { 	LOG_DEBUG(LOG_DB_SERVER, "Not Find Query_Advert\n"); return false; }
	LOG_DEBUG(LOG_DB_SERVER, "Count = %d\n", res.size());
	memset(&tInfo, 0, sizeof(AdvertInfo_t));
	Row row = *(res.begin());
	tInfo.dwAdID = dwAdID;
	LOG_DEBUG(LOG_DB_SERVER, "AdID = %d\n", dwAdID);
	tInfo.dwApplyType	= (DWORD)row[T_ADVERT_APPLYTYPE];
	LOG_DEBUG(LOG_DB_SERVER, "ApplyType = %d\n", tInfo.dwApplyType);
	tInfo.dwApplyID = (DWORD)row[T_ADVERT_APPLYID];
	tInfo.dwAdType = (DWORD)row[T_ADVERT_ADTYPE];
	tInfo.dwTimeStamp = (DWORD)row[T_ADVERT_TIMESTAMP];
	tInfo.dwSize = (DWORD)row[T_ADVERT_SIZE];
	tInfo.dwStoreID = (DWORD)row[T_ADVERT_STOREID];
	tInfo.dwStoreType = (DWORD)row[T_ADVERT_STORETYPE];
	std::string strFormat = (std::string)row[T_ADVERT_FORMAT];
	memcpy(tInfo.szFormat, strFormat.c_str(), strFormat.size());
	tInfo.dwUseType = (DWORD)row[T_ADVERT_USETYPE];
	tInfo.dwUsePosition = (DWORD)row[T_ADVERT_USEPOSITION];
	LOG_DEBUG(LOG_DB_SERVER, "AdID %d, ApType %d, ApID %d, AdType %d, Time %d, Size %d, StoreID %d, StoreType %d, UType %d, UPosition %d\n", 
		tInfo.dwAdID, tInfo.dwAdType, tInfo.dwApplyID, tInfo.dwApplyType, tInfo.dwTimeStamp, tInfo.dwSize, tInfo.dwStoreID, tInfo.dwStoreType, tInfo.dwUseType, tInfo.dwUsePosition);
	LOG_DEBUG(LOG_DB_SERVER, "szFormat = %s\n", tInfo.szFormat);
	return true;
}
//查询广告下载时所用的七牛token参数
bool COperationDataBaseHandle::Query_StorageKeys(DWORD dwStoreID, StorageAccount_t& tAccount)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s dwStoreID = %d\n", __FUNCTION__, dwStoreID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	//	query << "select accesskey, secretkey, bucket, domain from storage where storeid = 4";
	query << "SELECT " << T_STORAGE_ACCESSKEY << ", " 
		<< T_STORAGE_SECRETKEY << ", " 
		<< T_STORAGE_BUCKET << ", " 
		<< T_STORAGE_DOMAIN
		<< " FROM " << TABLE_STORAGE 
		<< " WHERE " << T_STORAGE_STOREID << "=" << dwStoreID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0) { 	LOG_DEBUG(LOG_DB_SERVER, "Can't Find StoreAccount\n"); return false; }

	Row row = *(res.begin());
	tAccount.dwStoreID = dwStoreID;
	LOG_DEBUG(LOG_DB_SERVER, "StoreID = %d\n", tAccount.dwStoreID);
	std::string strAccessKey = (std::string)row[T_STORAGE_ACCESSKEY];
	LOG_DEBUG(LOG_DB_SERVER, "strAccessKey = %s\n", strAccessKey.c_str());
	std::string strSecretKey = (std::string)row[T_STORAGE_SECRETKEY];
	LOG_DEBUG(LOG_DB_SERVER, "strSecretKey = %s\n", strSecretKey.c_str());
	std::string strBucket = (std::string)row[T_STORAGE_BUCKET];
	LOG_DEBUG(LOG_DB_SERVER, "strBucket = %s\n", strBucket.c_str());
	std::string strDomain = (std::string)row[T_STORAGE_DOMAIN];
	LOG_DEBUG(LOG_DB_SERVER, "strDomain = %s\n", strDomain.c_str());

	int nCpyLen = strAccessKey.size() < LENGTH_ACCESSKEY ? strAccessKey.size() : LENGTH_ACCESSKEY;
	memcpy(tAccount.szAccessKey, strAccessKey.c_str(), nCpyLen);
	nCpyLen = strSecretKey.size() < LENGTH_SECRETKEY ? strSecretKey.size() : LENGTH_SECRETKEY;
	memcpy(tAccount.szSecretKey, strSecretKey.c_str(), nCpyLen);
	nCpyLen = strBucket.size() < LENGTH_BUCKET ? strBucket.size() : LENGTH_BUCKET;
	memcpy(tAccount.szBucket, strBucket.c_str(), nCpyLen);
	nCpyLen = strDomain.size() < LENGTH_DOMAIN ? strDomain.size() : LENGTH_DOMAIN;
	memcpy(tAccount.szDomain, strDomain.c_str(), nCpyLen);
	return true;
}

// 将要进行的视频广告放入列表中
bool COperationDataBaseHandle::AddTaskList(DWORD dwDeviceID, LIST_DWORD& lstAdID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s DevID %d\n", __FUNCTION__, dwDeviceID);
	//更新本地视频广告列表
	const char* pLocalDir = "/home/www/default/VResource_d";
	if(false == GetLocalFileList(pLocalDir, m_listLocalFile))
	{
		LOG_DEBUG(LOG_DB_SERVER, "GetLocalFileList failed\n");
		return false;
	}

	DWORD dwCount = lstAdID.size();
	if (dwCount == 0 || dwDeviceID == 0) return true;
	LOG_DEBUG(LOG_DB_SERVER, "%s DevID = %d, Count = %d\n", __FUNCTION__, dwDeviceID, lstAdID.size());

	DevAdvertID_t tInfo;
	tInfo.dwDeviceID = dwDeviceID;
	//插入到任务列表
	LIST_DWORD::iterator iter = lstAdID.begin();
	for ( ; lstAdID.end() != iter; ++iter)
	{
		tInfo.dwAdvertID = *iter;
		if (false == CheckTaskList(tInfo)) continue;
		LOG_DEBUG(LOG_DB_SERVER, "%s AdID = %d\n", __FUNCTION__, tInfo.dwAdvertID);
	}
	LOG_DEBUG(LOG_DB_SERVER, "%s Total = %d\n", __FUNCTION__, m_lstDevAdID.size());
	return true;
}
// 上报本地下载进度
bool COperationDataBaseHandle::AddLocalProgress(DWORD dwDeviceID, LIST_PROGRESS& lstProgress)
{
	LIST_PROGRESS::iterator iter = lstProgress.begin();
	for (; lstProgress.end() != iter; ++iter)
	{
		if (dwDeviceID == m_tInfo.dwDeviceID && iter->dwProgress > 0)
		{		
			LOG_DEBUG(LOG_DB_SERVER, "%s DevID = %d, lstProgress.size() = %d\n", __FUNCTION__, dwDeviceID, lstProgress.size());
			if (m_dwProgress < iter->dwProgress)	
			{
					m_dwProgress = iter->dwProgress;
					m_dwTimeout = 0;
			}
			m_bFinish = iter->cFinish;
		}
	}
	return true;
}
// 获取并清空列表
bool COperationDataBaseHandle::GetQiniuList(LIST_DWORD& lstAdID)
{
//	LOG_DEBUG(LOG_DB_SERVER, "COperationDataBaseHandle::%s\n", __FUNCTION__);
	if (m_lstAdID.size() == 0) return false;
	LIST_DWORD::iterator iter = m_lstAdID.begin();
	for (; m_lstAdID.end() != iter; ++iter)
	{
		DWORD dwID = *iter;
		lstAdID.push_back(dwID);
	}
	return true;
}
//清除已经下载好的广告
bool COperationDataBaseHandle::ClearQiniuList(DWORD& dwAdID)
{
	LOG_DEBUG(LOG_DB_SERVER, "COperationDataBaseHandle::%s\n", __FUNCTION__);
	if (m_lstAdID.size() == 0) return false;
	LIST_DWORD::iterator iter = m_lstAdID.begin();
	for (; m_lstAdID.end() != iter; ++iter)
	{
		DWORD dwID = *iter;
		if (dwID == dwAdID)	
		{
			m_lstAdID.erase(iter);
			break;
		}
	}
	LocalDiskAdvertList();
	return true;
}


bool COperationDataBaseHandle::DeleteLstDevAdID(DevAdvertID_t& m_tInfo)
{
	LIST_DEV_ADVERT_ID::iterator iter = m_lstDevAdID.begin();
	for (; m_lstDevAdID.end() != iter; ++iter)
	{
		if (iter->dwDeviceID == m_tInfo.dwDeviceID && iter->dwAdvertID == m_tInfo.dwAdvertID)
		{
			m_lstDevAdID.erase(iter);
			return true;
		}
	}
	return false;
}
bool COperationDataBaseHandle::LocalDiskAdvertList()
{
	LOG_DEBUG(LOG_DB_SERVER, "COperationDataBaseHandle::%s\n", __FUNCTION__);
	const char* pLocalDir = "/home/www/default/VResource_d";
	if(false == GetLocalFileList(pLocalDir, m_listLocalFile))
	{
		LOG_DEBUG(LOG_DB_SERVER, "GetLocalFileList failed\n");
		return false;
	}
	return true;
}

bool COperationDataBaseHandle::CheckTaskList(DevAdvertID_t& tInfo)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	//检查列表中是否存在
	LIST_DEV_ADVERT_ID::iterator iter_check = m_lstDevAdID.begin();
	for ( ; m_lstDevAdID.end() != iter_check; ++iter_check)
	{
		if(tInfo.dwDeviceID == iter_check->dwDeviceID && tInfo.dwAdvertID == iter_check->dwAdvertID)
			return false;
	}
	m_lstDevAdID.push_back(tInfo);
	return true;
}
bool COperationDataBaseHandle::CheckQiniuList(DWORD dwAdID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	LIST_DWORD::iterator iter = m_lstAdID.begin();
	for (; m_lstAdID.end() != iter; ++iter)
	{
		DWORD dwID = *iter;
		if(dwID == dwAdID) return false;
	}
	m_lstAdID.push_back(dwAdID);
	return true;
}
bool COperationDataBaseHandle::CheckLstAdName(CSTRING& AdName)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	LIST_LOCAL_FILE::iterator iter_name = m_listLocalFile.begin();
	for (; m_listLocalFile.end() != iter_name; ++iter_name)
	{
		if( 0 == iter_name->strFileName.compare(AdName) ) return true;
	}
	return false;
}

int COperationDataBaseHandle::CatchException(Query& query)
{
	try {
		query.store();
	}
	catch (const mysqlpp::BadQuery& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle any query errors
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s BadQuery\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::BadConversion& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle bad conversions; e.g. type mismatch populating 'stock'
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s BadConversion\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::Exception& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Catch-all for any other MySQL++ exceptions
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s Any Other Exception\n", __FUNCTION__);
		return -1;
	}
	return 0;
}
int COperationDataBaseHandle::CatchException(Query& query, StoreQueryResult& res)
{
	try {
		res = query.store();
	}
	catch (const mysqlpp::BadQuery& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle any query errors
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s BadQuery\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::BadConversion& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle bad conversions; e.g. type mismatch populating 'stock'
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s BadConversion\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::Exception& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Catch-all for any other MySQL++ exceptions
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s Any Other Exception\n", __FUNCTION__);
		return -1;
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////
