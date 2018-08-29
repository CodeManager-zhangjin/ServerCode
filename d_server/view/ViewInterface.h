#ifndef _VIEW_INTERFACE_H_
#define _VIEW_INTERFACE_H_

#include "DataStruct.h"

bool ViewInit();
void ViewFinish();

class IViewHandle
{
public:
	virtual ~IViewHandle(){}
	// �����ϱ������б�
	virtual void View_UserConfigureIndex(DWORD dwVendorID, DWORD dwUserID, DWORD dwConfigureIndex) = 0;
	// �����ϱ��豸״̬
	virtual void View_ReportDeviceStatus(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2) = 0;
	// �豸״̬��Ӧ
	virtual void View_GetDeviceStatusRep(GetDeviceStatusRep_t& tInfo) = 0;
	// �ۿ������Ӧ
	virtual void View_TransServerInfo(TransServerInfo_t& tInfo) = 0;
	// ��ȡ�ͻ����б�
	virtual void View_GetUserList(LIST_USERSTATUS& listInfo) = 0;

	// ��ʾ����Token�û��˺�������λ�õ�¼
	virtual void View_LoginOtherPlace(int nReason, BYTE bOpr, ClientTokenArray_t& tInfo) = 0;

	virtual void View_Permission(int nPermission) = 0;

	virtual void View_SdkTunnelRep(SdkTunnel_t& tInfo) = 0;

	// �ϴ�ƾ֤
	virtual void View_Qiniu_UploadToken(StorageTag_t& tTag, PUCHAR pUploadToken) = 0;
	// ��������
	virtual void View_Qiniu_DownloadUrls(StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo) = 0;

	//���ڻ�
	virtual void View_ReportIndoorStatus(DWORD dwUserID, ReportDevStatus& devStatus) = 0;

};
IViewHandle* View_GetHandle();

class IViewHandleSink
{
public:
	virtual ~IViewHandleSink(){}
	// �յ��ͻ��˹ۿ�����
	virtual void View_OnTransClientInfo(TransClientInfo_t& tInfo, bool bRelay,TransServerInfo_t& tServerInfo) = 0;
	// �յ��ͻ��˻�ȡ�豸״̬����
	virtual void View_OnGetDeviceStatus(GetDeviceStatus_t& tInfo) = 0;
	// �ϱ�״̬
	virtual void View_OnReportUserStatus(UserStatus_t& tInfo) = 0;

	// �豸ģ�鱣�汨��״̬
	virtual void View_CacheDeviceStatus(DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg) = 0;
	// �յ��ͻ���SDK͸������
	virtual void View_OnSdkTunnel(SdkTunnel_t& tInfo) = 0;

	// �ƴ洢
	virtual void View_OnQiniu_GetUploadToken(StorageTag_t& tTag) = 0;
	virtual void View_OnQiniu_GetDownloadUrls(StorageTag_t& tTag, StoreKey_t& tKey) = 0;
	virtual void View_OnQiniu_GetDownloadUrls2(StorageTag_t& tTag, StoreVisitor_t& tVisitor) = 0;
};
void View_SetSink(IViewHandleSink* pSink);

#endif
