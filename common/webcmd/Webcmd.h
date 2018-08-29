#pragma once

#include "UtilityInterface.h"
#include "singleton.h"
#include "putbuffer.h"
#include "ElemSet_Webcmd.h"
#include "NetListenInterface.h"

class CWebCmd : public INetConnectionSink, public ITimerSink
{
public:
	CWebCmd();
	~CWebCmd();

	void SetNetConnection(INetConnection *pCon);

	// INetConnectionSink
	int OnConnect		(int nReason, INetConnection* pCon){ return 0; }
	int OnDisconnect	(int nReason, INetConnection* pCon);
	int OnReceive		(PUCHAR pData, int nLen, INetConnection* pCon);
	int OnSend			(INetConnection* pCon){ return 0; }
	int OnCommand		(PUCHAR pData, int nLen, INetConnection* pCon);

	// ITimerSink
	void OnTimer(TimerReason_e eReason, ITimerSink* pSink);
private:
	int OnPieceOfSerialNO		(CSTRING& strParam);
	int OnDserverConfigureIndex	(CSTRING& strParam);
	int OnUserConfigureIndex	(CSTRING& strParam);
	int OnUpdateDeviceRoom		(CSTRING& strParam);// ���·�����Ϣ
	int OnClearRooms			(CSTRING& strParam);// ɾ���豸�����з���
	int OnUpdateDevice			(CSTRING& strParam);// �����豸�·�����Ϣ
	int OnUpdateDeviceEx		(CSTRING& strParam);
	int OnQueryDevice			(CSTRING& strParam);// ��ѯ�豸��Ϣ
	int OnUpdateDeviceDeadLine  (CSTRING& strParam);// ��ѯ�豸ʹ�ý�������
	int OnGetQiNiuDownloadToken (CSTRING& strParam);// ��ȡ��ţ����ƾ֤
	int OnUpdatePropertyAnnounce(CSTRING& strParam);// ������I����
	int OnUpdateFirmwareRequest (CSTRING& strParam);// �豸�̼�����
	int OnDeleteDeviceOnline	(CSTRING& strParam);// ��̨�豸���������豸����ע��
	int OnUpdateAdvertInfo		(CSTRING& strParam);// ���¹����Ϣ

	int AppendData(PUCHAR pData, int nLen);

	int ProcessCommand(CSTRING& strData);
	int SendResponse(bool bSuccessful);

	INetConnection* m_pCon;

	typedef int (CWebCmd::*PMFHANDLER)(CSTRING&);
	typedef struct tagHandlerEntry
	{
		PCHAR		pCmd;
		PMFHANDLER  pmfHandler;
	} HandlerEntry;
	static const HandlerEntry m_handlers[];
	static BYTE m_szBuffer[MAX_PACKET_LEN];

	CSTRING m_strBuffer;
	//
	typedef struct tagQiNiuToken
	{
		DWORD storeid;
		BYTE  downloadToken[32];
	}QiNiuToken;
};

class CCGICenter : public IConDispatcherSink, public CElemSet_Webcmd<CWebCmd>
{
	DECLARE_SINGLETON(CCGICenter)
public:
	CCGICenter();
	~CCGICenter();

	bool Start();

	// IConDispatcherSink
	int OnDispatchConnection(INetConnection* pCon, int nNetType, PUCHAR pData, int nLen);

};
