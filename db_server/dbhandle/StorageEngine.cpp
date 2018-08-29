#include "StorageEngine.h"
#include "UtilityInterface.h"
#include "Log.h"
#include "qiniuInterface.h"

#define MAX_JOB_NUM			100000
#define ONCE_GET_JOB_NUM	10

IMPLEMENT_SINGLETON( CStorageEngine )
CStorageEngine::CStorageEngine()
{
}

CStorageEngine::~CStorageEngine()
{
	EngineStop();
}

void CStorageEngine::ThreadLoop()
{
	Qiniu_Init();

	while (m_bRunning)
	{
		HangUpThread();

		LIST_QINIUSTOREKEY lstJob;
		Get_Job(lstJob);
		WorkJob(lstJob);
	}

	Qiniu_Finish();
	LOG_DEBUG(LOG_MAIN, "CStorageEngine::%s TreadID:%d Exit\n", __FUNCTION__, GetThreadID());
}

int CStorageEngine::Add_Job(LIST_QINIUSTOREKEY& lstJob)
{
	Lock();
	m_lstJob.insert(m_lstJob.end(), lstJob.begin(), lstJob.end());
	int nSize = m_lstJob.size();

//	LOG_DEBUG(LOG_MAIN, "CStorageEngine ThreadPool_Add_Job JobCount %d\n", nSize);

	// 检查最大job数
	int nPopSize = 0;
	if (nSize > MAX_JOB_NUM)
	{
		nPopSize = nSize - MAX_JOB_NUM;
		LOG_WARN(LOG_MAIN, "CStorageEngine Skip %d Jobs\n", nPopSize);
		for (int i = 0; i < nPopSize; i++)
		{
			m_lstJob.pop_front();
		}
	}
	UnLock();

	for (int i = 0; i < nSize; i++)
	{
		ActivateThread();
	}
	return 0;
}

void CStorageEngine::Get_Job(LIST_QINIUSTOREKEY& lstJob)
{
	Lock();
	if (m_lstJob.size() <= 0)
	{
		UnLock();
		return;
	}

	lstJob.clear();
	int i = 0;
	LIST_QINIUSTOREKEY::iterator iter = m_lstJob.begin(), iterTemp;
	while (iter != m_lstJob.end())
	{
		iterTemp = iter; iterTemp++;
		lstJob.push_back(*iter);
		m_lstJob.pop_front();
		if (++i >= ONCE_GET_JOB_NUM)
		{
			break;
		}
		iter = iterTemp;
	}
	UnLock();
}

void CStorageEngine::WorkJob(LIST_QINIUSTOREKEY& lstJob)
{
	LIST_QINIUSTOREKEY::iterator iter = lstJob.begin();
	for (; iter != lstJob.end(); iter++)
	{
		bool ret = Qiniu_DeleteFile((const char*)iter->accesskey, 
									(const char*)iter->secretkey, 
									(const char*)iter->bucket, 
									(const char*)iter->key);
		LOG_DEBUG(LOG_DB_SERVER, "TEST_LOG 99 DeleteFile accesskey:%s secretkey:%s bucket:%s key:%s ret:%d\n",
			iter->accesskey, iter->secretkey, iter->bucket, iter->key, ret);
	}
}

bool CStorageEngine::EngineStart()
{
	return ThreadStart();
}

void CStorageEngine::EngineStop()
{
	ThreadStop();
}
