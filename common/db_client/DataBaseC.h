#pragma once
#include "DataBaseInterface.h"
#include "singleton.h"
#include "ElemMap_DataBase.h"

class CDataBaseC : public IStorageBusiness
{
public:
	CDataBaseC(DWORD dwID, IStorageBusinessSink* pSink);
	virtual ~CDataBaseC();

	DWORD GetID(){return m_dwID;}
	IStorageBusinessSink* GetDataBaseSink() { return m_pSink; }

	// IStorageBusiness interface
	int GetAdvertInfo			(DWORD dwDeviceID,LIST_DWORD& lstAdvertID);
	int ReportAdvertProgress(DWORD dwDeviceID, LIST_PROGRESS& lstProgress);
private:
	DWORD m_dwID;
	IStorageBusinessSink* m_pSink;
};

class CDataBaseCMgr : public CElemMapDataBase<CDataBaseC>
{
	DECLARE_SINGLETON(CDataBaseCMgr)
public:
	CDataBaseCMgr(){}
	~CDataBaseCMgr(){}

	IStorageBusinessSink* GetDataBaseSink(DWORD dwID);
};