#pragma once

#include <pthread.h>

class CLock  
{
public:
	CLock();
	virtual ~CLock();
	int Lock	(bool bPrint = false, int nIndex = 0);
	int UnLock	(bool bPrint = false, int nIndex = 0);
private:
	pthread_mutex_t m_mutex;
};
