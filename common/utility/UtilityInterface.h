#ifndef UTILITY_INTERFACE_H
#define UTILITY_INTERFACE_H

#include "UtilityDataStruct.h"
#include "Protocol.h"
#include "getbuffer.h"
#include "putbuffer.h"
#include <sys/time.h>
#include <stdlib.h>
#include <iconv.h>
#include <string>
#include <sys/statvfs.h>

bool   UtilityInit(DWORD dwIP = 0);
void   UtilityFinish();
void   ShowVerion();

/************************************************************************/
/* �������																					*/
/************************************************************************/
class IDealMulPktSink
{
public:
	virtual int OnSendNext(DWORD dwCmdFlag, PUCHAR pData, int nLen) = 0;
	virtual ~IDealMulPktSink(){}
};

// dwCmdFlag: ÿ����Ƭ��1
// wSegFlag : ÿ�������İ���1
void   AddMulPkt(DWORD dwDeviceID, MulPkt_e eMulPkt, PUCHAR pData, int nLen, DWORD dwCmdFlag, WORD wSegFlag);
void   StartSendMulPkt(DWORD dwDeviceID, MulPkt_e eMulPkt, IDealMulPktSink* pSink);
void   DelMulPkt(IDealMulPktSink* pSink);
void   AddMulPktAck(DWORD dwDeviceID, MulPkt_e eMulPkt, DWORD dwCmdFlag);


/************************************************************************/
/* ��ʱ��                                                               */
/************************************************************************/
class ITimerSink
{
public:
	virtual void OnTimer(TimerReason_e eReason, ITimerSink* pSink) = 0;
	virtual ~ITimerSink(){}
};

//Interval Unit: second
void   AddTimer(TimerReason_e eReason, DWORD dwInterval, ITimerSink* pSink);
void   DelTimer(TimerReason_e eReason, ITimerSink* pSink);


/************************************************************************/
/* http����                                                             */
/************************************************************************/
class IHttp
{
public:
	virtual int SetHttp(const char* pHost) = 0;
	virtual int SetHttp(const char* pHost, WORD wPort) = 0;
	virtual int SetHttp(DWORD dwHost, WORD wPort) = 0;
	virtual int HttpRequest(CSTRING& request) = 0;
	virtual ~IHttp() {}
};

class IHttpSink
{
public:
	virtual int OnHttpResponse(IHttp* pHttp, PUCHAR pData, int nLen) = 0;
	virtual int OnHttpError(IHttp* pHttp, int nResult) = 0;
	virtual ~IHttpSink() {}
};

IHttp*	RegisterHttp(IHttpSink* pSink);
void	UnRegisterHttp(IHttp* pHttp);

DWORD  SN2ID(char* pSerialNum);
void   GetTime(timeval& packetTime);

bool   JoinMutiGroup(INetConnection* pCon, const char* pAddr);
DWORD  Net_GetEthIP(const char* pifName);
char*  IpDword2Str(DWORD dwIP);
DWORD  IpStr2Dword(char* pStrIP);

/************************************************************************/
/* Url�����                                                            */
/************************************************************************/
void   UrlEncode(const std::string &strIn, std::string &strOut, bool isUpper = true);
void   UrlDecode(const std::string &strIn, std::string &strOut);

/************************************************************************/
/* Base64�����                                                         */
/************************************************************************/
int    Base64EncVal(char * base64code, const char * src, int src_len);
int    Base64DecVal(char * buf, const char * base64code, int src_len);
int    Base64_encode_len(int len);
int	   Base64_encode(char *encoded, const char *string, int len);
int    Base64_decode_len(const char *bufcoded, int buflen);
int    Base64_decode(char *bufplain, const char *bufcoded, int buflen);

PUCHAR CalMd5Val(PUCHAR pInStr, int nLen);
PUCHAR CalDesVal(PUCHAR pKey, PUCHAR pIn, bool bEnc = true);
PUCHAR EncDesA(PUCHAR pKey, PUCHAR pIn, int& nOutLen);

int    USDK_DESEncode(PUCHAR pKey, PUCHAR pInput, int nInLen, PUCHAR pOut);
int    USDK_DESDecode(PUCHAR pKey, PUCHAR pInput, int nInLen, PUCHAR pOut);

void   CalCrc32Val(DWORD* pCrc32, PUCHAR pData, DWORD uSize);
int    Ascii2HexStr(char* pOutHexStr, char* pInASCIIData,  int nInDataLen);
int    HexStr2Ascii(char* pOutAscii, char* pInHexStr, int nInLen);
void   string2timeval(char szBuf[], timeval& tv);

//////////////////////////////////////////////////////////////////////////
// �ַ�����ת��
// such as convert utf-8 to gb2312 charset
// ����ת���ֽ���; ʧ��ʱ���� CHARSET_CONVERT_ERR;
int CharsetConvert(char* From, char* To, char* pInBuf, size_t nInLen, char* pOutBuf, size_t nOutLen);
int CharsetConvert(char *from, char *to, char* pInBuf, size_t nInLen, std::string &strOut);

unsigned long GetHostIP(const char* pData);

void JsonEscape(std::string & str);
bool ParsePacketHeader(PUCHAR pData, int nLen, PacketHeader_t& tHeader);
void CalcAuthDigist(PUCHAR pDigist, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pChallenge);
void CalcAuthDigistA(PUCHAR pDigist, PUCHAR pUserName, PUCHAR pMD5Password, PUCHAR pChallenge);
void GenerateChallenge(PUCHAR pChallenge);

int DividePair		(CSTRING& strData, CSTRING& strKey, CSTRING& strValue, const CSTRING& strItem);
int DivideParam		(CSTRING& strData, MAP_CSTRING& tMap);
int DivideStr		(CSTRING& strData, LIST_CSTRING& tList, const CSTRING& strItem);

void PutVariableStr	(CPutBuffer& buf, PUCHAR pStr);
void PutBase64Str	(CPutBuffer& buf, PUCHAR pStr);
bool GetVariableStr	(CGetBuffer& buf, PUCHAR pStr, int nMaxLen, int nTotalLen, int& nNeedLen);
bool GetBase64Str	(CGetBuffer& buf, PUCHAR pStr, int nMaxLen, int nTotalLen, int& nNeedLen);

void PutBuffer_ConnectInfo	(CPutBuffer& buffer, ConnectInfo_t& tInfo);
void PutBuffer_NetInfo		(CPutBuffer& buffer, NetInfo_t& tInfo);
void GenerateTmpUserInfo	(PUCHAR pUserName, PUCHAR pPassword);

void Clear_ConnectInfo(ConnectInfo_t& tInfo);
void Clear_NetInfo(NetInfo_t& tInfo);
void Clear_TransClientInfo(TransClientInfo_t& tInfo);
void Clear_TransServerInfo(TransServerInfo_t& tInfo);


DWORD CalSendCount_ServerInfo(LIST_SERVERINFO& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_UserDevice(MAP_DEVROOMINFO& mapDevRoomInfo, LIST_DEVICEINFO& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_UserGroup(LIST_GROUPINFO& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_UserRoom(LIST_ROOMINFO& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_DeviceUser(LIST_SMSINFO& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_DevicePush(LIST_PUSHINFO& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_DserverConfigureIndex(LIST_SERVERINFO& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_RegisterInfo(LIST_SERVERINFO& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_ReportDeviceStatus(PUCHAR pMsg, LIST_NOTIFY_PUSHINFO& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_GetDeviceStatusRep(LIST_DEVICESTATUS& listInfo, LIST_DWORD& listCount);


//////////////////////////////////////////////////////////////////////////
// �ƴ洢�������
DWORD CalSendCount_ListStoreAccountKeys(LIST_STORE_ACCOUNTKEYS& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_StoreAccountKeys(int nMaxLen, StoreAccountKeys_t& tInfo, LIST_DWORD& listCount);
void Packet_StoreAccountKeys(CPutBuffer& buffer, StoreAccountKeys_t& tInfo);
DWORD CalSendCount_DownloadUrlsRep(LIST_STORE_KEYURL& listInfo, LIST_DWORD& listCount);

//////////////////////////////////////////////////////////////////////////
// ���㱨����Ϣ���������
void  CalcMaxMix(PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2, int& nCount1, int& nCount2);
int   CalcPushInfoCount(int nMaxLen, LIST_PUSHINFO& listInfo, int& nCount);


//////////////////////////////////////////////////////////////////////////
// �豸������Ϣ�������
DWORD CalSendCount_DeviceRoomSum(LIST_ROOMSUM& listInfo, LIST_DWORD& listCount);

DWORD CalSendCount_DeviceRoomPush(LIST_ROOMPUSH& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_DeviceRoomUser(LIST_ROOMUSER& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_DeviceRoomCard(LIST_ROOMCARD& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_DeviceRoomIndoor(LIST_ROOMINDOOR2& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_DeviceRoomPushSwitch(LIST_ROOMPUSHSWITCH& listInfo, LIST_DWORD& listCount);

DWORD CalSendCount_RoomPushInfo(int nMaxLen, RoomPush_t& tInfo, LIST_DWORD& listCount);
DWORD CalSendCount_RoomUserInfo(int nMaxLen, RoomUser_t& tInfo, LIST_DWORD& listCount);
DWORD CalSendCount_RoomCardInfo(int nMaxLen, RoomCard_t& tInfo, LIST_DWORD& listCount);
DWORD CalSendCount_RoomIndoorInfo(int nMaxLen, RoomIndoor2_t& tInfo, LIST_DWORD& listCount);
DWORD CalSendCount_RoomPushSwitch(int nMaxLen, RoomPushSwitch_t& tInfo, LIST_DWORD& listCount);

DWORD CalSendCount_DeviceRoomOther(LIST_ROOMOTHER& listInfo, LIST_DWORD& listCount);

DWORD CalSendCount_RoomIndex(LIST_DWORD& listInfo, LIST_DWORD& listCount);
DWORD CalSendCount_ClearRooms(MAP_DWORD& mapDeviceID, LIST_DWORD& listCount);
DWORD CalSendCount_UpdateDevice(MAP_DWORD& mapDeviceID, LIST_DWORD& listCount);
DWORD CalSendCount_UpdateDeviceEx(LIST_DWORD& lstDevID, LIST_DWORD& listCount);
DWORD CalSendCount_UpdateSpecialCrowd(LIST_SPECIALCROWD& m_lstSpecialCrowd, LIST_DWORD& listCount);

DWORD CalSendCount_DeleteRoom(LIST_DWORD& listInfo, LIST_DWORD& listCount);

void Packet_RoomPushInfo(CPutBuffer& buffer, RoomPush_t& tRoomInfo);
void Packet_RoomUserInfo(CPutBuffer& buffer, RoomUser_t& tRoomInfo);
void Packet_RoomCardInfo(CPutBuffer& buffer, RoomCard_t& tRoomInfo);
void Packet_RoomCardTimeLimit(CPutBuffer& buffer, RoomCard_t& tRoomInfo);
void Packet_RoomIndoorInfo(CPutBuffer& buffer, RoomIndoor2_t& tRoomInfo);
void Packet_RoomPushSwitch(CPutBuffer& buffer, RoomPushSwitch_t& tRoomInfo);
//////////////////////////////////////////////////////////////////////////

DWORD CalcRealVendorID(DWORD dwVendorID);
DWORD CalcVendorAppID(DWORD dwVendorID);

void CfgLineEncode(PUCHAR pKey, CSTRING& line);
void CfgLineDecode(PUCHAR pKey, CSTRING& line);

bool GetDBServerCfg		(ServerCfg_t& tCfgInfo);
bool GetDServerCfg		(ServerCfg_t& tCfgInfo);
bool GetRelayServerCfg	(ServerCfg_t& tCfgInfo);
bool GetLgnServerCfg	(ServerCfg_t& tCfgInfo);
bool GetNtyServerCfg	(ServerCfg_t& tCfgInfo);
bool GetStatServerCfg	(ServerCfg_t& tCfgInfo);
bool GetSDBServerCfg	(ServerCfg_t& tCfgInfo);
bool GetSBServerCfg		(ServerCfg_t& tCfgInfo);
void PrintServerCfg		(ServerCfg_t& tCfgInfo);
bool GetSystemCfg		(SystemCfg_t& tCfgInfo);
void PrintSystemCfg		(SystemCfg_t& tCfgInfo);

extern DWORD g_dwSameIP;
extern BYTE g_bSameSvrType;
class FindSameServer
{
public:
	bool operator() (LIST_SERVERINFO::value_type& pos)
	{
		if ( (g_dwSameIP == pos.dwIP) && (g_bSameSvrType == pos.bServerType) ) return true;
		return false;
	}
};

// ������ͬIP��ַ��ͬ���͵�server�����������ע�����������ͬһ̨��������
void RemoveSameServer(LIST_SERVERINFO& lstInfo);

// �ж��ֻ�����Ϸ���
// 1��11λ 2��ȫ����
bool IsValidMobilePhone(const char* pMobilePhone);

// �и�������ͨ��
// nPermission �ٷ���
bool PermissionPass(int nPermission);

void GenerateTimeStamp(PUCHAR pTimeStamp);
void GenerateDeviceStatusMsg(PUCHAR pDeviceName, PUCHAR pAlarmMsg, PUCHAR pTimeStamp, PUCHAR pOutMsg);

// �����ƴ洢�˺�ID�ַ���
DWORD ParseStoreIDStr(CSTRING& storeid);

void GenerateStoreKey(StoreKey_t& tKey, PUCHAR pKey);

//���ݿ������ת��
void SelectPwdMark(std::string& pVall);
void InsertPwdMark(char* pVall);

//ȡ����ҵ���ֻ�����
bool ProPhone(char strPhone[64]);

//��ȡ�����ļ��б�
bool GetLocalFileList(const char* pLocalDir, LIST_LOCAL_FILE& lstFileInfo);
//��ȡ����ʹ�����
bool GetVstatdisk(const char *dirName,UInt64& freeMegaBytes,UInt64& totalMegaBytes);
//�����̿ռ䣬�жϴ���ʣ��ռ��Ƿ�С��10%
bool CheckDiskSpace();
//��ȡ�����б����ҵ��������ع����ļ�
bool GetDelFile(const char* pLocalDir);

bool ExCommand( const char* pCommand, LIST_CSTRING& listStr, bool bPrint /*= false*/ );
#endif
