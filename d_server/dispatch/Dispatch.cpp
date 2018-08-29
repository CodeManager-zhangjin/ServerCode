#include "Dispatch.h"
#include "Log.h"
#include "UtilityInterface.h"

IMPLEMENT_SINGLETON(CDispatch)
CDispatch::CDispatch()
{
}

CDispatch::~CDispatch()
{
}

bool CDispatch::Start()
{
	Device_SetSink(this);
	View_SetSink(this);
	ServerApp_SetSink(this);
	return true;
}
//����1
void CDispatch::Dev_OnReportDeviceStatus(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, LIST_SMSINFO& list1, LIST_PUSHINFO& list2, ReportDevStatus& devStatus)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s dwDeviceID %d st=%d\n", __FUNCTION__, dwDeviceID, dwStatus);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
	IViewHandle* pViewHandle = View_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);

	// list1 ����֪ͨ
//	if ( (AT_ONLINE != dwStatus) && (AT_OFFLINE != dwStatus) )
//	{
//		if (false == pSAHandle->SA_Sms_Notification(pMsg, list1, 0))
//		{
//			LOG_WARN(LOG_MAIN, "SMS Notify Failed\n");
//		}
//	}

	BYTE szTimeStamp[LENGTH_TIMESTAMP+1] = {0};
	GenerateTimeStamp((PUCHAR)szTimeStamp);

	// ֪ͨ��ע����������߿ͻ���
	pViewHandle->View_ReportDeviceStatus(dwDeviceID, dwStatus, pMsg, (PUCHAR)szTimeStamp, list1, list2);

	// ֪ͨ״̬������
	pSAHandle->SA_ReportDeviceStatus_Status(dwDeviceID, dwStatus, pMsg, (PUCHAR)szTimeStamp, list1, list2);
	
	// ֪֪ͨͨ�����������ͣ�
	if ( (AT_ONLINE != dwStatus) && (AT_OFFLINE != dwStatus) )
	{
		if (false == pSAHandle->SA_Push_Notification(dwStatus, pMsg, (PUCHAR)szTimeStamp, dwDeviceID, list2))
		{
			LOG_WARN(LOG_MAIN, "PUSH Notify Failed\n");
			devStatus.dwResult = 1;
		}
		devStatus.dwResult = 0;
	}
}

void CDispatch::Dev_OnSmsSpecialCrowd(SmsInfo2_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN," CDispatch::%s\n", __FUNCTION__);
	LIST_SMSINFO list;
	SmsInfo_t tSms;
	list.clear();
	memset(&tSms, 0, sizeof(tSms));
	tSms.dwUserID = tInfo.dwUserID;
	tSms.dwVendorID = tInfo.dwVendorID;
	tSms.bLanguage = tInfo.bLanguage;
	LOG_DEBUG(LOG_MAIN,"UserID = %d, VendorID = %d, Language = %d\n",tInfo.dwUserID, tInfo.dwVendorID, tInfo.bLanguage);
	memcpy(tSms.szMobilePhone, tInfo.szMobilePhone, sizeof(tInfo.szMobilePhone));
	list.push_back(tSms);
	LOG_DEBUG(LOG_DB_CLIENT,"Phone %s\n", tInfo.szMobilePhone);
	memcpy(tSms.szMobilePhone, tInfo.szPropertyPhone, sizeof(tInfo.szPropertyPhone));
	list.push_back(tSms);
	LOG_DEBUG(LOG_DB_CLIENT,"property Phone %s\n", tInfo.szPropertyPhone);
	if(tInfo.lstPhone.size() != 0)
	{
		LIST_PHONE::iterator iphone = tInfo.lstPhone.begin();
		for(; tInfo.lstPhone.end() != iphone; iphone++)
		{
			LOG_DEBUG(LOG_DB_CLIENT,"In the same room number, Phone %s\n", iphone->szMobilePhone);
			memcpy(tSms.szMobilePhone, iphone->szMobilePhone, sizeof(iphone->szMobilePhone));
			list.push_back(tSms);
		}
	}
	//////////////////////////////////////////////////////////////////////////
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
	BYTE szMessage[LENGTH_MSGCONTENT+1] = {"123456"};
	Base64DecVal((char*)szMessage, (const char*)szMessage, (int)6);
	//���ŷ���ע�͵�
//	if (false == pSAHandle->SA_Sms_Notification((PUCHAR)szMessage, list, 0))
//	{
//		LOG_WARN(LOG_MAIN, "SMS Notify Failed\n");
//	}
	LOG_DEBUG(LOG_MAIN,"%s ===> END <====\n",__FUNCTION__);
}

void CDispatch::Dev_OnSdkTunnel(SdkTunnel_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s SrcType %d\n", __FUNCTION__, tInfo.bSrcType);
	if (tInfo.bSrcType == SRC_TYPE_STATUS)
	{
		IServerAppHandle* pSAHandle = ServerApp_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
		pSAHandle->SA_SdkTunnelRep_Status(tInfo);
	}
	else if (tInfo.bSrcType == SRC_TYPE_CLIENT)
	{
		IViewHandle* pViewHandle = View_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);
		pViewHandle->View_SdkTunnelRep(tInfo);
	}
}

//��ȡ���豸��������״̬
void CDispatch::Dev_OnGetIndoorBindDevStatus(LIST_DEVSTATUS& lstDevStatus)
{
	LOG_DEBUG(LOG_MAIN, "[2004] CDispatch::%s\n", __FUNCTION__);
	//�ڵ�ǰ�������ϵ��豸
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
	pDevHandle->Dev_GetDeviceStatus(lstDevStatus);
	LOG_DEBUG(LOG_MAIN,"[2006]  CView::%s\n",__FUNCTION__);
}

void CDispatch::View_OnTransClientInfo(TransClientInfo_t& tInfo, bool bRelay, TransServerInfo_t& tServerInfo)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);

	// 1����ǰ�豸���ߣ�֪ͨת��������������ʱ�û�
	//    ת��������ʱ�û�ʧ�ܣ�ֱ�ӻ�Ӧ�ͻ��˽��
	//    ����ȴ�ת����������Ӧ���Ӧ�ͻ��˽��
	// 2����ǰ�豸�����ߣ�����״̬������
	//    ״̬�����������ߣ�ֱ�ӻ�Ӧ�ͻ��˽��
	//    ����ȴ�״̬��������Ӧ���Ӧ�ͻ��˽��
//	TransServerInfo_t tServerInfo;
	Clear_TransServerInfo(tServerInfo);
	
	LOG_DEBUG(LOG_MAIN,"CDispatch::%s\n",__FUNCTION__);

	DWORD dwAutoRelay = RELAY_TYPE_NO;
	bool bLocal = pDevHandle->Dev_GetDeviceConnectInfo(tInfo, tServerInfo, false, dwAutoRelay);// �豸�Ƿ�ע���ڱ�ע���������
	LOG_DEBUG(LOG_MAIN,"CDispatch::%s\n",__FUNCTION__);
	
	if (tServerInfo.bSrcType == SRC_TYPE_INDOOR)
	{
		tInfo.bSrcType = SRC_TYPE_CLIENT;
	}
	else if ( ( bLocal && (false == pSAHandle->SA_AddTempUser_Relay(tInfo, bRelay, dwAutoRelay)) ) ||
		 ( (false == bLocal) && (false == pSAHandle->SA_TransClientInfo_Status(tInfo, bRelay)) )
		 )
	{
		if (bLocal) pDevHandle->Dev_TransClientInfo(tInfo);

		IViewHandle* pViewHandle = View_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);
		pViewHandle->View_TransServerInfo(tServerInfo);
	}
}



void CDispatch::View_OnGetDeviceStatus(GetDeviceStatus_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
	IViewHandle* pViewHandle = View_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);

	// �ڵ�ǰע��������ϵ��豸��ֱ�ӻ�Ӧ
	GetDeviceStatusRep_t tRepInfo;
	tRepInfo.dwUserID = tInfo.dwUserID;
	tRepInfo.dwSessionID = tInfo.dwSessionID;
	pDevHandle->Dev_GetDeviceStatus(tInfo.dwUserID, tInfo.listDeviceID, tRepInfo.listInfo);
	if (tRepInfo.listInfo.size()) pViewHandle->View_GetDeviceStatusRep(tRepInfo);

	// ���ཻ��״̬����������
	// ��״̬������û�л�Ӧʱ���ͻ�����Ϊ�豸������
	if (tInfo.listDeviceID.size()) pSAHandle->SA_GetDeviceStatus_Status(tInfo);
}

void CDispatch::View_OnReportUserStatus( UserStatus_t& tInfo )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
	pSAHandle->SA_ReportUserStatus_Status(tInfo);
}

void CDispatch::View_CacheDeviceStatus( DWORD dwUserID, DWORD dwDeviceID, DWORD dwStatus, PUCHAR pStatusMsg )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
	pDevHandle->Dev_CacheDeviceStatus(dwUserID, dwDeviceID, dwStatus, pStatusMsg);
}

void CDispatch::View_OnSdkTunnel(SdkTunnel_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);

	// �ڵ�ǰע��������ϵ��豸
	if (false == pDevHandle->Dev_SdkTunnel(tInfo))
	{
		// ���ཻ��״̬����������
		pSAHandle->SA_SdkTunnel_Status(tInfo);
	}
}

void CDispatch::SA_OnReportDeviceStatus_Status(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IViewHandle* pViewHandle = View_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);

	// ֪ͨ���߿ͻ���
	pViewHandle->View_ReportDeviceStatus(dwDeviceID, dwStatus, pMsg, pTimeStamp, list1, list2);
}

void CDispatch::SA_OnGetDeviceStatus_Status(DWORD dwServerID, GetDeviceStatus_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);

	// �ڵ�ǰע��������ϵ��豸��ֱ�ӻ�Ӧ
	GetDeviceStatusRep_t tRepInfo;
	tRepInfo.dwUserID = tInfo.dwUserID;
	tRepInfo.dwSessionID = tInfo.dwSessionID;
	pDevHandle->Dev_GetDeviceStatus(tInfo.dwUserID, tInfo.listDeviceID, tRepInfo.listInfo);
	if (tRepInfo.listInfo.size())
	{
		pSAHandle->SA_GetDeviceStatusRep_Status(dwServerID, tRepInfo);
	}
}

void CDispatch::SA_OnGetDeviceStatusRep_Status(GetDeviceStatusRep_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IViewHandle* pViewHandle = View_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);

	pViewHandle->View_GetDeviceStatusRep(tInfo);
}

void CDispatch::SA_OnTransClientInfo_Status(TransClientInfo_t& tInfo, bool bRelay)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);

	// 1����ǰ�豸���ߣ�֪ͨת��������������ʱ�û�
	//    ת��������ʱ�û�ʧ�ܣ�ֱ�ӻ�Ӧ״̬���������
	//    ����ȴ�ת����������Ӧ���Ӧ״̬���������
	// 2����ǰ�豸�����ߣ�ֱ�ӻ�Ӧ״̬���������
	TransServerInfo_t tServerInfo;
	Clear_TransServerInfo(tServerInfo);

	DWORD dwAutoRelay = 0;
	bool bLocal = pDevHandle->Dev_GetDeviceConnectInfo(tInfo, tServerInfo, false, dwAutoRelay);// �豸�Ƿ�ע���ڱ�ע���������
	if ( ( bLocal && (false == pSAHandle->SA_AddTempUser_Relay(tInfo, bRelay, dwAutoRelay)) ) ||
		 (false == bLocal)
		)
	{
		if (bLocal) pDevHandle->Dev_TransClientInfo(tInfo);
		pSAHandle->SA_TransServerInfo_Status(tServerInfo);
	}
}

void CDispatch::SA_OnTransServerInfo_Status(TransServerInfo_t& tInfo)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IViewHandle* pViewHandle = View_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);
	
	pViewHandle->View_TransServerInfo(tInfo);
}

void CDispatch::SA_OnAddTempUser_Relay(TransClientInfo_t& tInfo, TransServerInfo_t& tServerInfo)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s SrcType %d\n", __FUNCTION__, tInfo.bSrcType);
	if (tInfo.bSrcType == SRC_TYPE_CLIENT)
	{
		// ���Կͻ��˵�����
		IViewHandle* pViewHandle = View_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);
		IDeviceHandle* pDevHandle = Device_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);

		DWORD dwAutoRelay = RELAY_TYPE_NO;
		pDevHandle->Dev_GetDeviceConnectInfo(tInfo, tServerInfo, true, dwAutoRelay);
		pViewHandle->View_TransServerInfo(tServerInfo);
	}
	else if (tInfo.bSrcType == SRC_TYPE_STATUS)
	{
		// ����״̬������������
		IDeviceHandle* pDevHandle = Device_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
		IServerAppHandle* pSAHandle = ServerApp_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);

		DWORD dwAutoRelay = RELAY_TYPE_NO;
		pDevHandle->Dev_GetDeviceConnectInfo(tInfo, tServerInfo, true, dwAutoRelay);
		pSAHandle->SA_TransServerInfo_Status(tServerInfo);
	}
}

void CDispatch::SA_OnTransClientInfo_Relay( TransClientInfo_t& tInfo, DWORD dwServerID )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);

	DWORD dwAutoRelay = RELAY_TYPE_NO;
	TransServerInfo_t tServerInfo;
	Clear_TransServerInfo(tServerInfo);
	pDevHandle->Dev_GetDeviceConnectInfo(tInfo, tServerInfo, true, dwAutoRelay);		// �豸�Ƿ�ע���ڱ�ע���������
	pSAHandle->SA_TransServerInfo_Relay(tServerInfo, dwServerID);	// ��Ӧת�����������
}

void CDispatch::SA_Online_Status()
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
	IViewHandle* pViewHandle = View_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);

	LIST_DEVICESTATUS listDevStatus;
	pDevHandle->Dev_GetDeviceList(listDevStatus);
	LIST_DEVICESTATUS::iterator iter = listDevStatus.begin();
	for (; iter != listDevStatus.end(); iter++)
	{
		LIST_SMSINFO list1;
		LIST_PUSHINFO list2;
		BYTE szMsg[LENGTH_MSGCONTENT+1] = {0};
		BYTE szTimeStamp[LENGTH_TIMESTAMP+1] = {0};
		pSAHandle->SA_ReportDeviceStatus_Status(iter->dwDeviceID, iter->dwStatus, (PUCHAR)szMsg, (PUCHAR)szTimeStamp, list1, list2);
	}
	
	LIST_USERSTATUS listUserStatus;
	pViewHandle->View_GetUserList(listUserStatus);
	LIST_USERSTATUS::iterator pos = listUserStatus.begin();
	for (; pos != listUserStatus.end(); pos++)
	{
		pSAHandle->SA_ReportUserStatus_Status(*pos);
	}
}

void CDispatch::SA_OnSdkTunnel_Status( SdkTunnel_t& tInfo )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IDeviceHandle* pDevHandle = Device_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
	
	// �ڵ�ǰע��������ϵ��豸
	pDevHandle->Dev_SdkTunnel(tInfo);
}

void CDispatch::SA_OnSdkTunnelRep_Status( SdkTunnel_t& tInfo )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IViewHandle* pViewHandle = View_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);
	pViewHandle->View_SdkTunnelRep(tInfo);
}

void CDispatch::Dev_OnQiniu_GetUploadToken( StorageTag_t& tTag )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
	pSAHandle->SA_GetUploadToken_Storage(tTag);
}

void CDispatch::Dev_ReportAlarmStatus(AlarmStatus_t& alarmStatus)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
	pSAHandle->SA_ReportAlarmStatus(alarmStatus);
}

void CDispatch::Dev_OnQiniu_GetDownloadUrls( StorageTag_t& tTag, StoreKey_t& tKey )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
	pSAHandle->SA_GetDownloadUrls_Storage(tTag, tKey);
}

void CDispatch::Dev_OnQiniu_ReportUploadResult(StoreKey_t& tKey)
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
	pSAHandle->SA_ReportUploadResult_Storage(tKey);
}
void CDispatch::View_OnQiniu_GetUploadToken( StorageTag_t& tTag )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
	pSAHandle->SA_GetUploadToken_Storage(tTag);
}

void CDispatch::View_OnQiniu_GetDownloadUrls( StorageTag_t& tTag, StoreKey_t& tKey )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
	pSAHandle->SA_GetDownloadUrls_Storage(tTag, tKey);
}

void CDispatch::View_OnQiniu_GetDownloadUrls2( StorageTag_t& tTag, StoreVisitor_t& tVisitor )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	IServerAppHandle* pSAHandle = ServerApp_GetHandle();
	LOG_ASSERT_RETVOID(LOG_MAIN, pSAHandle);
	pSAHandle->SA_GetDownloadUrls_Storage2(tTag, tVisitor);
}

void CDispatch::SA_OnQiniu_GetUploadToken_Storage( StorageTag_t& tTag, PUCHAR pUploadToken )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	if (tTag.bSrcType == SRC_TYPE_DEVICE)
	{
		LOG_DEBUG(LOG_MAIN,"SRC_TYPE_DEVICE\n");
		IDeviceHandle* pDevHandle = Device_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
		pDevHandle->Dev_Qiniu_UploadToken(tTag, pUploadToken);
	}
	else if (tTag.bSrcType == SRC_TYPE_CLIENT)
	{
		LOG_DEBUG(LOG_MAIN,"SRC_TYPE_CLIENT\n");
		IViewHandle* pViewHandle = View_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);
		pViewHandle->View_Qiniu_UploadToken(tTag, pUploadToken);
	}
}

void CDispatch::SA_OnQiniu_GetDownloadUrls_Storage( StorageTag_t& tTag, LIST_STORE_KEYURL& lstInfo )
{
	LOG_DEBUG(LOG_MAIN, "CDispatch::%s\n", __FUNCTION__);
	if (tTag.bSrcType == SRC_TYPE_DEVICE)
	{
		IDeviceHandle* pDevHandle = Device_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pDevHandle);
		pDevHandle->Dev_Qiniu_DownloadUrls(tTag, lstInfo);
	}
	else if (tTag.bSrcType == SRC_TYPE_CLIENT)
	{
		IViewHandle* pViewHandle = View_GetHandle();
		LOG_ASSERT_RETVOID(LOG_MAIN, pViewHandle);
		pViewHandle->View_Qiniu_DownloadUrls(tTag, lstInfo);
	}
}
