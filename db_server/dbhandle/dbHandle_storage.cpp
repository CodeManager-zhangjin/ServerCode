#include "dbHandle_storage.h"
#include "UtilityInterface.h"
#include "Log.h"
#include "dbTag.h"
#include "Protocol.h"
#include "putbuffer.h"
#include "StorageEngine.h"
#include <algorithm>
#include <map>  
using namespace std;


#define DEBUG_LOG_SDB 1

const DWORD MAX_STOREKEY_COUNT = 1000000;
const DWORD MAX_STOREKEYBAK_COUNT = 3 * MAX_STOREKEY_COUNT;

const DWORD ONCE_DELETE_STOREKEY_COUNT = 10;
const DWORD ONCE_DELETE_STOREKEYBAK_COUNT = 3 * ONCE_DELETE_STOREKEY_COUNT;

const int KEEP_DAYS_STOREKEY = 30;
const int KEEP_DAYS_STOREKEYBAK = 90;

IMPLEMENT_SINGLETON( CStorageDataBaseHandle )
CStorageDataBaseHandle::CStorageDataBaseHandle()
{
	m_nError = 0;
	m_nStoreKeyCount = 0;
	m_nStoreKeybakCount = 0;
	m_bConnectSuccess = false;
#if defined(DBSERVER_SDB)
	ThreadStart();
	AddTimer(TIMER_DELETEUNLOCKRECORD,7200,this);
	AddTimer(TIMER_INSERTUNLOCKRECORD,10,this);
#endif
}

CStorageDataBaseHandle::~CStorageDataBaseHandle()
{
#if defined(DBSERVER_SDB)
	InsertUnlockRecords(1);
	Insert_StoreKey(1);//访客
	InsertAlarmRecords(1);
	m_clsSrcDBCon.disconnect();
	ThreadStop();
	DelTimer(TIMER_DELETEUNLOCKRECORD,this);
	DelTimer(TIMER_INSERTUNLOCKRECORD,this);
#endif
}

bool CStorageDataBaseHandle::Connect(PUCHAR pHost, PUCHAR pDatabase, PUCHAR pUserName, PUCHAR pPassword)
{
	m_bConnectSuccess = false;
	try
	{
		m_bConnectSuccess = m_clsSrcDBCon.connect( (const char*)pDatabase, (const char*)pHost, (const char*)pUserName, (const char*)pPassword );
		if ( m_bConnectSuccess )
		{
			m_bConnectSuccess = m_clsSrcDBCon.select_db( (const char*)pDatabase );
			Query query = m_clsSrcDBCon.query();
			query.exec("SET NAMES 'utf8'");
		}
	}
	catch ( BadQuery er )
	{
		LOG_ERR(LOG_DB_SERVER, "Connection database(%s) with error: %s\n", pDatabase, er.what());
	}
	LOG_DETAIL(LOG_DB_SERVER, "Connection database(%s) successful\n", pDatabase);
	return m_bConnectSuccess;
}

//查询访客留影总数  //不用了
bool CStorageDataBaseHandle::Query_TotalCount()
{
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT count(*) FROM " << TABLE_STOREKEY << " WHERE 1";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s size %d\n", __FUNCTION__, res.size());
		return false;
	}

	Row row = *(res.begin());
	m_nStoreKeyCount = (int)row["count(*)"];

	query << "SELECT count(*) FROM " << TABLE_STOREKEYBAK << " WHERE 1";
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s size %d\n", __FUNCTION__, res.size());
		return false;
	}

	row = *(res.begin());
	m_nStoreKeybakCount = (int)row["count(*)"];
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 00 %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
	return true;
}

void CStorageDataBaseHandle::Keepalive()
{
#if defined(DBSERVER_SDB)
	LOG_DEBUG(LOG_DB_SERVER, "CStorageDataBaseHandle::%s\n", __FUNCTION__);
	DWORD dwStoreID = 1;
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT * FROM " << TABLE_STORAGE << " WHERE " << T_STORAGE_STOREID << "=" << dwStoreID;
	CatchException(query);
#endif
}

void CStorageDataBaseHandle::OnTimer()
{
#if defined(DBSERVER_SDB)
	// Keepalive();
	Timer_Delete_StoreKey();
#endif
}

void CStorageDataBaseHandle::OnTimer(TimerReason_e eReason, ITimerSink* pSink)
{
#if defined(DBSERVER_SDB)
//	LOG_DEBUG(LOG_DB_SERVER,"%s\n",__FUNCTION__);
	if(eReason == TIMER_INSERTUNLOCKRECORD)
	{
		InsertUnlockRecords(0);
		Insert_StoreKey(0);
		InsertAlarmRecords(0);
	}
	else if(eReason == TIMER_DELETEUNLOCKRECORD)
	{
		ActivateThread();
	}
#endif
}

int CStorageDataBaseHandle::GetError()
{
	int nError = m_nError;
	m_nError = 0;
	return nError;
}

int CStorageDataBaseHandle::CatchException(Query& query)
{
	try {
		query.store();
	}
	catch (const mysqlpp::BadQuery& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle any query errors
		LOG_ERR(LOG_DB_SERVER, "CStorageDataBaseHandle::%s BadQuery\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::BadConversion& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle bad conversions; e.g. type mismatch populating 'stock'
		LOG_ERR(LOG_DB_SERVER, "CStorageDataBaseHandle::%s BadConversion\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::Exception& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Catch-all for any other MySQL++ exceptions
		LOG_ERR(LOG_DB_SERVER, "CStorageDataBaseHandle::%s Any Other Exception\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

int CStorageDataBaseHandle::CatchException(Query& query, StoreQueryResult& res)
{
	try {
		res = query.store();
	}
	catch (const mysqlpp::BadQuery& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle any query errors
		LOG_ERR(LOG_DB_SERVER, "CStorageDataBaseHandle::%s BadQuery\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::BadConversion& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle bad conversions; e.g. type mismatch populating 'stock'
		LOG_ERR(LOG_DB_SERVER, "CStorageDataBaseHandle::%s BadConversion\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::Exception& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Catch-all for any other MySQL++ exceptions
		LOG_ERR(LOG_DB_SERVER, "CStorageDataBaseHandle::%s Any Other Exception\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

bool CStorageDataBaseHandle::Query_StorageInfo(StorageAccount_t& tAccount)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s StoreID %d\n", __FUNCTION__, tAccount.dwStoreID);
#if defined(DBSERVER_SDB)
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT * FROM " << TABLE_STORAGE << " WHERE " << T_STORAGE_STOREID << "=" << tAccount.dwStoreID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0) { 	LOG_DEBUG(LOG_DB_SERVER, "Can't Find StoreAccount\n"); return false; }
	
	Row row = *(res.begin());

	memset(&tAccount, 0, sizeof(StorageAccount_t));
	tAccount.dwStoreID = (DWORD)row[T_STORAGE_STOREID];

	std::string strAccessKey = (std::string)row[T_STORAGE_ACCESSKEY];
	std::string strSecretKey = (std::string)row[T_STORAGE_SECRETKEY];
	std::string strBucket = (std::string)row[T_STORAGE_BUCKET];
	std::string strDomain = (std::string)row[T_STORAGE_DOMAIN];

	int nCpyLen = strAccessKey.size() < LENGTH_ACCESSKEY ? strAccessKey.size() : LENGTH_ACCESSKEY;
	memcpy(tAccount.szAccessKey, strAccessKey.c_str(), nCpyLen);
	
	nCpyLen = strSecretKey.size() < LENGTH_SECRETKEY ? strSecretKey.size() : LENGTH_SECRETKEY;
	memcpy(tAccount.szSecretKey, strSecretKey.c_str(), nCpyLen);
	
	nCpyLen = strBucket.size() < LENGTH_BUCKET ? strBucket.size() : LENGTH_BUCKET;
	memcpy(tAccount.szBucket, strBucket.c_str(), nCpyLen);
	
	nCpyLen = strDomain.size() < LENGTH_DOMAIN ? strDomain.size() : LENGTH_DOMAIN;
	memcpy(tAccount.szDomain, strDomain.c_str(), nCpyLen);
#endif
	return true;
}

//存放报警记录到列表
 bool CStorageDataBaseHandle::StorageAlarmRecordList(LIST_ALARMSTATUS& listInfo)
 {
	 LOG_DEBUG(LOG_DB_SERVER,"CStorageDataBaseHandle::%s Count %d\n",__FUNCTION__, listInfo.size());
#if defined(DBSERVER_SDB)
	 if(!listInfo.empty()) m_listAlarmStatusRecords.splice(m_listAlarmStatusRecords.end(),listInfo);
#endif
	 return true;
 }
 //从m_listAlarmStatusRecords中取报警记录信息并调用Insert_AlarmStatusRecords函数存数据库
int CStorageDataBaseHandle::InsertAlarmRecords(DWORD Flag)//Flag==0:默认值，Flag==1:程序退出，在析构函数中使用。
 {
	 LOG_DEBUG(LOG_DB_SERVER,"CStorageDataBaseHandle::%s, Flag=%d\n",__FUNCTION__,Flag);
	 LOG_DEBUG(LOG_DB_SERVER,"%s Count %d\n",__FUNCTION__, m_listAlarmStatusRecords.size());
#if defined(DEBUG_LOG_SDB)
	 const DWORD iMaxCount = 100;
	 if(m_listAlarmStatusRecords.empty()) return 0;
	 DWORD Count = m_listAlarmStatusRecords.size();

	 if((Count <= iMaxCount) || (Flag == 1)){
		 LOG_DEBUG(LOG_DB_SERVER,"Total=%d,Storage:%d,Flag=%d\n",Count,Count,Flag);
		 LIST_ALARMSTATUS listInfo;
		 LIST_ALARMSTATUS::iterator iter = m_listAlarmStatusRecords.begin();
		 for(int i = 1; i <= Count; i++){
			 listInfo.push_back(*iter);
			 iter = m_listAlarmStatusRecords.erase(iter);
		 }
		 Insert_AlarmStatusRecords(listInfo);
		 return 0;
	 }
	 if(Count > iMaxCount){
		 LOG_DEBUG(LOG_DB_SERVER,"Total=%d,Storage:%d,Flag=%d\n",Count,iMaxCount,Flag);
		 LIST_ALARMSTATUS listInfo;
		 LIST_ALARMSTATUS::iterator iter = m_listAlarmStatusRecords.begin();
		 for(int i = 1; i <= iMaxCount; i++){
			 listInfo.push_back(*iter);
			 iter = m_listAlarmStatusRecords.erase(iter);
		 }
		 Insert_AlarmStatusRecords(listInfo);
		 return 0;
	 }
#endif
	 return 0;
 }
//根据时间戳将报警记录信息存放到不同的数据库表中
bool CStorageDataBaseHandle::Insert_AlarmStatusRecords(LIST_ALARMSTATUS& listInfo)
{
	LOG_DEBUG(LOG_DB_CLIENT,"CStorageDataBaseHandle::%s Count = %d\n",__FUNCTION__, listInfo.size());
#if defined(DBSERVER_SDB)
	CSTRING tabName;
	const int date2016 = 1451577600;
	multimap<CSTRING,AlarmStatus_t> mump;
	LIST_ALARMSTATUS::iterator iter = listInfo.begin();
	for(; iter != listInfo.end(); iter++)
	{
		/*
		if( false == GetAlarmTabName(*iter, tabName))
		{
			GetAlarmTabName(tabName, *iter);
			LOG_DEBUG(LOG_DB_SERVER,"%s SVR Name:%s Time %s\n", __FUNCTION__, tabName.c_str(), iter->szTimeStamp);
		}
		else
		{
			LOG_DEBUG(LOG_DB_SERVER,"%s DEV Name:%s Time %s\n", __FUNCTION__, tabName.c_str(), iter->szTimeStamp);
		}
		*/
		GetAlarmTabName(tabName, *iter);//获取平台时间和表名
		iter->dwTimeStamp = iter->dwTimeStamp - date2016;
		mump.insert(make_pair(tabName,*iter));
	}

	DWORD tFlag = 0;
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	multimap<CSTRING,AlarmStatus_t>::iterator it = mump.begin();
	CSTRING tName = it->first;
	for( ;it != mump.end(); ++it)
	{
		if(tName == it->first)//判断表名是否相同
		{
			if(tFlag == 0)
			{
				LOG_DEBUG(LOG_DB_SERVER,"start %s ......\n",  it->first.c_str());
				LOG_DEBUG(LOG_DB_SERVER,">hello %d %d %d %d %d\n", it->second.dwDeviceID, it->second.dwRoomID, it->second.dwType, it->second.dwSubType, it->second.dwTimeStamp);
				query << "INSERT INTO " << it->first << "(" << T_ALARM_DEVICEID << "," << T_ALARM_ROOMID << "," << T_ALARM_TYPE << "," << T_ALARM_SUBTYPE  << "," << T_ALARM_TIMESTAMP << ")"
					<< "VALUES (" << it->second.dwDeviceID << "," << it->second.dwRoomID << "," << it->second.dwType << "," << it->second.dwSubType <<",'" << it->second.dwTimeStamp << "')";
				tFlag = 1;
			}
			else
			{
				LOG_DEBUG(LOG_DB_SERVER,">hello %d %d %d %d %d\n", it->second.dwDeviceID, it->second.dwRoomID, it->second.dwType, it->second.dwSubType, it->second.dwTimeStamp);
				query << "," << " (" << it->second.dwDeviceID << "," << it->second.dwRoomID << "," << it->second.dwType << "," << it->second.dwSubType <<",'" << it->second.dwTimeStamp << "')";
			}
		}
		else 
		{
			LOG_DEBUG(LOG_DB_SERVER,"end ......\n");
			if (-1 == CatchException(query)){
				LOG_DEBUG(LOG_DB_SERVER,"insert error\n");
			}
			tFlag = 0;
			tName = it->first;
			--it;
			continue;
		}
	}
	LOG_DEBUG(LOG_DB_SERVER,"end ......\n");
	if (-1 == CatchException(query)) return false;
#endif
	return true;
}


//存放开门记录到列表m_listUnlockRecords中
bool CStorageDataBaseHandle::StorageUnlockList(LIST_DEVICE_UNLOCK& listInfo)
{
	if(!listInfo.empty()) m_listUnlockRecords.splice(m_listUnlockRecords.end(),listInfo);
	return true;
}
//从m_listUnlockRecords中取开门记录信息并调用Insert_OpenDoorRecords函数存数据库
int CStorageDataBaseHandle::InsertUnlockRecords(DWORD Flag)//Flag==0:默认值，Flag==1:程序退出，在析构函数中使用。
{
#if defined(DBSERVER_SDB)
//	LOG_DEBUG(LOG_DB_SERVER,"%s, Flag=%d\n",__FUNCTION__,Flag);
	const DWORD iMaxCount = 100;
	if(m_listUnlockRecords.empty()) return 0;
	DWORD Count = m_listUnlockRecords.size();

	if((Count <= iMaxCount) || (Flag == 1)){
		LOG_DEBUG(LOG_DB_SERVER,"Total=%d,Storage:%d,Flag=%d\n",Count,Count,Flag);
		LIST_DEVICE_UNLOCK listInfo;
		LIST_DEVICE_UNLOCK::iterator iter = m_listUnlockRecords.begin();
		for(int i = 1; i <= Count; i++){
			listInfo.push_back(*iter);
			iter = m_listUnlockRecords.erase(iter);
		}
		Insert_OpenDoorRecords(listInfo);
		return 0;
	}

	if(Count > iMaxCount){
		LOG_DEBUG(LOG_DB_SERVER,"Total=%d,Storage:%d,Flag=%d\n",Count,iMaxCount,Flag);
		LIST_DEVICE_UNLOCK listInfo;
		LIST_DEVICE_UNLOCK::iterator iter = m_listUnlockRecords.begin();
		for(int i = 1; i <= iMaxCount; i++){
			listInfo.push_back(*iter);
			iter = m_listUnlockRecords.erase(iter);
		}
		Insert_OpenDoorRecords(listInfo);
		return 0;
	}
#endif
	return 0;
}
//根据时间戳将开门记录信息存放到不同的数据库表中
bool CStorageDataBaseHandle::Insert_OpenDoorRecords(LIST_DEVICE_UNLOCK& listInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s\n",__FUNCTION__);
#if defined(DBSERVER_SDB)
	CHAR tabFirstSeq[2];
	std::string tableName("unlocklog");
	CSTRING tabName;
	
	multimap<CSTRING,UnlockInfo_t> mump;
	LIST_DEVICE_UNLOCK::iterator iter = listInfo.begin();
	for(; iter != listInfo.end(); iter++)
	{
		TimeStampTransDate_t  timeData;
		if(-1 == GetTimeStampTransDate(timeData, iter->dwTimestamp)){
			LOG_ERR(LOG_DB_SERVER,"DeviceID %d DateTime > PlatformDateTime.\n", iter->dwDeviceID);
			continue;
		}
		DWORD month = atoi(timeData.dateInfo.Month);
		DWORD tableFirstSeq = (month <= 6) ? month : (month -6);
		sprintf(tabFirstSeq,"%d",tableFirstSeq);
		tabName = tableName + tabFirstSeq;
		if(atoi(timeData.dateInfo.Day) <= 15)
			tabName = tabName + '1';
		else
			tabName = tabName + '2';
		mump.insert(make_pair(tabName,*iter));
	}

	DWORD tFlag = 0;
	const int date2016 = 1451577600;
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	multimap<CSTRING,UnlockInfo_t>::iterator it = mump.begin();
	CSTRING tName = it->first;
	for( ;it != mump.end(); ++it)
	{
		if(tName == it->first)//判断表名是否相同
		{
			if(tFlag == 0)
			{
//				LOG_DEBUG(LOG_DB_SERVER,"start ......\n");
//				LOG_DEBUG(LOG_DB_SERVER,"=>hello\n");
				query << "INSERT INTO " << it->first << "(" << T_UNLOCKLOG_DEVICEID << "," << T_UNLOCKLOG_ROOMID << "," << T_UNLOCKLOG_USERID << "," << T_UNLOCKLOG_CARDNUM << "," << T_UNLOCKLOG_TYPE << "," << T_UNLOCKLOG_TIMESTAMP << ")"
					<< "VALUES (" << it->second.dwDeviceID << "," << it->second.dwRoomID << "," << it->second.dwUserID << ",'" << (char*)it->second.ComNumber << "'," << it->second.dwUnlockType <<"," << it->second.dwTimestamp-date2016 << ")";
				tFlag = 1;
			}
			else
			{
//				LOG_DEBUG(LOG_DB_SERVER,"=>hello\n");
				query << "," << " (" << it->second.dwDeviceID << "," << it->second.dwRoomID << "," << it->second.dwUserID << ",'" << (char*)it->second.ComNumber << "'," << it->second.dwUnlockType <<"," << it->second.dwTimestamp-date2016 << ")";
			}
		}
		else 
		{
//			LOG_DEBUG(LOG_DB_SERVER,"end ......\n");
			if (-1 == CatchException(query)){
				LOG_DEBUG(LOG_DB_SERVER,"insert error\n");
			}
			tFlag = 0;
			tName = it->first;
			--it;
			continue;
		}
	}
//	LOG_DEBUG(LOG_DB_SERVER,"end ......\n");
	if (-1 == CatchException(query)) return false;
#endif
	return true;
}
//删除数据库表中6个月前对应当天以及前三天的开门记录数据,比如(2016-11-10)->(2016-5-10,9,8,7);
bool CStorageDataBaseHandle::Delete_OpenDoorRecords()
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
#if defined(DBSERVER_SDB)
	const int date2016 = 1451577600;

	for(int iFlag = 1; iFlag <= 6; iFlag++)
	{
		DelTabNameTimeStamp_t tab;
		if(false == RecordsDel(tab,iFlag)) {
			LOG_DEBUG(LOG_DB_SERVER,"Not Del Time, Exit.\n");
			return true;
		}
		LOG_DEBUG(LOG_DB_SERVER,"delete from %s where timestamp > %d and timestamp < %d\n",tab.Name,tab.start,tab.end);
		Query query = m_clsSrcDBCon.query();
		query.exec("SET NAMES 'utf8'");

		LOG_DEBUG(LOG_DB_SERVER,"delete from %s where timestamp > %d and timestamp < %d\n",tab.Name,tab.start -date2016,tab.end-date2016);
		query << "DELETE FROM " << tab.Name << " WHERE " << T_UNLOCKLOG_TIMESTAMP << ">" << tab.start-date2016 << " AND " << T_UNLOCKLOG_TIMESTAMP << "<" << tab.end-date2016; 
		if(-1 == CatchException(query)){
			LOG_DEBUG(LOG_DB_SERVER,"%s,iFlag=%d\n",__FUNCTION__,iFlag);
			continue;
		}
		LOG_DEBUG(LOG_DB_SERVER,"delete from %s where timestamp > %d and timestamp < %d\n",tab.Name1,tab.start1 -date2016,tab.end1-date2016);
		query << "DELETE FROM " << tab.Name1 << " WHERE " << T_UNLOCKLOG_TIMESTAMP << ">" << tab.start1-date2016 << " AND " << T_UNLOCKLOG_TIMESTAMP << "<" << tab.end1-date2016; 
		if(-1 == CatchException(query)){
			LOG_DEBUG(LOG_DB_SERVER,"%s,iFlag=%d\n",__FUNCTION__,iFlag);
			continue;
		}
		LOG_DEBUG(LOG_DB_SERVER,"delete from %s where timestamp > %d and timestamp < %d\n",tab.Name2,tab.start2 -date2016,tab.end2-date2016);
		query << "DELETE FROM " << tab.Name2 << " WHERE " << T_UNLOCKLOG_TIMESTAMP << ">" << tab.start2-date2016 << " AND " << T_UNLOCKLOG_TIMESTAMP << "<" << tab.end2-date2016; 
		if(-1 == CatchException(query)){
			LOG_DEBUG(LOG_DB_SERVER,"%s,iFlag=%d\n",__FUNCTION__,iFlag);
			continue;
		}
		LOG_DEBUG(LOG_DB_SERVER,"delete from %s where timestamp > %d and timestamp < %d\n\n",tab.Name3,tab.start3 -date2016,tab.end3-date2016);
		query << "DELETE FROM " << tab.Name3 << " WHERE " << T_UNLOCKLOG_TIMESTAMP << ">" << tab.start3-date2016 << " AND " << T_UNLOCKLOG_TIMESTAMP << "<" << tab.end3-date2016; 
		if(-1 == CatchException(query)){
			LOG_DEBUG(LOG_DB_SERVER,"%s,iFlag=%d\n",__FUNCTION__,iFlag);
			continue;
		}
		sleep(600);
	}
#endif
	return true;
}
//在线程里调用删除开门记录函数
void CStorageDataBaseHandle::ThreadLoop()
{
	while(m_bRunning)
	{
		HangUpThread();
		LOG_DEBUG(LOG_DB_SERVER,"CStorageDataBaseHandle::%s\n",__FUNCTION__);
		Delete_OpenDoorRecords();
	}
}

bool CStorageDataBaseHandle::Query_QiniuInfo(LIST_DWORD& lstStoreID)
{
	LOG_DEBUG(LOG_DB_SERVER,"4444===============================>%s,%d\n",__FUNCTION__,lstStoreID.size());
#if defined(DBSERVER_SDB)
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");

	LIST_DWORD::iterator iter = lstStoreID.begin();
	for(; iter != lstStoreID.end(); iter++)
	{
		printf("storeid=%d\n",*iter);
		query << "SELECT " << T_STORAGE_ACCESSKEY << "," << T_STORAGE_SECRETKEY << ","
			<< T_STORAGE_BUCKET << "," << T_STORAGE_DOMAIN 
			<< " FROM " <<  TABLE_STORAGE 
			<< " WHERE " << T_STORAGE_STOREID << "=" << *iter;
		StoreQueryResult res;
		if (-1 == CatchException(query,res)) continue;

		StoreQueryResult::iterator iter2 = res.begin();
		Row row = *iter2;
		DWORD storeid = *iter;
		std::string pAccessKey = (std::string)row[T_STORAGE_ACCESSKEY];
		std::string pSecretkey = (std::string)row[T_STORAGE_SECRETKEY];
		std::string pBucket    = (std::string)row[T_STORAGE_BUCKET];
		std::string pDomain    = (std::string)row[T_STORAGE_DOMAIN];
	}
#endif
	return true;
}

bool CStorageDataBaseHandle::Query_StorageKeys(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_STORE_ACCOUNTKEYS& lstAccountKeys)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s DeviceID %d RoomID count %d\n", __FUNCTION__, dwDeviceID, lstRoomID.size());
	
#if defined(DBSERVER_SDB)
	char pName[20] = {0};
	SelectVisitorTabName(pName);
	LOG_DEBUG(LOG_DB_SERVER,"%s\n",pName);

	if (lstRoomID.size() <= 0) return true;
	CSTRING strRoomID; MkRoomIDStr(pName, lstRoomID, strRoomID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << pName << "." << T_STOREKEY_DEVICEID << ","
		<< pName << "." << T_STOREKEY_ROOMID << ","
		<< pName << "." << T_STOREKEY_REASON << ","
		<< pName << "." << T_STOREKEY_TYPE << ","
		<< pName << "." << T_STOREKEY_TIMESTAMP << ","
		<< pName << "." << T_STOREKEY_SIZE << ","
		<< pName << "." << T_STOREKEY_STOREID << ","
		<< TABLE_STORAGE << "." << T_STORAGE_ACCESSKEY << ","
		<< TABLE_STORAGE << "." << T_STORAGE_SECRETKEY << ","
		<< TABLE_STORAGE << "." << T_STORAGE_BUCKET << ","
		<< TABLE_STORAGE << "." << T_STORAGE_DOMAIN
		<< " FROM " << pName << "," << TABLE_STORAGE
		<< " WHERE " << pName << "." << T_STOREKEY_DEVICEID << "=" << dwDeviceID
		<< " AND " << strRoomID.c_str()
		<< " AND " << pName << "." << T_STOREKEY_STOREID << "=" << TABLE_STORAGE << "." << T_STORAGE_STOREID;

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	StoreAccountKeys_t tAccountKeys;
	StoreKey_t tKey;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		memset(&(tAccountKeys.tAccount), 0, sizeof(StorageAccount_t));
		tAccountKeys.lstKey.clear();
		memset(&tKey, 0, sizeof(StoreKey_t));

		// tAccount
		tAccountKeys.tAccount.dwStoreID = (DWORD)row[T_STOREKEY_STOREID];
		std::string strAccessKey = (std::string)row[T_STORAGE_ACCESSKEY];
		std::string strSecretKey = (std::string)row[T_STORAGE_SECRETKEY];
		std::string strBucket = (std::string)row[T_STORAGE_BUCKET];
		std::string strDomain = (std::string)row[T_STORAGE_DOMAIN];

		int nCpyLen = strAccessKey.size() < LENGTH_ACCESSKEY ? strAccessKey.size() : LENGTH_ACCESSKEY;
		memcpy(tAccountKeys.tAccount.szAccessKey, strAccessKey.c_str(), nCpyLen);

		nCpyLen = strSecretKey.size() < LENGTH_SECRETKEY ? strSecretKey.size() : LENGTH_SECRETKEY;
		memcpy(tAccountKeys.tAccount.szSecretKey, strSecretKey.c_str(), nCpyLen);

		nCpyLen = strBucket.size() < LENGTH_BUCKET ? strBucket.size() : LENGTH_BUCKET;
		memcpy(tAccountKeys.tAccount.szBucket, strBucket.c_str(), nCpyLen);

		nCpyLen = strDomain.size() < LENGTH_DOMAIN ? strDomain.size() : LENGTH_DOMAIN;
		memcpy(tAccountKeys.tAccount.szDomain, strDomain.c_str(), nCpyLen);

		// tKey
		tKey.dwDeviceID = dwDeviceID;
		tKey.dwRoomID = (DWORD)row[T_STOREKEY_ROOMID];
		tKey.bType = (BYTE)row[T_STOREKEY_TYPE];
		tKey.bRecReason = (BYTE)row[T_STOREKEY_REASON];
		tKey.dwSize = (DWORD)row[T_STOREKEY_SIZE];
		tKey.dwStoreID = tAccountKeys.tAccount.dwStoreID;

		std::string strTimestamp = (std::string)row[T_STOREKEY_TIMESTAMP];
		//if (strTimestamp.size() >= 16)
		//{
		//	strTimestamp.erase(16, 1);
		//	strTimestamp.erase(13, 1);
		//	strTimestamp.erase(10, 1);
		//	strTimestamp.erase(7, 1);
		//	strTimestamp.erase(4, 1);
		//}
		nCpyLen = strTimestamp.size() < LENGTH_TIMESTAMP2 ? strTimestamp.size() : LENGTH_TIMESTAMP2;
		memcpy(tKey.szTimeStamp, strTimestamp.c_str(), nCpyLen);

		LIST_STORE_ACCOUNTKEYS::iterator posList = std::find_if( lstAccountKeys.begin(), lstAccountKeys.end(), FindStoreAcutByStoreID(tKey.dwStoreID) );
		if (posList != lstAccountKeys.end())
		{
			posList->lstKey.push_back(tKey);
		}
		else
		{
			tAccountKeys.lstKey.push_back(tKey);
			lstAccountKeys.push_back(tAccountKeys);
		}
	}
#endif
	return true;
}
//查询广告下载时所用的七牛token参数
bool CStorageDataBaseHandle::Query_StorageKeys(DWORD dwStoreID, StorageAccount_t& tAccount)
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

void CStorageDataBaseHandle::MkRoomIDStr( PCHAR pName, LIST_DWORD& lstRoomID, CSTRING& strRoomID )
{
	strRoomID += "(";
	LIST_DWORD::iterator iter = lstRoomID.begin();
	for (; iter != lstRoomID.end();)
	{
		DWORD dwRoomID = *iter;
		BYTE szTemp[20] = {0};
		memset(szTemp, 0, 20);
		sprintf((char*)szTemp, "%lu", dwRoomID);
		std::string strTemp; strTemp.assign((const char*)szTemp);
		strRoomID = strRoomID + pName + "." + T_STOREKEY_ROOMID + "=";
		strRoomID = strRoomID + strTemp;
		if(++iter != lstRoomID.end()) strRoomID += " or "; 
	}
	strRoomID += ")";
}

void CStorageDataBaseHandle::MkStoreIDStr(SET_DWORD& setStoreID, CSTRING& strStoreID)
{
	strStoreID += "(";
	SET_DWORD::iterator iter = setStoreID.begin();
	for (; iter != setStoreID.end();)
	{
		DWORD dwStoreID = *iter;
		BYTE szTemp[20] = {0};
		memset(szTemp, 0, 20);
		sprintf((char*)szTemp, "%lu", dwStoreID);
		std::string strTemp; strTemp.assign((const char*)szTemp);
		strStoreID = strStoreID + T_STOREKEY_STOREID + "=";
		strStoreID = strStoreID + strTemp;
		if(++iter != setStoreID.end()) strStoreID += " or "; 
	}
	strStoreID += ")";
}

bool CStorageDataBaseHandle::ToStorageEngine(LIST_STOREKEY& lstKey)
{
#if defined(DBSERVER_SDB)
	if (lstKey.size() <= 0)
	{
		return true;
	}
	SET_DWORD setStoreID;
	LIST_STOREKEY::iterator iter = lstKey.begin();
	for (; iter != lstKey.end(); ++iter)
	{
		setStoreID.insert(iter->dwStoreID);
	}
	CSTRING strStoreID;
	MkStoreIDStr(setStoreID, strStoreID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT * FROM " << TABLE_STORAGE << " WHERE " << strStoreID.c_str();

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	typedef std::map<DWORD, StorageAccount_t>		MAP_STORAGEACCOUNT;
	StorageAccount_t tAccount;
	MAP_STORAGEACCOUNT mapAccount;
	StoreQueryResult::iterator pos = res.begin();
	for (; pos != res.end(); pos++)
	{
		Row row = *pos;
		memset(&tAccount, 0, sizeof(StorageAccount_t));

		// tAccount
		tAccount.dwStoreID = (DWORD)row[T_STORAGE_STOREID];
		std::string strAccessKey = (std::string)row[T_STORAGE_ACCESSKEY];
		std::string strSecretKey = (std::string)row[T_STORAGE_SECRETKEY];
		std::string strBucket = (std::string)row[T_STORAGE_BUCKET];

		int nCpyLen = strAccessKey.size() < LENGTH_ACCESSKEY ? strAccessKey.size() : LENGTH_ACCESSKEY;
		memcpy(tAccount.szAccessKey, strAccessKey.c_str(), nCpyLen);

		nCpyLen = strSecretKey.size() < LENGTH_SECRETKEY ? strSecretKey.size() : LENGTH_SECRETKEY;
		memcpy(tAccount.szSecretKey, strSecretKey.c_str(), nCpyLen);

		nCpyLen = strBucket.size() < LENGTH_BUCKET ? strBucket.size() : LENGTH_BUCKET;
		memcpy(tAccount.szBucket, strBucket.c_str(), nCpyLen);
		mapAccount.insert(std::make_pair(tAccount.dwStoreID, tAccount));
	}	

	QiniuStoreKey_t tQiniuKey;
	LIST_QINIUSTOREKEY lstJob;
	iter = lstKey.begin();
	LIST_STOREKEY::iterator iterTemp;
	while (iter != lstKey.end())
	{
		iterTemp = iter; iterTemp++;

		MAP_STORAGEACCOUNT::iterator pos2 = mapAccount.find(iter->dwStoreID);
		if (mapAccount.end() == pos2)
		{
			LOG_DEBUG(LOG_DB_SERVER, "Warn:Storage is not exist! dwDeviceID %d dwRoomID %d dwStoreID %d szTimeStamp %s\n",
				iter->dwDeviceID, iter->dwRoomID, iter->dwStoreID, iter->szTimeStamp);
			lstKey.erase(iter);
		}
		else
		{
			memset(&tQiniuKey, 0, sizeof(QiniuStoreKey_t));

			// tAccount
			memcpy(tQiniuKey.accesskey, pos2->second.szAccessKey, LENGTH_ACCESSKEY);
			memcpy(tQiniuKey.secretkey, pos2->second.szSecretKey, LENGTH_SECRETKEY);
			memcpy(tQiniuKey.bucket, pos2->second.szBucket, LENGTH_BUCKET);

			GenerateStoreKey(*iter, (PUCHAR)tQiniuKey.key);
			lstJob.push_back(tQiniuKey);
			// LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 01 %s dwDeviceID %d dwRoomID %d dwStoreID %d szTimeStamp %s\n",
			// 	__FUNCTION__, tKey.dwDeviceID, tKey.dwRoomID, tKey.dwStoreID, tKey.szTimeStamp);
		}

		iter = iterTemp;
	}
	CStorageEngine::Instance()->Add_Job(lstJob);
	// batch delete qiniu files
#endif
	return true;
}

bool CStorageDataBaseHandle::Delete_StoreKey(DWORD dwDeviceID, DWORD dwRoomID, int nStoreCount)
{
#if defined(DBSERVER_SDB)
	if (m_nStoreKeyCount < nStoreCount)
	{
		Query_TotalCount();
		if (m_nStoreKeyCount < nStoreCount)
		{
			return true;
		}
	}
	// DELETE FROM `storekey` WHERE 1 ORDER BY timestamp ASC LIMIT 2
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");

	query << "SELECT " << T_STOREKEY_DEVICEID << ","
		<< T_STOREKEY_ROOMID << ","
		<< T_STOREKEY_REASON << ","
		<< T_STOREKEY_TYPE << ","
		<< T_STOREKEY_TIMESTAMP << ","
		<< T_STOREKEY_SIZE << ","
		<< T_STOREKEY_STOREID
		<< " FROM " << TABLE_STOREKEY
		<< " WHERE " << T_STOREKEY_DEVICEID << "=" << dwDeviceID
		<< " AND " << T_STOREKEY_ROOMID << "=" << dwRoomID
		<< " ORDER BY " << T_STOREKEY_TIMESTAMP << " ASC LIMIT " << nStoreCount;

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	StoreKey_t tKey;
	LIST_STOREKEY lstKey;

	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		memset(&tKey, 0, sizeof(StoreKey_t));

		// tKey
		tKey.dwDeviceID = (DWORD)row[T_STOREKEY_DEVICEID];
		tKey.dwRoomID = (DWORD)row[T_STOREKEY_ROOMID];
		tKey.bType = (BYTE)row[T_STOREKEY_TYPE];
		tKey.bRecReason = (BYTE)row[T_STOREKEY_REASON];
		tKey.dwSize = (DWORD)row[T_STOREKEY_SIZE];
		tKey.dwStoreID = (DWORD)row[T_STOREKEY_STOREID];

		std::string strTimestamp = (std::string)row[T_STOREKEY_TIMESTAMP];
		
		int nCpyLen = strTimestamp.size() < LENGTH_TIMESTAMP2 ? strTimestamp.size() : LENGTH_TIMESTAMP2;
		memcpy(tKey.szTimeStamp, strTimestamp.c_str(), nCpyLen);

		lstKey.push_back(tKey);
#if DEBUG_LOG_SDB
		LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 02 %s dwDeviceID %d dwRoomID %d dwStoreID %d szTimeStamp %s\n",
			__FUNCTION__, tKey.dwDeviceID, tKey.dwRoomID, tKey.dwStoreID, tKey.szTimeStamp);
#endif
	}
	ToStorageEngine(lstKey);

	query << "DELETE FROM " << TABLE_STOREKEY
		<< " WHERE " << T_STOREKEY_DEVICEID << "=" << dwDeviceID
		<< " AND " << T_STOREKEY_ROOMID << "=" << dwRoomID
		<< " ORDER BY " << T_STOREKEY_TIMESTAMP << " ASC LIMIT " << nStoreCount;
	if (-1 == CatchException(query)) return false;

#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 02 %s dwDeviceID %d dwRoomID %d nStoreCount %d\n", __FUNCTION__, dwDeviceID, dwRoomID, nStoreCount);
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 02 before %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
	m_nStoreKeyCount -= nStoreCount;
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 02 after %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
#endif // #if defined(DBSERVER_SDB)
	return true;
}


/*
bool CStorageDataBaseHandle::Insert_StoreKey( int nStoreLimit, StoreKey_t& tKey )
{
	char tName[20];
	InsertVisitorTabName((const char*)tKey.szTimeStamp,tName);
	LOG_DEBUG(LOG_DB_SERVER,"===|%s|,|%s|,|%s|===\n",__FUNCTION__,tKey.szTimeStamp,tName);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "INSERT INTO " << tName << " ("
		<< T_STOREKEY_DEVICEID << "," 
		<< T_STOREKEY_ROOMID << "," 
		<< T_STOREKEY_REASON << ","
		<< T_STOREKEY_TYPE << "," 
		<< T_STOREKEY_TIMESTAMP << "," 
		<< T_STOREKEY_SIZE << "," 
		<< T_STOREKEY_STOREID
		<< ") VALUES ("
		<< tKey.dwDeviceID << "," 
		<< tKey.dwRoomID << ","
		<< (int)tKey.bRecReason  << ","
		<< (int)tKey.bType  << ",'"
		<< tKey.szTimeStamp << "',"
		<< tKey.dwSize << ","
		<< tKey.dwStoreID
		<< ")";
	if (-1 == CatchException(query)) return false;
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 04 %s DeviceID %d RoomID %d Reason %d Type %d Timestamp %s Size %d StoreID %d\n",
		__FUNCTION__, tKey.dwDeviceID, tKey.dwRoomID, tKey.bRecReason, tKey.bType, tKey.szTimeStamp, tKey.dwSize, tKey.dwStoreID);
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 04 before %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
	return true;
}
*/
//访客留影缓存
bool CStorageDataBaseHandle::StoregeVisitorList(LIST_STOREKEY& listInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s\n",__FUNCTION__);
	if(!listInfo.empty()) m_listVisitorRecords.splice(m_listVisitorRecords.end(),listInfo);
	return true;
}
//从m_listVisitorRecords中取出访客留影信息并调用Insert_StoreKey
bool CStorageDataBaseHandle::Insert_StoreKey(DWORD Flag)//Flag==0:默认值，Flag==1:程序退出，在析构函数中使用。
{
//	LOG_DEBUG(LOG_DB_SERVER,"%s, Flag=%d\n",__FUNCTION__,Flag);
	const DWORD iMaxCount = 100;
	if(m_listVisitorRecords.empty()) return 0;
	DWORD Count = m_listVisitorRecords.size();

	if((Count <= iMaxCount) || (Flag == 1)){
		LOG_DEBUG(LOG_DB_SERVER,"Total=%d,Storage:%d,Flag=%d\n",Count,Count,Flag);
		LIST_STOREKEY listInfo;
		LIST_STOREKEY::iterator iter = m_listVisitorRecords.begin();
		for(int i = 1; i <= Count; i++){
			listInfo.push_back(*iter);
			iter = m_listVisitorRecords.erase(iter);
		}
		Insert_StoreKey(listInfo);
		return 0;
	}

	if(Count > iMaxCount){
		LOG_DEBUG(LOG_DB_SERVER,"Total=%d,Storage:%d,Flag=%d\n",Count,iMaxCount,Flag);
		LIST_STOREKEY listInfo;
		LIST_STOREKEY::iterator iter = m_listVisitorRecords.begin();
		for(int i = 1; i <= iMaxCount; i++){
			listInfo.push_back(*iter);
			iter = m_listVisitorRecords.erase(iter);
		}
		Insert_StoreKey(listInfo);
		return 0;
	}
}
//根据时间将访客记录信息存放到不同的数据库表中
bool CStorageDataBaseHandle::Insert_StoreKey(LIST_STOREKEY& listInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s\n",__FUNCTION__);
#if defined(DBSERVER_SDB)
	char Name[20];
	CSTRING tabName;
	multimap<CSTRING,StoreKey_t> mump;
	LIST_STOREKEY::iterator iter = listInfo.begin();
	for(; iter != listInfo.end(); iter++)
	{
		InsertVisitorTabName((const char*)iter->szTimeStamp,Name);
		tabName.assign(Name);
		mump.insert(make_pair(tabName,*iter));
	}

	DWORD tFlag = 0;
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	multimap<CSTRING,StoreKey_t>::iterator it = mump.begin();
	CSTRING tName = it->first;
	for( ;it != mump.end(); ++it)
	{
		if(tName == it->first)//判断表名是否相同
		{
			if(tFlag == 0)
			{
				LOG_DEBUG(LOG_DB_SERVER,"start ......\n");
//				LOG_DEBUG(LOG_DB_SERVER,"=>hello\n");
				query << "INSERT INTO " << it->first << " (" << T_STOREKEY_DEVICEID << "," << T_STOREKEY_ROOMID << "," << T_STOREKEY_REASON << ","
					<< T_STOREKEY_TYPE << "," << T_STOREKEY_TIMESTAMP << "," << T_STOREKEY_SIZE << "," << T_STOREKEY_STOREID
					<< ") VALUES ("
					<< it->second.dwDeviceID << "," << it->second.dwRoomID << "," << (int)it->second.bRecReason  << "," << (int)it->second.bType  << ",'"
					<< it->second.szTimeStamp << "'," << it->second.dwSize << "," << it->second.dwStoreID << ")";
				LOG_DEBUG(LOG_DB_SERVER,"Name:%s DevID:%d\n",it->first.c_str(),it->second.dwDeviceID);
				tFlag = 1;
			}
			else
			{
//				LOG_DEBUG(LOG_DB_SERVER,"=>hello\n");
				query << "," << " (" << it->second.dwDeviceID << "," << it->second.dwRoomID << "," << (int)it->second.bRecReason  << "," << (int)it->second.bType  << ",'"
					<< it->second.szTimeStamp << "'," << it->second.dwSize << "," << it->second.dwStoreID << ")";
				LOG_DEBUG(LOG_DB_SERVER,"DevID:%d\n",it->second.dwDeviceID);
			}
		}
		else 
		{
			LOG_DEBUG(LOG_DB_SERVER,"end ......\n");
			if (-1 == CatchException(query)){
				LOG_DEBUG(LOG_DB_SERVER,"insert error\n");
			}
			tFlag = 0;
			tName = it->first;
			--it;
			continue;
		}
	}
	LOG_DEBUG(LOG_DB_SERVER,"end ......\n");
	if (-1 == CatchException(query)) return false;
#endif
	return true;
}



bool CStorageDataBaseHandle::Query_RoomStoreCount(DWORD dwDeviceID, DWORD dwRoomID, int& nStoreCount)
{
#if defined(DBSERVER_SDB)
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT count(*) FROM " << TABLE_STOREKEY << " WHERE "
		<< T_STOREKEY_DEVICEID << "=" << dwDeviceID << " AND "
		<< T_STOREKEY_ROOMID << "=" << dwRoomID;
	
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s size %d\n", __FUNCTION__, res.size());
		return false;
	}

	Row row = *(res.begin());
	nStoreCount = (int)row["count(*)"];
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 05 %s dwDeviceID %d dwRoomID %d nStoreCount %d\n", __FUNCTION__, dwDeviceID, dwRoomID, nStoreCount);	
#endif
#endif // #if defined(DBSERVER_SDB)
	return true;
}

void CStorageDataBaseHandle::Timer_Delete_StoreKey()
{
	CheckOvertime_StoreKey();
	CheckTotalCount_StoreKey();
}

bool CStorageDataBaseHandle::Delete_StoreKeyByCount()
{
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 06 %s Count %d\n", __FUNCTION__, ONCE_DELETE_STOREKEY_COUNT);
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 06 before %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
#if defined(DBSERVER_SDB)
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "INSERT INTO " << TABLE_STOREKEYBAK
		<< " SELECT * FROM " << TABLE_STOREKEY 
		<< " WHERE 1 ORDER BY " << T_STOREKEY_TIMESTAMP << " ASC LIMIT " << ONCE_DELETE_STOREKEY_COUNT;
	if (-1 == CatchException(query)) return false;
	m_nStoreKeybakCount += ONCE_DELETE_STOREKEY_COUNT;

	// DELETE FROM `storekey` WHERE 1 ORDER BY timestamp ASC LIMIT 2
	query << "DELETE FROM " << TABLE_STOREKEY 
		<< " WHERE 1 ORDER BY " << T_STOREKEY_TIMESTAMP << " ASC LIMIT " << ONCE_DELETE_STOREKEY_COUNT;
	if (-1 == CatchException(query)) return false;
	m_nStoreKeyCount -= ONCE_DELETE_STOREKEY_COUNT;
	
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 07 after %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
#endif // #if defined(DBSERVER_SDB)
	return true;
}

bool CStorageDataBaseHandle::Delete_StoreKeybakByCount()
{
#if defined(DBSERVER_SDB)
	//DELETE FROM `storekeybak` WHERE 1 ORDER BY timestamp ASC LIMIT 2
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");

	query << "SELECT " << T_STOREKEY_DEVICEID << ","
		<< T_STOREKEY_ROOMID << ","
		<< T_STOREKEY_REASON << ","
		<< T_STOREKEY_TYPE << ","
		<< T_STOREKEY_TIMESTAMP << ","
		<< T_STOREKEY_SIZE << ","
		<< T_STOREKEY_STOREID
		<< " FROM " << TABLE_STOREKEYBAK
		<< " WHERE 1 ORDER BY " << T_STOREKEY_TIMESTAMP << " ASC LIMIT " << ONCE_DELETE_STOREKEYBAK_COUNT;

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	StoreKey_t tKey;
	LIST_STOREKEY lstKey;

	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		memset(&tKey, 0, sizeof(StoreKey_t));

		// tKey
		tKey.dwDeviceID = (DWORD)row[T_STOREKEY_DEVICEID];
		tKey.dwRoomID = (DWORD)row[T_STOREKEY_ROOMID];
		tKey.bType = (BYTE)row[T_STOREKEY_TYPE];
		tKey.bRecReason = (BYTE)row[T_STOREKEY_REASON];
		tKey.dwSize = (DWORD)row[T_STOREKEY_SIZE];
		tKey.dwStoreID = (DWORD)row[T_STOREKEY_STOREID];

		std::string strTimestamp = (std::string)row[T_STOREKEY_TIMESTAMP];
		
		int nCpyLen = strTimestamp.size() < LENGTH_TIMESTAMP2 ? strTimestamp.size() : LENGTH_TIMESTAMP2;
		memcpy(tKey.szTimeStamp, strTimestamp.c_str(), nCpyLen);

		lstKey.push_back(tKey);
#if DEBUG_LOG_SDB
		LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 09 %s dwDeviceID %d dwRoomID %d dwStoreID %d szTimeStamp %s\n",
			__FUNCTION__, tKey.dwDeviceID, tKey.dwRoomID, tKey.dwStoreID, tKey.szTimeStamp);
#endif
	}
	ToStorageEngine(lstKey);

#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 09 %s Count %d\n", __FUNCTION__, ONCE_DELETE_STOREKEYBAK_COUNT);
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 09 before %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
	query << "DELETE FROM " << TABLE_STOREKEYBAK 
		<< " WHERE 1 ORDER BY " << T_STOREKEY_TIMESTAMP << " ASC LIMIT " << ONCE_DELETE_STOREKEYBAK_COUNT;
	if (-1 == CatchException(query)) return false;
	m_nStoreKeybakCount -= ONCE_DELETE_STOREKEYBAK_COUNT;
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 10 after %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
#endif // #if defined(DBSERVER_SDB)
	return true;
}

bool CStorageDataBaseHandle::Delete_StoreKeyByTime()
{
#if defined(DBSERVER_SDB)
	//DELETE FROM storekey WHERE DATEDIFF(curdate(), timestamp)>=7
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT count(*) FROM " << TABLE_STOREKEY
		<< " WHERE DATEDIFF(curdate(), " << T_STOREKEY_TIMESTAMP << ")>=" << KEEP_DAYS_STOREKEY;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s size %d\n", __FUNCTION__, res.size());
		return false;
	}

	Row row = *(res.begin());
	int nOvertimeCount = (int)row["count(*)"];
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 11 %s nOvertimeCount %d\n", __FUNCTION__, nOvertimeCount);
#endif
	if (nOvertimeCount <= 0) return true;

	// reload Count
	if (nOvertimeCount > m_nStoreKeyCount)
	{
		Query_TotalCount();
		if (nOvertimeCount > m_nStoreKeyCount)
		{
			return true;
		}
	}

#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 12 before %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
	query << "INSERT INTO " << TABLE_STOREKEYBAK
		<< " SELECT * FROM " << TABLE_STOREKEY
		<< " WHERE DATEDIFF(curdate(), " << T_STOREKEY_TIMESTAMP << ")>=" << KEEP_DAYS_STOREKEY;
	if (-1 == CatchException(query)) return false;
	m_nStoreKeybakCount += nOvertimeCount;

	query << "DELETE FROM " << TABLE_STOREKEY
		<< " WHERE DATEDIFF(curdate(), " << T_STOREKEY_TIMESTAMP << ")>=" << KEEP_DAYS_STOREKEY;
	if (-1 == CatchException(query)) return false;
	m_nStoreKeyCount -= nOvertimeCount;
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 13 after %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
#endif // #if defined(DBSERVER_SDB)
	return true;
}

bool CStorageDataBaseHandle::Delete_StoreKeybakByTime()
{
#if defined(DBSERVER_SDB)
	//DELETE FROM storekeybak WHERE DATEDIFF(curdate(), timestamp)>=30
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_STOREKEY_DEVICEID << ","
		<< T_STOREKEY_ROOMID << ","
		<< T_STOREKEY_REASON << ","
		<< T_STOREKEY_TYPE << ","
		<< T_STOREKEY_TIMESTAMP << ","
		<< T_STOREKEY_SIZE << ","
		<< T_STOREKEY_STOREID
		<< " FROM " << TABLE_STOREKEYBAK
		<< " WHERE DATEDIFF(curdate(), " << T_STOREKEY_TIMESTAMP << ")>=" << KEEP_DAYS_STOREKEYBAK;

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	int nOvertimeCount = res.size();
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 15 %s nOvertimeCount %d\n", __FUNCTION__, nOvertimeCount);
#endif
	if (nOvertimeCount <= 0) return true;

	// reload Count
	if (nOvertimeCount > m_nStoreKeybakCount)
	{
		Query_TotalCount();
		if (nOvertimeCount > m_nStoreKeybakCount)
		{
			return true;
		}
	}

	StoreKey_t tKey;
	LIST_STOREKEY lstKey;

	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		memset(&tKey, 0, sizeof(StoreKey_t));

		// tKey
		tKey.dwDeviceID = (DWORD)row[T_STOREKEY_DEVICEID];
		tKey.dwRoomID = (DWORD)row[T_STOREKEY_ROOMID];
		tKey.bType = (BYTE)row[T_STOREKEY_TYPE];
		tKey.bRecReason = (BYTE)row[T_STOREKEY_REASON];
		tKey.dwSize = (DWORD)row[T_STOREKEY_SIZE];
		tKey.dwStoreID = (DWORD)row[T_STOREKEY_STOREID];

		std::string strTimestamp = (std::string)row[T_STOREKEY_TIMESTAMP];
		
		int nCpyLen = strTimestamp.size() < LENGTH_TIMESTAMP2 ? strTimestamp.size() : LENGTH_TIMESTAMP2;
		memcpy(tKey.szTimeStamp, strTimestamp.c_str(), nCpyLen);

		lstKey.push_back(tKey);
#if DEBUG_LOG_SDB
		LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 16 %s dwDeviceID %d dwRoomID %d dwStoreID %d szTimeStamp %s\n",
			__FUNCTION__, tKey.dwDeviceID, tKey.dwRoomID, tKey.dwStoreID, tKey.szTimeStamp);
#endif
	}
	ToStorageEngine(lstKey);

#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 16 before %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
	query << "DELETE FROM " << TABLE_STOREKEYBAK
		<< " WHERE DATEDIFF(curdate(), " << T_STOREKEY_TIMESTAMP << ")>=" << KEEP_DAYS_STOREKEYBAK;
	if (-1 == CatchException(query)) return false;

	m_nStoreKeybakCount -= nOvertimeCount;
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 17 after %s m_nStoreKeyCount %d m_nStoreKeybakCount %d\n", __FUNCTION__, m_nStoreKeyCount, m_nStoreKeybakCount);
#endif
#endif // #if defined(DBSERVER_SDB)
	return true;
}

void CStorageDataBaseHandle::CheckTotalCount_StoreKey()
{
#if DEBUG_LOG_SDB
	LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 18 %s m_nStoreKeyCount %d:%d m_nStoreKeybakCount %d:%d\n",
		__FUNCTION__, m_nStoreKeyCount, MAX_STOREKEY_COUNT, m_nStoreKeybakCount, MAX_STOREKEYBAK_COUNT);
#endif
	if (m_nStoreKeyCount > MAX_STOREKEY_COUNT)
	{
		Delete_StoreKeyByCount();
	}

	if (m_nStoreKeybakCount > MAX_STOREKEYBAK_COUNT)
	{
		Delete_StoreKeybakByCount();
	}
}

void CStorageDataBaseHandle::CheckOvertime_StoreKey()
{
	Delete_StoreKeyByTime();
	Delete_StoreKeybakByTime();
}

//查询访客
bool CStorageDataBaseHandle::Query_Visitor(StoreVisitor_t& tVisitor, LIST_DWORD& lstRoomID,LIST_STORE_ACCOUNTKEYS& lstAccountKeys)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s\n",__FUNCTION__);
#if defined(DBSERVER_SDB)
	DWORD startIndex = tVisitor.startIndex;
	DWORD tCount = tVisitor.dwCount;
	DWORD NeedCount = startIndex + tCount;
	DWORD dwDeviceID = tVisitor.tKey.dwDeviceID;
	LOG_DEBUG(LOG_DB_SERVER,"%d,%d,%d\n",startIndex,tCount,dwDeviceID);

	char pName[20] = {0};
	char pYM[10] = {0};
	GetNameYM(pName,pYM);
	LOG_DEBUG(LOG_DB_SERVER,"%s,%s\n",pName,pYM);

	DWORD nCount = 0;
	LIST_STOREKEY lstStoreKey;
	for(int i = 0; i < 24; i++)
	{
		LIST_STOREKEY lstKey;
		Query_StorageKeys2(pName, pYM, dwDeviceID, lstRoomID, lstKey);
		LOG_DEBUG(LOG_DB_SERVER,"Count:%d\n",lstKey.size());
		lstStoreKey.splice(lstStoreKey.end(),lstKey);
		nCount = lstStoreKey.size();
		LOG_DEBUG(LOG_DB_SERVER,"nCount:%d,Need:%d=(%d+%d)\n",nCount,NeedCount,startIndex,tCount);
		if(nCount >= NeedCount){ break; }
		GetNextNameYM(pName,pYM);
		LOG_DEBUG(LOG_DB_SERVER,"%s,%s\n",pName,pYM);
	}
	if(lstStoreKey.size() <= 0)		{LOG_DEBUG(LOG_DB_SERVER,"lstStoreKey.size <= 0\n"); return false;}
	
	StoreAccountKeys_t tAccountKeys;
	LIST_STOREKEY::iterator iter = lstStoreKey.begin();
	tAccountKeys.tAccount.dwStoreID = iter->dwStoreID;
	Query_StorageAcount2(tAccountKeys.tAccount);
		
	LOG_DEBUG(LOG_DB_SERVER,"startIndex=%d,tCount=%d,lstStoreKey.size=%d\n",startIndex,tCount,lstStoreKey.size());
	for(int j = 0; iter != lstStoreKey.end(); iter++,j++)
	{
		if( j >= startIndex && j < NeedCount)
		{
			tAccountKeys.lstKey.push_back(*iter);
			LOG_DEBUG(LOG_DB_SERVER,"%d,%d,%d,%s\n",j+1,iter->dwDeviceID,iter->dwRoomID,iter->szTimeStamp);
		}
	}
	lstAccountKeys.push_back(tAccountKeys);
#endif
	return true;
}

bool CStorageDataBaseHandle::Query_StorageKeys2(PCHAR pName,PCHAR pYM, DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_STOREKEY& listKey)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s DeviceID %d RoomID count %d\n", __FUNCTION__, dwDeviceID, lstRoomID.size());
#if defined(DBSERVER_SDB)
	if (lstRoomID.size() <= 0) return true;
	CSTRING strRoomID; MkRoomIDStr(pName, lstRoomID, strRoomID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << pName << "." << T_STOREKEY_DEVICEID << ","
		<< pName << "." << T_STOREKEY_ROOMID << ","
		<< pName << "." << T_STOREKEY_REASON << ","
		<< pName << "." << T_STOREKEY_TYPE << ","
		<< pName << "." << T_STOREKEY_TIMESTAMP << ","
		<< pName << "." << T_STOREKEY_SIZE << ","
		<< pName << "." << T_STOREKEY_STOREID 
		<< " FROM " << pName 
		<< " WHERE " << pName << "." << T_STOREKEY_DEVICEID << "=" << dwDeviceID
		<< " AND " << strRoomID.c_str()
		<< " AND " << pName << "." << T_STOREKEY_TIMESTAMP << " LIKE '" << pYM << "%'"
		<< " ORDER BY " << T_STOREKEY_TIMESTAMP << " DESC";
 
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() == 0) return false;
	
	StoreKey_t tKey;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		memset(&tKey, 0, sizeof(StoreKey_t));
		// tKey
		tKey.dwDeviceID = dwDeviceID;
		tKey.dwRoomID = (DWORD)row[T_STOREKEY_ROOMID];
		tKey.bType = (BYTE)row[T_STOREKEY_TYPE];
		tKey.bRecReason = (BYTE)row[T_STOREKEY_REASON];
		tKey.dwSize = (DWORD)row[T_STOREKEY_SIZE];
		tKey.dwStoreID = (DWORD)row[T_STOREKEY_STOREID];
		std::string strTimestamp = (std::string)row[T_STOREKEY_TIMESTAMP];
		int nCpyLen = strTimestamp.size() < LENGTH_TIMESTAMP2 ? strTimestamp.size() : LENGTH_TIMESTAMP2;
		memcpy(tKey.szTimeStamp, strTimestamp.c_str(), nCpyLen);
		LOG_DEBUG(LOG_DB_SERVER,"%d,%d,%s\n",tKey.dwDeviceID,tKey.dwRoomID,tKey.szTimeStamp);
		listKey.push_back(tKey);
	}
#endif
	return true;
}

bool CStorageDataBaseHandle::Query_StorageAcount2(StorageAccount_t& tAccount)
{
	LOG_DEBUG(LOG_DB_SERVER, "CStorageDataBaseHandle::%s\n", __FUNCTION__);
#if defined(DBSERVER_SDB)
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT * FROM " << TABLE_STORAGE << " WHERE " << T_STORAGE_STOREID << "=" << tAccount.dwStoreID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0) { 	LOG_DEBUG(LOG_DB_SERVER, "Can't Find StoreAccount\n"); return false; }

	Row row = *(res.begin());
	memset(&tAccount, 0, sizeof(StorageAccount_t));
	tAccount.dwStoreID = (DWORD)row[T_STORAGE_STOREID];

	std::string strAccessKey = (std::string)row[T_STORAGE_ACCESSKEY];
	std::string strSecretKey = (std::string)row[T_STORAGE_SECRETKEY];
	std::string strBucket = (std::string)row[T_STORAGE_BUCKET];
	std::string strDomain = (std::string)row[T_STORAGE_DOMAIN];

	int nCpyLen = strAccessKey.size() < LENGTH_ACCESSKEY ? strAccessKey.size() : LENGTH_ACCESSKEY;
	memcpy(tAccount.szAccessKey, strAccessKey.c_str(), nCpyLen);
	nCpyLen = strSecretKey.size() < LENGTH_SECRETKEY ? strSecretKey.size() : LENGTH_SECRETKEY;
	memcpy(tAccount.szSecretKey, strSecretKey.c_str(), nCpyLen);
	nCpyLen = strBucket.size() < LENGTH_BUCKET ? strBucket.size() : LENGTH_BUCKET;
	memcpy(tAccount.szBucket, strBucket.c_str(), nCpyLen);
	nCpyLen = strDomain.size() < LENGTH_DOMAIN ? strDomain.size() : LENGTH_DOMAIN;
	memcpy(tAccount.szDomain, strDomain.c_str(), nCpyLen);

	LOG_DEBUG(LOG_DB_SERVER,"AccessKey = %s\n",tAccount.szAccessKey);
	LOG_DEBUG(LOG_DB_SERVER,"SecretKey = %s\n",tAccount.szSecretKey);
	LOG_DEBUG(LOG_DB_SERVER,"Bucket = %s\n",tAccount.szBucket);
	LOG_DEBUG(LOG_DB_SERVER,"Domain = %s\n",tAccount.szDomain);
#endif
	return true;
}

//特殊人群短信通知入口
bool CStorageDataBaseHandle::SpecialCrowds(LIST_SPECIALCROWD& m_lstSpecialCrowd)
{
	LOG_DEBUG(LOG_DB_SERVER, "CStorageDataBaseHandle::%s\n",__FUNCTION__);
//	IDBHandle* pDBHandle = DBHandle_GetDBHandle();
//	pDBHandle->Query_SpecialUsers(m_lstSpecialCrowd);
//	LIST_SPECIALCROWD::iterator iter = m_lstSpecialCrowd.begin();
//	for(; m_lstSpecialCrowd.end() != iter;  ++iter)
//	{
//		LOG_DEBUG(LOG_DB_SERVER, "VendorID %d GroupID %d DeviceID %d RoomID %d UserID %d Phone %s PUserID %d PPhone %s Time %d\n", iter->dwVendorID, iter->dwGroupID,  iter->dwDevcieID,  iter->dwRoomID, iter->dwUserID, iter->szUserPhone,  iter->dwPropertyUserID,  iter->szPropertyPhone, iter->dwUnlockTime);
//	}
	return true;
}

//查询特殊人群最新开门记录时间
bool CStorageDataBaseHandle::Query_SpecialCrowdInfo(LIST_SPECIALCROWD& lstSpecialCrowd, LIST_SMSINFO2& lstSmsInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"CStorageDataBaseHandle::%s\n", __FUNCTION__);
	char tabName[LENGTH_TABLENAME] = {"unlocklog"};
	GetTabName(tabName);
	LOG_DEBUG(LOG_DB_SERVER,"%s tabName = %s\n",__FUNCTION__, tabName);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");

	LIST_SPECIALCROWD::iterator iter = lstSpecialCrowd.begin();
	for( ; iter != lstSpecialCrowd.end(); iter++)
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s DeviceID = %d UserID = %d\n", __FUNCTION__, iter->dwDevcieID, iter->dwUserID);
		query << "SELECT " << T_UNLOCKLOG_TIMESTAMP
			<< " FROM " << tabName 
			<< " WHERE " << T_UNLOCKLOG_USERID << "=" << iter->dwUserID
			<< " AND " << T_UNLOCKLOG_DEVICEID << "=" << iter->dwDevcieID
			<< " ORDER BY id DESC";
		StoreQueryResult res;
		if (-1 == CatchException(query, res)) return false;
		SmsInfo2_t tSmsInfo;
		if (res.size() <= 0) { 
//			LOG_DEBUG(LOG_DB_SERVER,"%s The %s query result is empty\n",__FUNCTION__, tabName);
			if( true == Query_OnSpecialCrowdInfo(tabName, *iter))
			{
				tSmsInfo.dwUserID = iter->dwUserID;
				tSmsInfo.dwRoomID = iter->dwRoomID;
				tSmsInfo.dwVendorID =  iter->dwVendorID;
				tSmsInfo.bLanguage = 0;
				memcpy(tSmsInfo.szMobilePhone,  iter->szUserPhone, sizeof(iter->szUserPhone));
				memcpy(tSmsInfo.szPropertyPhone,  iter->szPropertyPhone, sizeof(iter->szPropertyPhone));
				memcpy(tSmsInfo.szDeviceName, iter->szDeviceName, sizeof(iter->szDeviceName));
//				LOG_DEBUG(LOG_DB_SERVER,"UserID = %d, VendorID = %d, Language = %d, Phone = %s\n", 
//					tSmsInfo.dwUserID, tSmsInfo.dwVendorID,  tSmsInfo.bLanguage,  tSmsInfo.szMobilePhone);
				lstSmsInfo.push_back(tSmsInfo);
			}		
			continue;				
		}
		Row row = *res.begin();
		iter->dwUnlockTime = (DWORD)row[T_UNLOCKLOG_TIMESTAMP];
		LOG_DEBUG(LOG_DB_SERVER, "%s dwTimestamp = %d\n", __FUNCTION__, iter->dwUnlockTime);
		//判断是否发送短信
		if(true == IsSmsUser(iter->dwUnlockTime))
		{
			//发短信通知用户,调用发送短信接口
			LOG_DEBUG(LOG_DB_SERVER,"%s SMS notification Phone \n", __FUNCTION__);
			tSmsInfo.dwUserID =  iter->dwUserID;
			tSmsInfo.dwRoomID = iter->dwRoomID;
			tSmsInfo.dwVendorID =  iter->dwVendorID;
			tSmsInfo.bLanguage = 0;
			memcpy(tSmsInfo.szMobilePhone, iter->szUserPhone, sizeof(iter->szUserPhone));
			memcpy(tSmsInfo.szPropertyPhone,  iter->szPropertyPhone, sizeof(iter->szPropertyPhone));
			memcpy(tSmsInfo.szDeviceName, iter->szDeviceName, sizeof(iter->szDeviceName));
//			LOG_DEBUG(LOG_DB_SERVER,"UserID = %d, VendorID = %d, Language = %d, Phone = %s\n", 
//				tSmsInfo.dwUserID, tSmsInfo.dwVendorID,  tSmsInfo.bLanguage,  tSmsInfo.szMobilePhone);
			lstSmsInfo.push_back(tSmsInfo);
		}
	}
	return true;
}

bool CStorageDataBaseHandle::Query_OnSpecialCrowdInfo(char* pName, SpecialCrowd_t& tInfo)
{
//	LOG_DEBUG(LOG_DB_SERVER,"CStorageDataBaseHandle::%s %s\n", __FUNCTION__, pName);
	char tabName[LENGTH_TABLENAME] = {"unlocklog"};
	strcpy(tabName, pName);
	GetOnTabName(tabName);
//	LOG_DEBUG(LOG_DB_SERVER,"%s %s\n", __FUNCTION__, tabName);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_UNLOCKLOG_TIMESTAMP
		<< " FROM " << tabName 
		<< " WHERE " << T_UNLOCKLOG_USERID << "=" << tInfo.dwUserID 
		<< " AND " << T_UNLOCKLOG_DEVICEID << "=" << tInfo.dwDevcieID
		<< " ORDER BY id DESC";

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0) { 
		LOG_DEBUG(LOG_DB_SERVER,"The query result is empty\n");	
		tInfo.dwUnlockTime = 0;
		return false;
	}
	Row row = *res.begin();
	tInfo.dwUnlockTime = (DWORD)row[T_UNLOCKLOG_TIMESTAMP];
	if(true == IsSmsUser(tInfo.dwUnlockTime))
	{
		LOG_DEBUG(LOG_DB_SERVER,"%s SMS notification Phone \n", __FUNCTION__);
	}
	return true;
}