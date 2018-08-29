#include "dbHandle.h"
#include "UtilityInterface.h"
#include "qiniuInterface.h"
#include "Log.h"
#include "dbTag.h"
#include "Protocol.h"
#include "putbuffer.h"
#include <algorithm>
#include <stdio.h>
#include <time.h>
#include "ServerAppInterface.h"


DWORD g_dwGroupID = 0;
DWORD g_dwRoomID = 0;

const int DEVICE_TYPE_ANDROID	 = 0;
const int DEVICE_TYPE_IOS		 = 1;

#define VILLA_ROOM "0000"
const char* DEFAULT_URL = "http://www.dd121.com/villa/dongdong2.html";

const BYTE UpdateDevRoomType_UserIndex = 1;
const BYTE UpdateDevRoomType_PushIndex = 2;
const BYTE UpdateDevRoomType_CardIndex = 4;
const BYTE UpdateDevRoomType_Other = 8;
const BYTE UpdateDevRoomType_Delete = 16;
const BYTE UpdateDevRoomType_PushSwitchIndex = 32;

IMPLEMENT_SINGLETON( CDataBaseHandle )
CDataBaseHandle::CDataBaseHandle()
{
	m_nError = 0;
	m_pSink = NULL;
	m_bConnectSuccess = false;
	m_dwView = 0;
}

CDataBaseHandle::~CDataBaseHandle()
{
	m_clsSrcDBCon.disconnect();
}

bool CDataBaseHandle::Connect(PUCHAR pHost, PUCHAR pDatabase, PUCHAR pUserName, PUCHAR pPassword)
{
	m_bConnectSuccess = false;
	try
	{
		m_bConnectSuccess = m_clsSrcDBCon.connect( (const char*)pDatabase, (const char*)pHost, (const char*)pUserName, (const char*)pPassword );
		if ( m_bConnectSuccess )
		{
			m_bConnectSuccess = m_clsSrcDBCon.select_db( (const char*)pDatabase );
			Query query = m_clsSrcDBCon.query();
			query.exec("SET NAMES 'utf8'");
		}
	}
	catch ( BadQuery er )
	{
		LOG_ERR(LOG_DB_SERVER, "Connection database(%s) with error: %s\n", pDatabase, er.what());
	}
	LOG_DETAIL(LOG_DB_SERVER, "Connection database(%s) successful\n", pDatabase);
	return m_bConnectSuccess;
}

void CDataBaseHandle::Keepalive()
{
//	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s m_pSink %p\n", __FUNCTION__, m_pSink);
	LIST_PIECEOFSERIALNO listInfo; Query_PieceOfSerialNO(listInfo);
}

bool CDataBaseHandle::Query_ServerInfo( PUCHAR pServerType, DWORD dwVendorID, LIST_SERVERINFO& listInfo )
{
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s\n", __FUNCTION__);

	int nCount = strlen((const char*)pServerType);
	if (nCount <= 0) return true;
	std::string strServerType("(");
	for (int i = 0; i < nCount;)
	{
		char szType[11] = {0};
		snprintf( szType, 10, "%d", pServerType[i] );
		strServerType = strServerType + T_SERVER_TYPE + "=" + szType;
		if(nCount != (++i)) strServerType += " or "; 
	}
	strServerType += ")";
	LOG_DEBUG(LOG_DB_SERVER, "%s dwVendorID %d ServerType %s\n", __FUNCTION__, dwVendorID, strServerType.c_str());
	if (strServerType.size() <= 0) return true;

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT * FROM " << TABLE_SERVER << " WHERE " << strServerType.c_str();
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	ServerInfo_t tInfo;
	int nTemp = 0;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		memset(&tInfo, 0, sizeof(ServerInfo_t));

		tInfo.dwServerID = (DWORD)row[T_SERVER_ID];
		tInfo.dwVendorID = (DWORD)row[T_SERVER_VENDORID];
		nTemp = (int)row[T_SERVER_TYPE];
		tInfo.bServerType = (BYTE)nTemp;
		if (dwVendorID && 
			(tInfo.bServerType != SERVER_TYPE_LOGIN) && 
			(tInfo.bServerType != SERVER_TYPE_NOTIFICATION) &&
			(dwVendorID != tInfo.dwVendorID))
		{
			continue;
		}

		std::string strSN = (std::string)row[T_SERVER_SN];
		int nCpyLen = strSN.size() < LENGTH_SERIALNO ? strSN.size() : LENGTH_SERIALNO;
		memcpy(tInfo.szSerialNO, strSN.c_str(), nCpyLen);

		//std::string strName = (std::string)row[T_SERVER_NAME];
		//nCpyLen = strName.size() < LENGTH_NAME ? strName.size() : LENGTH_NAME;
		//memcpy(tInfo.szName, strName.c_str(), nCpyLen);

		std::string strUserName = (std::string)row[T_SERVER_USERNAME];
		nCpyLen = strUserName.size() < LENGTH_NAME ? strUserName.size() : LENGTH_NAME;
		memcpy(tInfo.szUserName, strUserName.c_str(), nCpyLen);

		std::string strMD5Password = (std::string)row[T_SERVER_PASSWORD];
		nCpyLen = strMD5Password.size() < 2*LENGTH_PASSWORD ? strMD5Password.size() : 2*LENGTH_PASSWORD;
		BYTE szMD5Password[2*LENGTH_PASSWORD+1] = {0};
		memcpy(szMD5Password, strMD5Password.c_str(), nCpyLen);
		HexStr2Ascii((char*)tInfo.szPassword, (char*)szMD5Password, 2*LENGTH_PASSWORD);

		std::string strIP = (std::string)row[T_SERVER_IP];
		tInfo.dwIP = IpStr2Dword((char*)strIP.c_str());

		tInfo.nNetID = (UINT)row[T_SERVER_NETID];

		std::string strPosition = (std::string)row[T_SERVER_POSITION];
		nCpyLen = strPosition.size() < LENGTH_POSITION ? strPosition.size() : LENGTH_POSITION;
		memcpy(tInfo.szPosition, strPosition.c_str(), nCpyLen);

		//LOG_DEBUG(LOG_DB_SERVER, "server id:%d, type:%d, sn:%s, name:%s, username:%s, password:%s, ip:%s, netid:%d, position:%s\n",
		//	tInfo.dwServerID, tInfo.bServerType, tInfo.szSerialNO, tInfo.szName, tInfo.szUserName, strMD5Password.c_str(),
		//	strIP.c_str(), tInfo.nNetID, tInfo.szPosition);
		listInfo.push_back(tInfo);
	}
	return true;
}

bool CDataBaseHandle::Query_ServerInfo(ServerInfo_t& tInfo)
{
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT * FROM " << TABLE_SERVER << " WHERE " << T_SERVER_SN << "='" << tInfo.szSerialNO << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "Can't Find Server %s\n", tInfo.szSerialNO);
		return false;
	}

	Row row = *(res.begin());
	tInfo.dwServerID = (DWORD)row[T_SERVER_ID];
	tInfo.dwVendorID = (DWORD)row[T_SERVER_VENDORID];
	int nTemp = (int)row[T_SERVER_TYPE];
	tInfo.bServerType = (BYTE)nTemp;

	std::string strSN = (std::string)row[T_SERVER_SN];
	int nCpyLen = strSN.size() < LENGTH_SERIALNO ? strSN.size() : LENGTH_SERIALNO;
	memcpy(tInfo.szSerialNO, strSN.c_str(), nCpyLen);

	//std::string strName = (std::string)row[T_SERVER_NAME];
	//nCpyLen = strName.size() < LENGTH_NAME ? strName.size() : LENGTH_NAME;
	//memcpy(tInfo.szName, strName.c_str(), nCpyLen);

	std::string strUserName = (std::string)row[T_SERVER_USERNAME];
	nCpyLen = strUserName.size() < LENGTH_NAME ? strUserName.size() : LENGTH_NAME;
	memcpy(tInfo.szUserName, strUserName.c_str(), nCpyLen);

	std::string strMD5Password = (std::string)row[T_SERVER_PASSWORD];
	nCpyLen = strMD5Password.size() < 2*LENGTH_PASSWORD ? strMD5Password.size() : 2*LENGTH_PASSWORD;
	BYTE szMD5Password[2*LENGTH_PASSWORD+1] = {0};
	memcpy(szMD5Password, strMD5Password.c_str(), nCpyLen);
	HexStr2Ascii((char*)tInfo.szPassword, (char*)szMD5Password, 2*LENGTH_PASSWORD);

	std::string strIP = (std::string)row[T_SERVER_IP];
	tInfo.dwIP = IpStr2Dword((char*)strIP.c_str());

	tInfo.nNetID = (UINT)row[T_SERVER_NETID];

	std::string strPosition = (std::string)row[T_SERVER_POSITION];
	nCpyLen = strPosition.size() < LENGTH_POSITION ? strPosition.size() : LENGTH_POSITION;
	memcpy(tInfo.szPosition, strPosition.c_str(), nCpyLen);

	return true;
}

bool CDataBaseHandle::Query_User( PUCHAR pUser )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s User %s\n", __FUNCTION__, pUser);
	if (strlen((const char*)pUser) <= 0) return false;

	PUCHAR p = (PUCHAR)strstr((const char*)pUser, "@");
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_USER_MOBILEPHONE << " FROM " << TABLE_USER
			<< " WHERE " << T_USER_NAME << "='" << pUser << "' OR ";
	if (p) query << T_USER_EMAIL;
	else query << T_USER_MOBILEPHONE;
	query << "='" << pUser << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0) { 	LOG_DEBUG(LOG_DB_SERVER, "Can't Find User %s\n", pUser); return false; }
	LOG_DEBUG(LOG_DB_SERVER, "User %s is Exist\n", pUser);
	return true;
}

// 字段T_USER_NAME && T_USER_MOBILEPHONE是唯一
bool CDataBaseHandle::Insert_User( DWORD dwVendorID, BYTE bLanguage, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pUser )
{
//	LOG_DEBUG(LOG_DB_SERVER, "%s dwVendorID %d bLanguage %d pUserName %s pMobilePhone %s\n", __FUNCTION__, dwVendorID, bLanguage, pUserName, pMobilePhone);
	dwVendorID = CalcRealVendorID(dwVendorID);
	int nUserNameLen = strlen((const char*)pUserName);
	int nMobilePhoneLen = strlen((const char*)pUser);
	if ( (nUserNameLen <= 0) && (nMobilePhoneLen <= 0) ) return true;

	BYTE szPwd[2*LENGTH_CHALLENGE+1] = {0};
	Ascii2HexStr((char*)szPwd, (char*)pPassword, LENGTH_CHALLENGE);
	if (strlen((const char*)szPwd) <= 0) return true;

	PUCHAR p = (PUCHAR)strstr((const char*)pUser, "@");

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_USER_ID << " FROM " << TABLE_USER << " WHERE ";
	if (p) query << T_USER_EMAIL;
	else query << T_USER_MOBILEPHONE;
	query << "='" << pUser << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	int nLanguage = (int)bLanguage;
	if (res.size() <= 0)
	{
		query << "INSERT INTO " << TABLE_USER
			   << " (" << T_USER_VENDORID << "," << T_USER_LANGUAGE << "," << T_USER_PASSWORD << ",";
		if (p) query << T_USER_EMAIL;
		else query << T_USER_MOBILEPHONE;
		query << ") VALUES ("
			   << dwVendorID << "," << nLanguage << ",'" << szPwd << "','" << pUser << "')";
		if (-1 == CatchException(query)) return false;
		LOG_DEBUG(LOG_DB_SERVER, "Insert User:VendorID %d Language %d Name %s MobilePhone %s\n", dwVendorID, nLanguage, pUserName, pUser);
	}
	else
	{
		query << "UPDATE " << TABLE_USER
			  << " SET " << T_USER_VENDORID << "=" << dwVendorID << " , "
						 << T_USER_LANGUAGE << "=" << nLanguage << " , "
						 << T_USER_PASSWORD << "='" << szPwd << "'"
			  << " WHERE ";
		if (p) query << T_USER_EMAIL;
		else query << T_USER_MOBILEPHONE;
		query << "='" << pUser << "'";
		if (-1 == CatchException(query)) return false;
		LOG_DEBUG(LOG_DB_SERVER, "Update User:VendorID %d Language %d Name %s MobilePhone %s\n", dwVendorID, nLanguage, pUserName, pUser);
	}
	return true;
}

bool CDataBaseHandle::Query_UserInfo(PUCHAR pUserName, DWORD dwAppVendorID, UserInfo_t& tInfo, ClientTokenArray_t& tArray)
{
//	LOG_DEBUG(LOG_DB_SERVER, "%s pUserName %s dwVendorID %d\n", __FUNCTION__, pUserName, dwVendorID);
	if (strlen((const char*)pUserName) <= 0) return true;

	PUCHAR p = (PUCHAR)strstr((const char*)pUserName, "@");
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_USER_ID << "," << T_USER_CONFIGUREINDEX << "," << T_USER_PASSWORD << ",";
	if (p) query << T_USER_EMAIL;
	else query << T_USER_MOBILEPHONE;
	query << " FROM " << TABLE_USER << " WHERE ";
	if (p) query << T_USER_EMAIL;
	else query << T_USER_MOBILEPHONE;
	query << "='" << pUserName << "' OR " << T_USER_NAME << "='" << pUserName << "'";

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0) return false;
	Row row = *(res.begin());

	memset(&tInfo, 0, sizeof(UserInfo_t));
	tInfo.dwUserID = (DWORD)row[T_USER_ID];
	tInfo.dwConfigureIndex = (DWORD)row[T_USER_CONFIGUREINDEX];

	int nUserNameLen = strlen((const char*)pUserName);
	int nCpyLen = nUserNameLen < LENGTH_NAME ? nUserNameLen : LENGTH_NAME;
	memcpy(tInfo.szUserName, pUserName, nCpyLen);

	std::string strMD5Password = (std::string)row[T_USER_PASSWORD];
	nCpyLen = strMD5Password.size() < 2*LENGTH_PASSWORD ? strMD5Password.size() : 2*LENGTH_PASSWORD;
	BYTE szMD5Password[2*LENGTH_PASSWORD+1] = {0};
	memcpy(szMD5Password, strMD5Password.c_str(), nCpyLen);
	HexStr2Ascii((char*)tInfo.szPassword, (char*)szMD5Password, 2*LENGTH_PASSWORD);

	std::string strMobilePhone;
	if (p) strMobilePhone = (std::string)row[T_USER_EMAIL];
	else strMobilePhone = (std::string)row[T_USER_MOBILEPHONE];
	nCpyLen = strMobilePhone.size() < LENGTH_MOBILEPHONE ? strMobilePhone.size() : LENGTH_MOBILEPHONE;
	memcpy(tInfo.szMobilePhone, strMobilePhone.c_str(), nCpyLen);

	// 查询url
	query << "SELECT " << T_VENDORURL_URL << " FROM " << TABLE_VENDORURL << " WHERE " << T_VENDORURL_VENDORID << "=" << dwAppVendorID;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		int nUrlLen = strlen(DEFAULT_URL);
		nCpyLen = nUrlLen < LENGTH_URL ? nUrlLen : LENGTH_URL;
		memcpy(tInfo.szUrl, DEFAULT_URL, nCpyLen);
//		LOG_DEBUG(LOG_DB_SERVER, "%s pUserName %s VendorID:%d Default Url:%s\n", __FUNCTION__, pUserName, dwAppVendorID, tInfo.szUrl);
	}
	else
	{
		row = *(res.begin());
		std::string strUrl = (std::string)row[T_VENDORURL_URL];
		nCpyLen = strUrl.size() < LENGTH_URL ? strUrl.size() : LENGTH_URL;
		memcpy(tInfo.szUrl, strUrl.c_str(), nCpyLen);
//		LOG_DEBUG(LOG_DB_SERVER, "%s pUserName %s VendorID:%d Url:%s\n", __FUNCTION__, pUserName, dwAppVendorID, tInfo.szUrl);
	}

	//////////////////////////////////////////////////////////////
	query << "SELECT " << T_PUSHINFO_OS << "," << T_PUSHINFO_GT_TOKEN << "," << T_PUSHINFO_BAIDU_TOKEN << "," 
		<< T_PUSHINFO_HW_TOKEN << "," << T_PUSHINFO_MI_TOKEN << "," << T_PUSHINFO_JG_TOKEN << "," << T_PUSHINFO_MZ_TOKEN
		<< " FROM " << TABLE_PUSHINFO
		<< " WHERE " << T_PUSHINFO_USERID << "=" << tInfo.dwUserID;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0) return false;
	row = *(res.begin());

	memset(&tArray, 0, sizeof(ClientTokenArray_t));
	tArray.nCount = 2;//?
	int nPushType = (int)row[T_PUSHINFO_OS];
	tArray.nPushSwitch = 0;
	//个推
	std::string strToken = (std::string)row[T_PUSHINFO_GT_TOKEN];
	nCpyLen = strToken.size() < LENGTH_TOKEN ? strToken.size() : LENGTH_TOKEN;
	if (nCpyLen > 0)
	{
		if (nPushType == DEVICE_TYPE_ANDROID) tArray.tToken[0].bPushType = PUSH_TYPE_ANDROID_GT;
		else if (nPushType == DEVICE_TYPE_IOS) tArray.tToken[0].bPushType = PUSH_TYPE_IOS_GT;
		memcpy(tArray.tToken[0].szToken, strToken.c_str(), nCpyLen);
	}
	//百度
	strToken = (std::string)row[T_PUSHINFO_BAIDU_TOKEN];
	nCpyLen = strToken.size() < LENGTH_TOKEN ? strToken.size() : LENGTH_TOKEN;
	if (nCpyLen > 0)
	{
		if (nPushType == DEVICE_TYPE_ANDROID) tArray.tToken[1].bPushType = PUSH_TYPE_ANDROID_BAIDU;
		else if (nPushType == DEVICE_TYPE_IOS) tArray.tToken[1].bPushType = PUSH_TYPE_IOS_BAIDU;
		memcpy(tArray.tToken[1].szToken, strToken.c_str(), nCpyLen);
	}
	//华为
	strToken = (std::string)row[T_PUSHINFO_HW_TOKEN];
	nCpyLen = strToken.size() < LENGTH_TOKEN ? strToken.size() : LENGTH_TOKEN;
	if (nCpyLen > 0)
	{
		if (nPushType == DEVICE_TYPE_ANDROID){ 
			tArray.tToken[2].bPushType = PUSH_TYPE_ANDROID_HW;
			memcpy(tArray.tToken[2].szToken, strToken.c_str(), nCpyLen);
		}
	}
	//小米
	strToken = (std::string)row[T_PUSHINFO_MI_TOKEN];
	nCpyLen = strToken.size() < LENGTH_TOKEN ? strToken.size() : LENGTH_TOKEN;
	if (nCpyLen > 0)
	{
		if (nPushType == DEVICE_TYPE_ANDROID){ 
			tArray.tToken[3].bPushType = PUSH_TYPE_ANDROID_MI;
			memcpy(tArray.tToken[3].szToken, strToken.c_str(), nCpyLen);
		}
	}
	//极光
	strToken = (std::string)row[T_PUSHINFO_JG_TOKEN];
	nCpyLen = strToken.size() < LENGTH_TOKEN ? strToken.size() : LENGTH_TOKEN;
	if (nCpyLen > 0)
	{
		if (nPushType == DEVICE_TYPE_ANDROID) tArray.tToken[4].bPushType = PUSH_TYPE_ANDROID_JG;
		else if (nPushType == DEVICE_TYPE_IOS) tArray.tToken[4].bPushType = PUSH_TYPE_IOS_JG;
		memcpy(tArray.tToken[4].szToken, strToken.c_str(), nCpyLen);
	}
	//魅族
	strToken = (std::string)row[T_PUSHINFO_MZ_TOKEN];
	nCpyLen = strToken.size() < LENGTH_TOKEN ? strToken.size() : LENGTH_TOKEN;
	if (nCpyLen > 0)
	{
		if (nPushType == DEVICE_TYPE_ANDROID){
			tArray.tToken[5].bPushType = PUSH_TYPE_ANDROID_MZ;
			memcpy(tArray.tToken[5].szToken, strToken.c_str(), nCpyLen);
		}
	}
	return true;
}

//SELECT `device`.`deviceid` , `device`.`devicesn` ,`userdevice` .`groupid` 
//FROM `device` , `userdevice` 
//WHERE `userdevice`.`userid` =1
//AND `userdevice`.`deviceid` = `device`.`deviceid` 
bool CDataBaseHandle::Query_UserDevice( DWORD dwUserID, LIST_DEVICEINFO& listInfo )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwUserID %d\n", __FUNCTION__, dwUserID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT DISTINCT " << TABLE_DEVICE << "." << T_DEVICE_ID << "," 
						<< TABLE_DEVICE << "." << T_DEVICE_OWNERID << "," 
						<< TABLE_DEVICE << "." << T_DEVICE_VENDORID << ","
						<< TABLE_DEVICE << "." << T_DEVICE_SN << ","
						<< TABLE_DEVICE << "." << T_DEVICE_GROUPID << ","
						<< TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICENAME
		   << " FROM "  << TABLE_DEVICE << "," << TABLE_USERDEVICE
		   << " WHERE " << TABLE_USERDEVICE << "." << T_USERDEVICE_USERID << "=" << dwUserID << " AND "
						<< TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID << "=" << TABLE_DEVICE << "." << T_DEVICE_ID;	
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	DeviceInfo_t tInfo;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		memset(&tInfo, 0, sizeof(DeviceInfo_t));

		tInfo.dwDeviceID = (DWORD)row[T_DEVICE_ID];
		DWORD dwOwnerID = (DWORD)row[T_DEVICE_OWNERID];
		tInfo.dwVendorID = (DWORD)row[T_DEVICE_VENDORID];
		tInfo.dwGroupID = (DWORD)row[T_DEVICE_GROUPID];
	
		std::string strSN = (std::string)row[T_DEVICE_SN];
		int nCpyLen = strSN.size() < LENGTH_SERIALNO ? strSN.size() : LENGTH_SERIALNO;
		memcpy(tInfo.szSerialNO, strSN.c_str(), nCpyLen);

		std::string strName = (std::string)row[T_USERDEVICE_DEVICENAME];
		nCpyLen = strName.size() < LENGTH_NAME ? strName.size() : LENGTH_NAME;
		memcpy(tInfo.szDeviceName, strName.c_str(), nCpyLen);

		//////////////////////////////////////////////////////////////////////////
		// 密码字段第一个字节表示是否为授权设备 1-授权 0-主
		if (dwUserID != dwOwnerID) tInfo.szPassword[0] = 1;
		//////////////////////////////////////////////////////////////////////////

		LOG_DEBUG(LOG_DB_SERVER, "userid:%d device id:%d, ownerid:%d groupid:%d, sn:%s, name:%s\n",
			dwUserID, tInfo.dwDeviceID, dwOwnerID, tInfo.dwGroupID, tInfo.szSerialNO, tInfo.szDeviceName);
		listInfo.push_back(tInfo);
	}
	return true;
}

//SELECT DISTINCT `group`.`groupname` FROM `group`,`device` WHERE `device`.`deviceid` IN( SELECT `userdevice`.`deviceid` FROM `userdevice` WHERE `userdevice`.`userid`=1)  AND `device`.`groupid`=`group`.`groupid`
// 0.0009

//SELECT `groupid` , `groupname` , `parentid` , `sequence` 
//FROM `groups` 
//WHERE `userid`=1
bool CDataBaseHandle::Query_UserGroup( DWORD dwUserID, LIST_GROUPINFO& listInfo )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwUserID %d\n", __FUNCTION__, dwUserID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT DISTINCT " << TABLE_GROUP << "." << T_GROUP_ID << "," 
								 << TABLE_GROUP << "." << T_GROUP_PARENTID << ","
								 << TABLE_GROUP << "." << T_GROUP_SEQUENCE << ","
								 << TABLE_GROUP << "." << T_GROUP_NAME
		   << " FROM "  << TABLE_GROUP << "," << TABLE_DEVICE << "," << TABLE_USERDEVICE
		   << " WHERE " << TABLE_USERDEVICE << "." << T_USERDEVICE_USERID << "=" << dwUserID
		   << " AND "   << TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID << "=" << TABLE_DEVICE << "." << T_DEVICE_ID
		   << " AND "   << TABLE_DEVICE << "." << T_DEVICE_GROUPID << "=" << TABLE_GROUP << "." << T_GROUP_ID;
/* 20170403 modify by steven: 下面的查询方式经测试需要耗时2秒以上，改为上面的方式查询，经测试耗时仅为0.01秒以上
		   << " FROM "  << TABLE_GROUP << "," << TABLE_DEVICE
		   << " WHERE (" << TABLE_DEVICE << "." << T_DEVICE_ID
		   << " IN (" << " SELECT " << T_USERDEVICE_DEVICEID
					  << " FROM " << TABLE_USERDEVICE
					  << " WHERE " << T_USERDEVICE_USERID << "=" << dwUserID << ")"
		   << " AND " << TABLE_GROUP << "." << T_GROUP_ID << "=" << TABLE_DEVICE << "." << T_DEVICE_GROUPID
		   << ") OR " << TABLE_GROUP << "." T_GROUP_USERID << "=" << dwUserID;
*/
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	LIST_GROUPINFO::iterator posList;
	GroupInfo_t tInfo;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		memset(&tInfo, 0, sizeof(GroupInfo_t));

		tInfo.dwGroupID = (DWORD)row[T_GROUP_ID];
		tInfo.dwParentID = (DWORD)row[T_GROUP_PARENTID];

		g_dwGroupID = tInfo.dwGroupID;
		posList = std::find_if( listInfo.begin(), listInfo.end(), FindGroupInfoByID() );
		if (posList != listInfo.end()) continue;

		tInfo.dwSequence = (DWORD)row[T_GROUP_SEQUENCE];
		std::string strName = (std::string)row[T_GROUP_NAME];
		int nCpyLen = strName.size() < LENGTH_NAME ? strName.size() : LENGTH_NAME;
		memcpy(tInfo.szGroupName, strName.c_str(), nCpyLen);

		LOG_DEBUG(LOG_DB_SERVER, "userid:%d group id:%d, parent id:%d sequence:%d, name:%s\n",
			dwUserID, tInfo.dwGroupID, tInfo.dwParentID, tInfo.dwSequence, tInfo.szGroupName);

		listInfo.push_back(tInfo);
	}
	return true;
}
bool CDataBaseHandle::Query_UserRoom(DWORD dwUserID, LIST_ROOMINFO& listInfo)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwUserID %d\n", __FUNCTION__, dwUserID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT DISTINCT " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID << "," 
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_DEVICEID << ","
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOM << ","
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_PASSWORD
		<< " FROM "  << TABLE_DEVICEROOM << "," << TABLE_USERDEVICE
		<< " WHERE " << TABLE_USERDEVICE << "." << T_USERDEVICE_USERID << "=" << dwUserID
		<< " AND " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID << "=" << TABLE_USERDEVICE << "." << T_USERDEVICE_ROOMID;

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	LIST_ROOMINFO::iterator posList;
	RoomInfo_t tInfo;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		memset(&tInfo, 0, sizeof(RoomInfo_t));

		tInfo.dwDeviceID = (DWORD)row[T_DEVICEROOM_DEVICEID];
		tInfo.dwRoomID = (DWORD)row[T_DEVICEROOM_ROOMID];
		
		std::string strRoom = (std::string)row[T_DEVICEROOM_ROOM];
		int nCpyLen = strRoom.size() < LENGTH_ROOM ? strRoom.size() : LENGTH_ROOM;
		memcpy(tInfo.szRoom, strRoom.c_str(), nCpyLen);

		std::string strMD5Password = (std::string)row[T_DEVICEROOM_PASSWORD];
//		LOG_DEBUG(LOG_DB_SERVER,"SelectPwdMark=1=>%s\n",strMD5Password.c_str());
		SelectPwdMark(strMD5Password);
//		LOG_DEBUG(LOG_DB_SERVER,"SelectPwdMark=1=>%s\n",strMD5Password.c_str());
		nCpyLen = strMD5Password.size() < 2*LENGTH_PASSWORD ? strMD5Password.size() : 2*LENGTH_PASSWORD;
		BYTE szMD5Password[2*LENGTH_PASSWORD+1] = {0};
		memcpy(szMD5Password, strMD5Password.c_str(), nCpyLen);
		HexStr2Ascii((char*)tInfo.szPassword, (char*)szMD5Password, 2*LENGTH_PASSWORD);

		LOG_DEBUG(LOG_DB_SERVER, "%s roomid %d deviceid %d room %s\n", __FUNCTION__, tInfo.dwRoomID, tInfo.dwDeviceID, tInfo.szRoom);
		listInfo.push_back(tInfo);
	}
	return true;
}
/*
//室内机-查询用户所有设备和房号信息
bool CDataBaseHandle::Query_UserDeviceRoomInfo(DWORD dwUserID,MAP_DEVROOMINFO& mapDevRoomInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s\n",__FUNCTION__);
	//根据userid在userdevice表中查询deviceid列表
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
//	query << "SELECT " << T_USERDEVICE_DEVICEID << " FROM " << TABLE_USERDEVICE << " WHERE " << T_USERDEVICE_USERID << "=" << dwUserID;
	query << "SELECT DISTINCT " << TABLE_DEVICE << "." << T_DEVICE_ID 
		<< " FROM "  << TABLE_DEVICE << "," << TABLE_USERDEVICE
		<< " WHERE " << TABLE_USERDEVICE << "." << T_USERDEVICE_USERID << "=" << dwUserID << " AND "
		<< TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID << "=" << TABLE_DEVICE << "." << T_DEVICE_ID;	
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	StoreQueryResult::iterator iter = res.begin();
	for(; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwDeviceID = (DWORD)row[T_DEVICE_ID];
		LOG_DEBUG(LOG_DB_SERVER,"===============>%d\n",dwDeviceID);
		LIST_ROOMINFO2	lstRoomInfo;
		query << "SELECT " << T_USERDEVICE_ROOMID << " FROM " << TABLE_USERDEVICE
			<< " WHERE " << T_USERDEVICE_USERID << "=" << dwUserID << " AND " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID;
		StoreQueryResult res2;
		if (-1 == CatchException(query, res2)) return false;
		StoreQueryResult::iterator iter2 = res2.begin();
		for(; iter2 != res2.end(); iter2++)
		{
			Row row2 = *iter2;
			RoomInfo_t2 tInfo2;
			memset(&tInfo2,0,sizeof(RoomInfo_t2));
			DWORD dwRoomID = (DWORD)row2[T_USERDEVICE_ROOMID];
			LOG_DEBUG(LOG_DB_SERVER,"==========>%d\n",dwRoomID);
			query << "SELECT " << T_DEVICEROOM_ROOM << " FROM " << TABLE_DEVICEROOM
			<< " WHERE " << T_DEVICEROOM_ROOMID << "=" << dwRoomID;
			StoreQueryResult res3;
			if (-1 == CatchException(query, res3)) return false;
			StoreQueryResult::iterator iter3 = res3.begin();
			for(; iter3 != res3.end(); iter3++){
				Row row3 = *iter3;
				std::string strRoom = (std::string)row3[T_DEVICEROOM_ROOM];
				int nCpyLen = strRoom.size() < LENGTH_USERROOM ? strRoom.size() : LENGTH_USERROOM;
				memcpy(tInfo2.szRoom, strRoom.c_str(), nCpyLen);
				LOG_DEBUG(LOG_DB_SERVER,"=====>%s\n",strRoom.c_str());
				tInfo2.dwRoomID = dwRoomID;
				lstRoomInfo.push_back(tInfo2);
				LOG_DEBUG(LOG_DB_SERVER,"deviceid:%d --> RoomID:%d - Room:%s\n",dwDeviceID, dwRoomID, strRoom.c_str());
			}
		}
		mapDevRoomInfo.insert(std::make_pair(dwDeviceID,lstRoomInfo));		
	}
	return true;
}
*/
bool CDataBaseHandle::Query_UserDeviceRoomInfo(DWORD dwUserID, LIST_DEVICEINFO& lstDevInfo, MAP_DEVROOMINFO& mapDevRoomInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s\n",__FUNCTION__);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	LIST_DEVICEINFO::iterator iter = lstDevInfo.begin();
	for(; iter != lstDevInfo.end(); iter++)
	{
		DWORD dwDeviceID = iter->dwDeviceID;
		LIST_ROOMINFO2	lstRoomInfo;
		LOG_DEBUG(LOG_DB_SERVER,"DeviceID=%d\n",dwDeviceID);

		query << "SELECT " << T_USERDEVICE_ROOMID << " FROM " << TABLE_USERDEVICE
			<< " WHERE " << T_DEVICE_ID << "=" << dwDeviceID << " AND " << T_USERDEVICE_USERID << "=" << dwUserID ;
		StoreQueryResult res;
		if (-1 == CatchException(query, res)) return false;
		LOG_DEBUG(LOG_DB_SERVER,"UserID=%d\n",dwUserID);

		StoreQueryResult::iterator it = res.begin();
		for(; it != res.end(); it++)
		{
			Row row = *it;
			RoomInfo_t2 tInfo;
			DWORD	dwRoomID = (DWORD)row[T_DEVICEROOM_ROOMID];
			query << "SELECT " << T_DEVICEROOM_ROOM << " FROM " << TABLE_DEVICEROOM
				<< " WHERE " << T_DEVICEROOM_ROOMID << "=" << dwRoomID;
			StoreQueryResult res2;
			if (-1 == CatchException(query, res2)) return false;
			LOG_DEBUG(LOG_DB_SERVER,"dwRoomID=%d\n",dwRoomID);

			if(res2.size() == 0){
				memcpy(tInfo.szRoom, "NULL", LENGTH_USERROOM);
				lstRoomInfo.push_back(tInfo);
				LOG_DEBUG(LOG_DB_SERVER,"lstRoomCout=%d\n",lstRoomInfo.size());
				continue;
			}
			StoreQueryResult::iterator it2 = res2.begin();
			Row row2 = *it2;
			std::string strRoom = (std::string)row2[T_DEVICEROOM_ROOM];
			int nCpyLen = strRoom.size() < LENGTH_USERROOM ? strRoom.size() : LENGTH_USERROOM;
			memcpy(tInfo.szRoom, strRoom.c_str(), nCpyLen);
			tInfo.dwRoomID = dwRoomID;
			LOG_DEBUG(LOG_DB_SERVER,"Room=%s\n",strRoom.c_str());

			lstRoomInfo.push_back(tInfo);
			LOG_DEBUG(LOG_DB_SERVER,"lstRoomCout=%d\n",lstRoomInfo.size());

		}
		mapDevRoomInfo.insert(std::make_pair(dwDeviceID,lstRoomInfo));
		LOG_DEBUG(LOG_DB_SERVER,"mapCount=%d\n",mapDevRoomInfo.size());

	}
//	LOG_DEBUG(LOG_DB_SERVER,"test 11\n");
	return true;
}



//5
bool CDataBaseHandle::Query_DeviceInfo( PUCHAR pSN, DeviceInfo_t& tInfo )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s pSN %s\n", __FUNCTION__, pSN);
	if (strlen((const char*)pSN) <= 0) return false;

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICE_ID << ","
						<< T_DEVICE_VENDORID << "," 
						<< T_DEVICE_GROUPID << ","
						<< T_DEVICE_NAME << ","
						<< T_DEVICE_OWNERID << ","
						<< T_DEVICE_AUTORELAY
		   << " FROM " << TABLE_DEVICE
		   << " WHERE " << T_DEVICE_SN << "='" << pSN << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0) return false;
	Row row = *(res.begin());

	memset(&tInfo, 0, sizeof(DeviceInfo_t));

	tInfo.dwDeviceID = (DWORD)row[T_DEVICE_ID];
	tInfo.dwVendorID = (DWORD)row[T_DEVICE_VENDORID];
	tInfo.dwGroupID = (DWORD)row[T_DEVICE_GROUPID];
	int nAutoRelay = (int)row[T_DEVICE_AUTORELAY];
	tInfo.dwAutoRelay = (DWORD)nAutoRelay;
	DWORD dwOwnerID = (DWORD)row[T_DEVICE_OWNERID];

	std::string strDeviceName = (std::string)row[T_DEVICE_NAME];
	int nCpyLen = strDeviceName.size() < LENGTH_NAME ? strDeviceName.size() : LENGTH_NAME;
	memcpy(tInfo.szDeviceName, strDeviceName.c_str(), nCpyLen);

	UINT tFlag = GetDeviceTID((char*)pSN);
	UINT callFlag = PhoneCall(tFlag);

	if(1 == callFlag)
	{
		//获取设备使用截至日期==========================================================================================//
		char DeadLine[9] = {0};
		query << "SELECT " << T_DEADLINE << " FROM " << T_DEVICE << " WHERE " << T_DEVICEID << "=" << tInfo.dwDeviceID << " ORDER BY " << T_CALLDATE << " DESC LIMIT 1";
		StoreQueryResult res2;
		if (-1 != CatchException(query,res2))
		{
			if (res2.size() >  0)
			{
				Row row2 = *(res2.begin());
				struct tm tm;
				time_t tick = (time_t)row2[T_DEADLINE];
				tm = *localtime(&tick);
				strftime(DeadLine, sizeof(DeadLine), "%Y%m%d", &tm);
				memcpy(tInfo.szDeadLine,DeadLine,LENGTH_DEADLINE);
			}
		}
	}
	//========================================================================================================//
	//std::string strMD5Password = (std::string)row[T_DEVICE_PASSWORD];
	//nCpyLen = strMD5Password.size() < 2*LENGTH_PASSWORD ? strMD5Password.size() : 2*LENGTH_PASSWORD;
	//BYTE szMD5Password[2*LENGTH_PASSWORD+1] = {0};
	//memcpy(szMD5Password, strMD5Password.c_str(), nCpyLen);
	//HexStr2Ascii((char*)tInfo.szPassword, (char*)szMD5Password, 2*LENGTH_PASSWORD);

	int nSNLen = strlen((const char*)pSN);
	nCpyLen = nSNLen < LENGTH_SERIALNO ? nSNLen : LENGTH_SERIALNO;
	memcpy(tInfo.szSerialNO, pSN, nCpyLen);

	RoomSum_t tRoomSum; memset(&tRoomSum, 0, sizeof(RoomSum_t));
	Query_DeviceMainRoomSum(tInfo.dwDeviceID, tRoomSum);
	tInfo.dwConfigureIndex = tRoomSum.dwUserIndex;
	tInfo.dwConfigureIndex2 = tRoomSum.dwPushIndex;

	tInfo.dwStoreID = QueryStoreIDByUserID(dwOwnerID);

	LOG_DEBUG(LOG_DB_SERVER, "%s SN %s DeviceID %d VendorID %d AutoRelay %d DeviceName %s StoreID %d\n",
		__FUNCTION__, pSN, tInfo.dwDeviceID, tInfo.dwVendorID, tInfo.dwAutoRelay, tInfo.szDeviceName, tInfo.dwStoreID);
	return true;
}

void CDataBaseHandle::GetVaildUserName( std::string& strMobilePhone, std::string& strEmail, std::string& strUserName, PUCHAR pUser )
{
	int nMobilePhoneLen = strMobilePhone.size();
	int nEmailLen = strEmail.size();
	int nUserNameLen = strUserName.size();
	int nCpyLen = 0;
	if (nMobilePhoneLen)
	{
		// 去除estate[]
		int pos = strMobilePhone.find("estate");
		if (std::string::npos != pos)
		{
			strMobilePhone.erase(pos, 7);
			strMobilePhone = strMobilePhone.substr(0, strMobilePhone.length()-1);
		}
		nCpyLen = nMobilePhoneLen < LENGTH_MOBILEPHONE ? nMobilePhoneLen : LENGTH_MOBILEPHONE;
		memcpy(pUser, strMobilePhone.c_str(), nCpyLen);
	}
	else if (nEmailLen)
	{
		// 去除estate[]
		int pos = strEmail.find("estate");
		if (std::string::npos != pos)
		{
			strEmail.erase(pos, 7);
			strEmail = strEmail.substr(0, strEmail.length()-1);
		}
		nCpyLen = nEmailLen < LENGTH_MOBILEPHONE ? nEmailLen : LENGTH_MOBILEPHONE;
		memcpy(pUser, strEmail.c_str(), nCpyLen);
	}
	else if (nUserNameLen)
	{
		nCpyLen = nUserNameLen < LENGTH_MOBILEPHONE ? nUserNameLen : LENGTH_MOBILEPHONE;
		memcpy(pUser, strUserName.c_str(), nCpyLen);
	}
}

//SELECT `configureindex`
//FROM `device`
//WHERE `deviceid`=2

//SELECT `user`.`userid`,`user`.`mobilephone`
//FROM `userdevice`,`user`
//WHERE `userdevice`.`deviceid`=2 and `userdevice`.`userid`=`user`.`userid`
bool CDataBaseHandle::Query_DeviceUser( DWORD dwDeviceID, DWORD& dwConfigureIndex, LIST_SMSINFO& listInfo )
{
	LIST_DWORD lstRoomID;
	LIST_ROOMUSER lstRoomUser;
	if (false == Query_DeviceRoomUser(dwDeviceID, lstRoomID, lstRoomUser)) return false;

	LIST_ROOMUSER::iterator iter = lstRoomUser.begin();
	for (; iter != lstRoomUser.end(); iter++)
	{
		// 只取第一个
		dwConfigureIndex = iter->dwUserIndex;
		listInfo.insert(listInfo.end(), iter->lstUserInfo.begin(), iter->lstUserInfo.end());
		break;
	}
	return true;
}

bool CDataBaseHandle::Query_DevicePush( DWORD dwDeviceID, DWORD& dwConfigureIndex2, LIST_PUSHINFO& listInfo )
{
	LIST_DWORD lstRoomID;
	LIST_ROOMPUSH lstRoomPush;
	if (false == Query_DeviceRoomPush(dwDeviceID, lstRoomID, lstRoomPush)) return false;
	
	LIST_ROOMPUSH::iterator iter = lstRoomPush.begin();
	for (; iter != lstRoomPush.end(); iter++)
	{
		// 只取第一个
		dwConfigureIndex2 = iter->dwPushIndex;
		listInfo.insert(listInfo.end(), iter->lstPushInfo.begin(), iter->lstPushInfo.end());
		break;
	}
	return true;
}

bool CDataBaseHandle::Query_DeviceRoomSum(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMSUM& lstRoomSum)
{
	bool bGetAll = (lstRoomID.size() == 0);
	if (bGetAll) return true;

	std::string strRoomID; MkRoomIDStr(lstRoomID, strRoomID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICEROOM_ROOMID << ","
		<< T_DEVICEROOM_ROOM << ","
		<< T_DEVICEROOM_PASSWORD << ","
		<< T_DEVICEROOM_USERINDEX << ","
		<< T_DEVICEROOM_CARDINDEX << ","
// 		<< T_DEVICEROOM_PUSHSWITCHINDEX << ","
		<< T_DEVICEROOM_INDOORINDEX << ","
		<< T_DEVICEROOM_PUSHINDEX
		<< " FROM " << TABLE_DEVICEROOM
		<< " WHERE " << T_DEVICEROOM_DEVICEID << "=" << dwDeviceID << " AND " << strRoomID.c_str();
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		RoomSum_t tRoomSum; memset(&tRoomSum, 0, sizeof(RoomSum_t));

		tRoomSum.dwRoomID    = (DWORD)row[T_DEVICEROOM_ROOMID];
		tRoomSum.dwPushIndex = (DWORD)row[T_DEVICEROOM_PUSHINDEX];
		tRoomSum.dwUserIndex = (DWORD)row[T_DEVICEROOM_USERINDEX];
		tRoomSum.dwCardIndex = (DWORD)row[T_DEVICEROOM_CARDINDEX];
		tRoomSum.dwIndoorIndex = (DWORD)row[T_DEVICEROOM_INDOORINDEX];
		tRoomSum.dwPushSwitchIndex = 1; // (DWORD)row[T_DEVICEROOM_PUSHSWITCHINDEX];
		std::string strRoom = (std::string)row[T_DEVICEROOM_ROOM];
		int nCpyLen = strRoom.size() < LENGTH_ROOM ? strRoom.size() : LENGTH_ROOM;
		memcpy(tRoomSum.szRoom, strRoom.c_str(), nCpyLen);

		std::string strPwd = (std::string)row[T_DEVICEROOM_PASSWORD];
//		LOG_DEBUG(LOG_DB_SERVER,"SelectPwdMark=2=>%s\n",strPwd.c_str());
		SelectPwdMark(strPwd);
//		LOG_DEBUG(LOG_DB_SERVER,"SelectPwdMark=2=>%s\n",strPwd.c_str());
		nCpyLen = strPwd.size() < LENGTH_ROOMPWD ? strPwd.size() : LENGTH_ROOMPWD;
		memcpy(tRoomSum.szPassword, strPwd.c_str(), nCpyLen);

// 		LOG_DEBUG(LOG_DB_SERVER, "RoomID %d UserIndex %d PushIndex %d CardIndex %d Room %s Pwd %s\n",
// 			tRoomSum.dwRoomID, tRoomSum.dwUserIndex, tRoomSum.dwPushIndex, tRoomSum.dwCardIndex, tRoomSum.szRoom, tRoomSum.szPassword);

		lstRoomSum.push_back(tRoomSum);
	}
	return true;
}

bool CDataBaseHandle::Query_DeviceRoomSum(DWORD dwDeviceID, LIST_ROOMSUM& listInfo)
{
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICEROOM_ROOMID << ","
		<< T_DEVICEROOM_ROOM << ","
		<< T_DEVICEROOM_PASSWORD << ","
		<< T_DEVICEROOM_USERINDEX << ","
		<< T_DEVICEROOM_CARDINDEX << ","
		<< T_DEVICEROOM_PUSHINDEX
		<< " FROM " << TABLE_DEVICEROOM
		<< " WHERE " << T_DEVICEROOM_DEVICEID << "=" << dwDeviceID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		RoomSum_t tRoomSum; memset(&tRoomSum, 0, sizeof(RoomSum_t));

		tRoomSum.dwRoomID = (DWORD)row[T_DEVICEROOM_ROOMID];
		tRoomSum.dwPushIndex = (DWORD)row[T_DEVICEROOM_PUSHINDEX];
		tRoomSum.dwUserIndex = (DWORD)row[T_DEVICEROOM_USERINDEX];
		tRoomSum.dwCardIndex = (DWORD)row[T_DEVICEROOM_CARDINDEX];
		std::string strRoom = (std::string)row[T_DEVICEROOM_ROOM];
		int nCpyLen = strRoom.size() < LENGTH_ROOM ? strRoom.size() : LENGTH_ROOM;
		memcpy(tRoomSum.szRoom, strRoom.c_str(), nCpyLen);
		
		std::string strPwd = (std::string)row[T_DEVICEROOM_PASSWORD];
//		LOG_DEBUG(LOG_DB_SERVER,"SelectPwdMark=3=>%s\n",strPwd.c_str());
		SelectPwdMark(strPwd);
//		LOG_DEBUG(LOG_DB_SERVER,"SelectPwdMark=3=>%s\n",strPwd.c_str());
		nCpyLen = strPwd.size() < 2*LENGTH_ROOMPWD ? strPwd.size() : 2*LENGTH_ROOMPWD;
		BYTE szMD5Password[2*LENGTH_ROOMPWD+1] = {0};
		memcpy(szMD5Password, strPwd.c_str(), nCpyLen);
		HexStr2Ascii((char*)tRoomSum.szPassword, (char*)szMD5Password, 2*LENGTH_ROOMPWD);

		//LOG_DEBUG(LOG_DB_SERVER, "Query_DeviceRoomSum DeviceID %d RoomID %d UserIndex %d PushIndex %d CardIndex %d Room %s Pwd %s\n",
		//	dwDeviceID, tRoomSum.dwRoomID, tRoomSum.dwUserIndex, tRoomSum.dwPushIndex, tRoomSum.dwCardIndex, tRoomSum.szRoom, tRoomSum.szPassword);

		listInfo.push_back(tRoomSum);
	}
	return true;
}

bool CDataBaseHandle::Query_DeviceRoomUser(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMUSER& listInfo)
{
	bool bGetAll = (lstRoomID.size() == 0);
	std::string strRoomID;
	if (false == bGetAll) MkRoomIDStr(lstRoomID, strRoomID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << TABLE_USER << "." << T_USER_ID << ","
		<< TABLE_USER << "." << T_USER_MOBILEPHONE << ","
		<< TABLE_USER << "." << T_USER_NAME << ","
		<< TABLE_USER << "." << T_USER_EMAIL << ","
		<< TABLE_USER << "." << T_USER_LANGUAGE << ","
		<< TABLE_USER << "." << T_USER_VENDORID << ","
		<< TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICENAME << ","
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID << ","
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_USERINDEX
		<< " FROM " << TABLE_USERDEVICE << "," << TABLE_USER << "," << TABLE_DEVICEROOM
		<< " WHERE " << TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID << "=" << dwDeviceID
		<< " AND " << TABLE_USERDEVICE << "." << T_USERDEVICE_USERID << "=" << TABLE_USER << "." << T_USER_ID
		<< " AND " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_DEVICEID << "=" << TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID
		<< " AND " << TABLE_USERDEVICE << "." << T_USERDEVICE_ROOMID << "=" << TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID;
	if (false == bGetAll)
	{
		query << " AND " << strRoomID.c_str();
	}

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	RoomUser_t tRoomUser;
	LIST_ROOMSUM lstRoomSum;
	SmsInfo_t tInfo;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		memset(&tInfo, 0, sizeof(SmsInfo_t));
		Row row = *iter;

		tRoomUser.dwRoomID = 0;
		tRoomUser.dwUserIndex = 0;
		tRoomUser.lstUserInfo.clear();
		memset(&tInfo, 0, sizeof(SmsInfo_t));

		tRoomUser.dwRoomID = (DWORD)row[T_DEVICEROOM_ROOMID];
		tRoomUser.dwUserIndex = (DWORD)row[T_DEVICEROOM_USERINDEX];

		tInfo.dwUserID = (DWORD)row[T_USER_ID];
		tInfo.dwVendorID = (DWORD)row[T_USER_VENDORID];
		int nLanguage = (int)row[T_USER_LANGUAGE];
		tInfo.bLanguage = (BYTE)nLanguage;

		std::string strMobilePhone = (std::string)row[T_USER_MOBILEPHONE];
		std::string strEmail = (std::string)row[T_USER_EMAIL];
		std::string strUserName = (std::string)row[T_USER_NAME];
		GetVaildUserName(strMobilePhone, strEmail, strUserName, (PUCHAR)tInfo.szMobilePhone);

		std::string strName = (std::string)row[T_USERDEVICE_DEVICENAME];
		int nCpyLen = strName.size() < LENGTH_NAME ? strName.size() : LENGTH_NAME;
		memcpy(tInfo.szDeviceName, strName.c_str(), nCpyLen);


		//LOG_DEBUG(LOG_DB_SERVER, "RoomID %d UserIndex %d UserID %d VendorID %d Language %d VaildUserName %s DeviceName %d:%s\n",
		//	tRoomUser.dwRoomID, tRoomUser.dwUserIndex, tInfo.dwUserID, tInfo.dwVendorID, tInfo.bLanguage, tInfo.szMobilePhone, nCpyLen, tInfo.szDeviceName);

		g_dwRoomID = tRoomUser.dwRoomID;
		LIST_ROOMUSER::iterator posList = std::find_if( listInfo.begin(), listInfo.end(), FindRoomUserByRoomID() );
		if (posList != listInfo.end())
		{
			posList->dwUserIndex = tRoomUser.dwUserIndex;
			posList->lstUserInfo.push_back(tInfo);
		}
		else
		{
			tRoomUser.lstUserInfo.push_back(tInfo);
			listInfo.push_back(tRoomUser);
		}
	}
	
	//////////////////////////////////////////////////////////////////////////
	// 查询房号该项内容为空的Index
	LIST_DWORD lstRemainIndex;
	LIST_DWORD::iterator iterIndex = lstRoomID.begin();
	for (; iterIndex != lstRoomID.end(); iterIndex++)
	{
		g_dwRoomID = *iterIndex;
		LIST_ROOMUSER::iterator posIndex = std::find_if( listInfo.begin(), listInfo.end(), FindRoomUserByRoomID() );
		if (posIndex == listInfo.end()) lstRemainIndex.push_back(g_dwRoomID);
	}
	Query_DeviceRoomSum(dwDeviceID, lstRemainIndex, lstRoomSum);

	//////////////////////////////////////////////////////////////////////////
	// 插入已经被删除的roomid/没有该项内容的roomid
	LIST_DWORD::iterator iterErase = lstRoomID.begin();
	for (; iterErase != lstRoomID.end(); iterErase++)
	{
		g_dwRoomID = *iterErase;
		LIST_ROOMUSER::iterator posList = std::find_if( listInfo.begin(), listInfo.end(), FindRoomUserByRoomID() );
		if (posList == listInfo.end())
		{
			tRoomUser.dwRoomID = g_dwRoomID;

			LIST_ROOMSUM::iterator posIndex2 = std::find_if( lstRoomSum.begin(), lstRoomSum.end(), FindRoomSumByRoomID() );
			if (posIndex2 == lstRoomSum.end()) tRoomUser.dwUserIndex = 0;
			else tRoomUser.dwUserIndex = posIndex2->dwUserIndex;
			//LOG_DEBUG(LOG_DB_SERVER, "###RoomID %d UserIndex %d###\n", g_dwRoomID, tRoomUser.dwUserIndex);
			tRoomUser.lstUserInfo.clear();
			listInfo.push_back(tRoomUser);
		}
	}
	return true;
}

void CDataBaseHandle::AppendRoomPush(CSTRING strToken, PushInfo_t& tPushInfo, RoomPush_t& tRoomPush, LIST_ROOMPUSH& listInfo)
{
	int nCpyLen = strToken.size() < LENGTH_TOKEN ? strToken.size() : LENGTH_TOKEN;
	if (nCpyLen <= 0) return;

	memset(tPushInfo.szToken, 0, LENGTH_TOKEN+1);
	memcpy(tPushInfo.szToken, strToken.c_str(), nCpyLen);

	LIST_ROOMPUSH::iterator posList = std::find_if( listInfo.begin(), listInfo.end(), FindRoomPushByRoomID() );
	if (posList != listInfo.end())
	{
		posList->dwPushIndex = tRoomPush.dwPushIndex;
		posList->lstPushInfo.push_back(tPushInfo);
	}
	else
	{
		tRoomPush.lstPushInfo.push_back(tPushInfo);
		listInfo.push_back(tRoomPush);
	}
}
bool CDataBaseHandle::Query_DeviceRoomPush(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMPUSH& listInfo)
{
	bool bGetAll = (lstRoomID.size() == 0);
	std::string strRoomID;
	if (false == bGetAll) MkRoomIDStr(lstRoomID, strRoomID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << TABLE_PUSHINFO << "." << T_PUSHINFO_USERID << ","
		<< TABLE_PUSHINFO << "." << T_PUSHINFO_OS << ","
		<< TABLE_PUSHINFO << "." << T_PUSHINFO_GT_TOKEN << ","
		<< TABLE_PUSHINFO << "." << T_PUSHINFO_BAIDU_TOKEN << ","
		<< TABLE_PUSHINFO << "." << T_PUSHINFO_HW_TOKEN << ","
		<< TABLE_PUSHINFO << "." << T_PUSHINFO_MI_TOKEN << ","
		<< TABLE_PUSHINFO << "." << T_PUSHINFO_JG_TOKEN << ","
		<< TABLE_PUSHINFO << "." << T_PUSHINFO_MZ_TOKEN << ","
		<< TABLE_PUSHINFO << "." << T_PUSHINFO_LANGUAGE << ","
		<< TABLE_PUSHINFO << "." << T_PUSHINFO_VENDORID << ","
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID << ","
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_PUSHINDEX
		<< " FROM " << TABLE_USERDEVICE << "," << TABLE_PUSHINFO << "," << TABLE_DEVICEROOM
		<< " WHERE " << TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID << "=" << dwDeviceID
		<< " AND " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_DEVICEID << "=" << TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID
		<< " AND " << TABLE_USERDEVICE << "." << T_USERDEVICE_USERID << "=" << TABLE_PUSHINFO << "." << T_PUSHINFO_USERID
		<< " AND " << TABLE_USERDEVICE << "." << T_USERDEVICE_ROOMID << "=" << TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID;
	if (false == bGetAll)
	{
		query << " AND " << strRoomID.c_str();
	}
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	RoomPush_t tRoomPush; PushInfo_t tPushInfo; 
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		tRoomPush.dwRoomID = 0;
		tRoomPush.dwPushIndex = 0;
		tRoomPush.lstPushInfo.clear();
		memset(&tPushInfo, 0, sizeof(PushInfo_t));

		tRoomPush.dwRoomID    = (DWORD)row[T_DEVICEROOM_ROOMID];
		tRoomPush.dwPushIndex = (DWORD)row[T_DEVICEROOM_PUSHINDEX];	
		tPushInfo.dwUserID    = (DWORD)row[T_PUSHINFO_USERID];
		tPushInfo.dwVendorID  = (DWORD)row[T_PUSHINFO_VENDORID];
		int nPushType		  = (int)  row[T_PUSHINFO_OS];
		int nLanguage		  = (int)  row[T_PUSHINFO_LANGUAGE];
		tPushInfo.bLanguage   = (BYTE)nLanguage;
		g_dwRoomID = tRoomPush.dwRoomID;

		// Gt
		std::string strToken = (std::string)row[T_PUSHINFO_GT_TOKEN];
		if (nPushType == DEVICE_TYPE_ANDROID)  tPushInfo.bPushType = PUSH_TYPE_ANDROID_GT;
		else if (nPushType == DEVICE_TYPE_IOS) tPushInfo.bPushType = PUSH_TYPE_IOS_GT;
		AppendRoomPush(strToken, tPushInfo, tRoomPush, listInfo);
		// baiduyun
		strToken = (std::string)row[T_PUSHINFO_BAIDU_TOKEN];
		if (nPushType == DEVICE_TYPE_ANDROID)  tPushInfo.bPushType = PUSH_TYPE_ANDROID_BAIDU;
		else if (nPushType == DEVICE_TYPE_IOS) tPushInfo.bPushType = PUSH_TYPE_IOS_BAIDU;
		AppendRoomPush(strToken, tPushInfo, tRoomPush, listInfo);
		//华为
		strToken = (std::string)row[T_PUSHINFO_HW_TOKEN];
		if (nPushType == DEVICE_TYPE_ANDROID){
			tPushInfo.bPushType = PUSH_TYPE_ANDROID_HW;
			AppendRoomPush(strToken, tPushInfo, tRoomPush, listInfo);
		}
		//小米
		strToken = (std::string)row[T_PUSHINFO_MI_TOKEN];
		if (nPushType == DEVICE_TYPE_ANDROID){
			tPushInfo.bPushType = PUSH_TYPE_ANDROID_MI;
			AppendRoomPush(strToken, tPushInfo, tRoomPush, listInfo);
		}
		//极光
		strToken = (std::string)row[T_PUSHINFO_JG_TOKEN];
		if (nPushType == DEVICE_TYPE_ANDROID)  tPushInfo.bPushType = PUSH_TYPE_ANDROID_JG;
		else if (nPushType == DEVICE_TYPE_IOS) tPushInfo.bPushType = PUSH_TYPE_IOS_JG;
		AppendRoomPush(strToken, tPushInfo, tRoomPush, listInfo);
		//魅族
		strToken = (std::string)row[T_PUSHINFO_MZ_TOKEN];
		if (nPushType == DEVICE_TYPE_ANDROID){
			tPushInfo.bPushType = PUSH_TYPE_ANDROID_MZ;
			AppendRoomPush(strToken, tPushInfo, tRoomPush, listInfo);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 查询房号该项内容为空的Index
	LIST_DWORD lstRemainIndex;
	LIST_DWORD::iterator iterIndex = lstRoomID.begin();
	for (; iterIndex != lstRoomID.end(); iterIndex++)
	{
		g_dwRoomID = *iterIndex;
		LIST_ROOMPUSH::iterator posIndex = std::find_if( listInfo.begin(), listInfo.end(), FindRoomPushByRoomID() );
		if (posIndex == listInfo.end()) lstRemainIndex.push_back(g_dwRoomID);
	}
	LIST_ROOMSUM lstRoomSum;
	Query_DeviceRoomSum(dwDeviceID, lstRemainIndex, lstRoomSum);

	//////////////////////////////////////////////////////////////////////////
	// 插入已经被删除的roomid/没有该项内容的roomid
	LIST_DWORD::iterator iterErase = lstRoomID.begin();
	for (; iterErase != lstRoomID.end(); iterErase++)
	{
		g_dwRoomID = *iterErase;
		LIST_ROOMPUSH::iterator posList = std::find_if( listInfo.begin(), listInfo.end(), FindRoomPushByRoomID() );
		if (posList != listInfo.end()) continue;

		tRoomPush.dwRoomID = g_dwRoomID;

		LIST_ROOMSUM::iterator posIndex2 = std::find_if( lstRoomSum.begin(), lstRoomSum.end(), FindRoomSumByRoomID() );
		if (posIndex2 == lstRoomSum.end()) tRoomPush.dwPushIndex = 0;
		else tRoomPush.dwPushIndex = posIndex2->dwPushIndex;
// 		LOG_DEBUG(LOG_DB_SERVER, "###RoomID %d dwPushIndex %d###\n", g_dwRoomID, tRoomPush.dwPushIndex);

		tRoomPush.lstPushInfo.clear();
		listInfo.push_back(tRoomPush);
	}
	return true;
}

bool CDataBaseHandle::Query_DeviceRoomCard(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMCARD& listInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s\n",__FUNCTION__);
	bool bGetAll = (lstRoomID.size() == 0);
	std::string strRoomID;
	if (false == bGetAll) MkRoomIDStr(lstRoomID, strRoomID);

	// SELECT `deviceroom`.`roomid`, `deviceroom`.`cardindex`, `card`.`cardnumber`, `card`.`cardtype` , `card`.`timelimit` FROM `roomcard`, `card`, `deviceroom` WHERE `deviceroom`.`deviceid`=251 AND `deviceroom`.`roomid`=`roomcard`.`roomid` AND `roomcard`.`cardid`=`card`.`cardid`
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID << ","
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_CARDINDEX << ","
		<< TABLE_CARD << "." << T_CARD_NUMBER << ","
		<< TABLE_CARD << "." << T_CARD_TYPE << ","
		<< TABLE_CARD << "." << T_CARD_TIMELIMIT
		<< " FROM " << TABLE_CARD << "," << TABLE_ROOMCARD << "," << TABLE_DEVICEROOM
		<< " WHERE " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_DEVICEID << "=" << dwDeviceID
		<< " AND " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID << "=" << TABLE_ROOMCARD << "." << T_ROOMCARD_ROOMID
		<< " AND " << TABLE_CARD << "." << T_CARD_ID << "=" << TABLE_ROOMCARD << "." << T_ROOMCARD_CARDID;
	if (false == bGetAll)
	{
		query << " AND " << strRoomID.c_str();
	}

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	RoomCard_t tRoomCard;
	LIST_ROOMSUM lstRoomSum;
	CardInfo_t tCardInfo;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		tRoomCard.dwRoomID = 0;
		tRoomCard.dwCardIndex = 0;
		tRoomCard.lstCardInfo.clear();
		memset(&tCardInfo, 0, sizeof(CardInfo_t));

		tRoomCard.dwRoomID = (DWORD)row[T_DEVICEROOM_ROOMID];
		tRoomCard.dwCardIndex = (DWORD)row[T_DEVICEROOM_CARDINDEX];

		int nCardType = (int)row[T_CARD_TYPE];
		tCardInfo.bCardType = (BYTE)nCardType;
		std::string strCardNumber = (std::string)row[T_CARD_NUMBER];
		int nCpyLen = strCardNumber.size() < LENGTH_CARDNUMBER ? strCardNumber.size() : LENGTH_CARDNUMBER;
		memcpy(tCardInfo.szCard, strCardNumber.c_str(), nCpyLen);
		
		tCardInfo.dwCardTimeLimit = (DWORD)row[T_CARD_TIMELIMIT];
		LOG_DEBUG(LOG_DB_SERVER, "RoomID %d CardIndex %d CardType %d CardNumber %s CardTimeLimit %d\n",
			tRoomCard.dwRoomID, tRoomCard.dwCardIndex, tCardInfo.bCardType, tCardInfo.szCard, tCardInfo.dwCardTimeLimit);

		g_dwRoomID = tRoomCard.dwRoomID;
		LIST_ROOMCARD::iterator posList = std::find_if( listInfo.begin(), listInfo.end(), FindRoomCardByRoomID() );
		if (posList != listInfo.end())
		{
			posList->dwCardIndex = tRoomCard.dwCardIndex;
			posList->lstCardInfo.push_back(tCardInfo);
		}
		else
		{
			tRoomCard.lstCardInfo.push_back(tCardInfo);
			listInfo.push_back(tRoomCard);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 查询房号该项内容为空的Index
	LIST_DWORD lstRemainIndex;
	LIST_DWORD::iterator iterIndex = lstRoomID.begin();
	for (; iterIndex != lstRoomID.end(); iterIndex++)
	{
		g_dwRoomID = *iterIndex;
		LIST_ROOMCARD::iterator posIndex = std::find_if( listInfo.begin(), listInfo.end(), FindRoomCardByRoomID() );
		if (posIndex == listInfo.end()) lstRemainIndex.push_back(g_dwRoomID);
	}
	Query_DeviceRoomSum(dwDeviceID, lstRemainIndex, lstRoomSum);

	//////////////////////////////////////////////////////////////////////////
	// 插入已经被删除的roomid/没有该项内容的roomid
	LIST_DWORD::iterator iterErase = lstRoomID.begin();
	for (; iterErase != lstRoomID.end(); iterErase++)
	{
		g_dwRoomID = *iterErase;
		LIST_ROOMCARD::iterator posList = std::find_if( listInfo.begin(), listInfo.end(), FindRoomCardByRoomID() );
		if (posList == listInfo.end())
		{
			tRoomCard.dwRoomID = g_dwRoomID;

			LIST_ROOMSUM::iterator posIndex2 = std::find_if( lstRoomSum.begin(), lstRoomSum.end(), FindRoomSumByRoomID() );
			if (posIndex2 == lstRoomSum.end()) tRoomCard.dwCardIndex = 0;
			else tRoomCard.dwCardIndex = posIndex2->dwCardIndex;
			//LOG_DEBUG(LOG_DB_SERVER, "###RoomID %d dwCardIndex %d###\n", g_dwRoomID, tRoomCard.dwCardIndex);

			tRoomCard.lstCardInfo.clear();
			listInfo.push_back(tRoomCard);
		}
	}
	return true;
}
bool CDataBaseHandle::Query_DeviceRoomIndoor(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMINDOOR2& listInfo)
{
	bool bGetAll = (lstRoomID.size() == 0);
	std::string strRoomID;
	if (false == bGetAll) MkRoomIDStr(lstRoomID, strRoomID);

	//SELECT deviceroom.roomid, deviceroom.indoorindex,indoor.indoorid,device.devicesn  FROM device,deviceroom,indoor WHERE deviceroom.deviceid=400004 AND deviceroom.roomid = indoor.roomid AND indoor.indoorid = device.deviceid;
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID << ","
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_INDOORINDEX << ","
		<< TABLE_DEVICE << "." << T_DEVICE_ID << ","
		<< TABLE_DEVICE << "." << T_DEVICE_SN 
		<< " FROM " <<  TABLE_DEVICE << "," << TABLE_DEVICEROOM << "," << TABLE_INDOOR
		<< " WHERE " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_DEVICEID << "=" << dwDeviceID
		<< " AND " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID << "=" << TABLE_INDOOR << "." << T_INDOOR_ROOMID
		<< " AND " << TABLE_DEVICE << "." << T_DEVICE_ID << "=" << TABLE_INDOOR << "." << T_INDOOR_ID;
		
	LOG_DEBUG(LOG_DB_SERVER,"query 1\n");
	if (false == bGetAll)
	{
		query << " AND " << strRoomID.c_str();
	}
	LOG_DEBUG(LOG_DB_SERVER,"query 2\n");
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) {
		LOG_DEBUG(LOG_DB_SERVER,"query 3\n");
		return false;
	}
	LOG_DEBUG(LOG_DB_SERVER,"query 4\n");
	RoomIndoor2_t tRoomIndoor;
	LIST_ROOMSUM lstRoomSum;
	InDoorInfo_t tIndoorInfo;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		LOG_DEBUG(LOG_DB_SERVER,"query 5\n");
		tRoomIndoor.dwRoomID = 0;
		tRoomIndoor.dwIndoorIndex = 0;
		tRoomIndoor.lstIndoorInfo.clear();
		memset(&tIndoorInfo, 0, sizeof(InDoorInfo_t));
		tRoomIndoor.dwRoomID = (DWORD)row[T_DEVICEROOM_ROOMID];
		tRoomIndoor.dwIndoorIndex = (DWORD)row[T_DEVICEROOM_INDOORINDEX];
		tIndoorInfo.dwInDoorID = (DWORD)row[T_DEVICE_ID];
		LOG_DEBUG(LOG_DB_SERVER,"query 6 %d %d %d\n",tRoomIndoor.dwRoomID, tRoomIndoor.dwIndoorIndex, tIndoorInfo.dwInDoorID);
	
		std::string strIndoorSN = (std::string)row[T_DEVICE_SN];
		int nCpyLen = strIndoorSN.size() < LENGTH_SERIALNO ? strIndoorSN.size() : LENGTH_SERIALNO;
		memcpy(tIndoorInfo.szSerialNO, strIndoorSN.c_str(), nCpyLen);
		
		LOG_DEBUG(LOG_DB_SERVER,"RoomID=%d, IndoorIndex=%d, IndoorID=%d, IndoorSN=%s\n", tRoomIndoor.dwRoomID, tRoomIndoor.dwIndoorIndex, tIndoorInfo.dwInDoorID, tIndoorInfo.szSerialNO);

		g_dwRoomID = tRoomIndoor.dwRoomID;
		LIST_ROOMINDOOR2::iterator posList = std::find_if( listInfo.begin(), listInfo.end(), FindRoomIndoorByRoomID() );
		if (posList != listInfo.end())
		{
			posList->dwIndoorIndex = tRoomIndoor.dwIndoorIndex;
			posList->lstIndoorInfo.push_back(tIndoorInfo);
		}
		else
		{
			tRoomIndoor.lstIndoorInfo.push_back(tIndoorInfo);
			listInfo.push_back(tRoomIndoor);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 查询房号该项内容为空的Index
	LIST_DWORD lstRemainIndex;
	LIST_DWORD::iterator iterIndex = lstRoomID.begin();
	for (; iterIndex != lstRoomID.end(); iterIndex++)
	{
		g_dwRoomID = *iterIndex;
		LIST_ROOMINDOOR2::iterator posIndex = std::find_if( listInfo.begin(), listInfo.end(), FindRoomIndoorByRoomID() );
		if (posIndex == listInfo.end()) lstRemainIndex.push_back(g_dwRoomID);
	}
	Query_DeviceRoomSum(dwDeviceID, lstRemainIndex, lstRoomSum);

	//////////////////////////////////////////////////////////////////////////
	// 插入已经被删除的roomid/没有该项内容的roomid
	LIST_DWORD::iterator iterErase = lstRoomID.begin();
	for (; iterErase != lstRoomID.end(); iterErase++)
	{
		g_dwRoomID = *iterErase;
		LIST_ROOMINDOOR2::iterator posList = std::find_if( listInfo.begin(), listInfo.end(), FindRoomIndoorByRoomID() );
		if (posList == listInfo.end())
		{
			tRoomIndoor.dwRoomID = g_dwRoomID;

			LIST_ROOMSUM::iterator posIndex2 = std::find_if( lstRoomSum.begin(), lstRoomSum.end(), FindRoomSumByRoomID() );
			if (posIndex2 == lstRoomSum.end()) tRoomIndoor.dwIndoorIndex = 0;
			else tRoomIndoor.dwIndoorIndex = posIndex2->dwIndoorIndex;
			//LOG_DEBUG(LOG_DB_SERVER, "###RoomID %d dwCardIndex %d###\n", g_dwRoomID, tRoomCard.dwCardIndex);

			tRoomIndoor.lstIndoorInfo.clear();
			listInfo.push_back(tRoomIndoor);
		}
	}
	
	return true;
}
/*
//室内机
bool CDataBaseHandle::GetDeviceRoomIndoor(DWORD dwDeviceID,LIST_DWORD& lstRoomID, RoomIndoor_t& tRoomIndoor)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s DeviceID=%d\n",__FUNCTION__, dwDeviceID);
	LIST_TABINDOORINFO lstTabIndoorInfo;//indoor表中信息列表 
	//1.根据deviceid查询indoor表中数据(indoorid,deviceid,roomid)
	bool res = Query_InDoorInfo(dwDeviceID,lstTabIndoorInfo);
	if(res == false){
		LOG_DEBUG(LOG_DB_SERVER,"Query_indoorInfo Failed\n"); return false;
	}
	tRoomIndoor.dwDeviceID = dwDeviceID;
	tRoomIndoor.dwRoomCount = lstRoomID.size();
	//比对lstRoomID列表
	LOG_DEBUG(LOG_DB_SERVER,"compare  Count= %d\n",tRoomIndoor.dwRoomCount);
	//////////////////////////////////////////////////////////////////////////
	LIST_TABINDOORINFO::iterator iter = lstTabIndoorInfo.begin();
	LIST_DWORD::iterator itRoom = lstRoomID.begin();
	DWORD dwIsIndoor = 0;//标记是否有相同的RoomID
	for(; itRoom != lstRoomID.end(); itRoom++)
	{
		LOG_DEBUG(LOG_DB_SERVER,"compare  1 \n");
		DWORD dwRoomID2 = *itRoom;
		for(; iter != lstTabIndoorInfo.end(); iter++)
		{
			LOG_DEBUG(LOG_DB_SERVER,"compare 2\n");
			if(dwRoomID2 == iter->dwRoomID)
			{
				dwIsIndoor = 1;
				DWORD dwDeviceID = iter->dwDeviceID;
				DWORD dwRoomID = iter->dwRoomID;
				DWORD dwIndoorID = iter->dwIndoorID;

				roomInDoorInfo tInfo;
				tInfo.dwCount = 0;
				tInfo.dwInDoorIndex = 0;
				tInfo.dwRoomID = 0;
				tInfo.lstInDoorInfo.clear();

				tInfo.dwRoomID = iter->dwRoomID;
				DWORD dwIndoorIndex = 0;
				Query_InDoorIndex(dwDeviceID,dwRoomID,dwIndoorIndex);
				tInfo.dwInDoorIndex = dwIndoorIndex;
				QueryRoomIndoorCount(dwDeviceID, dwRoomID, tInfo.lstInDoorInfo);
				tInfo.dwCount =  tInfo.lstInDoorInfo.size();
				LOG_DEBUG(LOG_DB_SERVER,"RoomID=%d,IndoorIndex=%d,Count=%d\n",tInfo.dwRoomID,tInfo.dwInDoorIndex,tInfo.dwCount);
				tRoomIndoor.lstRoomIndoorInfo.push_back(tInfo);
			}
		}
		if(dwIsIndoor != 1)
		{
			DWORD dwDeviceID = 0;
			DWORD dwRoomID = 0;
			DWORD dwIndoorID = 0;

			roomInDoorInfo tInfo;
			tInfo.dwCount = 0;
			tInfo.dwInDoorIndex = 0;
			tInfo.dwRoomID = 0;
			tInfo.lstInDoorInfo.clear();

			tInfo.dwRoomID = dwRoomID2;
			DWORD dwIndoorIndex = 0;
			tInfo.dwInDoorIndex = 0;
			tInfo.dwCount =  0;
			LOG_DEBUG(LOG_DB_SERVER,"RoomID=%d,IndoorIndex=%d,Count=%d\n",tInfo.dwRoomID,tInfo.dwInDoorIndex,tInfo.dwCount);
			tRoomIndoor.lstRoomIndoorInfo.push_back(tInfo);
		}
		dwIsIndoor = 0;
	}
	return true;
}
*/
bool CDataBaseHandle::Query_DeviceRoomOther(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMOTHER& listInfo)
{
	bool bGetAll = (lstRoomID.size() == 0);
	std::string strRoomID;
	if (false == bGetAll) MkRoomIDStr(lstRoomID, strRoomID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICEROOM_ROOMID << ","
		<< T_DEVICEROOM_ROOM << ","
		<< T_DEVICEROOM_PASSWORD << ","
		<< T_DEVICEROOM_CATEGORY
		<< " FROM " << TABLE_DEVICEROOM
		<< " WHERE ";
	if (false == bGetAll) query << strRoomID.c_str();
	else query << T_DEVICEROOM_DEVICEID  << "=" << dwDeviceID;

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		RoomOther_t tRoomOther; memset(&tRoomOther, 0, sizeof(RoomOther_t));

		tRoomOther.dwRoomID = (DWORD)row[T_DEVICEROOM_ROOMID];
		std::string strRoom = (std::string)row[T_DEVICEROOM_ROOM];
		int nCpyLen = strRoom.size() < LENGTH_ROOM ? strRoom.size() : LENGTH_ROOM;
		memcpy(tRoomOther.szRoom, strRoom.c_str(), nCpyLen);

		std::string strMD5Password = (std::string)row[T_DEVICEROOM_PASSWORD];
//		LOG_DEBUG(LOG_DB_SERVER,"SelectPwdMark=4=>%s\n",strMD5Password.c_str());
		SelectPwdMark(strMD5Password);
//		LOG_DEBUG(LOG_DB_SERVER,"SelectPwdMark=4=>%s\n",strMD5Password.c_str());
		nCpyLen = strMD5Password.size() < 2*LENGTH_ROOMPWD ? strMD5Password.size() : 2*LENGTH_ROOMPWD;
		BYTE szMD5Password[2*LENGTH_ROOMPWD+1] = {0};
		memcpy(szMD5Password, strMD5Password.c_str(), nCpyLen);
		HexStr2Ascii((char*)tRoomOther.szPassword, (char*)szMD5Password, 2*LENGTH_ROOMPWD);
		//LOG_DEBUG(LOG_DB_SERVER, "RoomID %d Room %s\n", tRoomOther.dwRoomID, tRoomOther.szRoom);
		tRoomOther.dwRoomAttr = (DWORD)row[T_DEVICEROOM_CATEGORY];
		listInfo.push_back(tRoomOther);
	}
	return true;
}

bool CDataBaseHandle::Query_DeviceRoomPushSwitch(DWORD dwDeviceID, LIST_DWORD& lstRoomID, LIST_ROOMPUSHSWITCH& listInfo)
{
	bool bGetAll = (lstRoomID.size() == 0);
	std::string strRoomID;
	if (false == bGetAll) MkRoomIDStr(lstRoomID, strRoomID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << TABLE_PUSHINFO << "." << T_PUSHINFO_USERID << ","
		<< TABLE_PUSHINFO << "." << T_PUSHINFO_SWITCH << ","
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID << ","
		<< TABLE_DEVICEROOM << "." << T_DEVICEROOM_PUSHSWITCHINDEX
		<< " FROM " << TABLE_USERDEVICE << "," << TABLE_PUSHINFO << "," << TABLE_DEVICEROOM
		<< " WHERE " << TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID << "=" << dwDeviceID
		<< " AND " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_DEVICEID << "=" << TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID
		<< " AND " << TABLE_USERDEVICE << "." << T_USERDEVICE_USERID << "=" << TABLE_PUSHINFO << "." << T_PUSHINFO_USERID
		<< " AND " << TABLE_USERDEVICE << "." << T_USERDEVICE_ROOMID << "=" << TABLE_DEVICEROOM << "." << T_DEVICEROOM_ROOMID;
	if (false == bGetAll)
	{
		query << " AND " << strRoomID.c_str();
	}
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	RoomPushSwitch_t tRoomPushSwitch;
	LIST_ROOMSUM lstRoomSum;
	PushSwitchInfo_t tPushSwitchInfo;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		tRoomPushSwitch.dwRoomID = 0;
		tRoomPushSwitch.dwPushSwitchIndex = 0;
		tRoomPushSwitch.lstPushSwitch.clear();
		memset(&tPushSwitchInfo, 0, sizeof(PushSwitchInfo_t));

		tRoomPushSwitch.dwRoomID = (DWORD)row[T_DEVICEROOM_ROOMID];
		tRoomPushSwitch.dwPushSwitchIndex = (DWORD)row[T_DEVICEROOM_PUSHSWITCHINDEX];
		
		tPushSwitchInfo.dwUserID = (DWORD)row[T_PUSHINFO_USERID];
		tPushSwitchInfo.nPushSwitch = (int)row[T_PUSHINFO_SWITCH];

		g_dwRoomID = tRoomPushSwitch.dwRoomID;
		LIST_ROOMPUSHSWITCH::iterator posList = std::find_if( listInfo.begin(), listInfo.end(), FindRoomPushSwitchByRoomID() );
		if (posList != listInfo.end())
		{
			posList->dwPushSwitchIndex = tRoomPushSwitch.dwPushSwitchIndex;
			posList->lstPushSwitch.push_back(tPushSwitchInfo);
		}
		else
		{
			tRoomPushSwitch.lstPushSwitch.push_back(tPushSwitchInfo);
			listInfo.push_back(tRoomPushSwitch);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 查询房号该项内容为空的Index
	LIST_DWORD lstRemainIndex;
	LIST_DWORD::iterator iterIndex = lstRoomID.begin();
	for (; iterIndex != lstRoomID.end(); iterIndex++)
	{
		g_dwRoomID = *iterIndex;
		LIST_ROOMPUSHSWITCH::iterator posIndex = std::find_if( listInfo.begin(), listInfo.end(), FindRoomPushSwitchByRoomID() );
		if (posIndex == listInfo.end()) lstRemainIndex.push_back(g_dwRoomID);
	}
	Query_DeviceRoomSum(dwDeviceID, lstRemainIndex, lstRoomSum);

	//////////////////////////////////////////////////////////////////////////
	// 插入已经被删除的roomid/没有该项内容的roomid
	LIST_DWORD::iterator iterErase = lstRoomID.begin();
	for (; iterErase != lstRoomID.end(); iterErase++)
	{
		g_dwRoomID = *iterErase;
		LIST_ROOMPUSHSWITCH::iterator posList = std::find_if( listInfo.begin(), listInfo.end(), FindRoomPushSwitchByRoomID() );
		if (posList == listInfo.end())
		{
			tRoomPushSwitch.dwRoomID = g_dwRoomID;

			LIST_ROOMSUM::iterator posIndex2 = std::find_if( lstRoomSum.begin(), lstRoomSum.end(), FindRoomSumByRoomID() );
			if (posIndex2 == lstRoomSum.end()) tRoomPushSwitch.dwPushSwitchIndex = 0;
			else tRoomPushSwitch.dwPushSwitchIndex = posIndex2->dwPushSwitchIndex;
			//LOG_DEBUG(LOG_DB_SERVER, "###RoomID %d dwPushIndex %d###\n", g_dwRoomID, tRoomPush.dwPushIndex);

			tRoomPushSwitch.lstPushSwitch.clear();
			listInfo.push_back(tRoomPushSwitch);
		}
	}
	return true;
}

bool CDataBaseHandle::Query_InDoorInfo(DWORD dwDeviceID,LIST_TABINDOORINFO& lstTabInDoorInfo)
{
	LOG_DEBUG(LOG_MAIN,"CDataBaseHandle::%s DeviceID = %d\n",__FUNCTION__, dwDeviceID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_INDOOR_ROOMID << "," << T_INDOOR_ID
		<< " FROM " << TABLE_INDOOR
		<< " WHERE " << T_INDOOR_DEVICEID << "=" << dwDeviceID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	LOG_DEBUG(LOG_DB_SERVER,"TEST\n");
	if (res.size() == 0) {
		LOG_DEBUG(LOG_MAIN,"CDataBaseHandle::%s res.size() = %d\n",__FUNCTION__, res.size());
		return false;
	}
	
	TabInDoorInfo_t tInfo;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		tInfo.dwDeviceID = dwDeviceID;
		tInfo.dwRoomID	 = (DWORD)row[T_INDOOR_ROOMID];
		tInfo.dwIndoorID = (DWORD)row[T_INDOOR_ID];
		lstTabInDoorInfo.push_back(tInfo);
	}
	return true;
}
bool CDataBaseHandle::Query_InDoorIndex(DWORD dwDeviceID,DWORD dwRoomID,DWORD& dwIndoorIndex)
{
	LOG_DEBUG(LOG_MAIN,"CDataBaseHandle::%s\n",__FUNCTION__);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	//select indoorindex from deviceroom where deviceid = 441 and roomid = 63838;
	query << "SELECT " << T_DEVICEROOM_INDOORINDEX
		<< " FROM " << TABLE_DEVICEROOM
		<< " WHERE " << T_DEVICEROOM_DEVICEID << "=" << dwDeviceID 
		<< " AND " << T_DEVICEROOM_ROOMID << "=" << dwRoomID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	LOG_DEBUG(LOG_DB_SERVER,"select indoorindex from deviceroom where deviceid = %d and roomid = %d;\n", dwDeviceID, dwRoomID);
	if (res.size() <= 0){
		LOG_DEBUG(LOG_DB_SERVER, "Query_InDoorIndex Not Exist\n"); 
		return false;
	}
	Row row = *(res.begin());
	dwIndoorIndex = (DWORD)row[T_DEVICEROOM_INDOORINDEX];
	return true;
}
bool CDataBaseHandle::QueryRoomIndoorCount(DWORD dwDeviceID, DWORD dwRoomID, LIST_INDOORINFO& lstIndoorInfo)
{
	LOG_DEBUG(LOG_MAIN,"CDataBaseHandle::%s\n",__FUNCTION__);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_INDOOR_ID
		<< " FROM " << TABLE_INDOOR
		<< " WHERE " << T_DEVICEROOM_DEVICEID << "=" << dwDeviceID 
		<< " AND " << T_DEVICEROOM_ROOMID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0){
		LOG_DEBUG(LOG_DB_SERVER, "QueryRoomIndoorCount Not Exist\n"); return false;
	}
	InDoorInfo_t tInfo;
	memset(&tInfo, 0, sizeof(InDoorInfo_t));
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		tInfo.dwInDoorID = (DWORD)row[T_INDOOR_ID];
		Query_InDoorSN(tInfo.dwInDoorID, tInfo.szSerialNO);
		lstIndoorInfo.push_back(tInfo);
	}
	return true;
}

//SELECT `deviceid` 
//FROM `device` 
//WHERE (
//	`devicesn` = 'SDFJLHUINHXHDLEIUH22'
//	OR `devicesn` = 'SDFJLHUINHXHDLEIUH33'
//)

//INSERT INTO `userdevice` ( `userid` , `deviceid` ) 
//VALUES (1,deviceid1),(1,deviceid2)
bool CDataBaseHandle::Insert_UserDevice( DWORD dwUserID, PUCHAR pSerialNO, PUCHAR pDevName, PUCHAR pUserName, PUCHAR pRoom )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwUserID %d SN %s DevName %s Room %s\n", __FUNCTION__, dwUserID, pSerialNO, pDevName, pRoom);
	if (strlen((const char*)pSerialNO) <= 0) return true;
	if (strlen((const char*)pDevName) <= 0) return true;
	if (strlen((const char*)pRoom) <= 0) return true;

	// 1、检查设备序列号是否存在
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICE_ID << "," << T_DEVICE_OWNERID << " FROM " << TABLE_DEVICE << " WHERE " << T_DEVICE_SN << "='" << pSerialNO << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "Insert_UserDevice Not Exist (Dev SN %s)\n", pSerialNO);
		m_nError = DATABASE_HANDLE_ERROR_NOT_EXIST; return false;
	}

	// 2、检查设备当前是否已经被添加
	Row row = *(res.begin());
	DWORD dwOwnerID = (DWORD)row[T_DEVICE_OWNERID];
	if (dwOwnerID)
	{
		query << "SELECT " << T_USER_NAME << "," << T_USER_MOBILEPHONE << "," << T_USER_EMAIL << " FROM " << TABLE_USER << " WHERE " << T_USER_ID << "=" << dwOwnerID;
		res = query.store();
		if (res.size() <= 0) memcpy(pUserName, "Unknow", 6);
		else
		{
			row = *(res.begin());
			std::string strMobilePhone = (std::string)row[T_USER_MOBILEPHONE];
			std::string strEmail = (std::string)row[T_USER_EMAIL];
			std::string strUserName = (std::string)row[T_USER_NAME];
			GetVaildUserName(strMobilePhone, strEmail, strUserName, pUserName);
		}
		LOG_DEBUG(LOG_DB_SERVER, "Insert_UserDevice Add again (Owner Name %s)\n", pUserName);
		m_nError = DATABASE_HANDLE_ERROR_ADD_AGAIN;
		return false;
	}

	// 数据库操作1、插入房号“0000”
	DWORD dwDeviceID = (DWORD)row[T_DEVICE_ID];
	Insert_Room(dwDeviceID, pRoom);
	
	// 数据库操作2、插入到userdevice
	query << "INSERT INTO " << TABLE_USERDEVICE << " (" << T_USERDEVICE_USERID << "," << T_USERDEVICE_DEVICEID << "," << T_USERDEVICE_DEVICENAME << "," << T_USERDEVICE_ROOMID
		<< ") SELECT " << dwUserID << "," << dwDeviceID << ",'" << pDevName << "'," << T_DEVICEROOM_ROOMID << " FROM " << TABLE_DEVICEROOM
		<< " WHERE " << T_DEVICEROOM_DEVICEID << "=" << dwDeviceID << " AND " << T_DEVICEROOM_ROOM << "='" << pRoom << "'";
	if (-1 == CatchException(query)) return false;
	
	// 数据库操作3、更新OwnerID/GroupID
	Update_DeviceOwnerIDGroupID(dwDeviceID, dwUserID);
	
	// 数据库操作4、更新User表格中Index
	SET_DWORD setUserID; setUserID.insert(dwUserID);
	Update_UserConfigureIndex(setUserID);

	// 通知1、通知设备清空所有房号
	Notify_ClearRooms(dwDeviceID);

	// 通知2、通知设备更新设备所有信息
	Notify_UpdateDevice(dwDeviceID);
	
	return true;
}

bool CDataBaseHandle::Insert_Room(DWORD dwDeviceID, PUCHAR pRoom)
{
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICEROOM_ROOMID
		  << " FROM " << TABLE_DEVICEROOM
		  << " WHERE " << T_DEVICEROOM_DEVICEID << "=" << dwDeviceID
		  << " AND " << T_DEVICEROOM_ROOM << "='" << pRoom << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0)
	{
		BYTE szMd516[LENGTH_MD516+1] = {0};
		BYTE szMd532[2*LENGTH_MD516+1] = {0};
		memcpy(szMd516, CalMd5Val((PUCHAR)"1234", 4), LENGTH_MD516);
		Ascii2HexStr((char*)szMd532, (char*)szMd516, LENGTH_MD516);
		PCHAR pPwd = (PCHAR)szMd532;

		query << "INSERT INTO " << TABLE_DEVICEROOM
			<< " (" << T_DEVICEROOM_DEVICEID << "," << T_DEVICEROOM_ROOM << ","
			<< T_DEVICEROOM_USERINDEX << "," << T_DEVICEROOM_PUSHINDEX << "," << T_DEVICEROOM_CARDINDEX << "," << T_DEVICEROOM_PASSWORD
			<< ") VALUES ("
			<< dwDeviceID << ",'" << pRoom << "',1,1,1,'" << pPwd << "')";
		if (-1 == CatchException(query)) return false;
		LOG_DEBUG(LOG_DB_SERVER, "Insert_Room:Insert DeviceID %d Room %s Pwd %s\n", dwDeviceID, pRoom, pPwd);
	}

	return true;
}

//bool CDataBaseHandle::Delete_OtherToken( PushInfo_t& tInfo, bool bTry )
//{
//	Query query = m_clsSrcDBCon.query();
//	query.exec("SET NAMES 'utf8'");
//	query << "SELECT " << T_PUSHINFO_USERID
//		<< " FROM " << TABLE_PUSHINFO
//		<< " WHERE " << T_PUSHINFO_USERID << "=" << tInfo.dwUserID
//		<< " AND " << T_PUSHINFO_VENDORID << "=" << tInfo.dwVendorID
//		<< " AND " << T_PUSHINFO_OS << "=" << (int)tInfo.bPushType;
//	StoreQueryResult res;
//	if (-1 == CatchException(query, res)) return false;
//
//	if (res.size() > 0)
//	{
//		// 旧版本默认强制删除
//		// 尝试设置推送时提示在其他位置登录
//		if (bTry == false)
//		{
//			query << "DELETE FROM " << TABLE_PUSHINFO
//				<< " WHERE " << T_PUSHINFO_USERID << "=" << tInfo.dwUserID
//				<< " AND " << T_PUSHINFO_VENDORID << "=" << tInfo.dwVendorID
//				<< " AND " << T_PUSHINFO_OS << "=" << (int)tInfo.bPushType;
//			if (-1 == CatchException(query, res)) return false;
//		}
//		else m_nError = DATABASE_HANDLE_LOGIN_OTHER_PLACE;
//	}
//	return true;
//}

bool CDataBaseHandle::Update_UserPush(DWORD dwUserID)
{
	LIST_DWORD lstUserID; lstUserID.push_back(dwUserID);
	return Update_UserPush(lstUserID);
}

bool CDataBaseHandle::Update_UserPush(LIST_DWORD& lstUserID)
{
	CSTRING strUserID; MkUserIDStr(lstUserID, strUserID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT DISTINCT " << T_USERDEVICE_DEVICEID << "," << T_USERDEVICE_ROOMID
		<< " FROM " << TABLE_USERDEVICE
		<< " WHERE " << strUserID.c_str();
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	LIST_DWORD lstRoomIDAll;
	MAP_DWORD_SETDWORD mapTemp;
	MAP_DWORD_SETDWORD::iterator pos;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwDeviceID = (DWORD)row[T_USERDEVICE_DEVICEID];
		DWORD dwRoomID = (DWORD)row[T_USERDEVICE_ROOMID];

		LOG_DEBUG(LOG_DB_SERVER, "%s DeviceID %d RoomID %d\n", __FUNCTION__, dwDeviceID, dwRoomID);
		
		lstRoomIDAll.push_back(dwRoomID);

		pos = mapTemp.find(dwDeviceID);
		if (pos != mapTemp.end()) pos->second.insert(dwRoomID);
		else
		{
			SET_DWORD tSet; tSet.insert(dwRoomID);
			mapTemp.insert(std::make_pair(dwDeviceID, tSet));
		}
	}

	pos = mapTemp.begin();
	for (; pos != mapTemp.end(); pos++)
	{
		LIST_DWORD lstRoomID;
		lstRoomID.clear();
		SET_DWORD::iterator iterSet = pos->second.begin();
		for (; iterSet != pos->second.end(); iterSet++)
		{
			lstRoomID.push_back(*iterSet);
		}
		Notify_UpdateDeviceRoom(pos->first, lstRoomID, UpdateDevRoomType_PushIndex);
	}
	return Update_RoomPushIndex(lstRoomIDAll);
}

bool CDataBaseHandle::Update_UserPushSwitch(LIST_DWORD& lstUserID)
{
	CSTRING strUserID; MkUserIDStr(lstUserID, strUserID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT DISTINCT " << T_USERDEVICE_DEVICEID << "," << T_USERDEVICE_ROOMID
		<< " FROM " << TABLE_USERDEVICE
		<< " WHERE " << strUserID.c_str();
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	LIST_DWORD lstRoomIDAll;
	MAP_DWORD_SETDWORD mapTemp;
	MAP_DWORD_SETDWORD::iterator pos;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwDeviceID = (DWORD)row[T_USERDEVICE_DEVICEID];
		DWORD dwRoomID = (DWORD)row[T_USERDEVICE_ROOMID];

		LOG_DEBUG(LOG_DB_SERVER, "%s DeviceID %d RoomID %d\n", __FUNCTION__, dwDeviceID, dwRoomID);
		
		lstRoomIDAll.push_back(dwRoomID);

		pos = mapTemp.find(dwDeviceID);
		if (pos != mapTemp.end()) pos->second.insert(dwRoomID);
		else
		{
			SET_DWORD tSet; tSet.insert(dwRoomID);
			mapTemp.insert(std::make_pair(dwDeviceID, tSet));
		}
	}

	pos = mapTemp.begin();
	for (; pos != mapTemp.end(); pos++)
	{
		LIST_DWORD lstRoomID;
		lstRoomID.clear();
		SET_DWORD::iterator iterSet = pos->second.begin();
		for (; iterSet != pos->second.end(); iterSet++)
		{
			lstRoomID.push_back(*iterSet);
		}
		Notify_UpdateDeviceRoom(pos->first, lstRoomID, UpdateDevRoomType_PushSwitchIndex);
	}
	return Update_RoomPushSwitchIndex(lstRoomIDAll);
}
/*
void CDataBaseHandle::GetToken(ClientTokenArray_t& tInfo, CSTRING& strGtToken, 
														  CSTRING& strBaiduToken, 
														  CSTRING& strHWToken, 
														  CSTRING& strMIToken, 
														  CSTRING& strJGToken, 
														  int& nOS)
{
	for (int i = 0; i < tInfo.nCount; i++)
	{
		if (0 == strlen((const char*)tInfo.tToken[i].szToken)) continue;
		BYTE bPushType = tInfo.tToken[i].bPushType;
		if ( (bPushType == PUSH_TYPE_IOS_GT) || (bPushType == PUSH_TYPE_ANDROID_GT) )
		{
			strGtToken.assign((const char*)tInfo.tToken[i].szToken);
		}
		else if ( (bPushType == PUSH_TYPE_IOS_BAIDU) || (bPushType == PUSH_TYPE_ANDROID_BAIDU) )
		{
			strBaiduToken.assign((const char*)tInfo.tToken[i].szToken);
		}
		else if(bPushType == PUSH_TYPE_ANDROID_HW)//华为
		{
			strHWToken.assign((const char*)tInfo.tToken[i].szToken);
		}
		else if(bPushType == PUSH_TYPE_ANDROID_MI)//小米
		{
			strMIToken.assign((const char*)tInfo.tToken[i].szToken);
		}
		else if( (bPushType == PUSH_TYPE_ANDROID_JG) || (bPushType == PUSH_TYPE_IOS_JG) )//极光
		{
			strJGToken.assign((const char*)tInfo.tToken[i].szToken);
		}

	
		if ( (bPushType == PUSH_TYPE_IOS_GT) || (bPushType == PUSH_TYPE_IOS_BAIDU) ||(bPushType == PUSH_TYPE_IOS_JG) )
		{
			nOS = DEVICE_TYPE_IOS;
		}
		else if ( (bPushType == PUSH_TYPE_ANDROID_GT) || (bPushType == PUSH_TYPE_ANDROID_BAIDU) || (bPushType == PUSH_TYPE_ANDROID_HW) || 
			(bPushType == PUSH_TYPE_ANDROID_MI) || (bPushType == PUSH_TYPE_ANDROID_JG) )
		{
			nOS = DEVICE_TYPE_ANDROID;
		}
	}
}
*/
//更换上面的代码
void CDataBaseHandle::GetToken(ClientTokenArray_t& tInfo ,PushTokenType_t& tTokenType, int& nOS)
{
	for (int i = 0; i < tInfo.nCount; i++)
	{
		if (0 == strlen((const char*)tInfo.tToken[i].szToken)) continue;
		BYTE bPushType = tInfo.tToken[i].bPushType;
		if ( (bPushType == PUSH_TYPE_IOS_GT) || (bPushType == PUSH_TYPE_ANDROID_GT) )
		{
			tTokenType.setGtToken.assign((const char*)tInfo.tToken[i].szToken);
		}
		else if ( (bPushType == PUSH_TYPE_IOS_BAIDU) || (bPushType == PUSH_TYPE_ANDROID_BAIDU) )
		{
			tTokenType.setBaiduToken.assign((const char*)tInfo.tToken[i].szToken);
		}
		else if(bPushType == PUSH_TYPE_ANDROID_HW)//华为
		{
			tTokenType.setHWToken.assign((const char*)tInfo.tToken[i].szToken);
		}
		else if(bPushType == PUSH_TYPE_ANDROID_MI)//小米
		{
			tTokenType.setMIToken.assign((const char*)tInfo.tToken[i].szToken);
		}
		else if( (bPushType == PUSH_TYPE_ANDROID_JG) || (bPushType == PUSH_TYPE_IOS_JG) )//极光
		{
			tTokenType.setJGToken.assign((const char*)tInfo.tToken[i].szToken);
		}
		else if(bPushType == PUSH_TYPE_ANDROID_MZ)//魅族
		{
			tTokenType.setMZToken.assign((const char*)tInfo.tToken[i].szToken);
		}

		if ( (bPushType == PUSH_TYPE_IOS_GT) || (bPushType == PUSH_TYPE_IOS_BAIDU) ||(bPushType == PUSH_TYPE_IOS_JG) )
		{
			nOS = DEVICE_TYPE_IOS;
		}
		else if ( (bPushType == PUSH_TYPE_ANDROID_GT) || (bPushType == PUSH_TYPE_ANDROID_BAIDU) || (bPushType == PUSH_TYPE_ANDROID_HW) || 
			(bPushType == PUSH_TYPE_ANDROID_MI) || (bPushType == PUSH_TYPE_ANDROID_JG) || (bPushType == PUSH_TYPE_ANDROID_MZ))
		{
			nOS = DEVICE_TYPE_ANDROID;
		}
	}
}

void CDataBaseHandle::MkTokenStr( ClientTokenArray_t& tInfo, CSTRING& strToken )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	bool bFirst = false;
	strToken += "(";
	for (int i = 0; i < tInfo.nCount; i++)
	{
		if (0 == strlen((const char*)tInfo.tToken[i].szToken)) continue;
		CSTRING token; token.assign((const char*)tInfo.tToken[i].szToken);
		BYTE bPushType = tInfo.tToken[i].bPushType;
		if ( (bPushType == PUSH_TYPE_IOS_GT) || (bPushType == PUSH_TYPE_ANDROID_GT) )//个推
		{
			if (bFirst) strToken += " or ";
			strToken = strToken + T_PUSHINFO_GT_TOKEN + "='";
			strToken = strToken + token;
			strToken = strToken + "'";
			bFirst = true;
		}
		else if ( (bPushType == PUSH_TYPE_IOS_BAIDU) || (bPushType == PUSH_TYPE_ANDROID_BAIDU) )//百度
		{
			if (bFirst) strToken += " or ";
			strToken = strToken + T_PUSHINFO_BAIDU_TOKEN + "='";
			strToken = strToken + token;
			strToken = strToken + "'";
			bFirst = true;
		}
		else if ( (bPushType == PUSH_TYPE_IOS_JG) || (bPushType == PUSH_TYPE_ANDROID_JG))//极光
		{
			if (bFirst) strToken += " or ";
			strToken = strToken + T_PUSHINFO_JG_TOKEN + "='";
			strToken = strToken + token;
			strToken = strToken + "'";
			bFirst = true;
		}
		else if ( bPushType == PUSH_TYPE_ANDROID_HW )//华为
		{
			if (bFirst) strToken += " or ";
			strToken = strToken + T_PUSHINFO_HW_TOKEN + "='";
			strToken = strToken + token;
			strToken = strToken + "'";
			bFirst = true;
		}
		else if ( bPushType == PUSH_TYPE_ANDROID_MI )//小米
		{
			if (bFirst) strToken += " or ";
			strToken = strToken + T_PUSHINFO_MI_TOKEN + "='";
			strToken = strToken + token;
			strToken = strToken + "'";
			bFirst = true;
		}
		else if ( bPushType == PUSH_TYPE_ANDROID_MZ )//魅族
		{
			if (bFirst) strToken += " or ";
			strToken = strToken + T_PUSHINFO_MZ_TOKEN + "='";
			strToken = strToken + token;
			strToken = strToken + "'";
			bFirst = true;
		}
	}
	strToken += ")";
	LOG_DEBUG(LOG_DB_SERVER, "%s strToken %s\n", __FUNCTION__, strToken.c_str());
}

bool CDataBaseHandle::IsSameDeviceToken(CSTRING& curToken, CSTRING& setToken)
{
	// 1 当前空   设置有值 false
	// 2 当前空   设置空   true
	// 3 当前有值 设置有值 token一样 true/ token不一样 false
	// 4 当前有值 设置空   true
	
	// 总结1 设置为空时返回true
	if (setToken.empty()) return true;
	// 总结2 设置为有值时token值一样返回true 不一样返回false
	if (0 == setToken.compare(curToken.c_str())) return true;
	return false;
}

// 更新相同PushType(OS)+Token的项(UserID发生变化，之前账号没有退出，使用新账号登录，需要通知之前账号下的设备，PushIndex发生变化)
bool CDataBaseHandle::Insert_PushInfoA(ClientTokenArray_t& tInfo, SamePushInfoItem_t& tItem)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s\n",__FUNCTION__);
	CSTRING strToken; MkTokenStr(tInfo, strToken);
	
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_PUSHINFO_ID << ", " << T_PUSHINFO_USERID << ", " << T_PUSHINFO_GT_TOKEN << ", " << T_PUSHINFO_BAIDU_TOKEN
		<< ", " << T_PUSHINFO_HW_TOKEN << ", " << T_PUSHINFO_MI_TOKEN << ", " << T_PUSHINFO_JG_TOKEN << ", " << T_PUSHINFO_MZ_TOKEN
		<< " FROM " << TABLE_PUSHINFO
		<< " WHERE " << strToken.c_str();

	StoreQueryResult res;
	if (-1 == CatchException(query, res)) {
		LOG_DEBUG(LOG_DB_SERVER,"%s select error\n",__FUNCTION__);
		return false;
	}
	LIST_DWORD lstUserID;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwUserID = (DWORD)row[T_PUSHINFO_USERID];
		LOG_DEBUG(LOG_DB_SERVER, "%s dwUserID %d tInfo.dwUserID %d\n", __FUNCTION__, dwUserID, tInfo.dwUserID);

		if (tInfo.dwUserID != dwUserID) lstUserID.push_back(dwUserID);
		else
		{
			PushTokenType_t tTokenType;
			int nOS = DEVICE_TYPE_ANDROID;
			GetToken(tInfo, tTokenType, nOS);
			LOG_DEBUG(LOG_DB_SERVER,"GT:%s BD:%s HW:%s MI:%s JG:%s MZ:%s\n",
				tTokenType.setGtToken.c_str(), 
				tTokenType.setBaiduToken.c_str(), 
				tTokenType.setHWToken.c_str(), 
				tTokenType.setMIToken.c_str(), 
				tTokenType.setJGToken.c_str(), 
				tTokenType.setMZToken.c_str());

			DWORD dwID = (DWORD)row[T_PUSHINFO_ID];
			CSTRING strGtToken = (CSTRING)row[T_PUSHINFO_GT_TOKEN];
			CSTRING strBaiduToken = (CSTRING)row[T_PUSHINFO_BAIDU_TOKEN];
			CSTRING strHWToken = (CSTRING)row[T_PUSHINFO_HW_TOKEN];
			CSTRING strMIToken = (CSTRING)row[T_PUSHINFO_MI_TOKEN];
			CSTRING strJGToken = (CSTRING)row[T_PUSHINFO_JG_TOKEN];
			CSTRING strMZToken = (CSTRING)row[T_PUSHINFO_MZ_TOKEN];

			bool bSameFlag1 = IsSameDeviceToken(strGtToken, tTokenType.setGtToken);
			bool bSameFlag2 = IsSameDeviceToken(strBaiduToken, tTokenType.setBaiduToken);
			bool bSameFlag3 = IsSameDeviceToken(strHWToken, tTokenType.setHWToken);
			bool bSameFlag4 = IsSameDeviceToken(strMIToken, tTokenType.setMIToken);
			bool bSameFlag5 = IsSameDeviceToken(strJGToken, tTokenType.setJGToken);
			bool bSameFlag6 = IsSameDeviceToken(strMZToken, tTokenType.setMZToken);

			if ( (false == bSameFlag1) || (false == bSameFlag2) || (false == bSameFlag3) || 
				 (false == bSameFlag4) || (false == bSameFlag5) || (false == bSameFlag6))
			{
				PUCHAR pGtToken = (PUCHAR)strGtToken.c_str();
				if (false == bSameFlag1) pGtToken = (PUCHAR)tTokenType.setGtToken.c_str();
				PUCHAR pBaiduToken = (PUCHAR)strBaiduToken.c_str();
				if (false == bSameFlag2) pBaiduToken = (PUCHAR)tTokenType.setBaiduToken.c_str();
				PUCHAR pHWToken = (PUCHAR)strHWToken.c_str();
				if (false == bSameFlag3) pHWToken = (PUCHAR)tTokenType.setHWToken.c_str();
				PUCHAR pMIToken = (PUCHAR)strMIToken.c_str();
				if (false == bSameFlag4) pMIToken = (PUCHAR)tTokenType.setMIToken.c_str();
				PUCHAR pJGToken = (PUCHAR)strJGToken.c_str();
				if (false == bSameFlag5) pJGToken = (PUCHAR)tTokenType.setJGToken.c_str();
				PUCHAR pMZToken = (PUCHAR)strMZToken.c_str();
				if (false == bSameFlag6) pMZToken = (PUCHAR)tTokenType.setMZToken.c_str();
			
				LOG_DEBUG(LOG_DB_SERVER,"GT:%s BD:%s HW:%s MI:%s JG:%s MZ:%s\n",pGtToken, pBaiduToken, pHWToken, pMIToken, pJGToken, pMZToken);
				query << "UPDATE " << TABLE_PUSHINFO
					<< " SET " << T_PUSHINFO_GT_TOKEN << "='" << pGtToken << "',"
					<< T_PUSHINFO_BAIDU_TOKEN << "='" << pBaiduToken << "',"
					<< T_PUSHINFO_HW_TOKEN << "='" << pHWToken << "',"
					<< T_PUSHINFO_MI_TOKEN << "='" << pMIToken << "',"
					<< T_PUSHINFO_JG_TOKEN << "='" << pJGToken << "',"
					<< T_PUSHINFO_MZ_TOKEN << "='" << pMZToken << "',"
					<< T_PUSHINFO_VENDORID << "=" << tInfo.dwVendorID
					<< " WHERE " << strToken.c_str();
				if (-1 == CatchException(query)) return false;
				LOG_DEBUG(LOG_DB_SERVER,"GT:%s BD:%s HW:%s MI:%s JG:%s MZ:%s\n",pGtToken, pBaiduToken, pHWToken, pMIToken, pJGToken, pMZToken);
				tItem.bUpdateFlag = true;
			}
			tItem.bSameFlag = true;
			tItem.dwPushInfoID = dwID;
			LOG_DEBUG(LOG_DB_SERVER, "%s dwPushInfoID %d bSameFlag %d\n", __FUNCTION__, tItem.dwPushInfoID, tItem.bSameFlag);
		}
	}
	
	if (lstUserID.size() <= 0) return true;

	CSTRING strUserID; MkPushUserIDStr(lstUserID, strUserID);

	query << "DELETE FROM " << TABLE_PUSHINFO
		<< " WHERE " << strToken.c_str() << " AND " << strUserID.c_str();
	if (-1 == CatchException(query, res)) return false;

	return Update_UserPush(lstUserID);
}

bool CDataBaseHandle::Delete_PushInfoByIDs( LIST_DWORD& lstID )
{
	if (lstID.size() <= 0) return 0;
	CSTRING strID; MkPushIDStr(lstID, strID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "DELETE FROM " << TABLE_PUSHINFO << " WHERE " << strID.c_str();
	if (-1 == CatchException(query)) return false;
}

bool CDataBaseHandle::Insert_PushInfo(ClientTokenArray_t& tInfo, bool bTry)
{
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s\n", __FUNCTION__);
	bool bSameApp = false;
	if(m_dwView != tInfo.dwView) m_dwView = tInfo.dwView;
	else bSameApp = true;
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s bSameApp=%d\n", __FUNCTION__, bSameApp);
	SamePushInfoItem_t tItem; memset(&tItem, 0, sizeof(SamePushInfoItem_t));
	Insert_PushInfoA(tInfo, tItem);
	if (bTry && tItem.bSameFlag)
	{
		if (tItem.bUpdateFlag) Update_UserPush(tInfo.dwUserID);
		LOG_DEBUG(LOG_DB_SERVER, "%s: Try But Same Exist\n", __FUNCTION__); return true;
	}
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	//////////////////////////////////////////////////////////////////////////
	PushTokenType_t tTokenType;
	int nOS = DEVICE_TYPE_ANDROID;
	GetToken(tInfo, tTokenType, nOS);
	LOG_DEBUG(LOG_DB_SERVER,"1 Gt:%s BD:%s HW:%s JG:%s MI:%s MZ:%s\n",
		tTokenType.setGtToken.c_str(), tTokenType.setBaiduToken.c_str(), tTokenType.setHWToken.c_str(), tTokenType.setJGToken.c_str(), tTokenType.setMIToken.c_str(), tTokenType.setMZToken.c_str());
	//新加代码
	if (bSameApp)
	{
		query << "SELECT " << T_PUSHINFO_GT_TOKEN  << "," << T_PUSHINFO_BAIDU_TOKEN << "," << T_PUSHINFO_HW_TOKEN << ","
			<< T_PUSHINFO_MI_TOKEN << "," << T_PUSHINFO_JG_TOKEN << "," << T_PUSHINFO_MZ_TOKEN
			<< " FROM " << TABLE_PUSHINFO 
			<< " WHERE " << T_PUSHINFO_USERID << "=" << tInfo.dwUserID;
		StoreQueryResult res2;
		if (-1 == CatchException(query, res2)) return false;
		StoreQueryResult::iterator iter = res2.begin();
		for (; iter != res2.end(); iter++)
		{
			Row row = *iter;
			std::string strGt	= (std::string)row[T_PUSHINFO_GT_TOKEN];
			std::string strBaidu = (std::string)row[T_PUSHINFO_BAIDU_TOKEN];
			std::string strHW = (std::string)row[T_PUSHINFO_HW_TOKEN];
			std::string strMI	 = (std::string)row[T_PUSHINFO_MI_TOKEN];
			std::string strJG	 = (std::string)row[T_PUSHINFO_JG_TOKEN];
			std::string strMZ = (std::string)row[T_PUSHINFO_MZ_TOKEN];
			LOG_DEBUG(LOG_DB_SERVER,"2 Gt:%s BD:%s HW:%s JG:%s MI:%s MZ:%s\n",tTokenType.setGtToken.c_str(), tTokenType.setBaiduToken.c_str(), tTokenType.setHWToken.c_str(), tTokenType.setJGToken.c_str(), tTokenType.setMIToken.c_str(), tTokenType.setMZToken.c_str());
			if(tTokenType.setGtToken.size() == 0)		{	tTokenType.setGtToken = strGt;	}
			if(tTokenType.setBaiduToken.size() == 0)	{	tTokenType.setBaiduToken = strBaidu;	}
			if(tTokenType.setHWToken.size() == 0)		{	tTokenType.setHWToken = strHW;	}
			if(tTokenType.setMIToken.size() == 0)		{	tTokenType.setMIToken = strMI;	}
			if(tTokenType.setJGToken.size() == 0)		{	tTokenType.setJGToken = strJG;	}
			if(tTokenType.setMZToken.size() == 0)		{	tTokenType.setMZToken = strMZ;	}
		}
	}
//	LOG_DEBUG(LOG_DB_SERVER,"<old> Gt:%s BD:%s HW:%s JG:%s MI:%s MZ:%s\n",tTokenType.setGtToken.c_str(), tTokenType.setBaiduToken.c_str(), tTokenType.setHWToken.c_str(),
//		tTokenType.setJGToken.c_str(), tTokenType.setMIToken.c_str(), tTokenType.setMZToken.c_str());
	//////////////////////////////////////////////////////////////////////////
	query << "SELECT " << T_PUSHINFO_ID << "," << T_PUSHINFO_CREATED
		<< " FROM " << TABLE_PUSHINFO
		<< " WHERE " << T_PUSHINFO_USERID << "=" << tInfo.dwUserID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() > 0)
	{
		if (bTry)
		{
			tInfo.bLoginOtherPlaceFlag = 1;
			Row row = *(res.begin());
			CSTRING strCreated = (std::string)row[T_PUSHINFO_CREATED];
			int nCpyLen = LENGTH_TIMESTAMP > strCreated.size() ? strCreated.size() : LENGTH_TIMESTAMP;
			memcpy(tInfo.szCreated, strCreated.c_str(), nCpyLen);

			LOG_DEBUG(LOG_DB_SERVER, "%s: LoginOtherPlace szCreated %s\n", __FUNCTION__, tInfo.szCreated);
			return true;
		}

		//////////////////////////////////////////////////////////////////////////
		// 获取本地时间
		GenerateTimeStamp((PUCHAR)tInfo.szCreated);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// 删除该UserID下的其他Token值的条目
		LIST_DWORD lstID;
		if (tItem.bSameFlag)
		{
			if (res.size() > 1)
			{
				StoreQueryResult::iterator iter = res.begin();
				for (; iter != res.end(); iter++)
				{
					Row row = *iter;
					DWORD dwID = (DWORD)row[T_PUSHINFO_ID];
					if (dwID != tItem.dwPushInfoID) lstID.push_back(dwID);
				}
				Delete_PushInfoByIDs(lstID);
			}
			if (tItem.bUpdateFlag) Update_UserPush(tInfo.dwUserID);
			return true;
		}
		tInfo.bLoginOtherPlaceFlag = 1;
		query << "DELETE FROM " << TABLE_PUSHINFO << " WHERE " << T_PUSHINFO_USERID << "=" << tInfo.dwUserID;
		if (-1 == CatchException(query)) return false;
	}

	//////////////////////////////////////////////////////////////////////////
	query << "INSERT INTO " << TABLE_PUSHINFO
			<< " (" << T_PUSHINFO_GT_TOKEN << "," 
			<< T_PUSHINFO_BAIDU_TOKEN << "," 
			<< T_PUSHINFO_HW_TOKEN << "," 
			<< T_PUSHINFO_MI_TOKEN << "," 
			<< T_PUSHINFO_JG_TOKEN << ","
			<< T_PUSHINFO_MZ_TOKEN << "," 
			<< T_PUSHINFO_USERID << "," 
			<< T_PUSHINFO_VENDORID << "," 
			<< T_PUSHINFO_OS << "," 
			<< T_PUSHINFO_LANGUAGE 
			<< ") VALUES ( '"
			<< tTokenType.setGtToken.c_str() << "','" 
			<< tTokenType.setBaiduToken.c_str() << "','" 
			<< tTokenType.setHWToken.c_str() << "','" 
			<< tTokenType.setMIToken.c_str() << "','" 
			<< tTokenType.setJGToken.c_str() << "','" 
			<< tTokenType.setMZToken.c_str() << "',"
			<< tInfo.dwUserID << "," 
			<< tInfo.dwVendorID << "," 
			<< nOS << "," 
			<< (int)tInfo.bLanguage << ")";

	if (-1 == CatchException(query)) return false;
	LOG_DEBUG(LOG_DB_SERVER, "INSERT GtToken %s BaiduToken %s HWToken %s MIToken %s JGToken %s MZToken %s UserID %d VendorID %d OS %d Language %d\n",
		tTokenType.setGtToken.c_str(), tTokenType.setBaiduToken.c_str(), tTokenType.setHWToken.c_str(), tTokenType.setMIToken.c_str(), tTokenType.setJGToken.c_str(), 
		tTokenType.setMZToken.c_str(), tInfo.dwUserID, tInfo.dwVendorID, nOS, tInfo.bLanguage);
	return Update_UserPush(tInfo.dwUserID);
}

bool CDataBaseHandle::Delete_PushInfo(ClientTokenArray_t& tInfo)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s: UserID %d\n", __FUNCTION__, tInfo.dwUserID);
	CSTRING strToken; MkTokenStr(tInfo, strToken);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "DELETE FROM " << TABLE_PUSHINFO
		<< " WHERE " << T_PUSHINFO_USERID << "=" << tInfo.dwUserID << " AND " << strToken.c_str();
		//<< " AND " << T_PUSHINFO_VENDORID << "=" << tInfo.dwVendorID;
		//<< " AND " << T_PUSHINFO_OS << "=" << (int)tInfo.bPushType
		//<< " AND " << T_PUSHINFO_GT_TOKEN << "='" << tInfo.szToken << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	return Update_UserPush(tInfo.dwUserID);
}

bool CDataBaseHandle::Query_DeviceIDByUserID(DWORD dwUserID, LIST_DWORD& lstDeviceID)
{
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT DISTINCT " << TABLE_DEVICE << "." << T_DEVICE_ID
		<< " FROM " << TABLE_DEVICE << "," << TABLE_USERDEVICE
		<< " WHERE " << TABLE_USERDEVICE << "." << T_USERDEVICE_USERID << "=" << dwUserID
		<< " AND " << TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID << "=" << TABLE_DEVICE << "." << T_DEVICE_ID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwDeviceID = (DWORD)row[T_DEVICE_ID];

		LOG_DEBUG(LOG_DB_SERVER, "%s UserID %d DeviceID %d\n", __FUNCTION__, dwUserID, dwDeviceID);
		lstDeviceID.push_back(dwDeviceID);
	}
	return true;
}

bool CDataBaseHandle::Update_DeviceOwnerIDGroupID(DWORD dwDeviceID, DWORD dwOwnerID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwDeviceID %d dwOwnerID %d\n", __FUNCTION__, dwDeviceID, dwOwnerID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "UPDATE " << TABLE_DEVICE
		<< " SET " << T_DEVICE_OWNERID << "=" << dwOwnerID;
	if (dwOwnerID == 0) query << "," << T_DEVICE_GROUPID << "=0";
	query << " WHERE " << T_DEVICE_ID << "=" << dwDeviceID;
	if (-1 == CatchException(query)) return false;
	return true;
}

void CDataBaseHandle::Notify_UserConfigureIndex(DWORD dwUserID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s UserID %d\n", __FUNCTION__, dwUserID);
	if (m_pSink) m_pSink->OnUserConfigureIndex(0, dwUserID);
}

void CDataBaseHandle::Notify_UpdateDeviceRoom(DWORD dwDeviceID, LIST_DWORD& lstRoomID, BYTE bType)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s DeviceID %d RoomID count %d Type %d\n", __FUNCTION__, dwDeviceID, lstRoomID.size(), bType);
	if (m_pSink) m_pSink->OnUpdateDeviceRoom(0, dwDeviceID, lstRoomID, bType);
}

void CDataBaseHandle::Notify_ClearRooms(DWORD dwDeviceID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s DeviceID %d\n", __FUNCTION__, dwDeviceID);
	if (m_pSink) m_pSink->OnClearRooms(0, dwDeviceID);
}

void CDataBaseHandle::Notify_UpdateDevice(DWORD dwDeviceID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s DeviceID %d\n", __FUNCTION__, dwDeviceID);
	if (m_pSink) m_pSink->OnUpdateDevice(0, dwDeviceID);
}

bool CDataBaseHandle::Query_DserverConfigureIndex( DWORD dwVendorID, MAP_DWORD& mapInfo )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s VendorID %d\n", __FUNCTION__, dwVendorID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DSERVERINDEX_VENDORID << "," << T_DSERVERINDEX_INDEX << " FROM " << TABLE_DSERVERINDEX;
	if (dwVendorID)
	{
		query << " WHERE " << T_DSERVERINDEX_VENDORID << "=" << dwVendorID;
	}
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwVendorID1 = (DWORD)row[T_DSERVERINDEX_VENDORID];
		DWORD dwIndex = (DWORD)row[T_DSERVERINDEX_INDEX];
		LOG_DEBUG(LOG_DB_SERVER, "VendorID1 %d Index %d\n", dwVendorID1, dwIndex);

		mapInfo.insert(std::make_pair(dwVendorID1, dwIndex));
	}
	return true;
}

bool CDataBaseHandle::Delete_DeviceUser(DWORD dwUserID, DWORD dwDeviceID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s UserID %d DeviceID %d\n", __FUNCTION__, dwUserID, dwDeviceID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICE_OWNERID << " FROM " << TABLE_DEVICE << " WHERE " << T_DEVICE_ID << "=" << dwDeviceID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0) return false;
	Row row = *(res.begin());
	DWORD dwOwnerID = (DWORD)row[T_DEVICE_OWNERID];

	if (dwOwnerID != dwUserID) return CancelAuthorize(dwUserID, dwDeviceID);
	return DeleteDeviceInUser(dwUserID, dwDeviceID);
}

// 授权
// 入参dwOwnerID   主账号UserID
// 入参pUser       授权账号
// 入参dwDeviceID  授权设备
// 入参dwUserID    授权账号UserID
bool CDataBaseHandle::Insert_UserDevice(DWORD dwOwnerID, PUCHAR pUser, DWORD dwDeviceID, DWORD& dwUserID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwOwnerID %d User %s dwDeviceID %d\n", __FUNCTION__, dwOwnerID, pUser, dwDeviceID);
	if (strlen((const char*)pUser) <= 0) return false;

	// 1、检查授权账号是否存在
	PUCHAR p = (PUCHAR)strstr((const char*)pUser, "@");
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_USER_ID << " FROM " << TABLE_USER
		<< " WHERE " << T_USER_NAME << "='" << pUser << "' OR ";
	if (p) query << T_USER_EMAIL;
	else query << T_USER_MOBILEPHONE;
	query << "='" << pUser << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "Insert_UserDevice Not Exist (User Name %s)\n", pUser);
		m_nError = DATABASE_HANDLE_ERROR_NOT_EXIST; return false;
	}

	// 2、检查授权账号UserID是否与OwnerID相同，相同表示已经为我的设备，不需要授权
	Row row = *(res.begin());
	dwUserID = (DWORD)row[T_USER_ID];
	if (dwOwnerID == dwUserID)
	{
		LOG_DEBUG(LOG_DB_SERVER, "Insert_UserDevice MyDevice\n"); return true;
	}

	// 3、检查OwnerID是否与设备OwnerID相同
	query << "SELECT " << T_DEVICE_OWNERID << " FROM " << TABLE_DEVICE << " WHERE " << T_DEVICE_ID << "=" << dwDeviceID;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "Insert_UserDevice Device Not Exist (DevID %s)\n", dwDeviceID);
		m_nError = DATABASE_HANDLE_ERROR_NOT_EXIST; return false;
	}
	row = *(res.begin());
	DWORD dwDevOwnerID = (DWORD)row[T_DEVICE_OWNERID];
	if (dwDevOwnerID != dwOwnerID)
	{
		LOG_DEBUG(LOG_DB_SERVER, "Insert_UserDevice Device OwnerID(%d) is not %d\n", dwDevOwnerID, dwOwnerID);
		m_nError = DATABASE_HANDLE_ERROR_NOT_EXIST; return false;
	}

	DWORD dwRoomID = 0;
	BYTE szDeviceName[LENGTH_NAME+1] = {0};
	// 4、检查授权账号是否有该设备，需要不存在
	if (Query_UserHasDevice(dwUserID, dwDeviceID, dwRoomID, (PUCHAR)szDeviceName)) return true;

	// 5、检查主账号是否有该设备，需要存在
	if (false == Query_UserHasDevice(dwOwnerID, dwDeviceID, dwRoomID, (PUCHAR)szDeviceName)) return false;


	// 数据库操作1、插入到userdevice
	query << "INSERT INTO " << TABLE_USERDEVICE
		<< " (" << T_USERDEVICE_USERID << "," << T_USERDEVICE_DEVICEID << "," << T_USERDEVICE_DEVICENAME << "," << T_USERDEVICE_ROOMID
		<< ") VALUES ("
		<< dwUserID << "," << dwDeviceID << ",'" << szDeviceName << "'," << dwRoomID << ")";
	if (-1 == CatchException(query)) return false;

	// 数据库操作2、更新deviceroom表格中的userindex以及pushindex
	LIST_DWORD lstRoomID; lstRoomID.push_back(dwRoomID);
	Update_RoomUserIndexPushIndex(lstRoomID);

	// 数据库操作3、更新user表格中configureindex
	SET_DWORD setUserID; setUserID.insert(dwUserID);
	Update_UserConfigureIndex(setUserID);

	// 通知
	BYTE bType = UpdateDevRoomType_UserIndex | UpdateDevRoomType_PushIndex;
	Notify_UpdateDeviceRoom(dwDeviceID, lstRoomID, bType);

	return true;
}

// 授权（冠林SDK）
// 入参dwUserID   授权账号ID
// 入参dwDeviceID 授权设备ID
// 入参pDevName   授权设备名称
// 入参pRoom      授权账号所在房号
bool CDataBaseHandle::Insert_UserDevice(DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDevName, PUCHAR pRoom)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwUserID %d dwDeviceID %d pDevName %s pRoom %s\n", __FUNCTION__, dwUserID, dwDeviceID, pDevName, pRoom);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICE_OWNERID << " FROM " << TABLE_DEVICE << " WHERE " << T_DEVICE_ID << "=" << dwDeviceID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "Insert_UserDevice Device Not Exist (DevID %s)\n", dwDeviceID);
		m_nError = DATABASE_HANDLE_ERROR_NOT_EXIST; return false;
	}

	Row row = *(res.begin());
	DWORD dwDevOwnerID = (DWORD)row[T_DEVICE_OWNERID];
	if ( (dwDevOwnerID == 0) || (dwDevOwnerID == dwUserID) )
	{
		LOG_DEBUG(LOG_DB_SERVER, "Insert_UserDevice Device Wrong OwnerID(%d)\n", dwDevOwnerID);
		m_nError = DATABASE_HANDLE_ERROR_NOT_EXIST; return false;
	}

	Insert_Room(dwDeviceID, pRoom);

	// 取roomid
	query << "SELECT " << T_DEVICEROOM_ROOMID
		<< " FROM " << TABLE_DEVICEROOM
		<< " WHERE " << T_DEVICEROOM_DEVICEID << "=" << dwDeviceID
		<< " AND " << T_DEVICEROOM_ROOM << "='" << pRoom << "'";
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "Insert_UserDevice Room Not Exist (DevID %s)\n", dwDeviceID);
		m_nError = DATABASE_HANDLE_ERROR_NOT_EXIST; return false;
	}

	row = *(res.begin());
	DWORD dwRoomID = (DWORD)row[T_DEVICEROOM_ROOMID];

	query << "SELECT " << T_USERDEVICE_DEVICENAME << " FROM " << TABLE_USERDEVICE
		<< " WHERE " << T_USERDEVICE_USERID << "=" << dwUserID
		<< " AND " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID
		<< " AND " << T_USERDEVICE_ROOMID << "=" << dwRoomID;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() > 0)
	{
		row = *(res.begin());
		std::string strDevName = (std::string)row[T_USERDEVICE_DEVICENAME];
		if (0 == strDevName.compare((const char*)pDevName))
		{
			// 数据库操作3、更新user表格中configureindex
			SET_DWORD setUserID; setUserID.insert(dwUserID);
			Update_UserConfigureIndex(setUserID);

			//Notify_UserConfigureIndex(dwUserID);
			return true;
		}
		query << "UPDATE " << TABLE_USERDEVICE
			<< " SET " << T_USERDEVICE_DEVICENAME << "='" << pDevName << "'"
			<< " WHERE " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID
			<< " AND " << T_USERDEVICE_USERID << "=" << dwUserID
			<< " AND " << T_USERDEVICE_ROOMID << "=" << dwRoomID;
		if (-1 == CatchException(query)) return false;
		return true;
	}
	
	// 数据库操作1、插入到userdevice
	query << "INSERT INTO " << TABLE_USERDEVICE
		<< " (" << T_USERDEVICE_USERID << "," << T_USERDEVICE_DEVICEID << "," << T_USERDEVICE_DEVICENAME << "," << T_USERDEVICE_ROOMID
		<< ") VALUES ("
		<< dwUserID << "," << dwDeviceID << ",'" << pDevName << "'," << dwRoomID << ")";
	if (-1 == CatchException(query)) return false;

	// 数据库操作2、更新deviceroom表格中的userindex以及pushindex
	LIST_DWORD lstRoomID; lstRoomID.push_back(dwRoomID);
	Update_RoomUserIndexPushIndex(lstRoomID);

	// 数据库操作3、更新user表格中configureindex
	SET_DWORD setUserID; setUserID.insert(dwUserID);
	Update_UserConfigureIndex(setUserID);

	// 通知
	BYTE bType = UpdateDevRoomType_UserIndex | UpdateDevRoomType_PushIndex;
	Notify_UpdateDeviceRoom(dwDeviceID, lstRoomID, bType);

	return true;
}

bool CDataBaseHandle::Query_PieceOfSerialNO( LIST_PIECEOFSERIALNO& listInfo )
{
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_PIECEOFSERIALNO_BEGIN << "," << T_PIECEOFSERIALNO_END << "," << T_PIECEOFSERIALNO_VENDORID << " FROM " << TABLE_PIECEOFSERIALNO;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		PieceOfSerialNO_t tInfo;
		tInfo.dwBegin = (DWORD)row[T_PIECEOFSERIALNO_BEGIN];
		tInfo.dwEnd = (DWORD)row[T_PIECEOFSERIALNO_END];
		tInfo.dwVendorID = (DWORD)row[T_PIECEOFSERIALNO_VENDORID];
		listInfo.push_back(tInfo);
	}
	return true;
}

void CDataBaseHandle::OnTimer()
{
	Keepalive();
}

void CDataBaseHandle::OnTimer(TimerReason_e eReason, ITimerSink* pSink)
{
	if(eReason == TIMER_SPECIALCROWD)
	{
//		Update_SpecialCrowdList();
	}
}

bool CDataBaseHandle::Update_DeviceNameSub( DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName )
{
	// 查找所有设备名不曾改变的房号
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_USERDEVICE_ROOMID << " FROM " << TABLE_USERDEVICE
		<< " WHERE " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID
		<< " AND " << T_USERDEVICE_USERID << "=" << dwUserID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	LIST_DWORD lstRoomID;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwRoomID = (DWORD)row[T_USERDEVICE_ROOMID];
		lstRoomID.push_back(dwRoomID);
	}

	// 更新User表格Index
	SET_DWORD setUserID;
	setUserID.insert(dwUserID);
	Update_UserConfigureIndex(setUserID);

	// 更新房号UserIndex
	Update_RoomUserIndex(lstRoomID);

	// 更新所有设备名不曾改变的房号
	query << "UPDATE " << TABLE_USERDEVICE
		<< " SET " << T_USERDEVICE_DEVICENAME << "='" << pDeviceName << "'"
		<< " WHERE " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID
		<< " AND " << T_USERDEVICE_USERID << "=" << dwUserID;
	if (-1 == CatchException(query)) return false;

	Notify_UpdateDeviceRoom(dwDeviceID, lstRoomID, UpdateDevRoomType_UserIndex);
	return true;
}

bool CDataBaseHandle::Update_DeviceNameMain( DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName )
{
	DWORD dwRoomID = 0;
	BYTE szDeviceName[LENGTH_NAME+1] = {0};
	if (false == Query_UserHasDevice(dwUserID, dwDeviceID, dwRoomID, (PUCHAR)szDeviceName)) return false;

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_USERDEVICE_USERID << "," << T_USERDEVICE_ROOMID << " FROM " << TABLE_USERDEVICE
		<< " WHERE " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID
		<< " AND " << T_USERDEVICE_DEVICENAME << "='" << szDeviceName << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	LIST_DWORD lstRoomID;
	SET_DWORD setUserID;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;

		DWORD dwRoomID = (DWORD)row[T_USERDEVICE_ROOMID];
		DWORD dwTempUserID = (DWORD)row[T_USERDEVICE_USERID];
		lstRoomID.push_back(dwRoomID);
		setUserID.insert(dwTempUserID);
	}

	// 更新User表格Index
	Update_UserConfigureIndex(setUserID);

	// 更新房号UserIndex
	Update_RoomUserIndex(lstRoomID);

	// 更新所有设备名不曾改变的房号
	query << "UPDATE " << TABLE_USERDEVICE
		<< " SET " << T_USERDEVICE_DEVICENAME << "='" << pDeviceName << "'"
		<< " WHERE " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID
		<< " AND " << T_USERDEVICE_DEVICENAME << "='" << szDeviceName << "'";
	if (-1 == CatchException(query)) return false;

	Notify_UpdateDeviceRoom(dwDeviceID, lstRoomID, UpdateDevRoomType_UserIndex);
	return true;
}

bool CDataBaseHandle::Update_DeviceName( DWORD dwVendorID, DWORD dwUserID, DWORD dwDeviceID, PUCHAR pDeviceName )
{
	LOG_DEBUG(LOG_DB_SERVER, "UserID %d DeviceID %d DeviceName %s\n", dwUserID, dwDeviceID, pDeviceName);
	if (strlen((const char*)pDeviceName) <= 0) return false;

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICE_OWNERID << " FROM " << TABLE_DEVICE << " WHERE " << T_DEVICE_ID << "=" << dwDeviceID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0) return false;
	Row row = *(res.begin());
	DWORD dwOwnerID = (DWORD)row[T_DEVICE_OWNERID];

	if (dwOwnerID != dwUserID) return Update_DeviceNameSub(dwVendorID, dwUserID, dwDeviceID, pDeviceName);
	return Update_DeviceNameMain(dwVendorID, dwUserID, dwDeviceID, pDeviceName);

}

int CDataBaseHandle::CatchException(Query& query)
{
	try {
		query.store();
	}
	catch (const mysqlpp::BadQuery& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle any query errors
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s BadQuery\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::BadConversion& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle bad conversions; e.g. type mismatch populating 'stock'
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s BadConversion\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::Exception& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Catch-all for any other MySQL++ exceptions
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s Any Other Exception\n", __FUNCTION__);
		return -1;
	}
	return 0;
}
int CDataBaseHandle::CatchException(Query& query, StoreQueryResult& res)
{
	try {
		res = query.store();
	}
	catch (const mysqlpp::BadQuery& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle any query errors
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s BadQuery\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::BadConversion& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Handle bad conversions; e.g. type mismatch populating 'stock'
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s BadConversion\n", __FUNCTION__);
		return -1;
	}
	catch (const mysqlpp::Exception& er) {
		m_nError = DATABASE_HANDLE_EXCEPTION;
		// Catch-all for any other MySQL++ exceptions
		LOG_ERR(LOG_DB_SERVER, "CDataBaseHandle::%s Any Other Exception\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

void CDataBaseHandle::MkRoomIDStr(LIST_DWORD& lstRoomID, CSTRING& strRoomID, bool bTableFlag /* = true */)
{
	strRoomID += "(";
	LIST_DWORD::iterator iter = lstRoomID.begin();
	for (; iter != lstRoomID.end();)
	{
		DWORD dwRoomID = *iter;
		BYTE szTemp[20] = {0};
		memset(szTemp, 0, 20);
		sprintf((char*)szTemp, "%lu", dwRoomID);
		std::string strTemp; strTemp.assign((const char*)szTemp);
		if (bTableFlag) strRoomID = strRoomID + TABLE_DEVICEROOM + "." + T_DEVICEROOM_ROOMID + "=";
		else strRoomID = strRoomID + T_DEVICEROOM_ROOMID + "=";
		strRoomID = strRoomID + strTemp;
		if(++iter != lstRoomID.end()) strRoomID += " or "; 
	}
	strRoomID += ")";
//	LOG_DEBUG(LOG_DB_SERVER, "MkRoomIDStr %s\n", strRoomID.c_str());
}

bool CDataBaseHandle::Query_DeviceMainRoomSum( DWORD dwDeviceID, RoomSum_t& tRoomSum )
{
	PUCHAR pRoom = (PUCHAR)VILLA_ROOM;
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICEROOM_ROOMID << ","
		<< T_DEVICEROOM_ROOM << ","
		<< T_DEVICEROOM_PASSWORD << ","
		<< T_DEVICEROOM_USERINDEX << ","
		<< T_DEVICEROOM_CARDINDEX << ","
		<< T_DEVICEROOM_PUSHINDEX
		<< " FROM " << TABLE_DEVICEROOM
		<< " WHERE " << T_DEVICEROOM_DEVICEID << "=" << dwDeviceID << " AND " << T_DEVICEROOM_ROOM << "='" << pRoom << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <=0) return true;

	Row row = *(res.begin());

	tRoomSum.dwRoomID = (DWORD)row[T_DEVICEROOM_ROOMID];
	tRoomSum.dwPushIndex = (DWORD)row[T_DEVICEROOM_PUSHINDEX];
	tRoomSum.dwUserIndex = (DWORD)row[T_DEVICEROOM_USERINDEX];
	tRoomSum.dwCardIndex = (DWORD)row[T_DEVICEROOM_CARDINDEX];
	std::string strRoom = (std::string)row[T_DEVICEROOM_ROOM];
	int nCpyLen = strRoom.size() < LENGTH_ROOM ? strRoom.size() : LENGTH_ROOM;
	memcpy(tRoomSum.szRoom, strRoom.c_str(), nCpyLen);

	std::string strPwd = (std::string)row[T_DEVICEROOM_PASSWORD];
//	LOG_DEBUG(LOG_DB_SERVER,"SelectPwdMark=5=>%s\n",strPwd.c_str());
	SelectPwdMark(strPwd);
//	LOG_DEBUG(LOG_DB_SERVER,"SelectPwdMark=5=>%s\n",strPwd.c_str());
	nCpyLen = strPwd.size() < LENGTH_ROOMPWD ? strPwd.size() : LENGTH_ROOMPWD;
	memcpy(tRoomSum.szPassword, strPwd.c_str(), nCpyLen);

	LOG_DEBUG(LOG_DB_SERVER, "Query_DeviceMainRoomSum RoomID %d UserIndex %d PushIndex %d CardIndex %d Room %s Pwd %s\n",
		tRoomSum.dwRoomID, tRoomSum.dwUserIndex, tRoomSum.dwPushIndex, tRoomSum.dwCardIndex, tRoomSum.szRoom, tRoomSum.szPassword);
	return true;
}

bool CDataBaseHandle::Query_UserHasDevice( DWORD dwUserID, DWORD dwDeviceID, DWORD& dwRoomID, PUCHAR pDeviceName )
{
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_USERDEVICE_ROOMID << "," << T_USERDEVICE_DEVICENAME << " FROM " << TABLE_USERDEVICE
		<< " WHERE " << T_USERDEVICE_USERID << "=" << dwUserID
		<< " AND " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "Query_UserHasDevice User(ID %d) Hasn't Device(ID %d)\n", dwUserID, dwDeviceID);
		m_nError = DATABASE_HANDLE_ERROR_SYSTEM; return false;
	}

	Row row = *(res.begin());
	dwRoomID = (DWORD)row[T_USERDEVICE_ROOMID];
	std::string strDeviceName = (std::string)row[T_DEVICE_NAME];
	int nCpyLen = strDeviceName.size() < LENGTH_NAME ? strDeviceName.size() : LENGTH_NAME;
	memcpy(pDeviceName, strDeviceName.c_str(), nCpyLen);
	LOG_DEBUG(LOG_DB_SERVER, "Query_UserHasDevice User(ID %d) Has Device(ID %d)\n", dwUserID, dwDeviceID);
	return true;
}

bool CDataBaseHandle::Query_UserHasDevice( DWORD dwUserID, DWORD dwDeviceID, LIST_DWORD& lstRoomID )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s UserID %d DeviceID %d\n", __FUNCTION__, dwUserID, dwDeviceID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_USERDEVICE_ROOMID << "," << T_USERDEVICE_DEVICENAME << " FROM " << TABLE_USERDEVICE
		<< " WHERE " << T_USERDEVICE_USERID << "=" << dwUserID
		<< " AND " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwRoomID = (DWORD)row[T_USERDEVICE_ROOMID];
		lstRoomID.push_back(dwRoomID);
	}
	return true;
}

bool CDataBaseHandle::CancelAuthorize( DWORD dwUserID, DWORD dwDeviceID )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s UserID %d DeviceID %d\n", __FUNCTION__, dwUserID, dwDeviceID);
	// 检查授权账号是否有该设备，需要存在
	LIST_DWORD lstRoomID;
	if (false == Query_UserHasDevice(dwUserID, dwDeviceID, lstRoomID)) return true;

	// 数据库操作1、更新user表格中configureindex
	SET_DWORD setUserID; setUserID.insert(dwUserID);
	Update_UserConfigureIndex(setUserID);

	// 数据库操作2、更新user表格中configureindex
	Update_RoomUserIndexPushIndex(lstRoomID);

	// DELETE `userdevice`.*, `deviceroom`.* FROM `userdevice` INNER JOIN `deviceroom` ON `userdevice`.`deviceid`=`deviceroom`.`deviceid` WHERE `userdevice`.`deviceid`=152
	// 数据库操作3、删除
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "DELETE FROM " << TABLE_USERDEVICE << " WHERE " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID
		<< " AND " << T_USERDEVICE_USERID << "=" << dwUserID;
	if (-1 == CatchException(query)) return false;

	// 通知
	BYTE bType = UpdateDevRoomType_UserIndex | UpdateDevRoomType_PushIndex;
	Notify_UpdateDeviceRoom(dwDeviceID, lstRoomID, bType);

	return true;
}

bool CDataBaseHandle::DeleteDeviceInUser( DWORD dwUserID, DWORD dwDeviceID )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s UserID %d DeviceID %d\n", __FUNCTION__, dwUserID, dwDeviceID);
	// 查找该设备所有账号UserID
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_USERDEVICE_USERID << " FROM " << TABLE_USERDEVICE << " WHERE " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	SET_DWORD tSetUserIDs;
	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwTempUserID = (DWORD)row[T_USERDEVICE_USERID];
		tSetUserIDs.insert(dwTempUserID);
	}

	// 数据库操作1、更新user表格中configureindex
	Update_UserConfigureIndex(tSetUserIDs);
	
	// 数据库操作2、更新设备下OwnerID/GroupID
	Update_DeviceOwnerIDGroupID(dwDeviceID, 0);

	// DELETE `userdevice`.*, `deviceroom`.* FROM `userdevice` INNER JOIN `deviceroom` ON `userdevice`.`deviceid`=`deviceroom`.`deviceid` WHERE `userdevice`.`deviceid`=152
	// 数据库操作3、删除所有
	query << "DELETE " << TABLE_USERDEVICE << ".*," << TABLE_DEVICEROOM << ".*"
		<< " FROM " << TABLE_USERDEVICE << " INNER JOIN " << TABLE_DEVICEROOM
		<< " ON " << TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID << "=" << TABLE_DEVICEROOM << "." << T_DEVICEROOM_DEVICEID
		<< " WHERE " << TABLE_USERDEVICE << "." << T_USERDEVICE_DEVICEID << "=" << dwDeviceID;
	if (-1 == CatchException(query)) return false;
	
	// 通知1、通知设备清空所有房号
	Notify_ClearRooms(dwDeviceID);

	// 通知2、通知设备更新设备所有信息
	Notify_UpdateDevice(dwDeviceID);	return true;

	return true;
}

bool CDataBaseHandle::Update_UserConfigureIndex(SET_DWORD& setUserID)
{
	CSTRING strUserID;
	strUserID += "(";
	SET_DWORD::iterator iter = setUserID.begin();
	for (; iter != setUserID.end();)
	{
		DWORD dwUserID = *iter;
		BYTE szTemp[20] = {0};
		memset(szTemp, 0, 20);
		sprintf((char*)szTemp, "%lu", dwUserID);
		std::string strTemp; strTemp.assign((const char*)szTemp);
		strUserID = strUserID + T_USER_ID + "=";
		strUserID = strUserID + strTemp;
		if(++iter != setUserID.end()) strUserID += " or "; 
	}
	strUserID += ")";
	LOG_DEBUG(LOG_DB_SERVER, "%s %s\n", __FUNCTION__, strUserID.c_str());

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "UPDATE " << TABLE_USER << " SET " << T_USER_CONFIGUREINDEX << "=" << T_USER_CONFIGUREINDEX << "+1" << " WHERE " << strUserID.c_str();
	if (-1 == CatchException(query)) return false;
	return true;
}

bool CDataBaseHandle::Update_RoomPushIndex(LIST_DWORD& lstRoomID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s lstRoomID size %d\n", __FUNCTION__, lstRoomID.size());
	if (lstRoomID.size() <= 0) return true;
	CSTRING strRoomID; MkRoomIDStr(lstRoomID, strRoomID, false);
	LOG_DEBUG(LOG_DB_SERVER, "%s %s\n", __FUNCTION__, strRoomID.c_str());

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "UPDATE " << TABLE_DEVICEROOM
		<< " SET " << T_DEVICEROOM_PUSHINDEX << "=" << T_DEVICEROOM_PUSHINDEX << "+1"
		<< " WHERE " << strRoomID.c_str();
	if (-1 == CatchException(query)) return false;

	return true;
}

bool CDataBaseHandle::Update_RoomPushSwitchIndex(LIST_DWORD& lstRoomID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s lstRoomID size %d\n", __FUNCTION__, lstRoomID.size());
	if (lstRoomID.size() <= 0) return true;
	CSTRING strRoomID; MkRoomIDStr(lstRoomID, strRoomID, false);
	LOG_DEBUG(LOG_DB_SERVER, "%s %s\n", __FUNCTION__, strRoomID.c_str());

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "UPDATE " << TABLE_DEVICEROOM
		<< " SET " << T_DEVICEROOM_PUSHSWITCHINDEX << "=" << T_DEVICEROOM_PUSHSWITCHINDEX << "+1"
		<< " WHERE " << strRoomID.c_str();
	if (-1 == CatchException(query)) return false;
	return true;
}

bool CDataBaseHandle::Update_RoomUserIndex(LIST_DWORD& lstRoomID)
{
	CSTRING strRoomID; MkRoomIDStr(lstRoomID, strRoomID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "UPDATE " << TABLE_DEVICEROOM
		<< " SET " << T_DEVICEROOM_USERINDEX << "=" << T_DEVICEROOM_USERINDEX << "+1"
		<< " WHERE " << strRoomID.c_str();
	if (-1 == CatchException(query)) return false;

	return true;
}

bool CDataBaseHandle::Update_RoomUserIndexPushIndex(LIST_DWORD& lstRoomID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	CSTRING strRoomID; MkRoomIDStr(lstRoomID, strRoomID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "UPDATE " << TABLE_DEVICEROOM
		<< " SET " << T_DEVICEROOM_USERINDEX << "=" << T_DEVICEROOM_USERINDEX << "+1,"
		<< T_DEVICEROOM_PUSHINDEX << "=" << T_DEVICEROOM_PUSHINDEX << "+1"
		<< " WHERE " << strRoomID.c_str();
	if (-1 == CatchException(query)) return false;

	return true;
}

bool CDataBaseHandle::Query_DServerInfo(DServerInfo_t& tInfo)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s pSN %s\n", __FUNCTION__, tInfo.szSerialNO);
	if (strlen((const char*)tInfo.szSerialNO) <= 0) return false;

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DSERVER_PERMISSION << "," << T_DSERVER_CAPACITY
		<< " FROM " << TABLE_DSERVER
		<< " WHERE " << T_DSERVER_SVRSN << "='" << tInfo.szSerialNO << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0) return false;
	Row row = *(res.begin());

	tInfo.nPermission = (DWORD)row[T_DSERVER_PERMISSION];
	tInfo.nCapacity = (DWORD)row[T_DSERVER_CAPACITY];
	
	LOG_DEBUG(LOG_DB_SERVER, "%s SN %s nPermission %d nCapacity %d\n", __FUNCTION__, tInfo.szSerialNO, tInfo.nPermission, tInfo.nCapacity);
	return true;
}
//公告
bool CDataBaseHandle::Query_NoticeIndex(DWORD dwVillageId, DWORD& dwNoticeIndex)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s\n",__FUNCTION__);
	if(dwVillageId == 0) return false;

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_NOTICEINDEX
		<< " FROM " << TABLE_PUBLICINDEX
		<< " WHERE " << T_VILLAGEID << "=" << dwVillageId;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0) return false;
	Row row = *(res.begin());
	dwNoticeIndex = (DWORD)row[T_NOTICEINDEX];
	LOG_DEBUG(LOG_DB_SERVER, "%s,%d\n", __FUNCTION__, dwNoticeIndex);
	return true;
}
//广告
bool CDataBaseHandle::Query_AdvertIndex(DWORD dwVillageId, DWORD& dwAdvertIndex)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s\n",__FUNCTION__);
	if(dwVillageId == 0) return false;

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_ADVERTINDEX
		<< " FROM " << TABLE_PUBLICINDEX
		<< " WHERE " << T_VILLAGEID << "=" << dwVillageId;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;

	if (res.size() <= 0) return false;
	Row row = *(res.begin());
	dwAdvertIndex = (DWORD)row[T_ADVERTINDEX];
	LOG_DEBUG(LOG_DB_SERVER, "%s,%d\n", __FUNCTION__, dwAdvertIndex);
	return true;
}
//访客配置
bool CDataBaseHandle::Query_VisitorCfg(DWORD dwVillageId, DWORD dwVisitorCfg)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s\n",__FUNCTION__);
	if(dwVillageId == 0) return false;
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_VISITORCFG
		<< " FROM " << TABLE_PUBLICINDEX
		<< " WHERE " << T_VILLAGEID << "=" << dwVillageId;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0) return false;
	Row row = *(res.begin());
	dwVisitorCfg = (DWORD)row[T_VISITORCFG];
	LOG_DEBUG(LOG_DB_SERVER, "%s,%d\n", __FUNCTION__, dwVisitorCfg);
	return true;
}

bool CDataBaseHandle::Query_UserDeviceRoom(DWORD dwUserID, DWORD dwDeviceID, LIST_DWORD& lstRoomID)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s UserID %d DeviceID %d\n", __FUNCTION__, dwUserID, dwDeviceID);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_USERDEVICE_ROOMID
		<< " FROM " << TABLE_USERDEVICE
		<< " WHERE " << T_USERDEVICE_USERID << "=" << dwUserID
		<< " AND " << T_USERDEVICE_DEVICEID << "=" << dwDeviceID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if ( res.size() <= 0) return false;

	StoreQueryResult::iterator iter = res.begin();
	for (; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwRoomID = (DWORD)row[T_USERDEVICE_ROOMID];
		lstRoomID.push_back(dwRoomID);
	}
	return true;
}

bool CDataBaseHandle::Query_StoreLimit(DWORD dwDeviceID, int& nStoreLimit)
{
//	LOG_DEBUG(LOG_DB_SERVER, "%s DeviceID %d\n", __FUNCTION__, dwDeviceID);
	// SELECT user.storelimit from user, device 
	// where device.deviceid=622 and device.ownerid=user.userid
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << TABLE_USER << "." << T_USER_STORELIMIT
		<< " FROM " << TABLE_USER << "," << TABLE_DEVICE
		<< " WHERE " << TABLE_DEVICE << "." << T_DEVICE_ID << "=" << dwDeviceID
		<< " AND " << TABLE_DEVICE << "." << T_DEVICE_OWNERID << "=" << TABLE_USER << "." << T_USER_ID;
// 		<< " AND " << TABLE_USER << "." << T_USER_TYPE << "=1";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s size %d deviceid: %d \n", __FUNCTION__, res.size(), dwDeviceID);
		return true;
	}
	Row row = *(res.begin());
	nStoreLimit = (int)row[T_USER_STORELIMIT];
	return true;
}

bool CDataBaseHandle::Query_UpdateDeviceCfg()
{
	return true;
}

bool CDataBaseHandle::Query_DeviceCfg(DWORD dwID, int nType, UcpaasInfo_t& tUcpaas)
{
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s dwID=%d, nType=%d\n", __FUNCTION__, dwID, nType);
	//SELECT ucpaas.username,ucpaas.password,ucpaasapp.appid FROM device,ucpaas,ucpaasapp WHERE device.deviceid=6 AND device.ownerid=ucpaasapp.userid
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	/*
	query << "SELECT " << TABLE_UCPAAS << "." << T_UCPAAS_USERNAME << ","
					   << TABLE_UCPAAS << "." << T_UCPAAS_PASSWORD << ","
					   << TABLE_UCPAASAPP << "." << T_UCPAASAPP_APPID
		<< " FROM " << TABLE_UCPAAS << "," << TABLE_UCPAASAPP  << "," << TABLE_DEVICE
		<< " WHERE " << TABLE_DEVICE << "." << T_DEVICE_ID << "=" << dwID
		<< " AND " << TABLE_DEVICE << "." << T_DEVICE_OWNERID << "=" << TABLE_UCPAASAPP << "." << T_UCPAASAPP_USERID;
	*/
	query << "SELECT " << TABLE_UCPAAS << "." << T_UCPAAS_USERNAME << ","
									<<TABLE_UCPAAS << "." << T_UCPAAS_PASSWORD << ","
									<<TABLE_UCPAASAPP << "." << T_UCPAASAPP_APPID
			 << " FROM "	<< TABLE_UCPAAS << "," << TABLE_UCPAASAPP;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s size %d\n", __FUNCTION__, res.size());
		return true;
	}
	Row row = *(res.begin());
	std::string strUsername = (std::string)row[T_UCPAAS_USERNAME];
	int nCpyLen = strUsername.size() < LENGTH_UCPAAS_USERNAME ? strUsername.size() : LENGTH_UCPAAS_USERNAME;
	memcpy(tUcpaas.szUsername, strUsername.c_str(), nCpyLen);

	std::string strPassword = (std::string)row[T_UCPAAS_PASSWORD];
	CfgLineEncode((PUCHAR)"dducpaas", strPassword);
	nCpyLen = strPassword.size() < LENGTH_UCPAAS_PASSWORD ? strPassword.size() : LENGTH_UCPAAS_PASSWORD;
	memcpy(tUcpaas.szPassword, strPassword.c_str(), nCpyLen);

	std::string strAppid = (std::string)row[T_UCPAASAPP_APPID];
	nCpyLen = strAppid.size() < LENGTH_UCPAAS_APPID ? strAppid.size() : LENGTH_UCPAAS_APPID;
	memcpy(tUcpaas.szAppid, strAppid.c_str(), nCpyLen);
	return true;
}

bool CDataBaseHandle::Query_VendorPhone(DWORD dwID, int nType, SystemCfg_t& tPhone)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s | dwID:%d | nType:%d \n", __FUNCTION__, dwID, nType);
	//select vendorphonenumber from vendor where vendorid=1;
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_VENDOR_PHONENUMBER << " FROM " << TABLE_VENDOR << " WHERE " << T_VENDOR_VENDORID << "=" << dwID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0)
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s size %d\n", __FUNCTION__, res.size());
		return true;
	}
	Row row = *(res.begin());
	std::string strPhone = (std::string)row[T_VENDOR_PHONENUMBER];
	int nCpyLen = strPhone.size() < LENGTH_SYSTEM_PHONENUMBER ? strPhone.size() : LENGTH_SYSTEM_PHONENUMBER;
	memcpy(tPhone.szPhoneNumber, strPhone.c_str(), nCpyLen);
	return true;
}

bool CDataBaseHandle::Query_UpdatePushSwitch(DWORD dwUserID, int nSwitch)
{
	//SELECT ucpaas.username,ucpaas.password,ucpaasapp.appid FROM device,ucpaas,ucpaasapp WHERE device.deviceid=6 AND device.ownerid=ucpaasapp.userid
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "UPDATE " << TABLE_PUSHINFO
		<< " SET " << T_PUSHINFO_SWITCH << "=" << nSwitch
		<< " WHERE " << T_PUSHINFO_USERID << "=" << dwUserID;
	if (-1 == CatchException(query)) return false;

	// notify devices
	LIST_DWORD lstUserID;
	lstUserID.push_back(dwUserID);
	return Update_UserPushSwitch(lstUserID);
}

//从数据库中查询设备截至日期
bool CDataBaseHandle::Query_DeviceDeadLine(LIST_DWORD& lstDevID,LIST_DEVICE_DEADLINE& devDeadLine)
{
	LOG_DEBUG(LOG_DB_SERVER, "104:%s \n", __FUNCTION__);
	char szDeadLine[9];
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");

	DeviceDeadLineInfo_t tDeviceDeadLine;
//	query << "select calldateline from callquery where deviceid=147 order by calldate desc limit 1";
	LIST_DWORD::iterator iter = lstDevID.begin();
	for( ; iter != lstDevID.end(); iter++)
	{
		query << "SELECT " << T_DEADLINE << " FROM " << T_DEVICE << " WHERE " << T_DEVICEID << "=" << *iter << " ORDER BY " << T_CALLDATE << " DESC LIMIT 1";
		StoreQueryResult res;
		if (-1 == CatchException(query,res)) continue;
		if (res.size() <= 0) continue;

		StoreQueryResult::iterator iter2 = res.begin();
		Row row = *iter2;
		tDeviceDeadLine.dwDeviceID = *iter;
		tDeviceDeadLine.dwDevDeadLine = (DWORD)row[T_DEADLINE];
		devDeadLine.push_back(tDeviceDeadLine);
	}
	return true;
}
// 查询小区设备
bool CDataBaseHandle::Query_GroupDevice(DWORD dwVillageId, LIST_DWORD& lstDevID)
{
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_DEVICE_ID << " FROM " << TABLE_DEVICE << " WHERE " << T_GROUP_ID << "=" << dwVillageId;
	StoreQueryResult res;
	if(-1 == CatchException(query,res)) return false;
	if(res.size() <= 0) {
		LOG_DEBUG(LOG_MAIN,"Query_GroupDevice res.size <= \n");
		return false;
	}

	StoreQueryResult::iterator iter = res.begin();
	for(; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwDeviceId = (DWORD)row[T_DEVICE_ID];
		lstDevID.push_back(dwDeviceId);
	}
	return true;
}
//室内机
bool CDataBaseHandle::Insert_IndoorInfo(PUCHAR pIndoorSN , LIST_BIND_INFO& listBindInfo, BindInfoRep_t& tBindRep)
{
	LOG_DEBUG(LOG_DB_SERVER,"==========================================================\n");
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s   SN:%s\n", __FUNCTION__, pIndoorSN);
	CSTRING strInDoor;
	bool result = CheckBindInfo(pIndoorSN,listBindInfo,tBindRep);
	if(result == false) return false;

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	//select deviceid from device where devicesn = OTOQM2L1EEJ6J62C030K;
	query << "SELECT " << T_DEVICE_ID << " FROM " << TABLE_DEVICE << " WHERE " << T_DEVICE_SN << "=" << "'" <<pIndoorSN << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size()==0) return false;
	StoreQueryResult::iterator iter = res.begin();
	Row row = *iter;
	DWORD dwIndoorID = (DWORD)row[T_DEVICE_ID];
	LOG_DEBUG(LOG_DB_SERVER, "IndoorID=%d\n",dwIndoorID);

	if( false == MkInDoorStr(dwIndoorID, listBindInfo, strInDoor))
	{
		LOG_DEBUG(LOG_DB_SERVER,"IndoorID failed\n");
		tBindRep.dwResult = 4;
		return false;
	}
	LOG_DEBUG(LOG_DB_SERVER,"====>%s\n",strInDoor.c_str());

	//DELETE FROM `indoor` WHERE 1
	query << "DELETE FROM " << TABLE_INDOOR << " WHERE " << T_INDOOR_ID << "=" << dwIndoorID;
	StoreQueryResult res1;
	if (-1 == CatchException(query, res1)) return false;

	//INSERT INTO `indoor`(`serialno`, `deviceid`, `roomid`) VALUES ([value-1],[value-2],[value-3])
	query << "INSERT INTO " << TABLE_INDOOR << "( " << T_INDOOR_ID << "," << T_INDOOR_DEVICEID << "," << T_INDOOR_ROOMID << ") "
		<< "VALUES " << strInDoor.c_str();
	StoreQueryResult res2;
	if (-1 == CatchException(query, res2)) return false;
	
	LOG_DEBUG(LOG_DB_SERVER,"==========================================================\n");
	return true;
}

bool CDataBaseHandle::MkInDoorStr( DWORD dwIndoorID, LIST_BIND_INFO& listBindInfo, CSTRING& strInDoor)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s\n",__FUNCTION__);
	strInDoor += "(";
	LIST_BIND_INFO::iterator iter = listBindInfo.begin();
	for (; iter != listBindInfo.end(); )
	{
		DWORD dwDeviceID = iter->dwDeviceID;
		if(dwDeviceID == dwIndoorID) return false;
		DWORD dwRoomID   = iter->dwRoomID;	
		BYTE szTemp[20] = {0};
		memset(szTemp, 0, 20);

		std::string strTemp, strTemp1, strTemp2;

		sprintf((char*)szTemp, "%lu", dwIndoorID);
		strTemp.assign((const char*)szTemp);

		sprintf((char*)szTemp, "%lu", dwDeviceID);
		strTemp1.assign((const char*)szTemp);

		sprintf((char*)szTemp, "%lu", dwRoomID);
		strTemp2.assign((const char*)szTemp);

		strInDoor = strInDoor + strTemp + "," + strTemp1 + "," + strTemp2;
		if(++iter != listBindInfo.end()) strInDoor += "),("; 
	}
	strInDoor += ")";
	return true;
}
//检查绑定信息合法性
bool CDataBaseHandle::CheckBindInfo(PUCHAR pIndoorSN , LIST_BIND_INFO& listBindInfo, BindInfoRep_t& tBindRep)
{
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s\n", __FUNCTION__);
	//检查SN合法性
	DWORD dwIndoorID = 0;
	bool ret = Query_InDoorID(pIndoorSN,dwIndoorID);
	if(ret == false) 
	{
		tBindRep.dwResult = 1;
		LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s BindDev IndoorID=%d,Result=%d\n", __FUNCTION__, tBindRep.dwIndoorID, tBindRep.dwResult);
		return false;
	}
	tBindRep.dwIndoorID = dwIndoorID;
	//检查DeviceID合法性
	DWORD res = Query_DevRoom(listBindInfo);
	if(res != 0) 
	{
		tBindRep.dwResult = ret;
		LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s BindDev IndoorID=%d,Result=%d\n", __FUNCTION__, tBindRep.dwIndoorID, tBindRep.dwResult);
		return false;
	}
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s BindDev IndoorID=%d,Result=%d\n", __FUNCTION__, tBindRep.dwIndoorID, tBindRep.dwResult);
	return true;
}

int CDataBaseHandle::Query_DevRoom(LIST_BIND_INFO& listBindInfo)
{
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s\n", __FUNCTION__);

	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	LIST_BIND_INFO::iterator iter = listBindInfo.begin();
	for(; iter != listBindInfo.end(); iter++)
	{
		//select count(*) from deviceroom where deviceid = 4;
		query << "SELECT COUNT(*) FROM " << TABLE_DEVICEROOM << " WHERE " << T_DEVICEROOM_DEVICEID << "=" << iter->dwDeviceID;
		
		StoreQueryResult res1;
		if (-1 == CatchException(query, res1)) return 4;
		if(res1.size() <= 0) {
			LOG_DEBUG(LOG_MAIN,"DeviceID Invalid\n");	return 2;
		}
		//select count(*) from deviceroom where roomid = 6;
		query << "SELECT COUNT(*) FROM " << TABLE_DEVICEROOM << " WHERE " << T_DEVICEROOM_ROOMID << "=" << iter->dwRoomID;
		StoreQueryResult res2;
		if (-1 == CatchException(query, res2)) return 4;
		if(res2.size() <= 0) {
			LOG_DEBUG(LOG_MAIN,"RoomID Invalid\n");	return 3;
		}
	}
	return 0;
}
//更新indoorindex+1
bool CDataBaseHandle::UpdateIndoorIndex(LIST_BIND_INFO& listBindInfo)
{
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s\n", __FUNCTION__);
	
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	LIST_BIND_INFO::iterator iter = listBindInfo.begin();
	for(; iter != listBindInfo.end(); iter++)
	{
		LOG_DEBUG(LOG_DB_SERVER,"DeviceID:%d, RoomID:%d\n",iter->dwDeviceID, iter->dwRoomID);
		//UPDATE `deviceroom` SET `indoorindex` = (`indoorindex` + 1) WHERE `roomid`= 501 AND`deviceid`= 147;
		query << "UPDATE " << TABLE_DEVICEROOM 
			<< " SET " << T_DEVICEROOM_INDOORINDEX << "=" << "(" << T_DEVICEROOM_INDOORINDEX << "+ 1" << ")"
			<< " WHERE " << T_DEVICEROOM_DEVICEID << "=" << iter->dwDeviceID 
			<< " AND " << T_DEVICEROOM_ROOMID << "=" << iter->dwRoomID;
		StoreQueryResult res;
		if (-1 == CatchException(query, res)) return false;
	}
	return true;
}

bool CDataBaseHandle::QueryIndoorBindDev(DWORD dwIndoorID, LIST_DEVICEINFO& lstDevInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s 1\n",__FUNCTION__);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_INDOOR_DEVICEID 
		<< " FROM " << TABLE_INDOOR 
		<< " WHERE " << T_INDOOR_ID << "=" << dwIndoorID;
	StoreQueryResult res2;
	if (-1 == CatchException(query, res2)) return false;
	
	StoreQueryResult::iterator iter2 = res2.begin();
	for(; iter2 != res2.end(); iter2++)
	{
		DeviceInfo_t tInfo;
		memset(&tInfo, 0, sizeof(tInfo));
		Row row2 = *iter2;
		tInfo.dwDeviceID = (DWORD)row2[T_INDOOR_DEVICEID];
		LOG_DEBUG(LOG_DB_SERVER,"DeviceID=%d\n",tInfo.dwDeviceID);
		lstDevInfo.push_back(tInfo);
	}
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s 2\n",__FUNCTION__);
	return true;
}
bool CDataBaseHandle::QueryDevInfo(LIST_DEVICEINFO& lstDevInfo)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s\n",__FUNCTION__);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	LIST_DEVICEINFO::iterator iter = lstDevInfo.begin();
	for(; iter != lstDevInfo.end(); iter++)
	{
		DWORD dwDeviceID = iter->dwDeviceID;
		query << "SELECT " << T_DEVICE_VENDORID << "," << T_DEVICE_GROUPID << "," << T_DEVICE_SN << "," << T_DEVICE_NAME
			<< " FROM " << TABLE_DEVICE
			<< " WHERE " << T_DEVICE_ID << "=" << dwDeviceID;
		StoreQueryResult res;
		if (-1 == CatchException(query, res)) return false;
		if (res.size() <= 0) return false;
		Row row = *(res.begin());

		iter->dwDeviceID = dwDeviceID;
		iter->dwVendorID = (DWORD)row[T_DEVICE_VENDORID];
		iter->dwGroupID = (DWORD)row[T_DEVICE_GROUPID];
		std::string strSN = (std::string)row[T_DEVICE_SN];
		int nCpyLen1 = strSN.size() < LENGTH_SERIALNO ? strSN.size() : LENGTH_SERIALNO;
		memcpy(iter->szSerialNO, strSN.c_str(), nCpyLen1);
		std::string strDeviceName = (std::string)row[T_DEVICE_NAME];
		int nCpyLen2 = strDeviceName.size() < LENGTH_NAME ? strDeviceName.size() : LENGTH_NAME;
		memcpy(iter->szDeviceName, strDeviceName.c_str(), nCpyLen2);
	}
	return true;
}
bool CDataBaseHandle::Query_DeviceIndoorID(DWORD dwDeviceID, LIST_DWORD& lstIndoorID)
{
	LOG_DEBUG(LOG_DB_SERVER,"CDataBaseHandle::%s\n",__FUNCTION__);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	query << "SELECT " << T_INDOOR_ID << " FROM " << TABLE_INDOOR << " WHERE " << T_INDOOR_DEVICEID << "=" << dwDeviceID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	StoreQueryResult::iterator iter = res.begin();
	for(; iter != res.end(); iter++)
	{
		Row row = *iter;
		DWORD dwIndoorID = (DWORD)row[T_INDOOR_ID];
		lstIndoorID.push_back(dwIndoorID);
	}
	return true;
}

bool CDataBaseHandle::Query_InDoorID(PUCHAR pIndoorSN, DWORD& dwIndoorID)
{
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s\n", __FUNCTION__);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	//SELECT `deviceid` FROM `device` WHERE `devicesn` = pIndoorSN
	query << "SELECT " << T_INDOOR_DEVICEID << " FROM " << TABLE_DEVICE << " WHERE " << T_DEVICE_SN << " = " <<  "'" << pIndoorSN << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if(res.size() <= 0) {
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);	return false;
	}
	StoreQueryResult::iterator iter = res.begin();
	Row row = *iter;
	dwIndoorID = (DWORD)row[T_INDOOR_DEVICEID];
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s IndoorID=%d\n",__FUNCTION__, dwIndoorID);
	return true;
}
bool CDataBaseHandle::Query_InDoorSN(DWORD dwIndoorID, PUCHAR pIndoorSN)
{
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s\n", __FUNCTION__);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	//SELECT `deviceid` FROM `device` WHERE `devicesn` = pIndoorSN
	query << "SELECT " <<  T_DEVICE_SN << " FROM " << TABLE_DEVICE << " WHERE " << T_INDOOR_DEVICEID  << " = " << dwIndoorID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if(res.size() <= 0) {
		LOG_DEBUG(LOG_MAIN,"%s failed\n",__FUNCTION__);	return false;
	}
	StoreQueryResult::iterator iter = res.begin();
	Row row = *iter;
	std::string strSN = (std::string)row[T_DEVICE_SN];
	memcpy(pIndoorSN, strSN.c_str(), LENGTH_SERIALNO);
	return true;
}

bool CDataBaseHandle::Query_InDoorIDBySN(PUCHAR pIndoorSN, DWORD dwIndoorID)
{
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s IndoorSN %s\n", __FUNCTION__, pIndoorSN);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	//SELECT `deviceid` FROM `device` WHERE `devicesn` = pIndoorSN
	query << "SELECT " << T_INDOOR_DEVICEID << " FROM " << TABLE_DEVICE << " WHERE " << T_DEVICE_SN << " = " <<  "'" << pIndoorSN << "'";
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if(res.size() <= 0) {
		LOG_DEBUG(LOG_DB_SERVER,"%s The query result is empty IndoorID %d\n",__FUNCTION__, dwIndoorID);	return false;
	}
	StoreQueryResult::iterator iter = res.begin();
	Row row = *iter;
	dwIndoorID = (DWORD)row[T_INDOOR_DEVICEID];
	return true;
}
//查询特殊用户列表
bool CDataBaseHandle::Query_SpecialUsers(LIST_SPECIALCROWD& m_lstSpecialCrowd)
{
	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s\n", __FUNCTION__);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	//select deviceroom.roomid, deviceroom.deviceid, userdevice.userid 
	//from deviceroom, userdevice where category = 2 and 
	//userdevice.deviceid = deviceroom.deviceid and userdevice.roomid = deviceroom.roomid;
	query << "SELECT " << TABLE_DEVICEROOM << "." << T_DEVICE_ROOMID << "," 
			<< TABLE_DEVICEROOM << "." << T_DEVICE_ID << "," 
			<< TABLE_USERDEVICE << "." << T_USERDEVICE_USERID << ","
			<< TABLE_USER << "." << T_USER_MOBILEPHONE 
			<< " FROM " << TABLE_DEVICEROOM << "," << TABLE_USERDEVICE << "," << TABLE_USER
			<< " WHERE " << TABLE_DEVICEROOM << "." << T_DEVICEROOM_CATEGORY << "=" << "2" //2 特殊人群
			<< " AND "	<< TABLE_DEVICEROOM << "." << T_DEVICE_ROOMID << "=" << TABLE_USERDEVICE << "." << T_USERDEVICE_ROOMID
			<< " AND "	<< TABLE_DEVICEROOM << "." << T_DEVICE_ID << "=" << TABLE_USERDEVICE << "." << T_DEVICE_ID
			<< " AND "  << TABLE_USERDEVICE << "." << T_USER_ID << "=" << TABLE_USER << "." << T_USER_ID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0) { LOG_DEBUG(LOG_DB_SERVER,"%s The query result is empty\n",__FUNCTION__);	return false;	}

	SpecialCrowd_t tInfo;
	StoreQueryResult::iterator iter = res.begin();
	for(; iter != res.end(); iter++)
	{
		Row row = *iter;
		tInfo.dwDevcieID = (DWORD)row[T_DEVICE_ID];
		tInfo.dwRoomID = (DWORD)row[T_DEVICE_ROOMID];
		tInfo.dwUserID = (DWORD)row[T_USERDEVICE_USERID];
		std::string strPhone = (std::string)row[T_USER_MOBILEPHONE];
		memcpy(tInfo.szUserPhone, strPhone.c_str(), strPhone.size());
		tInfo.dwUnlockTime = 0;
		Query_PropertyPhone(tInfo.dwDevcieID, tInfo);
//		LOG_DEBUG(LOG_DB_SERVER, "%s dwGroupID=%d, dwDeviceID=%d, dwRoomID=%d, dwUserID=%d, strPhone=%s, dwPropertyUserID=%d, strPropertyPhone=%s\n",
//			__FUNCTION__, tInfo.dwGroupID, tInfo.dwDevcieID, tInfo.dwRoomID, tInfo.dwUserID, tInfo.szUserPhone, tInfo.dwPropertyUserID, tInfo.szPropertyPhone);
		m_lstSpecialCrowd.push_back(tInfo);
	}
	return true;
}

bool CDataBaseHandle::Query_PropertyPhone(DWORD dwDeviceID, SpecialCrowd_t& tInfo)
{
//	LOG_DEBUG(LOG_DB_SERVER, "CDataBaseHandle::%s dwDeviceID = %d\n", __FUNCTION__, dwDeviceID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");
	//select device.groupid, groups.userid, user.mobilephone from device, groups, user 
	//where device.deviceid = 441 and groups.groupid = device.groupid and user.usertype = 1 and user.userid = groups.userid;
	query << "SELECT " << TABLE_DEVICE << "." << T_DEVICE_VENDORID << "," 
		<< TABLE_DEVICE << "." << T_DEVICE_GROUPID << ","
		<< TABLE_DEVICE << "." << T_DEVICE_NAME << ","
		<< TABLE_GROUP << "." << T_USER_ID << ","
		<< TABLE_USER << "." << T_USER_MOBILEPHONE
		<< " FROM " << TABLE_DEVICE << "," << TABLE_GROUP << "," << TABLE_USER
		<< " WHERE " << TABLE_DEVICE << "." << T_DEVICE_ID << "=" << dwDeviceID
		<< " AND " << TABLE_GROUP << "." << T_GROUP_ID << "=" << TABLE_DEVICE << "." << T_DEVICE_GROUPID
		<< " AND " << TABLE_USER << "." << T_USER_TYPE << "= 1"
		<< " AND " << TABLE_USER << "." << T_USER_ID << "=" << TABLE_GROUP << "." << T_GROUP_USERID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return false;
	if (res.size() <= 0) { LOG_DEBUG(LOG_DB_SERVER,"%s The query result is empty\n",__FUNCTION__);	return false;	}
	StoreQueryResult::iterator iter = res.begin();
	for(; iter != res.end(); iter++)
	{
		Row row = *iter;
		tInfo.dwVendorID = (DWORD)row[T_DEVICE_VENDORID];
		tInfo.dwGroupID = (DWORD)row[T_DEVICE_GROUPID];
		std::string strDevName = (std::string)row[T_DEVICE_NAME];
		memcpy(tInfo.szDeviceName, strDevName.c_str(), strDevName.size());
		tInfo.dwPropertyUserID = (DWORD)row[T_USER_ID];
		std::string strPhone = (std::string)row[T_USER_MOBILEPHONE];
		memcpy(tInfo.szPropertyPhone, strPhone.c_str(), strPhone.size());
		ProPhone((char*)tInfo.szPropertyPhone);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
int CDataBaseHandle::GetError()
{
	int nError = m_nError;
	m_nError = 0;
	return nError;
}

void CDataBaseHandle::MkUserIDStr( LIST_DWORD& lstUserID, CSTRING& strUserID )
{
	strUserID += "(";
	LIST_DWORD::iterator iter = lstUserID.begin();
	for (; iter != lstUserID.end();)
	{
		DWORD dwRoomID = *iter;
		BYTE szTemp[20] = {0};
		memset(szTemp, 0, 20);
		sprintf((char*)szTemp, "%lu", dwRoomID);
		std::string strTemp; strTemp.assign((const char*)szTemp);
		strUserID = strUserID + T_USERDEVICE_USERID + "=";
		strUserID = strUserID + strTemp;
		if(++iter != lstUserID.end()) strUserID += " or "; 
	}
	strUserID += ")";
}

void CDataBaseHandle::MkPushUserIDStr( LIST_DWORD& lstUserID, CSTRING& strUserID )
{
	strUserID += "(";
	LIST_DWORD::iterator iter = lstUserID.begin();
	for (; iter != lstUserID.end();)
	{
		DWORD dwRoomID = *iter;
		BYTE szTemp[20] = {0};
		memset(szTemp, 0, 20);
		sprintf((char*)szTemp, "%lu", dwRoomID);
		std::string strTemp; strTemp.assign((const char*)szTemp);
		strUserID = strUserID + T_PUSHINFO_USERID + "=";
		strUserID = strUserID + strTemp;
		if(++iter != lstUserID.end()) strUserID += " or "; 
	}
	strUserID += ")";
	LOG_DEBUG(LOG_DB_SERVER, "%s strUserID %s\n", __FUNCTION__, strUserID.c_str());
}

void CDataBaseHandle::MkPushIDStr( LIST_DWORD& lstID, CSTRING& strID )
{
	strID += "(";
	LIST_DWORD::iterator iter = lstID.begin();
	for (; iter != lstID.end();)
	{
		DWORD dwID = *iter;
		BYTE szTemp[20] = {0};
		memset(szTemp, 0, 20);
		sprintf((char*)szTemp, "%lu", dwID);
		std::string strTemp; strTemp.assign((const char*)szTemp);
		strID = strID + T_PUSHINFO_ID + "=";
		strID = strID + strTemp;
		if(++iter != lstID.end()) strID += " or "; 
	}
	strID += ")";
	LOG_DEBUG(LOG_DB_SERVER, "%s strPushID %s\n", __FUNCTION__, strID.c_str());
}

DWORD CDataBaseHandle::QueryStoreIDByUserID( DWORD dwUserID )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s dwUserID %d\n", __FUNCTION__, dwUserID);
	Query query = m_clsSrcDBCon.query();
	query.exec("SET NAMES 'utf8'");

	// 取账号的云存储账号ID
	query << "SELECT " << T_USER_STOREID << "," << T_USER_VENDORID
		<< " FROM " << TABLE_USER
		<< " WHERE " << T_USER_ID << "=" << dwUserID;
	StoreQueryResult res;
	if (-1 == CatchException(query, res)) return 0;
	if (res.size() <= 0) return 0;
	Row row = *(res.begin());
	CSTRING storeid = (CSTRING)row[T_USER_STOREID];
	DWORD dwStoreID = ParseStoreIDStr(storeid);
	LOG_DEBUG(LOG_DB_SERVER, "1 %s storeid %s=>dwStoreID %d\n", __FUNCTION__, storeid.c_str(), dwStoreID);
	if (dwStoreID) return dwStoreID; // 返回账号的云存储账号ID

	// 取上级账号的云存储账号ID
	DWORD dwVendorID = (DWORD)row[T_USER_VENDORID];
	for (int i = 0; i < 5; i++)
	{
		LOG_DEBUG(LOG_DB_SERVER, "%s dwVendorID %d\n", __FUNCTION__, dwVendorID);

		query << "SELECT " << T_VENDOR_STOREID
			<< " FROM " << TABLE_VENDOR
			<< " WHERE " << T_VENDOR_ID << "=" << dwVendorID
			<< " AND " << T_VENDOR_TYPE << "!=1";
		if (-1 == CatchException(query, res)) return 0;

		if (res.size() <= 0) return 0;
		Row row = *(res.begin());
		storeid = (CSTRING)row[T_VENDOR_STOREID];
		dwStoreID = ParseStoreIDStr(storeid);
		LOG_DEBUG(LOG_DB_SERVER, "2 %s storeid %s=>dwStoreID %d\n", __FUNCTION__, storeid.c_str(), dwStoreID);
		if (dwStoreID) return dwStoreID;
	}
	return 0;
}


char CDataBaseHandle::SNCharToVal(char ch)
{
	if (ch >= '0' && ch <= '9'){ return ch - '0'; }
	if (ch <= 'Z' && ch >= 'A'){ return ch - 'A' + 10; }
	if (ch <= 'z' && ch >= 'a'){ return ch - 'a' + 10; }
	return -1;
}

/*********************************************************
 *功能：通过串号查询型号ID
 *返回值：成功返回型号ID
 *		  失败返回0
 *说明：返回值13是七寸屏，15是无屏，17是13寸屏
 *********************************************************/
DWORD CDataBaseHandle::GetDeviceTID(char* pSerialNum)
{
	if(strlen(pSerialNum) < 20)    { return 0; }

	// 检查序列号标记位字符有效性
	if(SNCharToVal(pSerialNum[0+0]) < 0) return 0;
	DWORD tag = (DWORD)(SNCharToVal(pSerialNum[0+0])+ 15) % 36;
	if((3 != tag) && (4 != tag)) return 0;

	// 检查序列号设备型号ID位字符有效性
	unsigned char cIndex = 18; // V3
	if(4 == tag)  cIndex =  8; // V4
	if(SNCharToVal(pSerialNum[cIndex+0]) < 0){ return 0; }
	if(SNCharToVal(pSerialNum[cIndex+1]) < 0){ return 0; }

	DWORD tid = (DWORD)(SNCharToVal(pSerialNum[cIndex+0]));
	tid = (DWORD)(SNCharToVal(pSerialNum[cIndex+1])) + tid * 36;

	return tid;
}
/**********************************************************
 *功能：根据GetDeviceTid的返回值判断设备是否可以打电话
 *返回值：返回1，可以打
		  返回0，不可以
 *说明：暂无
 **********************************************************/
DWORD CDataBaseHandle::PhoneCall(DWORD tid)
{
	if(DEV_SCREEN_SIZE7 == tid){
		printf("7寸屏\n");
		return 1;
	}else if(DEV_SCREEN_SIZE0 == tid){
		printf("7寸屏\n");
		return 1;
	}else if(DEV_SCREEN_SIZE13 == tid){
		printf("7寸屏\n");
		return 1;
	}
	return 0;
}
