// TimerManager.cpp: implementation of the CTimerManager class.
//
//////////////////////////////////////////////////////////////////////

#include "TimerManager.h"
#include "Log.h"
#include <algorithm>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
IMPLEMENT_SINGLETON(CTimerManager)
CTimerManager::CTimerManager()
{
	m_u64TTL = 0;
	m_pCurSink = NULL;
	m_pTimer = CreateNetTimer(this);
	LOG_ASSERT_RETVOID(LOG_UTIL, m_pTimer);
	m_pTimer->Schedule(1000, NULL);
	m_bStopTimer = false;
}

void CTimerManager::StopTimer()
{
	if(m_pTimer) {
		NetworkDestroyTimer(m_pTimer);
		m_pTimer = NULL;
	}
	m_bStopTimer = true;
}

void CTimerManager::OnTimer(void *pArg, INetTimer *pTimer)
{
	if(m_pTimer != pTimer) return;
	++m_u64TTL; //after about 584942417355 years, m_u64TTL will comes back to zero, so don't care of it
	
	MAP_TIMERSINK::iterator posCurMap;
	while(1)
	{
		if(m_bStopTimer) break;
		posCurMap = m_mapTimerSink.begin();
		if(m_mapTimerSink.end() == posCurMap) break;	
		if(posCurMap->first > m_u64TTL) break;

		TimerObject_t tObject;
		tObject.dwInterval = posCurMap->second.dwInterval;
		tObject.eReason = posCurMap->second.eReason;
		tObject.pSink = posCurMap->second.pSink;
		INT64 u64TimeOutTTL = posCurMap->first+(INT64)(posCurMap->second.dwInterval);

		m_mapTimerSink.erase(posCurMap); 
		m_mapTimerSink.insert(std::make_pair(u64TimeOutTTL, tObject));
		if(tObject.pSink) tObject.pSink->OnTimer(tObject.eReason, tObject.pSink);
	}
	if(m_bStopTimer) DeleteInstance();
}

void CTimerManager::AddTimer(TimerReason_e eReason, DWORD dwInterval, ITimerSink* pSink)
{
	LOG_ASSERT_RETVOID(LOG_UTIL, pSink && (dwInterval > 0));
	m_pCurSink = pSink;
	m_eCurReason = eReason;
	MAP_TIMERSINK::iterator posMap = find_if( m_mapTimerSink.begin(),
											  m_mapTimerSink.end(),
											  FindTimerSink() );
	if(m_mapTimerSink.end() != posMap) m_mapTimerSink.erase(posMap);

	TimerObject_t tObject;
	tObject.dwInterval = dwInterval;
	tObject.eReason = eReason;
	tObject.pSink = pSink;
	m_mapTimerSink.insert(std::make_pair((INT64)(m_u64TTL+dwInterval), tObject));
}

void CTimerManager::DelTimer(TimerReason_e eReason, ITimerSink* pSink)
{
	LOG_ASSERT_RETVOID(LOG_UTIL, pSink);
	m_pCurSink = pSink;
	m_eCurReason = eReason;
	MAP_TIMERSINK::iterator posMap = find_if( m_mapTimerSink.begin(),
											  m_mapTimerSink.end(),
											  FindTimerSink() );
	if(m_mapTimerSink.end() != posMap) m_mapTimerSink.erase(posMap);
}
