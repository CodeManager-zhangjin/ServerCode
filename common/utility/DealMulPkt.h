#pragma once

#include "UtilityInterface.h"
#include "ElemMapUtility.h"
#include "singleton.h"

extern DWORD g_dwCmdFlag;
class FindMulPktByCmdFlag
{
public:
	bool operator() (LIST_MULPKT::value_type& pos)
	{
		if (g_dwCmdFlag == pos.dwCmdFlag) return true;
		return false;
	}
};

class CDealMulPktOne : public ITimerSink
{
public:
	CDealMulPktOne();
	virtual ~CDealMulPktOne();

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

	bool IsSendOver();
	IDealMulPktSink* GetSink(){ return m_pSink; }
	void SetInfo(PUCHAR pData, int nLen, DWORD dwCmdFlag, WORD wSegFlag);
	void AddMulPktAck(DWORD dwCmdFlag);
	void StartSendMulPkt(IDealMulPktSink* pSink);
private:
	void SendNext();

	LIST_MULPKT m_lstMulPkt;
	LIST_MULPKT::iterator m_Iter;
	int m_nTTL;
	WORD m_wSegFlag;

	IDealMulPktSink* m_pSink;
};


class CDealMulPkt : public ITimerSink
{
public:
	CDealMulPkt(DWORD dwDeviceID);
	virtual ~CDealMulPkt();

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);

	IDealMulPktSink* GetSink(){ return m_pSink; }
	void SetInfo(MulPkt_e eMulPkt, PUCHAR pData, int nLen, DWORD dwCmdFlag, WORD wSegFlag);
	void AddMulPktAck(MulPkt_e eMulPkt, DWORD dwCmdFlag);
	void StartSendMulPkt(MulPkt_e eMulPkt, IDealMulPktSink* pSink);
private:
	CDealMulPktOne m_clsSum;
	CDealMulPktOne m_clsUserindex;
	CDealMulPktOne m_clsPushindex;
	CDealMulPktOne m_clsCardindex;
	CDealMulPktOne m_clsOther;
	CDealMulPktOne m_clsIndoorindex;
	CDealMulPktOne m_clsPushSwitchindex;

	DWORD m_dwDeviceID;
	IDealMulPktSink* m_pSink;
};

// DeviceID<-->CDealMulPkt
class CDealMulPktMgr : public CElemMapUtility<CDealMulPkt>
{
	DECLARE_SINGLETON(CDealMulPktMgr)
public:
	CDealMulPktMgr();
	~CDealMulPktMgr();

	void AddMulPkt(DWORD dwDeviceID, MulPkt_e eMulPkt, PUCHAR pData, int nLen, DWORD dwCmdFlag, WORD wSegFlag);
	void DelMulPkt(IDealMulPktSink* pSink);
	void AddMulPktAck(DWORD dwDeviceID, MulPkt_e eMulPkt, DWORD dwCmdFlag);
	void StartSendMulPkt(DWORD dwDeviceID, MulPkt_e eMulPkt, IDealMulPktSink* pSink);
};
