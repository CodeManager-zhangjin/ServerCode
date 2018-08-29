#include "ServerSend.h"
#include "UtilityInterface.h"
#include "Log.h"

BYTE CServerSend::m_szBuffer[MAX_PACKET_LEN] = {0};
CServerSend::CServerSend()
{
	m_pCon = NULL;
	memset(&m_tHeader, 0, sizeof(PacketHeader_t));
}

CServerSend::~CServerSend()
{
	if (m_pCon) { m_pCon->Disconnect(); NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
}

int CServerSend::SendCmd_GetChallenge()
{
	DECLARE_PUTBUFFER(bufferPut)
	return SendPacket(bufferPut, CMD_GET_CHALLENGE, 0);
}

int CServerSend::SendCmd_Auth(PUCHAR pSN, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pChallenge)
{
	DECLARE_PUTBUFFER(buffer)
	buffer << CByteArrayBuffer(pSN, LENGTH_SERIALNO);
	PutVariableStr(buffer, pUserName);

	BYTE szDigist[LENGTH_CHALLENGE+1] = {0};
	CalcAuthDigist((PUCHAR)szDigist, pUserName, pPassword, pChallenge);
	buffer << CByteArrayBuffer(szDigist, LENGTH_CHALLENGE);
	return SendPacket(buffer, CMD_SERVER_AUTH, ERROR_NO);
}

int CServerSend::SendCmd_Sms(Notify_SmsInfo_t& tInfo, DWORD dwClientID)
{
	LOG_DEBUG(LOG_MAIN,"CServerSend::%s\n",__FUNCTION__);
	m_tHeader.reserved3 = dwClientID; // 用于标记发送短信客户端ID
	DECLARE_PUTBUFFER(buffer)
	PutBase64Str(buffer, (PUCHAR)tInfo.szMessageContent);
	buffer << (UINT)1 << tInfo.dwVendorID << tInfo.bLanguage;
	PutBase64Str(buffer, (PUCHAR)tInfo.szMobilePhone);
	return SendPacket(buffer, CMD_SMS, ERROR_NO);
}
//推送3      
int CServerSend::SendCmd_Push(DWORD dwStatus, DWORD dwDeviceID, LIST_NOTIFY_PUSHINFO& listInfo)
{
	BYTE szMessageContent[LENGTH_MSGCONTENT+1] = {0};
	UINT nCount = listInfo.size();
	if (nCount == 0)
	{
		DECLARE_PUTBUFFER(buffer)
		PutBase64Str(buffer, (PUCHAR)szMessageContent);
		buffer << dwDeviceID << dwStatus << nCount;
		return SendPacket(buffer, CMD_PUSH, ERROR_NO);
	}
	
	memcpy(szMessageContent, listInfo.front().szMessageContent, LENGTH_MSGCONTENT);
	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_ReportDeviceStatus((PUCHAR)szMessageContent, listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_NOTIFY_PUSHINFO::iterator iter = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		PutBase64Str(buffer, (PUCHAR)szMessageContent);
		buffer << dwDeviceID << dwStatus << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << iter->dwVendorID << iter->bPushType << iter->bLanguage;
			PutVariableStr(buffer, iter->szToken);
			iter++;
		}
		SendPacket(buffer, CMD_PUSH, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CServerSend::SendCmd_GetDeviceStatusRep( DWORD dwUserID, DWORD dwSessionID, LIST_DEVICESTATUS& listInfo )
{
	UINT nCount = listInfo.size();
	if (nCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwSessionID << nCount;
		return SendPacket(buffer, CMD_GET_DEVICE_STATUS_REP, ERROR_NO);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_GetDeviceStatusRep(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_DEVICESTATUS::iterator iter = listInfo.begin(), iter2 = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwSessionID << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << (DWORD)iter->dwDeviceID << (DWORD)iter->dwStatus;
			iter++;
		}
		for (int j = 0; j < dwPacketCount; j++)
		{
			PutBase64Str(buffer, (PUCHAR)iter2->szStatusMsg);
			iter2++;
		}
		SendPacket(buffer, CMD_GET_DEVICE_STATUS_REP, ERROR_NO, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CServerSend::SendCmd_ReportDeviceStatusA(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2)
{
	int nSmsCount = list1.size();
	int nPushCount = list2.size();

	DECLARE_PUTBUFFER( buffer )
	buffer << dwDeviceID << dwStatus;
	PutBase64Str(buffer, pMsg);
	buffer << nSmsCount << nPushCount;

	LIST_SMSINFO::iterator iter = list1.begin();
	for(; list1.end() != iter; ++iter)
	{
		buffer << iter->dwUserID << iter->dwVendorID << iter->bLanguage;
		PutBase64Str(buffer, (PUCHAR)iter->szMobilePhone);
		PutVariableStr(buffer, (PUCHAR)iter->szDeviceName);
	}

	LIST_PUSHINFO::iterator pos = list2.begin();
	for(; list2.end() != pos; ++pos)
	{
		buffer << pos->dwUserID << pos->dwVendorID << pos->bPushType << pos->bLanguage;
		PutVariableStr(buffer, (PUCHAR)pos->szToken);
		PutVariableStr(buffer, (PUCHAR)pos->szDeviceName);
	}
	PutVariableStr(buffer, pTimeStamp);
	return SendPacket(buffer, CMD_REPORT_DEVICE_STATUS, ERROR_NO);
}

int CServerSend::SendCmd_ReportDeviceStatus(DWORD dwDeviceID, DWORD dwStatus, PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2)
{
	LIST_SMSINFO lstTempSmsInfo;
	lstTempSmsInfo.insert(lstTempSmsInfo.end(), list1.begin(), list1.end());
	LIST_PUSHINFO lstTempPushInfo;
	lstTempPushInfo.insert(lstTempPushInfo.end(), list2.begin(), list2.end());

	if (lstTempSmsInfo.empty() && lstTempPushInfo.empty())
	{
		return SendCmd_ReportDeviceStatusA(dwDeviceID, dwStatus, pMsg, pTimeStamp, lstTempSmsInfo, lstTempPushInfo);
	}
	int nTTL = 0;
	while ( (lstTempSmsInfo.empty() == false) || (lstTempPushInfo.empty() == false) )
	{
		if (++nTTL > 500) return 0;

		int nCount1 = 0, nCount2 = 0;
		CalcMaxMix(pMsg, pTimeStamp, lstTempSmsInfo, lstTempPushInfo, nCount1, nCount2);
		LOG_DEBUG(LOG_MAIN, "SendCmd_ReportDeviceStatus CalcMaxMix nCount1:%d(%d) nCount2:%d(%d)\n", nCount1, lstTempSmsInfo.size(), nCount2, lstTempPushInfo.size());

		LIST_SMSINFO lstSmsInfo; lstSmsInfo.clear();
		LIST_PUSHINFO lstPushInfo; lstPushInfo.clear();
		for (int i = 0; i < nCount1; i++)
		{
			lstSmsInfo.push_back(lstTempSmsInfo.front()); lstTempSmsInfo.pop_front();
		}
		for (int j = 0; j < nCount2; j++)
		{
			lstPushInfo.push_back(lstTempPushInfo.front()); lstTempPushInfo.pop_front();
		}
		SendCmd_ReportDeviceStatusA(dwDeviceID, dwStatus, pMsg, pTimeStamp, lstSmsInfo, lstPushInfo);
	}
	return 0;
}

int CServerSend::SendCmd_ReportUserStatus(UserStatus_t& tInfo)
{
	int nTokenLen = strlen((const char*)tInfo.szToken);
	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwStatus << tInfo.bPushType << (BYTE)nTokenLen;
	buffer << CByteArrayBuffer((PUCHAR)tInfo.szToken, nTokenLen);
	return SendPacket(buffer, CMD_REPORT_USER_STATUS, ERROR_NO);
}

const int MAX_SEND_GETDEVICEST = 335; // (1400-44-12) / 4
int CServerSend::SendCmd_GetDeviceStatus(DWORD dwUserID, DWORD dwSessionID, LIST_DWORD& listDeviceID)
{
	UINT nCount = listDeviceID.size();
	if (nCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwSessionID << nCount;
		return SendPacket(buffer, CMD_GET_DEVICE_STATUS, ERROR_NO);
	}
	WORD wTotalSeg = nCount / MAX_SEND_GETDEVICEST;
	if(nCount % MAX_SEND_GETDEVICEST) wTotalSeg++;

	LIST_DWORD::iterator iter = listDeviceID.begin();
	UINT nPacketCount, nRemainCount = nCount, dwCount = MAX_SEND_GETDEVICEST;
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		nPacketCount = 0;
		if (dwCount > nRemainCount) dwCount = nRemainCount;	

		DECLARE_PUTBUFFER( buffer )
		buffer << dwUserID << dwSessionID << nCount;

		for(; listDeviceID.end() != iter; ++iter)
		{
			DWORD dwDeviceID = *iter;
			buffer << dwDeviceID;

			nRemainCount--;
			if(++nPacketCount < dwCount) continue;

			SendPacket(buffer, CMD_GET_DEVICE_STATUS, ERROR_NO, wTotalSeg, wSubSeg);	
			nPacketCount = 0; ++iter; break;
		}
	}
	return 0;
}

int CServerSend::SendCmd_TransClientInfo(TransClientInfo_t& tInfo, bool bRelay)
{
	LOG_DEBUG(LOG_MAIN,"==>CServerSend::%s bSrcType %d dwServerID %d dwUserID %d dwSessionID %d dwDeviceID %d bType %d\n",
		__FUNCTION__, tInfo.bSrcType, tInfo.dwServerID, tInfo.dwUserID, tInfo.dwSessionID,
		tInfo.dwDeviceID, tInfo.bType);

	m_tHeader.reserved0 = (BYTE)bRelay;
	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.bSrcType << tInfo.dwServerID << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwDeviceID << tInfo.bType;
	PutBuffer_NetInfo(buffer, tInfo.tNetInfo);
	buffer << tInfo.nTransmitSessionMode;
	return SendPacket(buffer, CMD_TRANS_CLIENTINFO, ERROR_NO);
}

int CServerSend::SendCmd_TransServerInfo(TransServerInfo_t& tInfo)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.bSrcType << tInfo.dwServerID << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwDeviceID << tInfo.bType;
	PutBuffer_ConnectInfo(buffer, tInfo.tConnectInfo[0]);
	PutBuffer_ConnectInfo(buffer, tInfo.tConnectInfo[1]);
	return SendPacket(buffer, CMD_TRANS_SERVERINFO, ERROR_NO);
}

int CServerSend::SendCmd_AddTempUser( TransClientInfo_t& tInfo )
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.bSrcType << tInfo.dwServerID << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwDeviceID << tInfo.bType;
	buffer << tInfo.dwDeviceID;
	PutBuffer_NetInfo(buffer, tInfo.tNetInfo);
	BYTE szUserName[LENGTH_CHALLENGE+1] = {0};
	BYTE szPassword[LENGTH_CHALLENGE+1] = {0};
	GenerateTmpUserInfo((PUCHAR)szUserName, (PUCHAR)szPassword);
	buffer << CByteArrayBuffer((PUCHAR)szUserName, LENGTH_CHALLENGE);
	buffer << CByteArrayBuffer((PUCHAR)szPassword, LENGTH_CHALLENGE);
	buffer << tInfo.nTransmitSessionMode;
	return SendPacket(buffer, CMD_ADD_TMP_USER, ERROR_NO);
}

int CServerSend::SendPacket( CPutBuffer& buffer, WORD wCommand, WORD wError, WORD wTotalSeg /*= 1*/, WORD wSubSeg /*= 1*/ )
{
	LOG_ASSERT_RET(LOG_MAIN, m_pCon, -1);
	int nLen = buffer.GetFilledSize();
	buffer.SetOffset(0);
	// Header
	buffer << (BYTE)m_bGroupCode << (WORD)wCommand/*Command ID*/ << (BYTE)m_tHeader.reserved0/*Reserved0*/
		<< (WORD)m_tHeader.version/*Version*/ << (WORD)m_tHeader.reserved1/*Reserved1*/
		<< (DWORD)this/*Source ID*/
		<< (DWORD)m_tBaseInfo.dwServerID/*Destination ID*/
		<< (DWORD)m_tHeader.commandflag/*Command Flag*/
		<< (WORD)wTotalSeg/*Total Segment*/ << (WORD)wSubSeg/*Sub Segment*/
		<< (WORD)0/*Segment Flag*/ << (WORD)m_tHeader.reserved2/*Reserved2*/
		<< (DWORD)m_tHeader.reserved3/*Reserved3*/;
	// Payload
	buffer << (WORD)wError/*Error Flag*/ << (WORD)0/*Reserved0*/
		<< (DWORD)0/*Checksum Type && Checksum Value*/
		<< (BYTE)0/*Checksum Value*/ << (BYTE)0/*Payload Version*/ << (WORD)0/*Payload Length*/;
	buffer.SetOffset(nLen);
	LOG_DEBUG(LOG_MAIN, "ToServer ServerType %d ServerID %d pCon %p SendData cmd:0x%04x err:0x%04x len:%d\n",
		m_tBaseInfo.bServerType, m_tBaseInfo.dwServerID, m_pCon, wCommand, wError, nLen);
	return m_pCon->SendCommand((PUCHAR)buffer, nLen);
}

int CServerSend::SendCmd_AddTempUserRep( TransClientInfo_t& tInfo, PUCHAR pUserName, PUCHAR pPassword )
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tInfo.bSrcType << tInfo.dwServerID << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwDeviceID << tInfo.bType;
	buffer << tInfo.dwDeviceID;
	PutBuffer_NetInfo(buffer, tInfo.tNetInfo);
	buffer << CByteArrayBuffer(pUserName, LENGTH_CHALLENGE);
	buffer << CByteArrayBuffer(pPassword, LENGTH_CHALLENGE);
	return SendPacket(buffer, CMD_ADD_TMP_USER_REP, ERROR_NO);
}
//上报报警状态
int CServerSend::SendCmd_AlarmStatus(AlarmStatus_t& alarmStatus, WORD wError)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << alarmStatus.dwDeviceID << alarmStatus.dwRoomID << alarmStatus.dwType << alarmStatus.dwSubType;
	buffer << CByteArrayBuffer(alarmStatus.szTimeStamp, LENGTH_TIMESTAMP);
	return SendPacket(buffer, CMD_REPORT_ALARMSTATUS, wError);
}

int CServerSend::SendCmd_Qiniu_UploadToken(StorageTag_t& tTag, WORD wError)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID;
	return SendPacket(buffer, CMD_GET_UPLOAD_TOKEN, wError);
}

int CServerSend::SendCmd_Qiniu_ReportUploadResult( StoreKey_t& tKey )
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tKey.dwDeviceID << tKey.dwRoomID << tKey.dwSize << tKey.dwStoreID << tKey.bType << tKey.bRecReason;
	buffer << CByteArrayBuffer((PUCHAR)tKey.szTimeStamp, LENGTH_TIMESTAMP2);
	return SendPacket(buffer, CMD_REPORT_UPLOAD_RESULT, ERROR_NO);
}

int CServerSend::SendCmd_Qiniu_DownloadUrls(StorageTag_t& tTag, StoreKey_t& tKey, WORD wError)
{
	LOG_DEBUG(LOG_MAIN,"CServerSend::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID;
	buffer << tKey.dwDeviceID << tKey.dwRoomID << tKey.dwSize << tKey.dwStoreID << tKey.bType << tKey.bRecReason;
	buffer << CByteArrayBuffer((PUCHAR)tKey.szTimeStamp, LENGTH_TIMESTAMP2);
	return SendPacket(buffer, CMD_GET_DOWNLOAD_URLS, wError);
}

int CServerSend::SendCmd_Qiniu_DownloadUrls2(StorageTag_t& tTag, StoreVisitor_t& tVisitor, WORD wError)
{
	LOG_DEBUG(LOG_MAIN,"CServerSend::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER( buffer )
	buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID;
	buffer << tVisitor.tKey.dwDeviceID << tVisitor.tKey.dwRoomID << tVisitor.tKey.dwSize << tVisitor.tKey.dwStoreID 
		<< tVisitor.tKey.bType << tVisitor.tKey.bRecReason;
	buffer << CByteArrayBuffer((PUCHAR)tVisitor.tKey.szTimeStamp, LENGTH_TIMESTAMP2);
	buffer << tVisitor.startIndex << tVisitor.dwCount;
	return SendPacket(buffer, CMD_GET_DOWNLOAD_URLS2, wError);
}

int CServerSend::SendCmd_Qiniu_UploadTokenRep(StorageTag_t& tTag, PUCHAR pUploadToken, WORD wError)
{
	DECLARE_PUTBUFFER( buffer )
	buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID;
	PutVariableStr(buffer, pUploadToken);
	return SendPacket(buffer, CMD_GET_UPLOAD_TOKEN_REP, wError);
}

int CServerSend::SendCmd_Qiniu_DownloadUrlsRep(StorageTag_t& tTag, LIST_STORE_KEYURL& listInfo, WORD wError)
{
	LOG_DEBUG(LOG_MAIN,"CServerSend::%s\n",__FUNCTION__);
	UINT nCount = listInfo.size();
	if (nCount == 0)
	{
		DECLARE_PUTBUFFER( buffer )
		buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID << nCount;
		return SendPacket(buffer, CMD_GET_DOWNLOAD_URLS_REP, wError);
	}

	LIST_DWORD listCount;
	WORD wTotalSeg = CalSendCount_DownloadUrlsRep(listInfo, listCount);

	LIST_DWORD::iterator posCount = listCount.begin();
	LIST_STORE_KEYURL::iterator iter = listInfo.begin(), iter2 = listInfo.begin();
	for (WORD wSubSeg = 1; wSubSeg <= wTotalSeg; wSubSeg++)
	{
		if (listCount.end() == posCount) break;
		DWORD dwPacketCount = *posCount; posCount++;

		DECLARE_PUTBUFFER( buffer )
		buffer << tTag.bSrcType << tTag.dwTagID1 << tTag.dwTagID2 << tTag.dwStoreID << dwPacketCount;

		for (int i = 0; i < dwPacketCount; ++i)
		{
			buffer << iter->tKey.dwDeviceID << iter->tKey.dwRoomID << iter->tKey.dwSize
				<< iter->tKey.dwStoreID << iter->tKey.bType << iter->tKey.bRecReason;
			buffer << CByteArrayBuffer((PUCHAR)iter->tKey.szTimeStamp, LENGTH_TIMESTAMP2);
			PutVariableStr(buffer, (PUCHAR)iter->szUrl);
			iter++;
		}
		
		SendPacket(buffer, CMD_GET_DOWNLOAD_URLS_REP, wError, wTotalSeg, wSubSeg);
	}
	return 0;
}

int CServerSend::SendCmd_DownLoadUrlsRep(DWORD dwDeviceID, AdvertInfo_t& tInfo, PUCHAR pUrl )
{
	LOG_DEBUG(LOG_MAIN,"CServerSend::%s\n",__FUNCTION__);
	DECLARE_PUTBUFFER(buffer)
	buffer << dwDeviceID;
	buffer << tInfo.dwAdID << tInfo.dwAdType << tInfo.dwApplyID << tInfo.dwApplyType << tInfo.dwSize 
		<< tInfo.dwStoreID << tInfo.dwStoreType << tInfo.dwTimeStamp << tInfo.dwUsePosition << tInfo.dwUseType;
	PutVariableStr(buffer, tInfo.szFormat);
	PutVariableStr(buffer, pUrl);
	SendPacket(buffer, CMD_GET_ADVERTURLS_REP, ERROR_NO);
}

int CServerSend::SendCmd_SdkTunnel( SdkTunnel_t& tInfo )
{
	DECLARE_PUTBUFFER(buffer)
	buffer << tInfo.bSrcType << tInfo.dwServerID << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwDeviceID << (WORD)tInfo.nTunnelDataLen;
	buffer << CByteArrayBuffer(tInfo.pTunnelData, tInfo.nTunnelDataLen);
	return SendPacket(buffer, CMD_SDK_TUNNEL, ERROR_NO);
}

int CServerSend::SendCmd_SdkTunnelRep( SdkTunnel_t& tInfo )
{
	DECLARE_PUTBUFFER(buffer)
	buffer << tInfo.bSrcType << tInfo.dwServerID << tInfo.dwUserID << tInfo.dwSessionID << tInfo.dwDeviceID << (WORD)tInfo.nTunnelDataLen;
	buffer << CByteArrayBuffer(tInfo.pTunnelData, tInfo.nTunnelDataLen);
	return SendPacket(buffer, CMD_SDK_TUNNEL_REP, ERROR_NO);
}
