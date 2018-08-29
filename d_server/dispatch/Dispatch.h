#pragma once

#include "DeviceInterface.h"
#include "ViewInterface.h"
#include "ServerAppInterface.h"
#include "singleton.h"

class CDispatch : public IDeviceHandleSink, public IViewHandleSink, public IServerAppHandleSink
{
	DECLARE_SINGLETON(CDispatch)
public:
	CDispatch();
	~CDispatch();

	bool Start();

	// IDeviceHandleSink
	// �豸�ϱ�״̬
	void Dev_OnReportDeviceStatus(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, LIST_SMSINFO& list1, LIST_PUSHINFO& list2, ReportDevStatus& devStatus);
	void Dev_OnSmsSpecialCrowd(SmsInfo2_t& tInfo);
	// ͸����Ϣ��Ӧ
	void Dev_OnSdkTunnel(SdkTunnel_t& tInfo);
	// �ƴ洢
	void Dev_ReportAlarmStatus(AlarmStatus_t& alarmStatus);
	void Dev_OnQiniu_GetUploadToken(StorageTag_t& tTag);
	void Dev_OnQiniu_GetDownloadUrls(StorageTag_t& tTag, StoreKey_t& tKey);
	void Dev_OnQiniu_ReportUploadResult(StoreKey_t& tKey);
	//
	void Dev_OnGetIndoorBindDevStatus(LIST_DEVSTATUS& lstDevStatus);

	// IViewHandleSink
	// �յ��ͻ��˹ۿ�����
	void View_OnTransClientInfo(TransClientInfo_t& tInfo, bool bRelay, TransServerInfo_t& tServerInfo);
	// �յ��ͻ��˻�ȡ�豸״̬����
	void View_OnGetDeviceStatus(GetDeviceStatus_t& tInfo);
	// �ϱ�״̬
	void View_OnReportUserStatus(UserStatus_t& tInfo);
	// �豸ģ�鱣�汨��״̬
	void View_CacheDeviceStatus(DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg);
	// �յ��ͻ���SDK͸������
	void View_OnSdkTunnel(SdkTunnel_t& tInfo);
	// �ƴ洢
	void View_OnQiniu_GetUploadToken(StorageTag_t& tTag);
	void View_OnQiniu_GetDownloadUrls(StorageTag_t& tTag, StoreKey_t& tKey);
	void View_OnQiniu_GetDownloadUrls2( StorageTag_t& tTag, StoreVisitor_t& tVisitor );
	// IServerAppHandleSink
	// �յ�״̬�������ϱ��豸״̬
	void SA_OnReportDeviceStatus_Status(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2);
	// �յ�״̬��������ȡ�豸״̬����
	void SA_OnGetDeviceStatus_Status(DWORD dwServerID, GetDeviceStatus_t& tInfo);
	// �յ�״̬��������ȡ�豸״̬��Ӧ
	void SA_OnGetDeviceStatusRep_Status(GetDeviceStatusRep_t& tInfo);
	// �յ�״̬�������ۿ�����
	void SA_OnTransClientInfo_Status(TransClientInfo_t& tInfo, bool bRelay);
	// �յ�״̬�������ۿ������Ӧ
	void SA_OnTransServerInfo_Status(TransServerInfo_t& tInfo);
	// �յ�ת����������Ӧ
	void SA_OnAddTempUser_Relay(TransClientInfo_t& tInfo, TransServerInfo_t& tServerInfo);
	// �յ�ת���������ۿ�����
	void SA_OnTransClientInfo_Relay(TransClientInfo_t& tInfo, DWORD dwServerID);
	// ��״̬��������������
	void SA_Online_Status();
	// �յ�֪ͨ���������Ͷ��Ż�Ӧ
	void SA_OnSmsRepsonse_Notify(DWORD dwClientID, int nReason){}
	// �յ�ע�������������ʱ�û�����
	void SA_OnAddTempUser_D(TempUser_t& tInfo){}

	void SA_OnSdkTunnel_Status(SdkTunnel_t& tInfo);
	void SA_OnSdkTunnelRep_Status(SdkTunnel_t& tInfo);

	// �ƴ洢
	void SA_OnQiniu_GetUploadToken_Storage(StorageTag_t& tTag, PUCHAR pUploadToken);
	void SA_OnQiniu_GetDownloadUrls_Storage(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo);
};
