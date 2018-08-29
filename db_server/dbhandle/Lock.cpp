// Lock.cpp: implementation of the CLock class.
//
//////////////////////////////////////////////////////////////////////

#include "Lock.h"
#include <stdio.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLock::CLock()
{
	pthread_mutex_init(&m_mutex, NULL);
}

CLock::~CLock()
{
	pthread_mutex_destroy(&m_mutex);
}

int CLock::Lock(bool bPrint /* = false */, int nIndex /* = 0 */)
{
	if (bPrint) printf("Lock Index %d\n", nIndex);
	return pthread_mutex_lock(&m_mutex);
}

int CLock::UnLock(bool bPrint /* = false */, int nIndex /* = 0 */)
{
	if (bPrint) printf("UnLock Index %d\n", nIndex);
	return pthread_mutex_unlock(&m_mutex);
}
