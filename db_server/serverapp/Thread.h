#pragma once

#include <semaphore.h>
#include "Lock.h"

class CThread : public CLock
{
public:
			 CThread();
	virtual ~CThread();
	virtual void ThreadLoop() = 0;

	int GetThreadID(){ return (int)m_nThreadID; }
	//////////////////////////////////////////////////////////////////////////
	bool ThreadStart	();	// �߳�����
	bool ThreadStop		();	// �߳�ֹͣ
	int  ActivateThread	();	// �����߳�
	int  HangUpThread	(); // �����߳�
	bool m_bRunning;

private:
	static void* ThreadFunction	(void* param);
	void DoTask();
	pthread_t m_nThreadID;
	sem_t m_sem;
};
