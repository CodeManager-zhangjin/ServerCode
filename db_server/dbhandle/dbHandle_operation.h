#pragma once
#include <stdio.h>
#include <errno.h>
#include <mysql++.h>
#include "singleton.h"
#include "DataStruct.h"
#include "qiniuInterface.h"
#include "UtilityInterface.h"
#include "dbHandleInterface.h"

using namespace mysqlpp;

class COperationDataBaseHandle : public IOperationDBHandle, public ITimerSink
{
	DECLARE_SINGLETON( COperationDataBaseHandle )
public:
	COperationDataBaseHandle();
	virtual ~COperationDataBaseHandle();
	int GetError();

	bool IsConnect(){ return m_bConnectSuccess; }
	bool Connect(PUCHAR pHost, PUCHAR pDatabase, PUCHAR pUserName, PUCHAR pPassword);
	
	void SetSink(IOperationDBHandleSink* pSink) {m_pSink = pSink;}
	void OnTimer();  
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

	bool AdvertDownloadMgr();
	bool AdvertProgressMgr();

	bool AdvertInfoMgr(DWORD& dwID, LIST_DWORD& lstAdvertID, LIST_ADVERTINFO& lstAdvertInfo, LIST_DWORD& lstVideoAdvertID);
	bool GetQiniuDownloadUrl(LIST_ADVERTINFO& lstAdvertInfo, StorageAccount_t& tAccount, LIST_ADVERTINFO_REP& lstAdvertInfoRep);		
	bool Qiniu_GetAdvertUrl(AdvertInfo_t& tInfo, StorageAccount_t&tAccount, CSTRING& AdUrl);
	bool Local_GetAdvertUrl(CSTRING& strName, CSTRING& AdUrl);//生成本地广告URL信息
	//query
	bool Query_Advert(DWORD dwAdID, AdvertInfo_t& tInfo); //查询广告信息
	bool Query_StorageKeys(DWORD dwStoreID, StorageAccount_t& tAccount);
public:	
	bool AddTaskList(DWORD dwDeviceID, LIST_DWORD& lstAdID);
	bool AddLocalProgress(DWORD dwDeviceID, LIST_PROGRESS& lstProgress);
	bool GetQiniuList(LIST_DWORD& lstAdID);
	bool ClearQiniuList(DWORD& dwAdID);

	bool CheckSetAdID(DWORD dwAdID);
	bool CheckLstAdName(CSTRING& AdName);
	bool DeleteLstDevAdID(DevAdvertID_t& m_tInfo);
	bool CheckTaskList(DevAdvertID_t& tInfo);
	bool CheckQiniuList(DWORD dwAdID);
	bool LocalDiskAdvertList();
private:
	// 所有已经从七牛下载好的广告列表
	LIST_LOCAL_FILE m_listLocalFile;
	// 所有没有从七牛下载过的广告列表
	LIST_DWORD m_lstAdID;
	// 所有需要下载视频广告的设备的列表
	LIST_DEV_ADVERT_ID m_lstDevAdID;
	// IOperationDBHandleSink回调用
	IOperationDBHandleSink* m_pSink;// 该Sink就是CAppServerMgr
	// 下载视频广告标识 0 - 无下载或无任务进行  1 - 有下载任务正在进行
	DWORD  m_dwAdFlag; 
	// 本地下载进度标识
	DWORD m_dwProgress;
	//本地是否完成下载 false - 没有完成下载 ， true - 完成下载或没有下载任务
	bool m_bFinish;
	//正在下载的设备和广告
	DevAdvertID_t m_tInfo;
	// 下载超时标志
	DWORD m_dwTimeout;
private:
	int CatchException(Query& query);
	int CatchException(Query& query, StoreQueryResult& res);
	Connection m_clsSrcDBCon;
	int m_nError;
	bool m_bConnectSuccess;
};
