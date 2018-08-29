#include "DealMulPkt.h"
#include "Log.h"
#include <algorithm>

DWORD g_dwCmdFlag = 0;

CDealMulPktOne::CDealMulPktOne()
{
	m_nTTL = 0;
	m_wSegFlag = 0;
	m_pSink = NULL;
	m_Iter = m_lstMulPkt.begin();
	AddTimer(TIMER_NORMAL, 3, this);
}

CDealMulPktOne::~CDealMulPktOne()
{
	DelTimer(TIMER_NORMAL, this);
}

void CDealMulPktOne::OnTimer(TimerReason_e eReason, ITimerSink* pSink)
{
	SendNext();
}

void CDealMulPktOne::SetInfo(PUCHAR pData, int nLen, DWORD dwCmdFlag, WORD wSegFlag)
{
	if (m_wSegFlag != wSegFlag) // 不同一组发送包，清空列表
	{
		m_wSegFlag = wSegFlag;
		m_lstMulPkt.clear();
	}

	MulPkt_t tMulPkt;
	tMulPkt.bSend = false;
	tMulPkt.dwCmdFlag = dwCmdFlag;
	tMulPkt.packet.assign((const char*)pData, nLen);
	m_lstMulPkt.push_back(tMulPkt);
//	LOG_DEBUG(LOG_UTIL, "Add MulPkt CmdFlag %d wSegFlag %d list size %d\n", dwCmdFlag, wSegFlag, m_lstMulPkt.size());
}

void CDealMulPktOne::AddMulPktAck( DWORD dwCmdFlag )
{
	if (NULL == m_pSink) return;
	if (m_lstMulPkt.empty()) return;

	if (m_lstMulPkt.front().dwCmdFlag == dwCmdFlag)
	{
		m_lstMulPkt.front().bSend = true;
		//LOG_DEBUG(LOG_UTIL, "1 Del MulPkt CmdFlag %d\n", dwCmdFlag);
	}
	else
	{
		g_dwCmdFlag = dwCmdFlag;
		LIST_MULPKT::iterator iter = std::find_if( m_lstMulPkt.begin(), m_lstMulPkt.end(), FindMulPktByCmdFlag() );
		if (iter != m_lstMulPkt.end())
		{
			iter->bSend = true;
			//LOG_DEBUG(LOG_UTIL, "2 Del MulPkt CmdFlag %d\n", dwCmdFlag);
		}
	}
	SendNext();
}

void CDealMulPktOne::StartSendMulPkt( IDealMulPktSink* pSink )
{
	m_pSink = pSink;

	// 发送第一个包
	if (m_lstMulPkt.empty()) return;

	m_nTTL = 0;
	m_Iter = m_lstMulPkt.begin();
	//LOG_DEBUG(LOG_UTIL, "StartSendMulPkt list size %d\n", m_lstMulPkt.size());
	SendNext();
}

// 发送下一个包并检查循环发送次数，当次数达到3次时，认为本次发送结束
void CDealMulPktOne::SendNext()
{
//	LOG_DEBUG(LOG_UTIL, "SendNext Iter %p Begin %p End %p list size %d\n", m_Iter, m_lstMulPkt.begin(), m_lstMulPkt.end(), m_lstMulPkt.size());
	// 跳过已经发送成功的包
	if (m_lstMulPkt.empty()) return;

	while ( (m_Iter != m_lstMulPkt.end()) && m_Iter->bSend )
	{
//		LOG_DEBUG(LOG_UTIL, "1 Iter %p\n", m_Iter);
		++m_Iter;
//		LOG_DEBUG(LOG_UTIL, "2 Iter %p\n", m_Iter);
	}

	if (m_Iter != m_lstMulPkt.end())
	{
		if (m_Iter->bSend == false) m_pSink->OnSendNext(m_Iter->dwCmdFlag, (PUCHAR)m_Iter->packet.c_str(), (int)m_Iter->packet.size());

		// 下一个发包重新计时
		AddTimer(TIMER_NORMAL, 3, this);

		if (++m_Iter == m_lstMulPkt.end())
		{
			if (++m_nTTL == 3) m_lstMulPkt.clear();
			m_Iter = m_lstMulPkt.begin();
			//LOG_DEBUG(LOG_UTIL, "Send MulPkt TTL %d list size %d\n", m_nTTL, m_lstMulPkt.size());
		}
	}
}

bool CDealMulPktOne::IsSendOver()
{
	if (m_lstMulPkt.empty()) return true;

	LIST_MULPKT::iterator iter = m_lstMulPkt.begin();
	for (; iter != m_lstMulPkt.end(); iter++)
	{
		if (iter->bSend == false) return false;
	}
	return true;
}

CDealMulPkt::CDealMulPkt(DWORD dwDeviceID)
{
	m_dwDeviceID = dwDeviceID;
	m_pSink = NULL;
	AddTimer(TIMER_NORMAL, 30, this);
}

CDealMulPkt::~CDealMulPkt()
{
	DelTimer(TIMER_NORMAL, this);
}

void CDealMulPkt::SetInfo(MulPkt_e eMulPkt, PUCHAR pData, int nLen, DWORD dwCmdFlag, WORD wSegFlag)
{
		 if (eMulPkt == MP_Sum)       m_clsSum.SetInfo(pData, nLen, dwCmdFlag, wSegFlag);
	else if (eMulPkt == MP_UserIndex) m_clsUserindex.SetInfo(pData, nLen, dwCmdFlag, wSegFlag);
	else if (eMulPkt == MP_PushIndex) m_clsPushindex.SetInfo(pData, nLen, dwCmdFlag, wSegFlag);
	else if (eMulPkt == MP_CardIndex) m_clsCardindex.SetInfo(pData, nLen, dwCmdFlag, wSegFlag);
	else if (eMulPkt == MP_Other)	  m_clsOther.SetInfo(pData, nLen, dwCmdFlag, wSegFlag);
	else if (eMulPkt == MP_IndoorIndex) m_clsIndoorindex.SetInfo(pData, nLen, dwCmdFlag, wSegFlag);
//	else if (eMulPkt == MP_PushSwitchIndex) m_clsPushSwitchindex.SetInfo(pData, nLen, dwCmdFlag, wSegFlag);
}

void CDealMulPkt::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	if (m_clsSum.IsSendOver() &&
		m_clsUserindex.IsSendOver() &&
		m_clsPushindex.IsSendOver() &&
		m_clsCardindex.IsSendOver() &&
		m_clsOther.IsSendOver() && 
		m_clsPushSwitchindex.IsSendOver()
		)
	{
		//LOG_DEBUG(LOG_UTIL, "Delete dwDeviceID %d pSink %p CDealMulPkt %p\n", m_dwDeviceID, m_pSink, this);
		CDealMulPktMgr::Instance()->DelElem(m_dwDeviceID);
	}
}

void CDealMulPkt::AddMulPktAck( MulPkt_e eMulPkt, DWORD dwCmdFlag )
{
	     if (eMulPkt == MP_Sum)       m_clsSum.AddMulPktAck(dwCmdFlag);
	else if (eMulPkt == MP_UserIndex) m_clsUserindex.AddMulPktAck(dwCmdFlag);
	else if (eMulPkt == MP_PushIndex) m_clsPushindex.AddMulPktAck(dwCmdFlag);
	else if (eMulPkt == MP_CardIndex) m_clsCardindex.AddMulPktAck(dwCmdFlag);
	else if (eMulPkt == MP_Other) m_clsOther.AddMulPktAck(dwCmdFlag);
	else if (eMulPkt == MP_IndoorIndex) m_clsIndoorindex.AddMulPktAck(dwCmdFlag);
//	else if (eMulPkt == MP_PushSwitchIndex) m_clsPushSwitchindex.AddMulPktAck(dwCmdFlag);
}

void CDealMulPkt::StartSendMulPkt( MulPkt_e eMulPkt, IDealMulPktSink* pSink )
{
	//LOG_DEBUG(LOG_UTIL, "Add dwDeviceID %d pSink %p CDealMulPkt %p\n", m_dwDeviceID, pSink, this);
	     if (eMulPkt == MP_Sum)       m_clsSum.StartSendMulPkt(pSink);
	else if (eMulPkt == MP_UserIndex) m_clsUserindex.StartSendMulPkt(pSink);
	else if (eMulPkt == MP_PushIndex) m_clsPushindex.StartSendMulPkt(pSink);
	else if (eMulPkt == MP_CardIndex) m_clsCardindex.StartSendMulPkt(pSink);
	else if (eMulPkt == MP_Other) m_clsOther.StartSendMulPkt(pSink);
	else if (eMulPkt == MP_IndoorIndex) m_clsIndoorindex.StartSendMulPkt(pSink);
//	else if (eMulPkt == MP_PushSwitchIndex) m_clsPushSwitchindex.StartSendMulPkt(pSink);
	m_pSink = pSink;
}

IMPLEMENT_SINGLETON(CDealMulPktMgr)
CDealMulPktMgr::CDealMulPktMgr()
{

}

CDealMulPktMgr::~CDealMulPktMgr()
{

}

void CDealMulPktMgr::AddMulPkt( DWORD dwDeviceID, MulPkt_e eMulPkt, PUCHAR pData, int nLen, DWORD dwCmdFlag, WORD wSegFlag )
{
	LOG_DEBUG(LOG_MAIN,"AddMulPkt\n");
	CDealMulPkt* p = GetElem(dwDeviceID);
	if (NULL == p)
	{
		try
		{
			p = new CDealMulPkt(dwDeviceID);
		}
		catch(std::bad_alloc &memExp)
		{
			LOG_DEBUG(LOG_UTIL, "new CDealMulPkt failed\n"); return;
		}
		AddElem(dwDeviceID, p);
	}
	p->SetInfo(eMulPkt, pData, nLen, dwCmdFlag, wSegFlag);
}

void CDealMulPktMgr::DelMulPkt( IDealMulPktSink* pSink )
{
	//LOG_DEBUG(LOG_UTIL, "DelMulPkt pSink %p\n", pSink);
	ELEM_MAP::iterator iter = m_mapElement.begin(), iterTemp;
	for (; iter != m_mapElement.end(); )
	{
		iterTemp = iter; ++iterTemp;
		CDealMulPkt* p = iter->second;
		if (p)
		{
			IDealMulPktSink* pSinkCmp = p->GetSink();
			if (pSink == pSinkCmp)
			{
//				LOG_DEBUG(LOG_UTIL, "DelMulPkt CDealMulPkt %p\n", CDealMulPkt);
				delete p;
				m_mapElement.erase(iter);
			}
		}
		iter = iterTemp;
	}
}

void CDealMulPktMgr::AddMulPktAck( DWORD dwDeviceID, MulPkt_e eMulPkt, DWORD dwCmdFlag )
{
	CDealMulPkt* p = GetElem(dwDeviceID);
	if (p) p->AddMulPktAck(eMulPkt, dwCmdFlag);
}

void CDealMulPktMgr::StartSendMulPkt( DWORD dwDeviceID, MulPkt_e eMulPkt, IDealMulPktSink* pSink )
{
	CDealMulPkt* p = GetElem(dwDeviceID);
	//LOG_DEBUG(LOG_UTIL, "StartSendMulPkt dwDeviceID %d eMulPkt %d pSink %p CDealMulPkt %p\n", dwDeviceID, eMulPkt, pSink, p);
	if (p) p->StartSendMulPkt(eMulPkt, pSink);
}
