// Thread.cpp: implementation of the CThread class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Thread.h"
#include "Log.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CThread::CThread()
{
	m_bRunning = false;
	m_nThreadID = 0;
}

CThread::~CThread()
{
	ThreadStop();
}

bool CThread::ThreadStart()
{
	if (m_bRunning) {
		LOG_DEBUG(LOG_MAIN, "Thread Already Start\n"); return true;
	}
	m_bRunning = true;
	LOG_DEBUG(LOG_MAIN, "ThreadStart Begin...ThreadID %d\n", m_nThreadID);
	//////////////////////////////////////////////////////////////////////////
	sem_init(&m_sem, 0, 0);
	
	int ret = pthread_create(&m_nThreadID, NULL, CThread::ThreadFunction, this);
	if (ret) {
		LOG_DEBUG(LOG_MAIN, "Create pthread error!\n"); return false;
	}
	ActivateThread();
	LOG_DEBUG(LOG_MAIN, "ThreadStart End! ThreadID %d\n", m_nThreadID);
	return true;
}

bool CThread::ThreadStop()
{
	if (false == m_bRunning) {
		LOG_DEBUG(LOG_MAIN, "Thread Already Stop\n"); return true;
	}
	m_bRunning = false;
	LOG_DEBUG(LOG_MAIN, "ThreadStop Begin...ThreadID %d\n", m_nThreadID);
	ActivateThread();
	pthread_join(m_nThreadID, NULL);
	LOG_DEBUG(LOG_MAIN, "pthread_join end\n");
	sem_destroy(&m_sem);

	LOG_DEBUG(LOG_MAIN, "ThreadStop End! ThreadID %d\n", m_nThreadID);
	return true;
}

int CThread::ActivateThread()
{
	return sem_post(&m_sem);
}

int CThread::HangUpThread()
{
	return sem_wait(&m_sem);
}

void* CThread::ThreadFunction(void* param)
{
	CThread* pThread = (CThread*)param;
	if (pThread) pThread->DoTask();
	return NULL;
}

void CThread::DoTask()
{
	ThreadLoop();
	LOG_DEBUG(LOG_MAIN, "::ThreadLoop out(ThreadID %d)\n", m_nThreadID);
}
