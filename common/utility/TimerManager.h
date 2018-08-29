// TimerManager.h: interface for the CTimerManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TIMERMANAGER_H__6553B3C4_EF44_4D86_A478_8952D812130E__INCLUDED_)
#define AFX_TIMERMANAGER_H__6553B3C4_EF44_4D86_A478_8952D812130E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "singleton.h"
#include "UtilityInterface.h"

typedef struct 
{
	TimerReason_e eReason;
	DWORD dwInterval;
	ITimerSink* pSink;
}TimerObject_t;
typedef std::multimap<INT64/*timeout ttl*/, TimerObject_t> MAP_TIMERSINK;

class CTimerManager : public INetTimerSink 
{
	DECLARE_SINGLETON(CTimerManager)
public:
	CTimerManager();
	virtual ~CTimerManager() {}

	//INetTimerSink
	virtual void OnTimer(void *pArg, INetTimer *pTimer);

	//Interval Unit: second
	void AddTimer(TimerReason_e eReason, DWORD dwInterval, ITimerSink* pSink);
	void DelTimer(TimerReason_e eReason, ITimerSink* pSink);
	INT64 GetTimerTTL() const { return m_u64TTL; }

	ITimerSink* GetCurSink() const { return m_pCurSink; }
	TimerReason_e GetCurReason() const { return m_eCurReason; }
	void StopTimer();

private:
	INetTimer* m_pTimer; 
	INT64 m_u64TTL;
	bool  m_bStopTimer;
	TimerReason_e m_eCurReason;  //for FindTimerSink only
	ITimerSink* m_pCurSink;   //for FindTimerSink only
	
	MAP_TIMERSINK m_mapTimerSink;
};

class FindTimerSink
{
public:
	bool operator() (MAP_TIMERSINK::value_type& pos)
	{
		CTimerManager* pTimerManager = CTimerManager::Instance();
		ITimerSink* pSink = pTimerManager->GetCurSink();
		TimerReason_e eReason = pTimerManager->GetCurReason();
		if( (pSink == pos.second.pSink) && (eReason == pos.second.eReason) ) return true;
		return false;
	}
};

#endif // !defined(AFX_TIMERMANAGER_H__6553B3C4_EF44_4D86_A478_8952D812130E__INCLUDED_)
