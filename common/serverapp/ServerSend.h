#pragma once

#include "Protocol.h"
#include "putbuffer.h"

class CServerSend
{
public:
	CServerSend();
	virtual ~CServerSend();

	void SetHeader_reserved3(DWORD dwReserved3) { m_tHeader.reserved3 = dwReserved3; }

	int SendCmd_GetChallenge();
	int SendCmd_Auth(PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pChallenge);
	int SendCmd_Sms(Notify_SmsInfo_t& tInfo, DWORD dwClientID);
	int SendCmd_Push(DWORD dwStatus, DWORD dwDeviceID, LIST_NOTIFY_PUSHINFO& listInfo);
	int SendCmd_SdkTunnel(SdkTunnel_t& tInfo);
	int SendCmd_SdkTunnelRep(SdkTunnel_t& tInfo);
	
	int SendCmd_ReportDeviceStatusA	(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2);
	int SendCmd_ReportDeviceStatus	(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2);
	int SendCmd_ReportUserStatus	(UserStatus_t& tInfo);
	int SendCmd_GetDeviceStatus		(DWORD dwUserID, DWORD dwSessionID, LIST_DWORD& listDeviceID);
	int SendCmd_GetDeviceStatusRep	(DWORD dwUserID, DWORD dwSessionID, LIST_DEVICESTATUS& listInfo);
	int SendCmd_TransClientInfo		(TransClientInfo_t& tInfo, bool bRelay);
	int SendCmd_TransServerInfo		(TransServerInfo_t& tInfo);
	int SendCmd_AddTempUser			(TransClientInfo_t& tInfo);
	int SendCmd_AddTempUserRep		(TransClientInfo_t& tInfo, PUCHAR pUserName, PUCHAR pPassword);

	int SendCmd_AlarmStatus(AlarmStatus_t& alarmStatus, WORD wError);
	int SendCmd_Qiniu_UploadToken	(StorageTag_t& tTag, WORD wError);
	int SendCmd_Qiniu_ReportUploadResult(StoreKey_t& tKey);
	int SendCmd_Qiniu_DownloadUrls	(StorageTag_t& tTag, StoreKey_t& tKey, WORD wError);
	int SendCmd_Qiniu_DownloadUrls2	(StorageTag_t& tTag, StoreVisitor_t& tVisitor, WORD wError);
	int SendCmd_Qiniu_UploadTokenRep(StorageTag_t& tTag, PUCHAR pUploadToken, WORD wError);
	int SendCmd_Qiniu_DownloadUrlsRep(StorageTag_t& tTag, LIST_STORE_KEYURL& listInfo, WORD wError);
	int SendCmd_DownLoadUrlsRep(DWORD dwDeviceID, AdvertInfo_t& tInfo, PUCHAR strUrl );

	int SendPacket(CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg = 1, WORD wSubSeg = 1);

	BYTE m_bGroupCode;
	INetConnection* m_pCon;
	PacketHeader_t m_tHeader;
	ServerInfo_t m_tBaseInfo;
	static BYTE m_szBuffer[MAX_PACKET_LEN];
};

#define DECLARE_PUTBUFFER( bufferPut ) \
	CPutBuffer bufferPut( m_szBuffer, MAX_PACKET_LEN ); \
	bufferPut.Skip ( PACKET_HEADER_SIZE );
