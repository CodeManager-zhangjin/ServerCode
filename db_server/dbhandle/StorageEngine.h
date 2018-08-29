#pragma once

#include "DataStruct.h"
#include "Thread.h"
#include "singleton.h"

struct QiniuStoreKey_t
{
	BYTE accesskey[LENGTH_ACCESSKEY+1];
	BYTE secretkey[LENGTH_SECRETKEY+1];
	BYTE bucket[LENGTH_BUCKET+1];
	BYTE key[LENGTH_STOREKEY+1];
};
typedef std::list<QiniuStoreKey_t>	LIST_QINIUSTOREKEY;

class CStorageEngine : public CThread
{
	DECLARE_SINGLETON(CStorageEngine)
public:
	CStorageEngine();
	virtual ~CStorageEngine();

	bool EngineStart();
	void EngineStop();

	// CThread
	void ThreadLoop();

	int  Add_Job(LIST_QINIUSTOREKEY& lstJob);
	void Get_Job(LIST_QINIUSTOREKEY& lstJob);
	void WorkJob(LIST_QINIUSTOREKEY& lstJob);

private:
	LIST_QINIUSTOREKEY m_lstJob;

};
