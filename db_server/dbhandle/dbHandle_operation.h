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
	bool Local_GetAdvertUrl(CSTRING& strName, CSTRING& AdUrl);//���ɱ��ع��URL��Ϣ
	//query
	bool Query_Advert(DWORD dwAdID, AdvertInfo_t& tInfo); //��ѯ�����Ϣ
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
	// �����Ѿ�����ţ���غõĹ���б�
	LIST_LOCAL_FILE m_listLocalFile;
	// ����û�д���ţ���ع��Ĺ���б�
	LIST_DWORD m_lstAdID;
	// ������Ҫ������Ƶ�����豸���б�
	LIST_DEV_ADVERT_ID m_lstDevAdID;
	// IOperationDBHandleSink�ص���
	IOperationDBHandleSink* m_pSink;// ��Sink����CAppServerMgr
	// ������Ƶ����ʶ 0 - �����ػ����������  1 - �������������ڽ���
	DWORD  m_dwAdFlag; 
	// �������ؽ��ȱ�ʶ
	DWORD m_dwProgress;
	//�����Ƿ�������� false - û��������� �� true - ������ػ�û����������
	bool m_bFinish;
	//�������ص��豸�͹��
	DevAdvertID_t m_tInfo;
	// ���س�ʱ��־
	DWORD m_dwTimeout;
private:
	int CatchException(Query& query);
	int CatchException(Query& query, StoreQueryResult& res);
	Connection m_clsSrcDBCon;
	int m_nError;
	bool m_bConnectSuccess;
};
