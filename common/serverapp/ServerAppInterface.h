#ifndef _SERVERAPP_INTERFACE_H_
#define _SERVERAPP_INTERFACE_H_

#include "DataStruct.h"

bool ServerAppInit(PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword);
void ServerAppFinish();

class IServerAppHandle
{
public:
	virtual ~IServerAppHandle(){}
	virtual bool SA_AddServer(ServerInfo_t& tInfo, INetConnection* pCon) = 0;
	virtual bool SA_GetDServers(DWORD& dwVendorID, DWORD& dwConfigureIndex, LIST_SERVERINFO& listInfo) = 0;
	virtual bool SA_DserverConfigureIndex(DWORD dwVendorID, DWORD dwConfigureIndex, LIST_SERVERINFO& listInfo) = 0;

	virtual bool SA_Sms_Notification(PUCHAR pMsg, LIST_SMSINFO& listSmsInfo, DWORD dwClientID) = 0;
	virtual bool SA_Push_Notification(DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, DWORD dwDeviceID, LIST_PUSHINFO& listToken) = 0;

	// ��״̬�������ϱ��豸״̬
	virtual bool SA_ReportDeviceStatus_Status(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2) = 0;
	// ��״̬�������ϱ��ͻ���״̬
	virtual bool SA_ReportUserStatus_Status(UserStatus_t& tInfo) = 0;
	// ��Ӧ״̬�������ۿ���Ϣ
	virtual bool SA_TransServerInfo_Status(TransServerInfo_t& tInfo) = 0;
	// ��Ӧת���������ۿ���Ϣ
	virtual bool SA_TransServerInfo_Relay(TransServerInfo_t& tInfo, DWORD dwServerID) = 0;
	// ��״̬��������ȡ�豸״̬
	virtual bool SA_GetDeviceStatus_Status(GetDeviceStatus_t& tInfo) = 0;
	// ��״̬�������ϱ���ȡ�豸״̬��Ӧ
	virtual bool SA_GetDeviceStatusRep_Status(DWORD dwServerID, GetDeviceStatusRep_t& tInfo) = 0;
	// �豸�ڵ�ǰע���������ע�ᣬͨ��ת���������ۿ�
	virtual bool SA_AddTempUser_Relay(TransClientInfo_t& tInfo, bool bRelay, DWORD dwAutoRelay) = 0;
	// �豸δ�ڵ�ǰע���������ע�ᣬͨ��״̬������͸����Ϣ
	virtual bool SA_TransClientInfo_Status(TransClientInfo_t& tInfo, bool bRelay) = 0;

	virtual bool SA_SdkTunnel_Status(SdkTunnel_t& tInfo) = 0;
	virtual bool SA_SdkTunnelRep_Status(SdkTunnel_t& tInfo) = 0;

	virtual bool SA_ReportAlarmStatus(AlarmStatus_t& alarmStatus) = 0;
	virtual bool SA_GetUploadToken_Storage(StorageTag_t& tTag) = 0;
	virtual bool SA_ReportUploadResult_Storage(StoreKey_t& tKey) = 0;
	virtual bool SA_GetDownloadUrls_Storage(StorageTag_t& tTag, StoreKey_t& tKey) = 0;
	virtual bool SA_GetDownloadUrls_Storage2(StorageTag_t& tTag, StoreVisitor_t& tVisitor) = 0;
};
IServerAppHandle* ServerApp_GetHandle();

class IServerAppHandleSink
{
public:
	virtual ~IServerAppHandleSink(){}
	// �յ�״̬�������ϱ��豸״̬
	virtual void SA_OnReportDeviceStatus_Status(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2) {}
	// �յ�״̬��������ȡ�豸״̬����
	virtual void SA_OnGetDeviceStatus_Status(DWORD dwServerID, GetDeviceStatus_t& tInfo) {}
	// �յ�״̬��������ȡ�豸״̬��Ӧ
	virtual void SA_OnGetDeviceStatusRep_Status(GetDeviceStatusRep_t& tInfo) {}
	// �յ�״̬�������ۿ�����
	virtual void SA_OnTransClientInfo_Status(TransClientInfo_t& tInfo, bool bRelay) {}
	// �յ�״̬�������ۿ������Ӧ
	virtual void SA_OnTransServerInfo_Status(TransServerInfo_t& tInfo) {}
	// �յ�ת����������Ӧ
	virtual void SA_OnAddTempUser_Relay(TransClientInfo_t& tInfo, TransServerInfo_t& tServerInfo) {}
	// �յ�ת���������ۿ�����
	virtual void SA_OnTransClientInfo_Relay(TransClientInfo_t& tInfo, DWORD dwServerID) {}
	// ��״̬��������������
	virtual void SA_Online_Status() {}
	// �յ�֪ͨ���������Ͷ��Ż�Ӧ
	virtual void SA_OnSmsRepsonse_Notify(DWORD dwClientID, int nReason) {}
	// �յ�ע�������������ʱ�û�����
	virtual void SA_OnAddTempUser_D(TempUser_t& tInfo) {}

	virtual void SA_OnSdkTunnel_Status(SdkTunnel_t& tInfo) {}
	virtual void SA_OnSdkTunnelRep_Status(SdkTunnel_t& tInfo) {}

	// �ƴ洢
	virtual void SA_OnQiniu_GetUploadToken_Storage(StorageTag_t& tTag, PUCHAR pUploadToken) {}
	virtual void SA_OnQiniu_GetDownloadUrls_Storage(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo) {}
};
void ServerApp_SetSink(IServerAppHandleSink* pSink);


#endif
