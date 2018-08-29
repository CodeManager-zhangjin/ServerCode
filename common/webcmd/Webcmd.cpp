#include "Webcmd.h"
#if defined(WEBCMD_DB)
#include "ServerAppInterface.h"
#elif defined(WEBCMD_ST)
#include "CacheInterface.h"
#endif
#include "putbuffer.h"
#include "getbuffer.h"
#include "Protocol.h"
#include "Log.h"
#include "qiniuInterface.h"

const BYTE GROUPCODE_WEB				= 0x70; // 'p'

// 关键字
#define KEY_SERVICE			"service"
#define KEY_PKT_HEADER		"packetheader"
#define KEY_PACKET_LEN		"packetlen"

#define KEY_VENDORID		"vendorid"
#define KEY_USERID			"userid"
#define KEY_DEVICEID		"deviceid"
#define KEY_INDEX			"index"
#define KEY_ROOMID			"roomid"
#define KEY_TYPE			"type"
#define KEY_ACCESS			"accesskey"
#define KEY_SECRET			"secretkey"
#define KEY_VERSION			"version"
#define KEY_GROUPID			"villageid"
#define KEY_NOTICEINDEX     "noticeindex"
#define KEY_ADVERTINDEX		"advertindex"
#define KEY_ADVERT_TYPE		"advert_type"

// service指令
#define S_PIECE_OF_SERIALNO				"pieceofserialno"
#define S_DSERVER_CONFIGUREINDEX		"dserverindex"
#define S_USER_CONFIGUREINDEX			"userindex"
#define S_ROOM_CONFIGUREINDEX			"roomindex"
#define S_CLEARROOMS					"clearrooms"
#define S_UPDATE_DEVICE					"updatedevice"
#define S_UPDATE_DEVICE_EX				"updatedevice_ex"
#define S_QUERY_DEVICE					"querydevice"
#define S_UPDATE_DEVICE_DEADLINE		"update_device_deadline" 
#define S_GET_QINIU_DOWNLOAD_TOKEN		"get_download_token"
#define S_UPDATE_PROPERTY_ANNOUNCE		"update_property_announce" 
#define S_UPDATE_FIRMWARE_REQUEST		"update_firmware_request"  
#define S_DELETE_DEVICE_ONLINE			"delete_device_online"//两台设备交换后
#define S_UPDATE_ADVERT_INFO			"update_advertising"//更新广告信息



const DWORD TIMER_INTERVAL = 600;

BYTE CWebCmd::m_szBuffer[MAX_PACKET_LEN] = {0};

const CWebCmd::HandlerEntry CWebCmd::m_handlers[] =
{
	{ (PCHAR)S_PIECE_OF_SERIALNO,		&CWebCmd::OnPieceOfSerialNO			},
	{ (PCHAR)S_DSERVER_CONFIGUREINDEX,	&CWebCmd::OnDserverConfigureIndex	},
	{ (PCHAR)S_USER_CONFIGUREINDEX,		&CWebCmd::OnUserConfigureIndex		},
	{ (PCHAR)S_ROOM_CONFIGUREINDEX,		&CWebCmd::OnUpdateDeviceRoom		},
	{ (PCHAR)S_CLEARROOMS,				&CWebCmd::OnClearRooms				},
	{ (PCHAR)S_UPDATE_DEVICE,			&CWebCmd::OnUpdateDevice			},
	{ (PCHAR)S_UPDATE_DEVICE_EX,		&CWebCmd::OnUpdateDeviceEx			},
	{ (PCHAR)S_QUERY_DEVICE,			&CWebCmd::OnQueryDevice				},
	{ (PCHAR)S_UPDATE_DEVICE_DEADLINE,	&CWebCmd::OnUpdateDeviceDeadLine	},
	{ (PCHAR)S_GET_QINIU_DOWNLOAD_TOKEN,&CWebCmd::OnGetQiNiuDownloadToken	},
	{ (PCHAR)S_UPDATE_PROPERTY_ANNOUNCE,&CWebCmd::OnUpdatePropertyAnnounce	},
	{ (PCHAR)S_UPDATE_FIRMWARE_REQUEST, &CWebCmd::OnUpdateFirmwareRequest	},
	{ (PCHAR)S_DELETE_DEVICE_ONLINE,	&CWebCmd::OnDeleteDeviceOnline		},
	{ (PCHAR)S_UPDATE_ADVERT_INFO,		&CWebCmd::OnUpdateAdvertInfo		},
};

CWebCmd::CWebCmd()
{
	m_pCon = NULL;
	AddTimer(TIMER_NORMAL, TIMER_INTERVAL, this);
}

CWebCmd::~CWebCmd()
{
	DelTimer(TIMER_NORMAL, this);
	if (m_pCon) { NetworkDestroyConnection(m_pCon); m_pCon = NULL; }
}

int CWebCmd::AppendData( PUCHAR pData, int nLen )
{
	const char* pHeader = strstr((const char*)pData, KEY_PKT_HEADER);
	if (pHeader) m_strBuffer.clear();
	m_strBuffer.append((const char*)pData, nLen);

	int nPosHeader = m_strBuffer.find(KEY_PKT_HEADER);
	if (nPosHeader == std::string::npos) { m_strBuffer.clear(); LOG_ERR(LOG_DB_SERVER, "AppendData 1\n"); return -1; }

	int nPosPktLen = m_strBuffer.find(KEY_PACKET_LEN);
	if (nPosPktLen == std::string::npos) { m_strBuffer.clear(); LOG_ERR(LOG_DB_SERVER, "AppendData 2\n"); return -1; }

	CSTRING strTemp = m_strBuffer.substr(nPosPktLen + strlen(KEY_PACKET_LEN) + 1);
	int nPosTemp = strTemp.find("&");
	if (nPosTemp == std::string::npos) { m_strBuffer.clear(); LOG_ERR(LOG_DB_SERVER, "AppendData 3\n"); return -1; }

	strTemp = strTemp.substr(0, nPosTemp);
	int nPktLen = atoi((const char*)strTemp.c_str()) - 1;
	LOG_DEBUG(LOG_DB_SERVER, "WebCmd nPktLen %d\n", nPktLen);

	int nPosSevice = m_strBuffer.find(KEY_SERVICE);
	if (nPosSevice == std::string::npos) { m_strBuffer.clear(); LOG_ERR(LOG_DB_SERVER, "AppendData 4\n"); return -1; }
	strTemp = m_strBuffer.substr(nPosSevice);

	//LOG_DEBUG(LOG_DB_SERVER, "WebCmd strTempLen %d\n", strTemp.length());
	// 数据收集完全
	if ( strTemp.length() >= nPktLen )
	{
		ProcessCommand(m_strBuffer); m_strBuffer.clear();
	}
	return 0;
}

int CWebCmd::OnReceive( PUCHAR pData, int nLen, INetConnection* pCon )
{
	try
	{
		AppendData(pData, nLen);
	}
	catch ( CParserException& e )
	{
		e.Descript();
	}
	catch ( ... )
	{
		LOG_ERR(LOG_DB_SERVER, "exception occured in CWebCmd\n");
	}
	return 0;
}

int CWebCmd::ProcessCommand( CSTRING& strData )
{
	LOG_DEBUG(LOG_DB_SERVER, "WebCmd %d:%s\n", strData.length(), strData.c_str());
	//"service=camerast&dsip=192.168.0.4&id=2001,2002"
	//service=camerast
	//dsip=192.168.0.4
	//id=2001,2002
	LIST_CSTRING tList;
	DivideStr(strData, tList, "&");
	int nSize = tList.size();//可去掉
	if (tList.size() < 4)
	{
		LOG_ERR(LOG_DB_SERVER, "Bad Request\n"); return SendResponse(false);
	}

	//get packetheader. example: packetheader=e10adc3949ba59abbe56e057f20f883e
	CSTRING strKey = "", strMd532 = "";
	LIST_CSTRING::iterator iter = tList.begin();
	DividePair(*iter, strKey, strMd532, "=");
	if (strKey.compare(KEY_PKT_HEADER))
	{
		LOG_ERR(LOG_DB_SERVER, "No packetheader\n"); return SendResponse(false);
	}
	LOG_DEBUG(LOG_DB_SERVER, "(%s) ==> key(%s) value(%s)\n", iter->c_str(), strKey.c_str(), strMd532.c_str());
	iter++;

	// get paramlen. example: paramlen=100
	iter++;

	//get service. example: service=extendinfo
	CSTRING strCmd = "";
	DividePair(*iter, strKey, strCmd, "=");
	if (strKey.compare(KEY_SERVICE))
	{
		LOG_ERR(LOG_DB_SERVER, "No service\n"); return SendResponse(false);
	}
	LOG_DEBUG(LOG_DB_SERVER, "(%s) ==> key(%s) value(%s)\n", iter->c_str(), strKey.c_str(), strCmd.c_str());
	iter++;

	//get param
	CSTRING strParam = *iter;

	// 检查md5
	CSTRING strMd5 = "dd121webkey";
	strMd5 += strCmd;
	strMd5 += strParam;
	BYTE szMd516[LENGTH_MD516+1] = {0};
	BYTE szMd532[2*LENGTH_MD516+1] = {0};
	memcpy(szMd516, CalMd5Val((PUCHAR)strMd5.c_str(), strMd5.size()), LENGTH_MD516);
	Ascii2HexStr((char*)szMd532, (char*)szMd516, LENGTH_MD516);
	if (strMd532.compare((const char*)szMd532))
	{
		LOG_ERR(LOG_DB_SERVER, "Wrong md532\n"); return SendResponse(false);
	}

	//find the function by the service string
	const int nHandlerCount = sizeof ( m_handlers )/sizeof(HandlerEntry);
	for ( int i = 0; i < nHandlerCount; ++i )
	{
		if (0 == strCmd.compare(m_handlers[i].pCmd))
		{
			PMFHANDLER pmfHandler = m_handlers[i].pmfHandler;
			return (this->*pmfHandler)(strParam);
		}
	}
	return SendResponse(false);
}

int CWebCmd::OnCommand( PUCHAR pData, int nLen, INetConnection* pCon )
{
	return OnReceive(pData, nLen, pCon);
}

int CWebCmd::OnPieceOfSerialNO(CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
#if defined(WEBCMD_DB)
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
	pHandle->PieceOfSerialNO();
#endif
	return SendResponse(true);
}

int CWebCmd::OnDserverConfigureIndex(CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);
	
	LIST_CSTRING tList;
	DivideStr(strParam, tList, "{}");

	LIST_CSTRING::iterator iter = tList.begin();
	for (; iter != tList.end(); iter++)
	{
		MAP_CSTRING tMap;
		if (-1 == DivideParam(*iter, tMap)) return SendResponse(false);

		DWORD dwVendorID = 0, dwConfigureIndex = 0;
		CSTRING strVendorID, strIndex;

		MAP_CSTRING::iterator pos = tMap.find(KEY_VENDORID);
		if (tMap.end() == pos)
		{
			LOG_DEBUG(LOG_DB_SERVER, "no vendorid\n"); return SendResponse(false);
		}
		strVendorID = pos->second;
		dwVendorID = (DWORD)atoi(strVendorID.c_str());

		pos = tMap.find(KEY_INDEX);
		if (tMap.end() == pos)
		{
			LOG_DEBUG(LOG_DB_SERVER, "no index\n"); return SendResponse(false);
		}
		strIndex = pos->second;
		dwConfigureIndex = (DWORD)atoi(strIndex.c_str());
		LOG_DEBUG(LOG_DB_SERVER, "VendorID %d dwConfigureIndex %d\n", dwVendorID, dwConfigureIndex);
#if defined(WEBCMD_DB)
		IServerAppHandle* pHandle = ServerApp_GetHandle();
		LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
		pHandle->DserverConfigureIndex(dwVendorID, dwConfigureIndex);
#endif
	}
	return SendResponse(true);
}

int CWebCmd::OnUserConfigureIndex(CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);

	LIST_CSTRING tList;
	DivideStr(strParam, tList, "{}");

	LIST_CSTRING::iterator iter = tList.begin();
	for (; iter != tList.end(); iter++)
	{
		MAP_CSTRING tMap;
		if (-1 == DivideParam(*iter, tMap)) continue;
		MAP_CSTRING::iterator pos = tMap.find(KEY_VENDORID);
		if (tMap.end() == pos)
		{
			LOG_DEBUG(LOG_DB_SERVER, "no vendorid\n"); return SendResponse(false);
		}
		CSTRING strVendorID = pos->second;
		DWORD dwVendorID = (DWORD)atoi(strVendorID.c_str());

		pos = tMap.find(KEY_USERID);
		if (tMap.end() == pos)
		{
			LOG_DEBUG(LOG_DB_SERVER, "no userid\n"); return SendResponse(false);
		}
		CSTRING strUserID = pos->second;
		DWORD dwUserID = (DWORD)atoi(strUserID.c_str());

		pos = tMap.find(KEY_INDEX);
		if (tMap.end() == pos)
		{
			LOG_DEBUG(LOG_DB_SERVER, "no index\n"); return SendResponse(false);
		}
		CSTRING strIndex = pos->second;
		DWORD dwConfigureIndex = (DWORD)atoi(strIndex.c_str());

		LOG_DEBUG(LOG_DB_SERVER, "VendorID %d UserID %d Index %d\n", dwVendorID, dwUserID, dwConfigureIndex);

#if defined(WEBCMD_DB)
		IServerAppHandle* pHandle = ServerApp_GetHandle();
		LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
		pHandle->UserConfigureIndex(dwVendorID, dwUserID);
#endif
	}
	return SendResponse(true);
}

int CWebCmd::OnUpdateDeviceRoom(CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);

	MAP_CSTRING tMap;
	if (-1 == DivideParam(strParam, tMap)) return SendResponse(false);

	MAP_CSTRING::iterator pos = tMap.find(KEY_VENDORID);
	if (tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER, "no vendorid\n"); return SendResponse(false);
	}
	CSTRING strVendorID = pos->second;
	DWORD dwVendorID = (DWORD)atoi(strVendorID.c_str());

	pos = tMap.find(KEY_DEVICEID);
	if (tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER, "no deviceid\n"); return SendResponse(false);
	}
	CSTRING strDeviceID = pos->second;
	DWORD dwDeviceID = (DWORD)atoi(strDeviceID.c_str());

	pos = tMap.find(KEY_TYPE);
	if (tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER, "no type\n"); return SendResponse(false);
	}
	CSTRING strType = pos->second;
	int nType = atoi(strType.c_str());

	pos = tMap.find(KEY_ROOMID);
	if (tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER, "no roomid\n"); return SendResponse(false);
	}
	
	LIST_DWORD lstRoomID;
	LIST_CSTRING tList;
	DivideStr(pos->second, tList, ",");
	LIST_CSTRING::iterator iter = tList.begin();
	for (; iter != tList.end(); iter++)
	{
		DWORD dwRoomID = (DWORD)atoi(iter->c_str());
		LOG_DEBUG(LOG_DB_SERVER, "VendorID %d DeviceID %d Type %d dwRoomID %d\n", dwVendorID, dwDeviceID, nType, dwRoomID);
		lstRoomID.push_back(dwRoomID);
	}
	
#if defined(WEBCMD_DB)
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
	pHandle->UpdateDeviceRoom(dwVendorID, dwDeviceID, nType, lstRoomID);
#endif
	return SendResponse(true);
}

int CWebCmd::OnClearRooms( CSTRING& strParam )
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);

	LIST_CSTRING tList;
	DivideStr(strParam, tList, "{}");
	
	MAP_DWORD mapDeviceID;
	LIST_CSTRING::iterator iter = tList.begin();
	for (; iter != tList.end(); iter++)
	{
		MAP_CSTRING tMap;
		if (-1 == DivideParam(*iter, tMap)) continue;

		MAP_CSTRING::iterator pos = tMap.find(KEY_VENDORID);
		if (tMap.end() == pos)
		{
			LOG_DEBUG(LOG_DB_SERVER, "no vendorid\n"); return SendResponse(false);
		}
		CSTRING strVendorID = pos->second;
		DWORD dwVendorID = (DWORD)atoi(strVendorID.c_str());

		pos = tMap.find(KEY_DEVICEID);
		if (tMap.end() == pos)
		{
			LOG_DEBUG(LOG_DB_SERVER, "no deviceid\n"); return SendResponse(false);
		}
		CSTRING strDeviceID = pos->second;
		DWORD dwDeviceID = (DWORD)atoi(strDeviceID.c_str());

		LOG_DEBUG(LOG_DB_SERVER, "VendorID %d DeviceID %d \n", dwVendorID, dwDeviceID);
		mapDeviceID.insert(std::make_pair(dwDeviceID, dwVendorID));
	}

#if defined(WEBCMD_DB)
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
	pHandle->ClearRooms(mapDeviceID);
#endif
	return SendResponse(true);
}

int CWebCmd::OnUpdateDevice(CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);

	LIST_CSTRING tList;
	DivideStr(strParam, tList, "{}");

	MAP_DWORD mapDeviceID;
	LIST_CSTRING::iterator iter = tList.begin();
	for (; iter != tList.end(); iter++)
	{
		MAP_CSTRING tMap;
		if (-1 == DivideParam(*iter, tMap)) continue;

		MAP_CSTRING::iterator pos = tMap.find(KEY_VENDORID);
		if (tMap.end() == pos)
		{
			LOG_DEBUG(LOG_DB_SERVER, "no vendorid\n"); return SendResponse(false);
		}
		CSTRING strVendorID = pos->second;
		DWORD dwVendorID = (DWORD)atoi(strVendorID.c_str());

		pos = tMap.find(KEY_DEVICEID);
		if (tMap.end() == pos)
		{
			LOG_DEBUG(LOG_DB_SERVER, "no deviceid\n"); return SendResponse(false);
		}
		CSTRING strDeviceID = pos->second;
		DWORD dwDeviceID = (DWORD)atoi(strDeviceID.c_str());

		LOG_DEBUG(LOG_DB_SERVER, "VendorID %d DeviceID %d \n", dwVendorID, dwDeviceID);
		mapDeviceID.insert(std::make_pair(dwDeviceID, dwVendorID));
	}

#if defined(WEBCMD_DB)
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
	pHandle->UpdateDevice(mapDeviceID);
#endif
	return SendResponse(true);
}

int CWebCmd::OnUpdateDeviceEx(CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);

	MAP_CSTRING tMap;
	if (-1 == DivideParam(strParam, tMap)) return SendResponse(false);

	MAP_CSTRING::iterator pos = tMap.find(KEY_TYPE);
	if (tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER, "no type\n"); return SendResponse(false);
	}
	CSTRING strType = pos->second;
	int nType = atoi(strType.c_str());

	pos = tMap.find(KEY_DEVICEID);
	if (tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER, "no deviceid\n"); return SendResponse(false);
	}
	LIST_DWORD lstDevID;
	LIST_CSTRING tList;
	DivideStr(pos->second, tList, ",");
	LIST_CSTRING::iterator iter = tList.begin();
	for (; iter != tList.end(); iter++)
	{
		DWORD dwDevID = (DWORD)atoi(iter->c_str());
		lstDevID.push_back(dwDevID);
	}
	
#if defined(WEBCMD_DB)
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
	pHandle->UpdateDeviceEx(nType, lstDevID);
#endif
	return SendResponse(true);
}

int CWebCmd::OnQueryDevice(CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER, "%s\n", __FUNCTION__);

	MAP_CSTRING tMap;
	if (-1 == DivideParam(strParam, tMap)) return SendResponse(false);

	MAP_CSTRING::iterator pos = tMap.find(KEY_DEVICEID);
	if (tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER, "no deviceid\n"); return SendResponse(false);
	}

	CSTRING strDevice = "";
	LIST_CSTRING tList;
	DivideStr(pos->second, tList, ",");
	LIST_CSTRING::iterator iter = tList.begin();
	for (; iter != tList.end();)
	{
		DWORD dwDeviceID = (DWORD)atoi(iter->c_str());

#if defined(WEBCMD_ST)
		ICacheHandle* pCacheHandle = Cache_GetHandle();
		LOG_ASSERT_RET(LOG_MAIN, pCacheHandle, -1);
		DWORD dwServerID = pCacheHandle->Cache_GetServerIDByDeviceID(dwDeviceID);
		LOG_DEBUG(LOG_DB_SERVER, "DeviceID %d On ServerID %d\n", dwDeviceID, dwServerID);

		strDevice = strDevice + *iter;
		if (dwServerID) strDevice = strDevice + "-1";
		else strDevice = strDevice + "-0";
		if(++iter != tList.end()) strDevice += ","; 
#endif
	}

	if (strDevice.size() <= 0) return SendResponse(false);
	LOG_DEBUG(LOG_DB_SERVER, "Return: %s\n", strDevice.c_str());
	return m_pCon->SendData((PUCHAR)strDevice.c_str(), strDevice.size());
}

//01电话充值获取设备ID
int CWebCmd::OnUpdateDeviceDeadLine(CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER, "101:%s\n", __FUNCTION__);
	MAP_CSTRING tMap;
	if (-1 == DivideParam(strParam, tMap)) return SendResponse(false);

	MAP_CSTRING::iterator pos = tMap.find(KEY_DEVICEID);
	if (tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER, "no deviceid\n"); return SendResponse(false);
	}

	LIST_DWORD lstDevID;
	LIST_CSTRING tList;
	DivideStr(pos->second, tList, ",");
	LIST_CSTRING::iterator iter = tList.begin();
	for (; iter != tList.end(); iter++)
	{
		DWORD dwDeviceID = (DWORD)atoi(iter->c_str());
		lstDevID.push_back(dwDeviceID);
		LOG_DEBUG(LOG_DB_SERVER, "deviceid:%d\n",dwDeviceID);
	}
#if defined(WEBCMD_DB)
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
	pHandle->UpdateDeviceDeadLine(lstDevID);
#endif
	return SendResponse(true);
}

//获取七牛下载凭证
int CWebCmd::OnGetQiNiuDownloadToken(CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER,"CWebCmd::%s\n",__FUNCTION__);

	MAP_CSTRING tMap;
	if (-1 == DivideParam(strParam, tMap)) return SendResponse(false);

	MAP_CSTRING::iterator pos = tMap.find(KEY_ACCESS);
	if(tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER,"no accesskey\n"); return SendResponse(false);
	}
	CSTRING strAccessKey = pos->second;
	MAP_CSTRING::iterator pos2 = tMap.find(KEY_SECRET);
	if(tMap.end() == pos2)
	{
		LOG_DEBUG(LOG_DB_SERVER,"no secretkey\n"); return SendResponse(false);
	}
	CSTRING strSecretKey = pos2->second;

#if defined(WEBCMD_DB)
	BYTE szToken[100] = {0};
	Qiniu_GetDownloadToken((const char*)strAccessKey.c_str(),(const char*)strSecretKey.c_str(),(char*)szToken);
	LOG_DEBUG(LOG_DB_SERVER,"szToken=%s\n",szToken);
	return m_pCon->SendData(szToken, 100);
#endif
	return 0;
}

//11.更新物業公告
int CWebCmd::OnUpdatePropertyAnnounce(CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s\n",__FUNCTION__);
	MAP_CSTRING tMap;
	if(-1 == DivideParam(strParam, tMap)) return SendResponse(false);

	MAP_CSTRING::iterator pos = tMap.find(KEY_GROUPID);
	if(tMap.end() == pos){
		LOG_DEBUG(LOG_DB_SERVER, "no villageid\n"); return SendResponse(false);
		return SendResponse(false);
	}
	CSTRING strData = pos->second;
	DWORD dwVillageId = atoi(strData.c_str());

	pos = tMap.find(KEY_NOTICEINDEX);
	if(tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER,"no noticeIndex\n"); return SendResponse(false);
	}
	strData = pos->second;
	DWORD dwNoticeIndex = atoi(strData.c_str());
	LOG_DEBUG(LOG_DB_SERVER,"VillageID:%d,NoticeIndex:%d\n",dwVillageId,dwNoticeIndex);
	//test_start
//	DWORD dwVillageId = 6;
//	DWORD dwNoticeIndex = 3;
	//test_end
#if defined(WEBCMD_DB)
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
	pHandle->UpdatePropertyAnnounce(dwVillageId, dwNoticeIndex);
#endif
	return SendResponse(true);
}


int CWebCmd::OnUpdateFirmwareRequest (CSTRING& strParam)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s\n",__FUNCTION__);
	/*
	MAP_CSTRING tMap;
	if(-1 == DivideParam(strParam, tMap)) return SendResponse(false);

	MAP_CSTRING::iterator pos = tMap.find(KEY_VERSION);
	if(tMap.end() == pos){
		LOG_DEBUG(LOG_DB_SERVER, "no version\n"); 
		return SendResponse(false);
	}
	CSTRING strVersion = pos->second;
	MAP_CSTRING::iterator pos = tMap.find(KEY_DEVICEID);
	if(tMap.end() == pos){
		LOG_DEBUG(LOG_DB_SERVER, "no deviceid\n");
		return SendResponse(false);
	}
	LIST_DWORD lstDevID;
	LIST_CSTRING tList;
	DivideStr(pos->second, tList, ",");
	LIST_CSTRING::iterator iter = tList.begin();
	for (; iter != tList.end(); iter++)
	{
		DWORD dwDeviceID = (DWORD)atoi(iter->c_str());
		lstDevID.push_back(dwDeviceID);
		LOG_DEBUG(LOG_DB_SERVER, "deviceid:%d\n",dwDeviceID);
	}
	*/
	//test_start
	const char *strVersion = "1.0.0.2";
//	PCHAR strVersion = p;
	LIST_DWORD lstDevID;
	for (int i = 1; i <= 10; i++){
		lstDevID.push_back(i);
		LOG_DEBUG(LOG_DB_SERVER, "deviceid=%d\n",i);
	}
	//test_end
#if defined(WEBCMD_DB)
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
	pHandle->UpdateFirmwareRequest((PCHAR)strVersion,lstDevID);
#endif
	return SendResponse(true);
}
//删除在线设备
int CWebCmd::OnDeleteDeviceOnline( CSTRING& strParam )
{
	LOG_DEBUG(LOG_DB_SERVER, "CWebCmd::%s\n", __FUNCTION__);
	MAP_CSTRING tMap;
	if (-1 == DivideParam(strParam, tMap)) return SendResponse(false);

	MAP_CSTRING::iterator pos = tMap.find(KEY_DEVICEID);
	if (tMap.end() == pos)
	{
		LOG_DEBUG(LOG_DB_SERVER, "no deviceid\n"); return SendResponse(false);
	}

	LIST_DWORD lstDevID;
	LIST_CSTRING tList;
	DivideStr(pos->second, tList, ",");
	LIST_CSTRING::iterator iter = tList.begin();
	for (; iter != tList.end(); iter++)
	{
		DWORD dwDeviceID = (DWORD)atoi(iter->c_str());
		lstDevID.push_back(dwDeviceID);
		LOG_DEBUG(LOG_DB_SERVER, "deviceid:%d\n",dwDeviceID);
	}
#if defined(WEBCMD_DB)
	IServerAppHandle* pHandle = ServerApp_GetHandle();
	LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
	pHandle->DeleteDeviceOnline(lstDevID);
#endif
	return SendResponse(true);
}
//更新广告信息
int CWebCmd::OnUpdateAdvertInfo( CSTRING& strParam )
{
	LOG_DEBUG(LOG_DB_SERVER, "CWebCmd::%s %s\n", __FUNCTION__, strParam.c_str());
	MAP_CSTRING tMap;
	if(-1 == DivideParam(strParam, tMap)) return SendResponse(false);

	MAP_CSTRING::iterator pos1 = tMap.find(KEY_GROUPID);
	if(tMap.end() == pos1){
		LOG_DEBUG(LOG_DB_SERVER, "no villageid\n"); 
		return SendResponse(false);
	}

	MAP_CSTRING::iterator pos2 = tMap.find(KEY_ADVERTINDEX);
	if(tMap.end() == pos2){
		LOG_DEBUG(LOG_DB_SERVER, "no Advertindex\n");
		return SendResponse(false);
	}
	
	LIST_CSTRING tList1, tList2;
	DivideStr(pos1->second, tList1, ",");
	DivideStr(pos2->second, tList2, ",");
	if(tList1.size() != tList2.size()){
		LOG_DEBUG(LOG_DB_SERVER, "tListVillage.size = %d, tListIndex.size = %d\n", tList1.size(), tList2.size() ); 
		return SendResponse(false);
	}

	LIST_CSTRING::iterator iter1 = tList1.begin();
	LIST_CSTRING::iterator iter2 = tList2.begin();
	for ( ; iter1 != tList1.end() ; iter1++, iter2++)
	{
		DWORD dwVillageId = (DWORD)atoi(iter1->c_str());
		DWORD dwAdvertIndex = (DWORD)atoi(iter2->c_str());
		LOG_DEBUG(LOG_DB_SERVER, "dwVillageId=%d, dwAdvertIndex=%d\n",dwVillageId, dwAdvertIndex);
#if defined(WEBCMD_DB)
		IServerAppHandle* pHandle = ServerApp_GetHandle();
		LOG_ASSERT_RET(LOG_DB_SERVER, pHandle, -1);
		pHandle->UpdateAdvertInfo(dwVillageId, dwAdvertIndex);
#endif
	}
	return SendResponse(true);
}

int CWebCmd::SendResponse( bool bSuccessful )
{
	LOG_ASSERT_RET(LOG_DB_SERVER, m_pCon, -1);
	if (bSuccessful) m_szBuffer[0] = '1';
	else m_szBuffer[0] = '0';
	LOG_DEBUG(LOG_DB_SERVER, "SendResponse bSuccessful %d\n", bSuccessful);
	return m_pCon->SendData(m_szBuffer, 1);
}

void CWebCmd::SetNetConnection( INetConnection *pCon )
{
	if (NULL == pCon) return;
	m_pCon = pCon;
	pCon->SetSink(this);
}

int CWebCmd::OnDisconnect( int nReason, INetConnection* pCon )
{
	CCGICenter::Instance()->DelElem(this);
	return 0;
}

void CWebCmd::OnTimer( TimerReason_e eReason, ITimerSink* pSink )
{
	CCGICenter::Instance()->DelElem(this);
}

IMPLEMENT_SINGLETON(CCGICenter)
CCGICenter::CCGICenter()
{

}

CCGICenter::~CCGICenter()
{
	UnRegisterNetListen(GROUPCODE_WEB, this);
}

bool CCGICenter::Start()
{
	RegisterNetListen(GROUPCODE_WEB, this);
	return true;
}

int CCGICenter::OnDispatchConnection(INetConnection* pCon, int nNetType, PUCHAR pData, int nLen)
{
	if (NULL == pCon) return -1;
	CWebCmd* p = NULL;
	try
	{
		p = new CWebCmd();
	}
	catch(std::bad_alloc &memExp)
	{
		LOG_DEBUG(LOG_DB_SERVER, "new CWebCmd failed\n");
		if (pCon) { pCon->Disconnect(); NetworkDestroyConnection(pCon); }
		return 0;
	}
	AddElem(p);
	p->SetNetConnection(pCon);
	return p->OnReceive(pData, nLen, pCon);
}
