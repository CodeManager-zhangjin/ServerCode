#ifndef _COMMON_DATASTRUCT_H_
#define _COMMON_DATASTRUCT_H_

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <list>
#include <map>
#include <set>

#include "NetworkInterface.h"

#define _CFGENC_KEY_		"1.0.0.1"

typedef enum
{
	PVID_Unknown,
	PVID_Dong,   // IP: 121.40.161.228   0x7928A1E4
	PVID_Aurine, // IP: 182.92.183.137   0xB65CB789
	PVID_Mili,   // IP: 121.43.153.1     0x792B9901
	PVID_Fuju    // IP: 120.26.132.253   0x781A84FD
}PlatformVID_e;

//data types define
typedef unsigned char		UCHAR;
typedef int					INT;
typedef char*				STRING;
typedef long				LONG;
typedef unsigned long		DWORD;
typedef unsigned long		ULONG;
typedef const char*			LPCTSTR;
typedef unsigned short		USHORT;
typedef char*				PCHAR;
typedef UCHAR*				PUCHAR;
typedef char				CHAR;
typedef unsigned char		BYTE;
typedef short				SHORT;
typedef unsigned short		WORD;
typedef unsigned short		USHORT;
typedef unsigned int		UINT;
typedef long				BOOL;
typedef unsigned long long	UInt64;
typedef long long			INT64;
typedef std::string			CSTRING;
typedef std::list<CSTRING>	LIST_CSTRING;
typedef std::list<DWORD>	LIST_DWORD;
typedef std::map<DWORD, DWORD>	MAP_DWORD;
typedef std::set<DWORD>		SET_DWORD;
typedef std::map<DWORD, CSTRING> MAP_DWORD_STRING;
typedef std::map<CSTRING, DWORD>	MAP_STRING_DWORD;
typedef std::map<CSTRING, CSTRING>	MAP_CSTRING;
typedef std::map<DWORD, SET_DWORD>	MAP_DWORD_SETDWORD;
typedef std::map<DWORD, LIST_DWORD>	MAP_DWORD_LSTDWORD;
typedef std::map<CSTRING, LIST_DWORD> MAP_STRING_LSTDWORD;

//////////////////////////////////////////////////////////////////////////
// AlarmType（告警类型）
const DWORD AT_NONE		= 0; // 未知报警
const DWORD AT_DI1		= 1; // 开关量1
const DWORD AT_DI2		= 2; // 开关量2
const DWORD AT_DI3		= 3; // 开关量3
const DWORD AT_BTNE		= 4; // 紧急求助
const DWORD AT_BTNB		= 5; // 业务咨询
const DWORD AT_ONLINE	= 6; // 上线
const DWORD AT_OFFLINE	= 7; // 下线
const DWORD AT_CALL		= 8; // 设备呼叫
const DWORD AT_BODY		= 9; // 人体感应
const DWORD AT_TUNNEL	= 10; // 透传
const DWORD AT_CALL_ANSWER = 11; // 呼叫已接听
const DWORD AT_CALL_END = 12;    // 呼叫已结束

//////////////////////////////////////////////////////////////////////////
// 常用量
const int MAX_PACKET_LEN			= 1400;

const int LENGTH_CHALLENGE			= 16;
const int LENGTH_NAME				= 64;
const int LENGTH_PASSWORD			= 16;
const int LENGTH_DEADLINE			= 8;//存放截至日期数组长度
const int LENGTH_POSITION			= 128;
const int LENGTH_MOBILEPHONE		= 64;
const int LENGTH_EMAIL				= 64;
const int LENGTH_QUESTION			= 256;
const int LENGTH_ANSWER				= 256;
const int LENGTH_SERIALNO			= 20;
const int LENGTH_IMAGEVERSION		= 20;
const int LENGTH_TOKEN				= 64;
const int LENGTH_SMSAUTHCODE		= 6;
const int LENGTH_MSGCONTENT			= 256;
const int LENGTH_URL				= 256;
const int LENGTH_ROOM				= 16;
const int LENGTH_USERROOM			= 8;
const int LENGTH_MD516				= 16;
const int LENGTH_ROOMPWD			= 16;
const int LENGTH_CARDNUMBER			= 32;
const int LENGTH_CARDTIMELIMIT		= 8;
const int LENGTH_COMNUMBER			= 32;
const int LENGTH_TIMESTAMP			= 19; // "2015-07-23 16:30:01"
const int LENGTH_TUNNELDATA			= 1200;
const int LENGTH_ACCESSKEY			= 64; // 云存储accesskey长度
const int LENGTH_SECRETKEY			= 64; // 云存储secretkey长度
const int LENGTH_BUCKET				= 32; // 云存储空间名长度
const int LENGTH_DOMAIN				= 32; // 云存储域名长度
const int LENGTH_UPLOADTOKEN		= 256; // 云存储上传凭证长度
const int LENGTH_TIMESTAMP2			= 14; // "20150723163001"
const int LENGTH_STOREKEY			= 72; // "/devid/roomid/type/timestamp-size-storeid.jpg"
const int LENGTH_STOREURL			= 256;
const int LENGTH_PHONENUMBER	= 20;//手机号码长度
const int LENGTH_FORMAT				= 5; //文件格式类型长度

const int LENGTH_UCPAAS_USERNAME	= 128;
const int LENGTH_UCPAAS_PASSWORD	= 128;
const int LENGTH_UCPAAS_APPID		= 128;

const int LENGTH_SYSTEM_PHONENUMBER = 15;//查询电话号码
const int LENGTH_TITLE				= 20;//物業公告標題長度
const int LENGTH_CONTENT			= 1000;//物業公告內容長度

const BYTE SERVER_TYPE_LOGIN		= 1;
const BYTE SERVER_TYPE_D			= 2;
const BYTE SERVER_TYPE_NB			= 3;
const BYTE SERVER_TYPE_STATUS		= 4;
const BYTE SERVER_TYPE_NOTIFICATION	= 5;
const BYTE SERVER_TYPE_STORAGE		= 6;

const WORD SERVER_PORT_D			= 3478;
const WORD SERVER_PORT_LOGIN		= 5432;
const WORD SERVER_PORT_STATUS		= 5434;
const WORD SERVER_PORT_NOTIFICATION	= 5435;
const WORD SERVER_PORT_DB			= 5438;
const WORD SERVER_PORT_NB			= 5439;
const WORD SERVER_PORT_WEB_DB		= 5437; // 网页消息到dbserver
const WORD SERVER_PORT_WEB_ST		= 5441; // 网页消息到statusserver

const WORD SERVER_PORT_SDB			= 5455; // 云存储数据库服务器
const WORD SERVER_PORT_SB			= 5456; // 云存储业务服务器
const WORD SERVER_PORT_SB_CALLBACK	= 5457; // 云存储服务器回调端口
const WORD SERVER_PORT_DB_LOCAL		= 45566;
const WORD SERVER_PORT_SDB_LOCAL	= 45567;
const WORD SERVER_PORT_SB_LOCAL		= 45568;

//////////////////////////////////////////////////////////////////////////
// 错误码
const WORD ERROR_NO						= 0;
const WORD ERROR_INVALID_SERIALNO		= 1;
const WORD ERROR_INVALID_USERNAME		= 2;
const WORD ERROR_INVALID_PASSWORD		= 3;
const WORD ERROR_NOT_EXIST_MOBILEPHONE	= 4;
const WORD ERROR_DBSERVER_BUSY			= 5;
const WORD ERROR_SYSTEM					= 6;
const WORD ERROR_INVALID_USERID			= 7;
const WORD ERROR_NOTIFYSERVER_OFFLINE	= 8;
const WORD ERROR_TIMEOUT				= 9;
const WORD ERROR_INVALID_SESSIONID		= 10;
const WORD ERROR_INVALID_MOBILEPHONE	= 11;
const WORD ERROR_INVALID_PUSHOPR		= 12;
const WORD ERROR_PERMISSION				= 13;
const WORD ERROR_INVALID_VENDORID		= 14;
const WORD ERROR_USEUP					= 15; //新增
const WORD ERROR_UNKNOW					= 99;

const int HTTP_ERROR_TIMEOUT = -2;
const int HTTP_ERROR_CONNECT = -3;

// 支持推送类型
// 1-苹果		2-百度云android		3-个推ios		4-个推android		5-百度云ios
// 6-极光ios	7-极光android		8-小米android	9-华为android		10-魅族android
const BYTE PUSH_TYPE_IOS			= 1;
const BYTE PUSH_TYPE_ANDROID_BAIDU	= 2;
const BYTE PUSH_TYPE_IOS_GT			= 3;
const BYTE PUSH_TYPE_ANDROID_GT		= 4;
const BYTE PUSH_TYPE_IOS_BAIDU		= 5;
const BYTE PUSH_TYPE_IOS_JG			= 6;
const BYTE PUSH_TYPE_ANDROID_JG		= 7;
const BYTE PUSH_TYPE_ANDROID_MI		= 8;
const BYTE PUSH_TYPE_ANDROID_HW		= 9;
const BYTE PUSH_TYPE_ANDROID_MZ		= 10;


const DWORD RELAY_TYPE_NO			= 0;
const DWORD RELAY_TYPE_AUTO			= 1;
const DWORD RELAY_TYPE_FORCE		= 2;

// 网元类型
// 0-客户端 1-注册服务器 2-状态服务器 3-转发服务器
// 4-设备 5-网页 6-云存储业务服务器
const BYTE SRC_TYPE_CLIENT = 0;
const BYTE SRC_TYPE_STATUS = 2;
const BYTE SRC_TYPE_RELAY = 3;
const BYTE SRC_TYPE_DEVICE = 4;
const BYTE SRC_TYPE_WEB = 5;
const BYTE SRC_TYPE_STORAGE = 6;
const BYTE SRC_TYPE_INDOOR	= 7;

const BYTE REC_REASON_MOB		= 1; // 手机开门
const BYTE REC_REASON_CARD		= 2; // 刷卡开门
const BYTE REC_REASON_WIFI		= 3; // Wifi开门
const BYTE REC_REASON_TEMPPWD	= 4; // 临时密码开门
const BYTE REC_REASON_PWD		= 5; // 住户密码开门
const BYTE REC_REASON_CALL		= 10;// 呼叫住户


#define LOG_DONG_DB_FILE					"/home/dong_server/.dong_db_trace.cfg"
#define LOG_DONG_D_FILE						"/home/dong_server/.dong_d_trace.cfg"
#define LOG_DONG_RELAY_FILE					"/home/dong_server/.dong_relay_trace.cfg"
#define LOG_DONG_NTY_FILE					"/home/dong_server/.dong_nty_trace.cfg"
#define LOG_DONG_LGN_FILE					"/home/dong_server/.dong_lgn_trace.cfg"
#define LOG_DONG_STATUS_FILE				"/home/dong_server/.dong_status_trace.cfg"
#define LOG_DONG_STORAGE_BUSINESS_FILE		"/home/dong_server/.dong_storagebusiness_trace.cfg"
#define LOG_DONG_STORAGE_DB_FILE			"/home/dong_server/.dong_storagedb_trace.cfg"
#define LOG_DONG_RCS_FILE					"./.dong_rcs_trace.cfg"//test
#define CONFIG_DONG_ENCODED_FILE			"/home/dong_server/.dongserver.cfg"
#define CONFIG_DONG_SYSTEM_FILE				"/home/dong_server/.dong_system.cfg" //

#define LEN_CFG_ITEM 192
struct ServerCfg_t
{
	char szDBIP[LEN_CFG_ITEM+1];
	char szSerialNO[LEN_CFG_ITEM+1];
	char szHost[LEN_CFG_ITEM+1];
	char szSrcDB[LEN_CFG_ITEM+1];
	char szSrcDB2[LEN_CFG_ITEM+1];
	char szUserName[LEN_CFG_ITEM+1];
	char szPassword[LEN_CFG_ITEM+1];
	char szWorkIP[LEN_CFG_ITEM+1];
	char szDServerIP[LEN_CFG_ITEM+1];
	char szSwitch[LEN_CFG_ITEM+1];
	char szExchange[LEN_CFG_ITEM+1];
	char szHost3[LEN_CFG_ITEM+1];
	char szSrcDB3[LEN_CFG_ITEM+1];
	char szUserName3[LEN_CFG_ITEM+1];
	char szPassword3[LEN_CFG_ITEM+1];
};

#define LEN_SYSTEM_ITEM 192 
struct SystemCfg_t
{
	BYTE szUserID[LEN_SYSTEM_ITEM+1];
	BYTE szAccountSid[LEN_SYSTEM_ITEM+1];
	BYTE szAuthToken[LEN_SYSTEM_ITEM+1];
	BYTE szAppKey[LEN_SYSTEM_ITEM+1];
	BYTE szPhoneNumber[LEN_SYSTEM_ITEM+1];
	char szSwitch[LEN_SYSTEM_ITEM+1];
};

// 外放注册服务器需要定时向咚咚登录服务器获取许可信息
struct DServerInfo_t // 注册服务器信息
{
	char szSerialNO[LENGTH_SERIALNO+1];
	int nPermission;	// 许可
	int nCapacity;		// 能力
};

struct ServerInfo_t
{
	DWORD		dwServerID;
	DWORD		dwVendorID;
	BYTE		bServerType;
	BYTE		szSerialNO[LENGTH_SERIALNO+1];
	BYTE		szName[LENGTH_NAME+1];
	BYTE		szUserName[LENGTH_NAME+1];
	BYTE		szPassword[LENGTH_PASSWORD+1];
	DWORD		dwIP;
	UINT		nNetID;
	BYTE		szPosition[LENGTH_POSITION+1];
};
typedef std::list<ServerInfo_t>		LIST_SERVERINFO;

struct PieceOfSerialNO_t
{
	DWORD		dwBegin;
	DWORD		dwEnd;
	DWORD		dwVendorID;
};
typedef std::list<PieceOfSerialNO_t>		LIST_PIECEOFSERIALNO;

struct UserInfo_t
{
	DWORD		dwUserID;
	DWORD		dwConfigureIndex;
	BYTE		szUserName[LENGTH_NAME+1];
	BYTE		szPassword[LENGTH_PASSWORD+1]; 
	BYTE		szMobilePhone[LENGTH_MOBILEPHONE+1];
	BYTE		szUrl[LENGTH_URL+1]; // 网页Url
};

struct DeviceInfo_t
{
	DWORD		dwDeviceID;
	DWORD		dwAutoRelay;
	DWORD		dwDeviceType;
	DWORD		dwVendorID;
	DWORD		dwGroupID;
	DWORD		dwConfigureIndex;  // 0000房号的dwUserIndex
	DWORD		dwConfigureIndex2; // 0000房号的dwPushIndex
	DWORD		dwStoreID; // 设备当前设定云存储账号ID
	BYTE		szSerialNO[LENGTH_SERIALNO+1];
	BYTE		szDeviceName[LENGTH_NAME+1];
	BYTE		szPassword[LENGTH_PASSWORD+1];
	BYTE		szDeadLine[LENGTH_DEADLINE+1];
	BYTE    szImageVer[LENGTH_IMAGEVERSION+1];
	BYTE		szRoom[LENGTH_USERROOM];
};
typedef std::list<DeviceInfo_t>		LIST_DEVICEINFO;

struct GroupInfo_t
{
	DWORD		dwGroupID;
	DWORD		dwParentID;
	DWORD		dwSequence;
	BYTE		szGroupName[LENGTH_NAME+1];
};
typedef std::list<GroupInfo_t>		LIST_GROUPINFO;

struct RoomInfo_t
{
	DWORD	dwRoomID;
	DWORD	dwDeviceID;
	BYTE	szPassword[LENGTH_PASSWORD+1];
	BYTE	szRoom[LENGTH_ROOM+1];
};
typedef std::list<RoomInfo_t>		LIST_ROOMINFO;

struct DeviceStatus_t
{
	DWORD dwDeviceID;
	DWORD dwStatus;
	BYTE szStatusMsg[LENGTH_MSGCONTENT+1];
};
typedef std::list<DeviceStatus_t>	LIST_DEVICESTATUS;

struct SmsInfo_t
{
	DWORD	dwUserID;
	DWORD	dwVendorID;
	BYTE	bLanguage;	// 0-简体中文; 1-繁体中文; 2-英文
	BYTE	szMobilePhone[LENGTH_MOBILEPHONE+1];
	BYTE	szDeviceName[LENGTH_NAME+1];
};
typedef std::list<SmsInfo_t>		LIST_SMSINFO;

struct Phone_t
{
	BYTE	szMobilePhone[LENGTH_MOBILEPHONE+1];
};
typedef std::list<Phone_t>		LIST_PHONE;

//用来通知特殊人群结构体
struct SmsInfo2_t
{
	DWORD	dwUserID;
	DWORD	dwRoomID;
	DWORD	dwVendorID;
	BYTE	bLanguage;	// 0-简体中文; 1-繁体中文; 2-英文
	BYTE	szMobilePhone[LENGTH_MOBILEPHONE+1];
	BYTE	szPropertyPhone[LENGTH_MOBILEPHONE+1];
	BYTE	szDeviceName[LENGTH_NAME+1];
	LIST_PHONE lstPhone;//同房号下的其它手机号码
};
typedef std::list<SmsInfo2_t>		LIST_SMSINFO2;


struct PushInfo_t
{
	DWORD	dwUserID;
	DWORD	dwVendorID;
	BYTE	bPushType;	// 见DataStruct.h
	BYTE	bLanguage;	// 0-简体中文; 1-繁体中文; 2-英文
	BYTE	szToken[LENGTH_TOKEN+1];
	BYTE	szDeviceName[LENGTH_NAME+1];
};
typedef std::list<PushInfo_t>		LIST_PUSHINFO;

struct CardInfo_t
{
	BYTE	bCardType;
	BYTE	szCard[LENGTH_CARDNUMBER+1];
	DWORD dwCardTimeLimit;
};
typedef std::list<CardInfo_t>		LIST_CARDINFO;

struct PushSwitchInfo_t
{
	DWORD	dwUserID;
	int	nPushSwitch;
};
typedef std::list<PushSwitchInfo_t>		LIST_PUSHSWITCH;

struct RoomSum_t
{
	DWORD	dwRoomID;
	BYTE	szRoom[LENGTH_ROOM+1];
	DWORD	dwPushIndex;
	DWORD	dwUserIndex;
	DWORD	dwCardIndex;
	DWORD	dwIndoorIndex;
	DWORD	dwPushSwitchIndex;
	BYTE	szPassword[LENGTH_ROOMPWD+1];
};
typedef std::list<RoomSum_t>		LIST_ROOMSUM;

struct RoomPush_t
{
	DWORD	dwRoomID;
	DWORD	dwPushIndex;
	LIST_PUSHINFO lstPushInfo;
};
typedef std::list<RoomPush_t>		LIST_ROOMPUSH;

struct RoomUser_t
{
	DWORD	dwRoomID;
	DWORD	dwUserIndex;
	LIST_SMSINFO lstUserInfo;
};
typedef std::list<RoomUser_t>		LIST_ROOMUSER;

struct RoomCard_t
{
	DWORD	dwRoomID;
	DWORD	dwCardIndex;
	LIST_CARDINFO lstCardInfo;
};
typedef std::list<RoomCard_t>		LIST_ROOMCARD;
//start
struct InDoorInfo_t
{
	DWORD dwInDoorID;
	BYTE szSerialNO[LENGTH_SERIALNO+1];
};
typedef std::list<InDoorInfo_t>		LIST_INDOORINFO;
struct RoomIndoor2_t
{
	DWORD dwRoomID;
	DWORD dwIndoorIndex;
	LIST_INDOORINFO lstIndoorInfo;
};
typedef std::list<RoomIndoor2_t>	LIST_ROOMINDOOR2;
//end
struct RoomOther_t
{
	DWORD	dwRoomID;
	DWORD	dwRoomAttr;
	BYTE	szRoom[LENGTH_ROOM+1];
	BYTE	szPassword[LENGTH_ROOMPWD+1];
};
typedef std::list<RoomOther_t>		LIST_ROOMOTHER;

struct RoomPushSwitch_t
{
	DWORD	dwRoomID;
	DWORD	dwPushSwitchIndex;
	LIST_PUSHSWITCH lstPushSwitch;
};
typedef std::list<RoomPushSwitch_t>		LIST_ROOMPUSHSWITCH;
/*
//2
struct roomInDoorInfo
{
	DWORD dwRoomID;
	DWORD dwInDoorIndex;
	DWORD dwCount;
	LIST_INDOORINFO lstInDoorInfo;
};
typedef std::list<roomInDoorInfo>	LIST_ROOMINDOORINFO;
//1
struct RoomIndoor_t
{
	DWORD	dwDeviceID;
	DWORD	dwRoomCount;
	LIST_ROOMINDOORINFO	lstRoomIndoorInfo;
};
typedef std::list<RoomIndoor_t> LIST_ROOMINDOOR;
*/
struct TabInDoorInfo_t
{
	DWORD	dwDeviceID;
	DWORD	dwRoomID;
	DWORD	dwIndoorID;
};
typedef std::list<TabInDoorInfo_t>		LIST_TABINDOORINFO;
//////////////////////////////////////////////////////////////////////////
struct Notify_SmsInfo_t
{
	DWORD dwVendorID;
	BYTE bLanguage;
	BYTE szMessageContent[LENGTH_MSGCONTENT+1];
	BYTE szMobilePhone[LENGTH_MOBILEPHONE+1];
};
typedef std::list<Notify_SmsInfo_t>	LIST_NOTIFY_SMSINFO;

struct Notify_MailInfo_t
{
	DWORD dwVendorID;
	BYTE bLanguage;
	BYTE szMessageContent[LENGTH_MSGCONTENT+1];
	BYTE szMail[LENGTH_EMAIL+1];
};
typedef std::list<Notify_MailInfo_t>	LIST_NOTIFY_MAILINFO;

struct Notify_PushInfo_t
{
	DWORD dwDeviceID;
	DWORD dwVendorID;
	DWORD dwStatus;
	BYTE bPushType;
	BYTE bLanguage;
	BYTE szMessageContent[LENGTH_MSGCONTENT+1];
	BYTE szToken[LENGTH_TOKEN+1];
};
typedef std::list<Notify_PushInfo_t>	LIST_NOTIFY_PUSHINFO;

struct NetInfo_t
{
	DWORD		dwPublicIP;
	WORD		wPublicPortTCP;
	WORD		wPublicPortUDP;
	WORD		wLocalPortUDP;
	LIST_DWORD	listLocalIPs;
	WORD		wNetworkType;  // 1-具有公网地址 2-udp通
};

struct ConnectInfo_t
{
	DWORD		dwID;
	BYTE		szUserName[LENGTH_CHALLENGE+1];
	BYTE		szPassword[LENGTH_CHALLENGE+1];
	NetInfo_t	tNetInfo;
};

struct TempUser_t
{
	BYTE		szUserName[LENGTH_CHALLENGE+1];
	BYTE		szPassword[LENGTH_CHALLENGE+1];
	int			nTTL;
};
typedef std::list<TempUser_t>		LIST_TEMP_USER;

struct GetDeviceStatus_t
{
	DWORD		dwUserID;		// 客户端UserID
	DWORD		dwSessionID;	// 客户端SessionID
	LIST_DWORD	listDeviceID;
};

struct GetDeviceStatusRep_t
{
	DWORD		dwUserID;		// 客户端UserID
	DWORD		dwSessionID;	// 客户端SessionID
	LIST_DEVICESTATUS	listInfo;
};

struct TransClientInfo_t
{
	BYTE		bSrcType;		// 0-客户端 1-注册服务器 2-状态服务器 3-转发服务器
	DWORD		dwServerID;		// 注1 ServerID
	DWORD		dwUserID;		// 客户端UserID
	DWORD		dwSessionID;	// 客户端SessionID
	DWORD		dwDeviceID;		// 观看设备DeviceID
	BYTE		bType;			// 0:实时
	NetInfo_t	tNetInfo;		// 客户端NetInfo
	UINT		nTransmitSessionMode;
};

struct TransServerInfo_t
{
	BYTE		bSrcType;
	DWORD		dwServerID;		// 注1 ServerID
	DWORD		dwUserID;		// 客户端UserID
	DWORD		dwSessionID;	// 客户端SessionID
	DWORD		dwDeviceID;		// 观看设备DeviceID
	BYTE		bType;			// 0:实时
	ConnectInfo_t	tConnectInfo[2];
};

struct SdkTunnel_t
{
	BYTE		bSrcType;		// 0-客户端 1-注册服务器 2-状态服务器 3-转发服务器
	DWORD		dwServerID;		// 注1 ServerID
	DWORD		dwUserID;		// 客户端UserID
	DWORD		dwSessionID;	// 客户端SessionID
	DWORD		dwDeviceID;		// 设备DeviceID
	int			nTunnelDataLen;
	PUCHAR		pTunnelData;	
};

struct StorageTag_t
{
	BYTE		bSrcType;		// 0-客户端 4-设备 5-网页 6-云存储业务服务器
	DWORD		dwTagID1;		// 
	DWORD		dwTagID2;		// 
	DWORD		dwStoreID;
};

struct StorageAccount_t
{
	DWORD		dwStoreID;
	BYTE		szUserName[LENGTH_NAME+1];
	BYTE		szAccessKey[LENGTH_ACCESSKEY+1];
	BYTE		szSecretKey[LENGTH_SECRETKEY+1];
	BYTE		szBucket[LENGTH_BUCKET+1];
	BYTE		szDomain[LENGTH_DOMAIN+1];
};
typedef std::list<StorageAccount_t>  LIST_ACCOUNT;

struct StoreKey_t
{
	DWORD		dwDeviceID;
	DWORD		dwRoomID;
	DWORD		dwSize;
	DWORD		dwStoreID;
	BYTE		bRecReason;		// 存储原因
	BYTE		bType;			// 文件类型 图片或视频
	BYTE		szTimeStamp[LENGTH_TIMESTAMP2+1];
};
typedef std::list<StoreKey_t>		LIST_STOREKEY;

struct StoreVisitor_t
{
	StoreKey_t	tKey;
	DWORD		startIndex;
	DWORD		dwCount;
};
typedef std::list<StoreVisitor_t>		LIST_STOREVISITOR;

struct StoreAccountKeys_t
{
	StorageAccount_t tAccount;
	LIST_STOREKEY lstKey;
};
typedef std::list<StoreAccountKeys_t>		LIST_STORE_ACCOUNTKEYS;

struct StoreKeyUrl_t
{
	StoreKey_t tKey;
	BYTE szUrl[LENGTH_STOREURL+1];
};
typedef std::list<StoreKeyUrl_t>		LIST_STORE_KEYURL;

struct UserStatus_t
{
	DWORD	dwUserID;
	DWORD	dwSessionID;
	DWORD	dwStatus;
	BYTE	bPushType;
	BYTE	szToken[LENGTH_TOKEN+1];
};
typedef std::list<UserStatus_t>				LIST_USERSTATUS;

enum MulPkt_e
{
	MP_NULL,
	MP_Sum,
	MP_UserIndex,
	MP_PushIndex,
	MP_CardIndex,
	MP_Other,
	MP_IndoorIndex,
	MP_PushSwitchIndex
};

struct MulPkt_t
{
	DWORD	dwCmdFlag;
	bool	bSend;
	CSTRING	packet;
};
typedef std::list<MulPkt_t>				LIST_MULPKT;

struct ClientToken_t
{
	unsigned char		bMainFlag;		// 主类型标识，用于通知客户端
	unsigned char		bPushType;		// 支持推送类型：1-苹果		2-百度云android		3-个推ios	   4-个推android   5-百度云ios
										//				 6-极光ios  7-华为android       8-小米android  9-极光android
	unsigned char		szToken[LENGTH_TOKEN+1];
};

struct ClientTokenArray_t
{
	DWORD	dwUserID;
	DWORD	dwVendorID;
	BYTE	bLanguage;
	int		nCount;
	ClientToken_t tToken[5];
	BYTE	bLoginOtherPlaceFlag;
	BYTE	szCreated[LENGTH_TIMESTAMP+1];
	UINT	nPushSwitch;
	DWORD dwView;
};

struct UcpaasInfo_t
{
	BYTE szUsername[LENGTH_UCPAAS_USERNAME+1];
	BYTE szPassword[LENGTH_UCPAAS_PASSWORD+1];
	BYTE szAppid[LENGTH_UCPAAS_APPID+1];
};

//新增：存放截至日期的结构体
struct DeviceDeadLineInfo_t 
{
	DWORD		dwDeviceID;
	time_t		dwDevDeadLine;
};
typedef std::list<DeviceDeadLineInfo_t>		LIST_DEVICE_DEADLINE;

//开门记录
struct UnlockInfo_t
{
	DWORD       dwUnlockIndex;
	DWORD		dwUnlockType;//1.手机App开门；2.刷卡开门；3.wifi开门；4.临时密码开门；5.住户密码开门；6.平台查询卡号开门；7.蓝牙开门10.手机App接通访客来电#号开门
	DWORD		dwDeviceID;
	DWORD		dwRoomID;
	DWORD		dwUserID;
	BYTE 		ComNumber[LENGTH_COMNUMBER+1];
	DWORD		dwTimestamp;
};
typedef std::list<UnlockInfo_t>	LIST_DEVICE_UNLOCK;

struct UnlockRep_t
{
	DWORD		dwDeviceID;
	DWORD		dwResult;
	DWORD		dwCount;
	DWORD		UnlockIndex[25];
};

struct DevUpgrad_t
{
	DWORD		dwDeviceID;
	BYTE		DevVersion[LENGTH_IMAGEVERSION+1];
};

//推送结构体
struct strToken_t
{
	LIST_CSTRING lstInfo_gt_ios;
	LIST_CSTRING lstInfo_gt_android;
	LIST_CSTRING lstInfo_baidu_ios;
	LIST_CSTRING lstInfo_baidu_android;
	LIST_CSTRING lstInfo_jg_ios;
	LIST_CSTRING lstInfo_jg_android;
	LIST_CSTRING lstInfo_hw_android;
	LIST_CSTRING lstInfo_mi_android;
	LIST_CSTRING lstInfo_mz_android;
};

//室内机绑定门口机
struct BindInfo_t
{
	DWORD	dwDeviceID;	
	DWORD	dwRoomID;
};
typedef std::list<BindInfo_t> LIST_BIND_INFO;

struct BindInfoRep_t
{
	DWORD	dwResult;
	DWORD	dwIndoorID;
};

//新增回调
struct ReportDevStatus
{
	DWORD	dwDeviceID;
	DWORD	dwResult;
	DWORD	dwStatus;
};
//室内机获取房号列表
struct RoomInfo_t2
{
	DWORD dwRoomID;
	BYTE  szRoom[LENGTH_USERROOM+1];
};
typedef std::list<RoomInfo_t2> LIST_ROOMINFO2;
typedef std::map<DWORD, LIST_ROOMINFO2> MAP_DEVROOMINFO;

//室内机获取门口机在线离线状态
struct DevStatus
{
	DWORD dwDeviceID;
	DWORD dwStatus;
};
typedef std::list<DevStatus>	LIST_DEVSTATUS;

struct DevAdvert_t
{
	DWORD dwDeviceID;
	DWORD dwAdvertIndex;
};
typedef std::list<DevAdvert_t>  LIST_ADVERT;

//特殊人群
struct SpecialCrowd_t
{
	DWORD dwVendorID;
	DWORD dwGroupID;
	DWORD dwDevcieID;
	DWORD dwRoomID;
	DWORD dwUserID;
	DWORD dwPropertyUserID;
	DWORD dwUnlockTime;
	BYTE	  szUserPhone[LENGTH_PHONENUMBER];
	BYTE	  szPropertyPhone[LENGTH_PHONENUMBER];
	BYTE	  szDeviceName[LENGTH_NAME];
};
typedef std::list<SpecialCrowd_t> LIST_SPECIALCROWD;

//报警状态
struct AlarmStatus_t
{
	DWORD dwDeviceID;
	DWORD dwRoomID;
	DWORD dwType;
	DWORD dwSubType;
	DWORD dwTimeStamp;
	BYTE		szTimeStamp[LENGTH_TIMESTAMP+1];
};
typedef std::list<AlarmStatus_t>	LIST_ALARMSTATUS;

//广告结构体
struct AdvertInfo_t
{
	DWORD dwAdID; //广告ID
	DWORD dwApplyType; // 应用级别
	DWORD dwApplyID; // 应用ID
	DWORD dwAdType; // 广告类型
	DWORD dwTimeStamp; // 时间戳
	DWORD dwSize; // 文件大小
	DWORD dwStoreID; // 存储ID
	DWORD dwStoreType; // 存储类型
	BYTE		  szFormat[LENGTH_FORMAT+1]; // 文件格式
	DWORD dwUseType; // 移动端位置
	DWORD dwUsePosition; // 用途
};
typedef std::list<AdvertInfo_t> LIST_ADVERTINFO;

//回调给设备结构体
struct AdvertInfoRep_t
{
	AdvertInfo_t tInfo;
	BYTE		szUrl[LENGTH_STOREURL+1];
};
typedef std::list<AdvertInfoRep_t> LIST_ADVERTINFO_REP;

struct DevAdvertID_t
{
	DWORD dwDeviceID;
	DWORD dwAdvertID;
};
typedef std::list<DevAdvertID_t>	LIST_DEV_ADVERT_ID;

//本地文件列表结构
struct FileInfo_t
{
	CSTRING strFileName;
	time_t		cTime;
};
typedef std::list<FileInfo_t> LIST_LOCAL_FILE;

struct Progress_t
{
	DWORD dwAdID;
	DWORD dwProgress;
	BYTE		 cFinish;
};
typedef std::list<Progress_t> LIST_PROGRESS;
#endif //_COMMON_DATASTRUCT_H_