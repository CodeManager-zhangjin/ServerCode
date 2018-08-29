#pragma once

#include "DataBaseInterface.h"
#include "singleton.h"
#include "ElemMap_DataBase.h"

class CDataBaseB : public IStorageDB
{
public:
	CDataBaseB(DWORD dwID, IStorageDBSink* pSink);
	virtual ~CDataBaseB();

	DWORD GetID(){return m_dwID;}
	IStorageDBSink* GetDataBaseSink() { return m_pSink; }

	// IStorageDB interface
//	virtual int SDB_AddUnlockItem(LIST_DEVICE_UNLOCK& listInfo);
	virtual int SDB_UnlockItemTunel(PUCHAR pData, int nLen);
	virtual int SDB_ReportUploadResult(PUCHAR pData, int nLen);//d_sdb
	virtual int SDB_AlarmStatus(PUCHAR pData, int nLen);
private:
	DWORD m_dwID;
	IStorageDBSink* m_pSink;
};

class CDataBaseBMgr : public CElemMapDataBase<CDataBaseB>
{
	DECLARE_SINGLETON(CDataBaseBMgr)
public:
	CDataBaseBMgr(){}
	~CDataBaseBMgr(){}

	IStorageDBSink* GetDataBaseSink(DWORD dwID);
};
