#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <algorithm>
#include <map>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "UtilityInterface.h"
#include "MD5Sum.h"
#include "3des.h"
#include "crc32.h"
#include "base64.h"
#include "../include/Log.h"
#include "TimerManager.h"
#include "Http.h"
#include "vurl.h"
#include "getbuffer.h"
#include "DealMulPkt.h"

BYTE g_szBase64Enc[512];

bool UtilityInit(DWORD dwIP /* = 0 */)
{
	return true;
}

void UtilityFinish()
{
	CTimerManager::Instance()->StopTimer();
	CTimerManager::DeleteInstance();

	CHttpMgr::DeleteInstance();
	CDealMulPktMgr::DeleteInstance();
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
}

void ShowVerion()
{ 
	printf("Cur			Version: 1.0.0.0\n");
	printf("DataBase	Version: 1.0.0.0\n");
	printf("Push		Version: 1.0.0.0\n");
	printf("Sms			Version: 1.0.0.0\n");
}

CVUrl g_clsUrl;
void UrlEncode(const std::string &strIn,std::string &strOut, bool isUpper /* = true */)
{
	g_clsUrl.UrlEncode(strIn, strOut, isUpper);
}

void UrlDecode(const std::string &strIn, std::string &strOut)
{
	g_clsUrl.UrlDecode(strIn, strOut);
}

MD5Sum g_clsUtilityMd5;
PUCHAR CalMd5Val(PUCHAR pInStr, int nLen)
{
	g_clsUtilityMd5.Calculate(pInStr, nLen);
	return (PUCHAR)g_clsUtilityMd5.GetRawHash();
}

BYTE g_szBuffer[LEN_CHALLENGESTRING + 1];
PUCHAR CalDesVal(PUCHAR pKey, PUCHAR pIn, bool bEnc /* = true */)
{
	des3_context desCtx;
	memset(g_szBuffer, 0, sizeof(g_szBuffer));
	des3_set_2keys(&desCtx, pKey, pKey + 8);
	if(bEnc) {
		des3_encrypt(&desCtx, pIn, g_szBuffer);
		des3_encrypt(&desCtx, pIn + 8, g_szBuffer + 8);
	} else {
		des3_decrypt(&desCtx, pIn, g_szBuffer);
		des3_decrypt(&desCtx, pIn + 8, g_szBuffer + 8);
	}
	return (PUCHAR)g_szBuffer;
}

BYTE g_szDesABuffer[256 + 1];
PUCHAR EncDesA(PUCHAR pKey, PUCHAR pIn, int& nOutLen)
{
	memset(g_szDesABuffer, 0, sizeof(g_szDesABuffer));
	nOutLen = DESEncodeB((uint8*)pKey, (uint8*)pIn, strlen((char*)pIn), g_szDesABuffer);
	return (PUCHAR)g_szDesABuffer;
}

int USDK_DESEncode(PUCHAR pKey, PUCHAR pInput, int nInLen, PUCHAR pOut)
{
	return DESEncode(pKey, pInput, nInLen, pOut);
}

int USDK_DESDecode(PUCHAR pKey, PUCHAR pInput, int nInLen, PUCHAR pOut)
{
	return DESDecode(pKey, pInput, nInLen, pOut);
}

char* IpDword2Str(unsigned long dwIP)
{
	in_addr addr; 
	addr.s_addr = htonl(dwIP); 
	return inet_ntoa(addr); 
}

unsigned long IpStr2Dword(char* pStrIP)
{
	LOG_ASSERT_RET(LOG_UTIL, NULL != pStrIP, 0);
	DWORD dwIP = inet_addr((const char*)pStrIP);
	if(INADDR_NONE == dwIP) {
		LOG_WARN(LOG_UTIL, "Invalid IP(%s)\n", pStrIP);
		dwIP = 0;
	} else  dwIP = ntohl(dwIP);
	return dwIP;	
}

void AddTimer(TimerReason_e eReason, DWORD dwInterval, ITimerSink* pSink)
{
	CTimerManager::Instance()->AddTimer(eReason, dwInterval, pSink);
}

void DelTimer(TimerReason_e eReason, ITimerSink* pSink)
{
	CTimerManager::Instance()->DelTimer(eReason, pSink);
}

char SNCharToVal(char ch)
{
	if (ch >= '0' && ch <= '9'){ return ch - '0'; }
	if (ch <= 'Z' && ch >= 'A'){ return ch - 'A' + 10; }
	if (ch <= 'z' && ch >= 'a'){ return ch - 'a' + 10; }
	return -1;
}
DWORD SN2ID(char* sn)
{
	if(strlen(sn) < 20)    { return 0; }

	unsigned long id = 0;
	id = static_cast<unsigned long>(SNCharToVal(sn[1+1])+12) % 36;
	id = static_cast<unsigned long>(SNCharToVal(sn[1+4])+34) % 36 + id * 36;
	id = static_cast<unsigned long>(SNCharToVal(sn[1+2])+10) % 36 + id * 36;
	id = static_cast<unsigned long>(SNCharToVal(sn[1+5])+21) % 36 + id * 36;
	id = static_cast<unsigned long>(SNCharToVal(sn[1+6])+30) % 36 + id * 36;
	id = static_cast<unsigned long>(SNCharToVal(sn[1+3])+25) % 36 + id * 36;
	const unsigned long IDMAX6 = 0xFFFFFFFF / 36;
	const unsigned long IDMAX7 = 0xFFFFFFFF % 36;
	unsigned long idmax6 = id;
	unsigned long idmax7 = static_cast<unsigned long>(SNCharToVal(sn[1+0])+ 4) % 36;
	if(idmax6 > IDMAX6){ return 0; }
	if(idmax6 == IDMAX6 && idmax7 > IDMAX7){ return 0; }
	id = idmax6 * 36 + idmax7;
	return id;
}

void GetTime(timeval& packetTime)
{
	gettimeofday(&packetTime, NULL);
	time_t tm1 = time(NULL);

//    struct tm *t2 = gmtime(&tm1);
//    time_t tm2 = mktime(t2);
	//gmtime线程不安全，改用gmtime_r
	struct tm t2;
	memset(&t2, 0, sizeof(tm));
	gmtime_r(&tm1, &t2);
    time_t tm2 = mktime(&t2);	

    long dtime = (long)(tm2 - tm1);
	packetTime.tv_sec -= dtime;
}

int Base64EncVal(char * base64code, const char * src, int src_len)
{
	return Base64Encode(base64code, src, src_len);
}

int Base64DecVal(char * buf, const char * base64code, int src_len)
{
	return Base64Decode(buf, base64code, src_len);
}

int Base64_encode_len(int len)
{
	return apr_base64_encode_len(len);
}

int	Base64_encode(char *encoded, const char *string, int len)
{
	return apr_base64_encode(encoded, string, len);
}

int Base64_decode_len(const char *bufcoded, int buflen)
{
	return apr_base64_decode_len(bufcoded, buflen);
}	

int Base64_decode(char *bufplain, const char *bufcoded, int buflen)
{
	return apr_base64_decode(bufplain, bufcoded, buflen);
}

void CalCrc32Val(DWORD* pCrc32, PUCHAR pData, DWORD uSize)
{
	crc32Compute(pCrc32, pData, uSize);
}

bool JoinMutiGroup(INetConnection* pCon, const char* pAddr)
{
	LOG_ASSERT_RET(LOG_UTIL, pCon && pAddr, false);

	int nSocketID;
	pCon->GetOpt(NETWORK_TRANSPORT_OPT_GET_FD, &nSocketID);
	struct ip_mreq  mreq;
	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr.s_addr = inet_addr(pAddr);
	mreq.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(nSocketID, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
		LOG_ERR(LOG_UTIL, "setsockopt(fd: %d IP_ADD_MEMBERSHIP) Failed err=%d(%s)\n", 
			   nSocketID, errno, strerror(errno));
		return true;
	}
	unsigned char flag = 0;
	if (setsockopt(nSocketID, IPPROTO_IP, IP_MULTICAST_LOOP, &flag, sizeof(flag)) < 0) {
		LOG_ERR(LOG_UTIL, "setsockopt(fd: %d IP_MULTICAST_LOOP) Failed err=%d(%s)\n", 
			   nSocketID, errno, strerror(errno));
		return true;
	}
	LOG_DEBUG(LOG_UTIL, "Join Group(%s) !!!\n", pAddr);
	return true;
}
DWORD  Net_GetEthIP(const char* pifName)
{
	int fd_arp = socket(AF_INET, SOCK_PACKET, htons(0x0806));
	if(fd_arp < 0) return 0;

	struct ifreq ifr; strcpy(ifr.ifr_name, pifName);
	if (ioctl(fd_arp, SIOCGIFADDR, &ifr) < 0) {
		close(fd_arp);
		return 0;
	}
	close(fd_arp);

	return ntohl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
}
int Ascii2HexStr(char* pOutHexStr, char* pInASCIIData,  int nInDataLen)
{
	if((NULL == pOutHexStr) || (NULL == pInASCIIData))
		return 0;

	BYTE cCurVal = 0, cCurHighVal = 0, cCurLowVal = 0;
	for(int i = 0; i < nInDataLen; ++i)
	{ 
		cCurVal = pInASCIIData[i];
		cCurHighVal = cCurVal & 0xF0;
		cCurHighVal >>= 4;

		if(cCurHighVal < 10)
		{
			pOutHexStr[i*2] = '0' + cCurHighVal;
		}
		else if((cCurHighVal > 9) && (cCurHighVal < 16))
		{
			pOutHexStr[i*2] = 'a' + cCurHighVal - 10;
		}

		cCurLowVal = cCurVal & 0x0F;
		if(cCurLowVal < 10)
		{
			pOutHexStr[i*2+1] = '0' + cCurLowVal;
		}
		else if((cCurLowVal > 9) && (cCurLowVal < 16))
		{
			pOutHexStr[i*2+1] = 'a' + cCurLowVal - 10;
		}
	}

	return strlen(pOutHexStr);
}

BYTE Char2Hex( char szChar )
{
	if ( ( szChar >= '0' ) && ( szChar <= '9' ) )
		return szChar - '0';

	if ( ( szChar >= 'a' ) && ( szChar <= 'f' ) )
		return szChar - 'a' + 10;

	if ( ( szChar >= 'A' ) && ( szChar <= 'F' ) )
		return szChar - 'A' + 10;

	return 0;
}

int HexStr2Ascii( char* pOutAscii, char* pInHexStr, int nInLen )
{
	BYTE szMD5Byte;
	for ( int i = 0; i < LENGTH_CHALLENGE; ++i )
	{
		szMD5Byte = Char2Hex( pInHexStr[2*i] ) << 4;
		szMD5Byte |= Char2Hex( pInHexStr[2*i+1] ); 
		pOutAscii[i] = szMD5Byte;
	}
	return 0;
}

int substring2int (char szBuf[], int nFrom, int nTo, int nLen)
{
	if (nTo > nLen) {
		return 0;
	}
	char ch [5] = {0};
	memcpy (ch, szBuf + nFrom, nTo - nFrom);
	return atoi (ch);
}

void string2timeval(char szBuf[], timeval& tv)
{
	int nLen = strlen (szBuf);
	tm t = {0};
	t.tm_year = substring2int (szBuf, 0, 4, nLen) - 1900;
	t.tm_mon = substring2int (szBuf, 4, 6, nLen) - 1;
	t.tm_mday = substring2int (szBuf, 6, 8, nLen);
	t.tm_hour = substring2int (szBuf, 8, 10, nLen);
	t.tm_min = substring2int (szBuf, 10, 12, nLen);
	t.tm_sec = substring2int (szBuf, 12, 14, nLen);
	tv.tv_sec = mktime (&t);
	tv.tv_usec = 0;
}

int CharsetConvert(char* From, char* To, char* pInBuf, size_t *nInLen, char* pOutBuf, size_t *nOutLen)
{
	char **pin = &pInBuf;
	char **pout = &pOutBuf;
	int ret = -1;
	iconv_t nConvert = iconv_open(To, From);
	if (nConvert == 0) return ret;
	ret = iconv(nConvert, pin, nInLen, pout, nOutLen);
	iconv_close(nConvert);
	return ret;
}

int CharsetConvert(char *from, char *to, char* pInBuf, size_t nInLen, std::string &strOut)
{
	if (nInLen <= 0) return 0;

	size_t nOutLen = 4 * nInLen;
	char *pOutBuf = new char[nOutLen + 1];
	if(!pOutBuf) return -1;

	memset(pOutBuf, 0, nOutLen + 1);
	int ret = -1;
	try {
		ret = CharsetConvert(from, to, pInBuf, &nInLen, pOutBuf, &nOutLen);
		if(-1 != ret) {
			strOut.assign(pOutBuf);
		}
	} catch(...) { }
	delete[] pOutBuf;
	return ret;
}

unsigned long GetHostIP(const char* pData)
{
	struct hostent *host;
	host = gethostbyname(pData);
	if (NULL == host) return 0;
	unsigned long dwAddr = 0;
	memcpy(&dwAddr, (struct in_addr *)host->h_addr, sizeof(ULONG));
	dwAddr = ntohl(dwAddr);
	return dwAddr;
}

IHttp* RegisterHttp( IHttpSink* pSink )
{
	CHttp* pHttp = NULL;
	try
	{
		pHttp = new CHttp(pSink);
	}
	catch(std::bad_alloc &memExp)
	{
		printf("new CHttp failed\n"); return NULL;
	}
	CHttpMgr::Instance()->AddElem(pHttp);
	return pHttp;
}

void UnRegisterHttp( IHttp* pHttp )
{
	if (NULL == pHttp) return;
	CHttpMgr::Instance()->DelElem((CHttp*)pHttp);
}

void JsonEscape(std::string & str)
{
	int found = -2;
	while ((found = str.find('\\',found + 2)) != std::string::npos) {
		str.replace(found, 1, "\\\\");
	}
	found = -2;
	while ((found = str.find('"',found + 2)) != std::string::npos) {
		str.replace(found, 1, "\\\"");
	}
	found = -2;
	while ((found = str.find('\n',found + 2)) != std::string::npos) {
		str.replace(found, 1, "\\n");
	}
	found = -2;
	while ((found = str.find('/',found + 2)) != std::string::npos) {
		str.replace(found, 1, "\\/");
	}
}

bool ParsePacketHeader( PUCHAR pData, int nLen, PacketHeader_t& tHeader )
{
	if (nLen < PACKET_HEADER_SIZE)
	{
		LOG_DEBUG(LOG_UTIL, "ParsePacketHeader nLen %d too short\n", nLen);
		return false;
	}
	memset(&tHeader, 0, sizeof(PacketHeader_t));
	CGetBuffer bufferGet(pData, nLen);
	bufferGet >> tHeader.groupcode;
	bufferGet >> tHeader.commandid >> tHeader.reserved0 
		>> tHeader.version >> tHeader.reserved1 
		>> tHeader.sourceid >> tHeader.destinationid
		>> tHeader.commandflag >> tHeader.totalsegment
		>> tHeader.subsegment >> tHeader.segmentflag
		>> tHeader.reserved2 >> tHeader.reserved3;
	bufferGet >> tHeader.error;
	bufferGet.Skip(7);
	bufferGet >> tHeader.payloadversion;
//	LOG_DEBUG(LOG_UTIL, "%p %p\n", tHeader.groupcode, tHeader.commandid);
	return true;
}

void CalcAuthDigist( PUCHAR pDigist, PUCHAR pUserName, PUCHAR pPassword, PUCHAR pChallenge )
{
	// Digist = md5(UserName +md5(Password,16) + Challenge, 16)
	BYTE szPassword[LENGTH_CHALLENGE+1] = {0};
	memcpy(szPassword, CalMd5Val((PUCHAR)pPassword, strlen((const char*)pPassword)), LENGTH_CHALLENGE);
	
	//BYTE szHexDigist[2*LENGTH_CHALLENGE+1] = {0};
	//Ascii2HexStr((char*)szHexDigist, (char*)szPassword, LENGTH_CHALLENGE);
	//printf("%s\n", pPassword);
	//printf("%s\n", szHexDigist);

	BYTE szSrc[LENGTH_NAME+2*LENGTH_CHALLENGE+1] = {0};
	int nUserNameLen = strlen((const char*)pUserName);

	memcpy(szSrc, pUserName, nUserNameLen);
	memcpy(szSrc+nUserNameLen, szPassword, LENGTH_CHALLENGE);
	int nOffset = nUserNameLen+LENGTH_CHALLENGE;
	memcpy(szSrc+nOffset, pChallenge, LENGTH_CHALLENGE);
	nOffset += LENGTH_CHALLENGE;

	memcpy(pDigist, CalMd5Val((PUCHAR)szSrc, nOffset), LENGTH_CHALLENGE);
}

// 16 MD5Password
void CalcAuthDigistA( PUCHAR pDigist, PUCHAR pUserName, PUCHAR pMD5Password, PUCHAR pChallenge )
{
	// Digist = md5(UserName +md5(Password,16) + Challenge, 16)

	BYTE szSrc[LENGTH_NAME+2*LENGTH_CHALLENGE+1] = {0};
	int nUserNameLen = strlen((const char*)pUserName);

	memcpy(szSrc, pUserName, nUserNameLen);
	memcpy(szSrc+nUserNameLen, pMD5Password, LENGTH_CHALLENGE);
	int nOffset = nUserNameLen+LENGTH_CHALLENGE;
	memcpy(szSrc+nOffset, pChallenge, LENGTH_CHALLENGE);
	nOffset += LENGTH_CHALLENGE;

	memcpy(pDigist, CalMd5Val((PUCHAR)szSrc, nOffset), LENGTH_CHALLENGE);
}

void GenerateChallenge( PUCHAR pChallenge )
{
	srand( (int)time(0) );
	for (int k = 0; k < LENGTH_CHALLENGE; k++)
	{
		pChallenge[k] = 'a' + (int) (1.0*26*rand()/(RAND_MAX+1.0));
	}
}

/*
 * strData:	dsip=60.191.34.147
 * key:		dsip
 * value:	60.191.34.147
 */
int DividePair(CSTRING& strData, CSTRING& strKey, CSTRING& strValue, const CSTRING& strItem)
{
	CSTRING::size_type pos = strData.find_first_of(strItem);
	if (pos == CSTRING::npos) return -1;
	strKey = strData.substr(0, pos);
	pos += strItem.length();
	strValue = strData.substr(pos);
	return 0;
}

/*
 * strData:		user=steven,pwd=steven,id=2001,2002
 * tMap:		Key			Value
 *				user		steven
 *				pwd			steven
 *				id			2001,2002
 */
int DivideParam(CSTRING& strData, MAP_CSTRING& tMap)
{
	tMap.clear();
	CSTRING::size_type nStart = 0, nEqual1 = 0, nEqual2 = 0, nEnd = 0;
	CSTRING strKey, strValue;
	while(1) {
		nEqual1 = strData.find_first_of("=", nStart);
		if (nEqual1 == CSTRING::npos) break;
		strKey = strData.substr(nStart, nEqual1 - nStart);
		
		nEqual2 = strData.find_first_of("=", nEqual1+1);//查找下一个“=”
		if (nEqual2 == CSTRING::npos) {
			strValue = strData.substr(nEqual1+1);
			tMap.insert(std::make_pair(strKey, strValue));
			break;
		} else {
			nEnd = strData.find_last_of(",", nEqual2);
			if (nEnd == CSTRING::npos) return -1;
			if (nEnd < nEqual1) return -1;
			strValue = strData.substr(nEqual1+1, nEnd - nEqual1 - 1);
		}

		tMap.insert(std::make_pair(strKey, strValue));
		nStart = nEnd + 1;
	}
	return 0;
}

//根据字符串strItem来分割字符串
int DivideStr(CSTRING& strData, LIST_CSTRING& tList, const CSTRING& strItem)
{
	tList.clear();
	CSTRING::size_type nStart = 0, nEnd = 0;
	while(1) {
		nEnd = strData.find_first_of(strItem, nStart);
		if (nEnd == CSTRING::npos) {
			tList.push_back(strData.substr(nStart));
			break;
		}
		tList.push_back(strData.substr(nStart, nEnd - nStart));
		nStart = nEnd + strItem.length();
	}
	return 0;
}

void PutVariableStr( CPutBuffer& buf, PUCHAR pStr )
{
	if (NULL == pStr) { buf << (BYTE)0; return; }
	BYTE bLen = (BYTE)strlen((const char*)pStr);
	if ( (buf.GetFilledSize() + sizeof(BYTE) + bLen) > MAX_PACKET_LEN ) return;
	buf << bLen;
//	LOG_DEBUG(LOG_UTIL, "bLen = %d\n", bLen);
	buf << CByteArrayBuffer(pStr, (int)bLen);
}

void PutBase64Str( CPutBuffer& buf, PUCHAR pStr )
{
	if (NULL == pStr) { buf << (BYTE)0; return; }
	int nLen = strlen((const char*)pStr);
	int nBase64Len = Base64EncVal((char*)g_szBase64Enc, (const char*)pStr, nLen);
	if ( (buf.GetFilledSize() + sizeof(BYTE) + nBase64Len) > MAX_PACKET_LEN ) return;
	buf << (BYTE)nBase64Len;
	buf << CByteArrayBuffer(g_szBase64Enc, nBase64Len);
}

bool GetVariableStr( CGetBuffer& buf, PUCHAR pStr, int nMaxLen, int nTotalLen, int& nNeedLen )
{
	nNeedLen += sizeof(BYTE);
	if (nTotalLen < nNeedLen)
	{
		LOG_DEBUG(LOG_UTIL, "1 wrong packet TotalLen:%d NeedLen:%d\n", nTotalLen, nNeedLen); return false;
	}
	BYTE bLen = 0;
	buf >> bLen;
//	LOG_DEBUG(LOG_UTIL, "bLen = %d\n", bLen);

	if ((int)bLen > nMaxLen)
	{
		LOG_DEBUG(LOG_UTIL, "GetVariableStr Failed Len %d MaxLen %d\n", bLen, nMaxLen); return false;
	}
	nNeedLen += (int)bLen;
	if (nTotalLen < nNeedLen)
	{
		LOG_DEBUG(LOG_UTIL, "2 wrong packet TotalLen:%d NeedLen:%d\n", nTotalLen, nNeedLen); return false;
	}
	buf >> CByteArrayBuffer(pStr, (int)bLen);
	return true;
}

BYTE g_szBase64Dec[512];
bool GetBase64Str( CGetBuffer& buf, PUCHAR pStr, int nMaxLen, int nTotalLen, int& nNeedLen )
{
	nNeedLen += sizeof(BYTE);
	if (nTotalLen < nNeedLen)
	{
		LOG_DEBUG(LOG_UTIL, "1 wrong packet TotalLen:%d NeedLen:%d\n", nTotalLen, nNeedLen); return false;
	}
	BYTE bLen = 0;
	buf >> bLen;

	nNeedLen += (int)bLen;
	if ((int)bLen > 512)
	{
		LOG_DEBUG(LOG_UTIL, "GetBase64Str Failed Len %d(Base64 MaxLen 512)\n", bLen); return false;
	}
	if (nTotalLen < nNeedLen)
	{
		LOG_DEBUG(LOG_UTIL, "2 wrong packet TotalLen:%d NeedLen:%d\n", nTotalLen, nNeedLen); return false;
	}
	buf >> CByteArrayBuffer(g_szBase64Dec, (int)bLen);
	LOG_DEBUG(LOG_UTIL,"<=SMS=>%s\n", g_szBase64Dec);
	int nBase64DecLen = Base64DecVal((char*)g_szBase64Dec, (const char*)g_szBase64Dec, (int)bLen);
	if (nBase64DecLen > nMaxLen)
	{
		LOG_DEBUG(LOG_UTIL, "GetVariableStr Failed Base64DecLen %d MaxLen %d\n", nBase64DecLen, nMaxLen); return false;
	}
	memcpy(pStr, g_szBase64Dec, nBase64DecLen);
	return true;
}

void PutBuffer_ConnectInfo( CPutBuffer& buffer, ConnectInfo_t& tInfo )
{
	buffer << tInfo.dwID;
	PutBuffer_NetInfo(buffer, tInfo.tNetInfo);
	buffer << CByteArrayBuffer((PUCHAR)tInfo.szUserName, LENGTH_CHALLENGE);
	buffer << CByteArrayBuffer((PUCHAR)tInfo.szPassword, LENGTH_CHALLENGE);
}

void PutBuffer_NetInfo( CPutBuffer& buffer, NetInfo_t& tInfo )
{
	buffer << tInfo.dwPublicIP << tInfo.wPublicPortTCP << tInfo.wPublicPortUDP << tInfo.wLocalPortUDP;
	WORD wCount = (WORD)tInfo.listLocalIPs.size();
	buffer << wCount;
	LIST_DWORD::iterator iter = tInfo.listLocalIPs.begin();
	for (; iter != tInfo.listLocalIPs.end(); iter++)
	{
		DWORD dwIP = *iter;
		buffer << dwIP;
	}
	buffer << tInfo.wNetworkType;
}

void Packet_RoomPushInfo(CPutBuffer& buffer, RoomPush_t& tRoomInfo)
{
	int nCount = tRoomInfo.lstPushInfo.size();
	buffer << tRoomInfo.dwRoomID << tRoomInfo.dwPushIndex << nCount;

	LIST_PUSHINFO::iterator iter = tRoomInfo.lstPushInfo.begin();
	for (; iter != tRoomInfo.lstPushInfo.end(); iter++)
	{
		buffer << iter->dwUserID << iter->dwVendorID << iter->bPushType << iter->bLanguage;
		PutVariableStr(buffer, (PUCHAR)iter->szToken);
		//LOG_DEBUG(LOG_UTIL, "Packet_RoomPushInfo: RoomID %d PushIndex %d UserID %d VendorID %d PushType %d Language %d Token %s\n",
		//	tRoomInfo.dwRoomID, tRoomInfo.dwPushIndex, iter->dwUserID, iter->dwVendorID, iter->bPushType, iter->bLanguage, iter->szToken);
	}
}

void Packet_RoomUserInfo(CPutBuffer& buffer, RoomUser_t& tRoomInfo)
{
	int nCount = tRoomInfo.lstUserInfo.size();
	buffer << tRoomInfo.dwRoomID << tRoomInfo.dwUserIndex << nCount;

	LIST_SMSINFO::iterator iter = tRoomInfo.lstUserInfo.begin();
	for (; iter != tRoomInfo.lstUserInfo.end(); iter++)
	{
		buffer << iter->dwUserID << iter->dwVendorID << iter->bLanguage;
		PutBase64Str(buffer, (PUCHAR)iter->szMobilePhone);
		PutVariableStr(buffer, (PUCHAR)iter->szDeviceName);
		//LOG_DEBUG(LOG_UTIL, "Packet_RoomUserInfo: RoomID %d UserIndex %d UserID %d VendorID %d Language %d Name %s DevName %s\n",
		//	tRoomInfo.dwRoomID, tRoomInfo.dwUserIndex, iter->dwUserID, iter->dwVendorID, iter->bLanguage,
		//	iter->szMobilePhone, iter->szDeviceName);
	}
}

void Packet_RoomCardInfo(CPutBuffer& buffer, RoomCard_t& tRoomInfo)
{
	int nCount = tRoomInfo.lstCardInfo.size();
	buffer << tRoomInfo.dwRoomID << tRoomInfo.dwCardIndex << nCount;

	LIST_CARDINFO::iterator iter = tRoomInfo.lstCardInfo.begin();
	for (; iter != tRoomInfo.lstCardInfo.end(); iter++)
	{
		PutVariableStr(buffer, (PUCHAR)iter->szCard);
		buffer << iter->bCardType;
		//LOG_DEBUG(LOG_UTIL, "Packet_RoomCardInfo: RoomID %d Index %d Card %s CardType %d\n",
		//	tRoomInfo.dwRoomID, tRoomInfo.dwCardIndex, iter->szCard, iter->bCardType);
	}
}
void Packet_RoomCardTimeLimit(CPutBuffer& buffer, RoomCard_t& tRoomInfo)
{
	int nCount = tRoomInfo.lstCardInfo.size();
	buffer << nCount;

	LIST_CARDINFO::iterator iter = tRoomInfo.lstCardInfo.begin();
	for(; iter != tRoomInfo.lstCardInfo.end(); iter++)
	{
		buffer << iter->dwCardTimeLimit;
		LOG_DEBUG(LOG_UTIL,"Packet_RoomCardTimeLimit CardTimeLimit %d\n", iter->dwCardTimeLimit);
	}
}

void Packet_RoomIndoorInfo(CPutBuffer& buffer, RoomIndoor2_t& tRoomInfo)
{
	int nCount = tRoomInfo.lstIndoorInfo.size();
	buffer << tRoomInfo.dwRoomID << tRoomInfo.dwIndoorIndex << nCount;
	LOG_DEBUG(LOG_UTIL,"RoomID=%d IndoorIndex=%d Count=%d\n",tRoomInfo.dwRoomID, tRoomInfo.dwIndoorIndex, nCount);
	LIST_INDOORINFO::iterator iter = tRoomInfo.lstIndoorInfo.begin();
	for (; iter != tRoomInfo.lstIndoorInfo.end(); iter++)
	{
		buffer << CByteArrayBuffer((PUCHAR)iter->szSerialNO,LENGTH_SERIALNO);
		buffer << iter->dwInDoorID;
//		PutVariableStr(buffer, (PUCHAR)iter->szSerialNO);
		LOG_DEBUG(LOG_UTIL, "Packet_RoomCardInfo: SN %s IndoorID %d\n", iter->szSerialNO, iter->dwInDoorID);
	}
}

void Packet_RoomPushSwitch(CPutBuffer& buffer, RoomPushSwitch_t& tRoomInfo)
{
	int nCount = tRoomInfo.lstPushSwitch.size();
	buffer << tRoomInfo.dwRoomID << tRoomInfo.dwPushSwitchIndex << nCount;

	LIST_PUSHSWITCH::iterator iter = tRoomInfo.lstPushSwitch.begin();
	for (; iter != tRoomInfo.lstPushSwitch.end(); iter++)
	{
		buffer << iter->dwUserID << iter->nPushSwitch;
	}
}

void GenerateTmpUserInfo(PUCHAR pUserName, PUCHAR pPassword)
{
	time_t   ti;
	struct tm *tm;

	time(&ti); 
	tm = localtime(&ti);
	char szMin[3] = {0};
	char szSec[3] = {0};
	snprintf( szMin, 3, "%02d", tm->tm_min);
	snprintf( szSec, 3, "%02d", tm->tm_sec);

	GenerateChallenge(pUserName);
	pUserName[0] = 0x40; //tmp user flag: '@'
	memcpy(pUserName + 4, szMin, 2);
	memcpy(pUserName + 6, szSec, 2);
	GenerateChallenge(pPassword);
}

void Clear_ConnectInfo(ConnectInfo_t& tInfo)
{
	tInfo.dwID = 0;
	memset(tInfo.szUserName, 0, LENGTH_CHALLENGE+1);
	memset(tInfo.szPassword, 0, LENGTH_CHALLENGE+1);
	Clear_NetInfo(tInfo.tNetInfo);
}

void Clear_NetInfo(NetInfo_t& tInfo)
{
	tInfo.dwPublicIP = 0;
	tInfo.wPublicPortTCP = 0;
	tInfo.wPublicPortUDP = 0;
	tInfo.wLocalPortUDP = 0;
	tInfo.wNetworkType = 0;
	tInfo.listLocalIPs.clear();
}

void Clear_TransClientInfo(TransClientInfo_t& tInfo)
{
	tInfo.bSrcType = 0;
	tInfo.bType = 0;
	tInfo.dwDeviceID = 0;
	tInfo.dwServerID = 0;
	tInfo.dwSessionID = 0;
	tInfo.dwUserID = 0;
	tInfo.nTransmitSessionMode = 0;
	Clear_NetInfo(tInfo.tNetInfo);
}

void Clear_TransServerInfo(TransServerInfo_t& tInfo)
{
	tInfo.bSrcType = 0;
	tInfo.bType = 0;
	tInfo.dwDeviceID = 0;
	tInfo.dwServerID = 0;
	tInfo.dwSessionID = 0;
	tInfo.dwUserID = 0;
	Clear_ConnectInfo(tInfo.tConnectInfo[0]);
	Clear_ConnectInfo(tInfo.tConnectInfo[1]);
}

DWORD CalSendCount_ServerInfo(LIST_SERVERINFO& listInfo, LIST_DWORD& listCount)
{
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = 2*sizeof(DWORD) + sizeof(BYTE) + LENGTH_SERIALNO + sizeof(BYTE) + strlen((const char*)iter->szUserName) +
			LENGTH_PASSWORD + sizeof(DWORD) + sizeof(UINT) + sizeof(BYTE) + strlen((const char*)iter->szPosition);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_UserDevice(MAP_DEVROOMINFO& mapDevRoomInfo, LIST_DEVICEINFO& listInfo, LIST_DWORD& listCount)
{
	LOG_DEBUG(LOG_MAIN,"%s start\n",__FUNCTION__);
	LIST_DEVICEINFO::iterator iter = listInfo.begin();
	MAP_DEVROOMINFO::iterator iter2 = mapDevRoomInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 3*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	LOG_DEBUG(LOG_MAIN,"nMaxLen=%d\n",nMaxLen);
	for (; listInfo.end() != iter; ++iter, ++iter2)
	{
		DWORD dwNum = iter2->second.size();
//		LOG_DEBUG(LOG_MAIN,"Num=%d\n",dwNum);
		DWORD dwLen = 3*sizeof(DWORD) + LENGTH_SERIALNO +
					  sizeof(BYTE) + strlen((const char*)iter->szDeviceName) + 
					  LENGTH_PASSWORD + 
					  sizeof(DWORD) + LENGTH_IMAGEVERSION + LENGTH_USERROOM +
					  2*sizeof(DWORD) + dwNum * (sizeof(DWORD) + LENGTH_USERROOM);
		LOG_DEBUG(LOG_MAIN,"Num %d, dwLen %d\n",dwNum, dwLen);
//		LOG_DEBUG(LOG_MAIN,"3\n");
		if (dwSendLen + dwLen > nMaxLen)
		{
			LOG_DEBUG(LOG_MAIN,"4\n");
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			LOG_DEBUG(LOG_MAIN,"====>Count:%d\n",dwCount);
			dwCount = 1;
		}
		else
		{
//			LOG_DEBUG(LOG_MAIN,"5\n");
			dwSendLen += dwLen;
			dwCount++;
		}
	}
//	LOG_DEBUG(LOG_MAIN,"6\n");
	if (dwCount > 0) listCount.push_back(dwCount);
	LOG_DEBUG(LOG_MAIN,"%s end\n",__FUNCTION__);
	return listCount.size();
}

DWORD CalSendCount_UserDeviceInfo(MAP_DEVROOMINFO& mapDevRoomInfo,LIST_DWORD& listCount)
{
	MAP_DEVROOMINFO::iterator iter = mapDevRoomInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE;
	DWORD dwSendLen = 0, dwCount = 0;
	for (; mapDevRoomInfo.end() != iter; ++iter)
	{
		DWORD dwNum = iter->second.size();
		DWORD dwLen = 2 * sizeof(DWORD) + dwNum * (sizeof(DWORD) + LENGTH_USERROOM);

		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_UserGroup(LIST_GROUPINFO& listInfo, LIST_DWORD& listCount)
{
	LIST_GROUPINFO::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 3*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = 3*sizeof(DWORD) + sizeof(BYTE) + strlen((const char*)iter->szGroupName);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_UserRoom(LIST_ROOMINFO& listInfo, LIST_DWORD& listCount)
{
	LIST_ROOMINFO::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 3*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = 2*sizeof(DWORD) + sizeof(BYTE) + strlen((const char*)iter->szRoom) + LENGTH_PASSWORD;
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_DeviceUser(LIST_SMSINFO& listInfo, LIST_DWORD& listCount)
{
	LIST_SMSINFO::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 3*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		int nBase64Len = Base64EncVal((char*)g_szBase64Enc, (const char*)iter->szMobilePhone, strlen((const char*)iter->szMobilePhone));

		DWORD dwLen = 2*sizeof(DWORD) + 3*sizeof(BYTE) + nBase64Len + strlen((const char*)iter->szDeviceName);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_DevicePush(LIST_PUSHINFO& listInfo, LIST_DWORD& listCount)
{
	LIST_PUSHINFO::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 3*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = 2*sizeof(DWORD) + 3*sizeof(BYTE) + strlen((const char*)iter->szToken);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_DeleteRoom(LIST_DWORD& listInfo, LIST_DWORD& listCount)
{
	LIST_DWORD::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 2*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = sizeof(DWORD);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_RoomIndex(LIST_DWORD& listInfo, LIST_DWORD& listCount)
{
	LIST_DWORD::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 3*sizeof(DWORD) - sizeof(BYTE);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = sizeof(DWORD);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_ClearRooms( MAP_DWORD& mapDeviceID, LIST_DWORD& listCount )
{
	MAP_DWORD::iterator iter = mapDeviceID.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; mapDeviceID.end() != iter; ++iter)
	{
		DWORD dwLen = sizeof(DWORD);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_UpdateDevice( MAP_DWORD& mapDeviceID, LIST_DWORD& listCount )
{
	MAP_DWORD::iterator iter = mapDeviceID.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; mapDeviceID.end() != iter; ++iter)
	{
		DWORD dwLen = sizeof(DWORD);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_UpdateDeviceEx(LIST_DWORD& lstDevID, LIST_DWORD& listCount)
{
	LIST_DWORD::iterator iter = lstDevID.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 2*sizeof(UINT);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; lstDevID.end() != iter; ++iter)
	{
		DWORD dwLen = sizeof(DWORD);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_UpdateSpecialCrowd(LIST_SPECIALCROWD& m_lstSpecialCrowd, LIST_DWORD& listCount)
{
	LIST_SPECIALCROWD::iterator iter = m_lstSpecialCrowd.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; m_lstSpecialCrowd.end() != iter; ++iter)
	{
		DWORD dwLen = 7*sizeof(DWORD) + 2*LENGTH_PHONENUMBER;
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();

}

DWORD CalSendCount_DeviceRoomSum(LIST_ROOMSUM& listInfo, LIST_DWORD& listCount)
{
	LIST_ROOMSUM::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 2*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = 5*sizeof(DWORD) + 2*sizeof(BYTE) + LENGTH_ROOMPWD + strlen((const char*)iter->szRoom);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_DeviceRoomOther(LIST_ROOMOTHER& listInfo, LIST_DWORD& listCount)
{
	LIST_ROOMOTHER::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 2*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = 2*sizeof(DWORD) + 2*sizeof(BYTE) + LENGTH_ROOMPWD + strlen((const char*)iter->szRoom) + 2 * sizeof(DWORD);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen;
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_RoomPushInfo(int nMaxLen, RoomPush_t& tInfo, LIST_DWORD& listCount)
{
	listCount.clear();
	DWORD dwSendLen = 0, dwCount = 0, dwTotalLen = 3*sizeof(DWORD);
	nMaxLen -= dwTotalLen;
	LIST_PUSHINFO::iterator iter = tInfo.lstPushInfo.begin();
	for (; tInfo.lstPushInfo.end() != iter; ++iter)
	{
		DWORD dwLen = 2*sizeof(DWORD) + 3*sizeof(BYTE) + strlen((const char*)iter->szToken);
		dwTotalLen += dwLen;
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back((1<<16)|dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back((1<<16)|dwCount);
	return dwTotalLen;
}

DWORD CalSendCount_RoomUserInfo(int nMaxLen, RoomUser_t& tInfo, LIST_DWORD& listCount)
{
	listCount.clear();
	DWORD dwSendLen = 0, dwCount = 0, dwTotalLen = 3*sizeof(DWORD);
	nMaxLen -= dwTotalLen;
	LIST_SMSINFO::iterator iter = tInfo.lstUserInfo.begin();
	for (; tInfo.lstUserInfo.end() != iter; ++iter)
	{
		int nBase64Len = Base64EncVal((char*)g_szBase64Enc, (const char*)iter->szMobilePhone, strlen((const char*)iter->szMobilePhone));

		DWORD dwLen = 2*sizeof(DWORD) + 3*sizeof(BYTE) + strlen((const char*)iter->szDeviceName) + nBase64Len;
		dwTotalLen += dwLen;
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back((1<<16)|dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back((1<<16)|dwCount);
	return dwTotalLen;
}

DWORD CalSendCount_RoomCardInfo(int nMaxLen, RoomCard_t& tInfo, LIST_DWORD& listCount)
{
	listCount.clear();
	DWORD dwSendLen = 0, dwCount = 0, dwTotalLen = 4*sizeof(DWORD);//dwTotalLen = 3*sizeof(DWORD)
	nMaxLen -= dwTotalLen;
	LIST_CARDINFO::iterator iter = tInfo.lstCardInfo.begin();
	for (; tInfo.lstCardInfo.end() != iter; ++iter)
	{
//		DWORD dwLen = 2*sizeof(BYTE) + strlen((const char*)iter->szCard);
		DWORD dwLen = 2*sizeof(BYTE) + strlen((const char*)iter->szCard) + sizeof(DWORD);
		dwTotalLen += dwLen;
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back((1<<16)|dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back((1<<16)|dwCount);
	return dwTotalLen;
}

DWORD CalSendCount_RoomIndoorInfo(int nMaxLen, RoomIndoor2_t& tInfo, LIST_DWORD& listCount)
{
	listCount.clear();
	DWORD dwSendLen = 0, dwCount = 0, dwTotalLen = 3*sizeof(DWORD);
	nMaxLen -= dwTotalLen;
	LIST_INDOORINFO::iterator iter = tInfo.lstIndoorInfo.begin();
	for (; tInfo.lstIndoorInfo.end() != iter; ++iter)
	{
		DWORD dwLen = sizeof(BYTE) + sizeof(DWORD) + strlen((const char*)iter->szSerialNO);
		dwTotalLen += dwLen;
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back((1<<16)|dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back((1<<16)|dwCount);
	return dwTotalLen;
}

DWORD CalSendCount_RoomPushSwitch(int nMaxLen, RoomPushSwitch_t& tInfo, LIST_DWORD& listCount)
{
	listCount.clear();
	DWORD dwSendLen = 0, dwCount = 0, dwTotalLen = 3*sizeof(DWORD);
	nMaxLen -= dwTotalLen;
	LIST_PUSHSWITCH::iterator iter = tInfo.lstPushSwitch.begin();
	for (; tInfo.lstPushSwitch.end() != iter; ++iter)
	{
		DWORD dwLen = sizeof(DWORD) + sizeof(int);
		dwTotalLen += dwLen;
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back((1<<16)|dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back((1<<16)|dwCount);
	return dwTotalLen;
}

DWORD CalSendCount_DeviceRoomCard(LIST_ROOMCARD& listInfo, LIST_DWORD& listCount)
{
	LIST_ROOMCARD::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 2*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		LIST_DWORD listCountTemp;
		DWORD dwLen = CalSendCount_RoomCardInfo(nMaxLen, *iter, listCountTemp);
		if (dwLen > nMaxLen)
		{
			if (dwCount > 0) listCount.push_back(dwCount);

			LIST_DWORD::iterator pos = listCountTemp.begin();
			for (; pos != listCountTemp.end(); pos++)
			{
				listCount.push_back(*pos);
			}

			dwSendLen = 0;
			dwCount = 0;
		}
		else if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen;
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}


//分包处理
DWORD CalSendCount_DeviceRoomIndoor(LIST_ROOMINDOOR2& listInfo, LIST_DWORD& listCount)
{
	LIST_ROOMINDOOR2::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 2*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		LIST_DWORD listCountTemp;
		DWORD dwLen = CalSendCount_RoomIndoorInfo(nMaxLen, *iter, listCountTemp);
		if (dwLen > nMaxLen)
		{
			if (dwCount > 0) listCount.push_back(dwCount);

			LIST_DWORD::iterator pos = listCountTemp.begin();
			for (; pos != listCountTemp.end(); pos++)
			{
				listCount.push_back(*pos);
			}

			dwSendLen = 0;
			dwCount = 0;
		}
		else if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen;
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_DeviceRoomUser(LIST_ROOMUSER& listInfo, LIST_DWORD& listCount)
{
	LIST_ROOMUSER::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 2*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		LIST_DWORD listCountTemp;
		DWORD dwLen = CalSendCount_RoomUserInfo(nMaxLen, *iter, listCountTemp);
		if (dwLen > nMaxLen)
		{
			if (dwCount > 0) listCount.push_back(dwCount);

			LIST_DWORD::iterator pos = listCountTemp.begin();
			for (; pos != listCountTemp.end(); pos++)
			{
				listCount.push_back(*pos);
			}

			dwSendLen = 0;
			dwCount = 0;
		}
		else if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen;
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_DeviceRoomPush( LIST_ROOMPUSH& listInfo, LIST_DWORD& listCount )
{
	LIST_ROOMPUSH::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 2*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		LIST_DWORD listCountTemp;
		DWORD dwLen = CalSendCount_RoomPushInfo(nMaxLen, *iter, listCountTemp);
		if (dwLen > nMaxLen)
		{
			if (dwCount > 0) listCount.push_back(dwCount);

			LIST_DWORD::iterator pos = listCountTemp.begin();
			for (; pos != listCountTemp.end(); pos++)
			{
				listCount.push_back(*pos);
			}
			
			dwSendLen = 0;
			dwCount = 0;
		}
		else if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen;
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_DeviceRoomPushSwitch(LIST_ROOMPUSHSWITCH& listInfo, LIST_DWORD& listCount)
{
	LIST_ROOMPUSHSWITCH::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 2*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		LIST_DWORD listCountTemp;
		DWORD dwLen = CalSendCount_RoomPushSwitch(nMaxLen, *iter, listCountTemp);
		if (dwLen > nMaxLen)
		{
			if (dwCount > 0) listCount.push_back(dwCount);

			LIST_DWORD::iterator pos = listCountTemp.begin();
			for (; pos != listCountTemp.end(); pos++)
			{
				listCount.push_back(*pos);
			}
			
			dwSendLen = 0;
			dwCount = 0;
		}
		else if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen;
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_DserverConfigureIndex(LIST_SERVERINFO& listInfo, LIST_DWORD& listCount)
{
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 3*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = 2*sizeof(DWORD) + sizeof(BYTE) + LENGTH_SERIALNO + sizeof(BYTE) + strlen((const char*)iter->szUserName) +
			LENGTH_PASSWORD + sizeof(DWORD) + sizeof(UINT) + sizeof(BYTE) + strlen((const char*)iter->szPosition);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_RegisterInfo( LIST_SERVERINFO& listInfo, LIST_DWORD& listCount )
{
	LIST_SERVERINFO::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 4*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = 2*sizeof(DWORD) + sizeof(UINT) + sizeof(BYTE) + strlen((const char*)iter->szPosition);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_ReportDeviceStatus( PUCHAR pMsg, LIST_NOTIFY_PUSHINFO& listInfo, LIST_DWORD& listCount )
{
	LIST_NOTIFY_PUSHINFO::iterator iter = listInfo.begin();
	int nBase64Len = Base64EncVal((char*)g_szBase64Enc, (const char*)pMsg, strlen((const char*)pMsg));
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 3*sizeof(DWORD) - sizeof(BYTE) - nBase64Len;
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = sizeof(DWORD) + 3*sizeof(BYTE) + strlen((const char*)iter->szToken);
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_GetDeviceStatusRep( LIST_DEVICESTATUS& listInfo, LIST_DWORD& listCount )
{
	LIST_DEVICESTATUS::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 3*sizeof(DWORD);
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		int nBase64Len = Base64EncVal((char*)g_szBase64Enc, (const char*)iter->szStatusMsg, strlen((const char*)iter->szStatusMsg));
		DWORD dwLen = 2*sizeof(DWORD) + sizeof(BYTE) + nBase64Len;
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

int CalcPushInfoCount( int nMaxLen, LIST_PUSHINFO& listInfo, int& nCount )
{
	LIST_PUSHINFO::iterator iter = listInfo.begin();
	int nSendLen = 0;
	nCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		int nLen = 2*sizeof(DWORD) + 4*sizeof(BYTE) + strlen((const char*)iter->szToken) + strlen((const char*)iter->szDeviceName);
		if (nSendLen + nLen > nMaxLen) return nSendLen;
		nSendLen += nLen;
		nCount++;
	}
	return nSendLen;
}

void CalcMaxMix( PUCHAR pMsg, PUCHAR pTimeStamp, LIST_SMSINFO& list1, LIST_PUSHINFO& list2, int& nCount1, int& nCount2 )
{
	nCount1 = 0;
	nCount2 = 0;
	int nBase64Len = Base64EncVal((char*)g_szBase64Enc, (const char*)pMsg, strlen((const char*)pMsg));
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE - 2*sizeof(DWORD) - 2*sizeof(BYTE) - nBase64Len - 2*sizeof(UINT) - strlen((const char*)pTimeStamp);

	if (list1.empty())
	{
		CalcPushInfoCount(nMaxLen, list2, nCount2); return;
	}

	int nSmsInfoLen = 0, nRemainLen = MAX_PACKET_LEN;
	int nTempCount1 = 0, nTempCount2 = 0;
	LIST_SMSINFO::iterator iterSmsInfo = list1.begin();
	for (; iterSmsInfo != list1.end(); iterSmsInfo++)
	{
		++nTempCount1;
		int nBase64SmsInfoLen = Base64EncVal((char*)g_szBase64Enc, (const char*)iterSmsInfo->szMobilePhone, strlen((const char*)iterSmsInfo->szMobilePhone));
		int nSmsInfoLen = 2*sizeof(DWORD) + 3*sizeof(BYTE) + nBase64SmsInfoLen + strlen((const char*)iterSmsInfo->szDeviceName);
		nMaxLen -= nSmsInfoLen;
		if (nMaxLen <= 0) break;
		int nTempSendLen = CalcPushInfoCount(nMaxLen, list2, nTempCount2);
		int nTempRemainLen = nMaxLen - nTempSendLen;
		if (nTempRemainLen < nRemainLen)
		{
			nRemainLen = nTempRemainLen;
			nCount1 = nTempCount1;
			nCount2 = nTempCount2;
		}
	}
}

DWORD CalcRealVendorID( DWORD dwVendorID )
{
	return (dwVendorID & 0xffffff);
}

DWORD CalcVendorAppID( DWORD dwVendorID )
{
	return (dwVendorID >> 24);
}

//////////////////////////////////////////////////////////////////////////
// 获取各服务器配置
#include <fstream>

#define CONFIG_DB_HOST				"dong_db_host"
#define CONFIG_DB_SRCDB				"dong_db_srcdb"
#define CONFIG_DB_USERNAME			"dong_db_username"
#define CONFIG_DB_PASSWORD			"dong_db_password"
#define CONFIG_DB_WORKIP			"dong_db_workip"
#define CONFIG_DB_SWITCH			"dong_db_switch"

#define CONFIG_D_DBIP				"dong_d_dbip"
#define CONFIG_D_SN					"dong_d_sn"
#define CONFIG_D_USERNAME			"dong_d_username"
#define CONFIG_D_PASSWORD			"dong_d_password"
#define CONFIG_D_WORKIP				"dong_d_workip"
#define CONFIG_D_EXCHANGE			"dong_d_exchange"
#define CONFIG_D_SWITCH				"dong_d_switch"

#define CONFIG_RELAY_SN				"dong_relay_sn"
#define CONFIG_RELAY_USERNAME		"dong_relay_username"
#define CONFIG_RELAY_PASSWORD		"dong_relay_password"
#define CONFIG_RELAY_WORKIP			"dong_relay_workip"
#define CONFIG_RELAY_DIP			"dong_relay_dip"
#define CONFIG_RELAY_SWITCH			"dong_relay_switch"

#define CONFIG_NTY_DBIP				"dong_nty_dbip"
#define CONFIG_NTY_SN				"dong_nty_sn"
#define CONFIG_NTY_USERNAME			"dong_nty_username"
#define CONFIG_NTY_PASSWORD			"dong_nty_password"
#define CONFIG_NTY_WORKIP			"dong_nty_workip"
#define CONFIG_NTY_SWITCH			"dong_nty_switch"

#define CONFIG_LGN_DBIP				"dong_lgn_dbip"
#define CONFIG_LGN_SN				"dong_lgn_sn"
#define CONFIG_LGN_USERNAME			"dong_lgn_username"
#define CONFIG_LGN_PASSWORD			"dong_lgn_password"
#define CONFIG_LGN_WORKIP			"dong_lgn_workip"
#define CONFIG_LGN_SWITCH			"dong_lgn_switch"

#define CONFIG_STATUS_DBIP			"dong_status_dbip"
#define CONFIG_STATUS_SN			"dong_status_sn"
#define CONFIG_STATUS_USERNAME		"dong_status_username"
#define CONFIG_STATUS_PASSWORD		"dong_status_password"
#define CONFIG_STATUS_WORKIP		"dong_status_workip"
#define CONFIG_STATUS_SWITCH		"dong_status_switch"

// 云存储数据库服务器
#define CONFIG_SDB_HOST				"dong_storagedb_host"
#define CONFIG_SDB_SRCDB			"dong_storagedb_srcdb"
#define CONFIG_SDB_SRCDB2			"dong_storagedb_srcdb2"
#define CONFIG_SDB_USERNAME			"dong_storagedb_username"
#define CONFIG_SDB_PASSWORD			"dong_storagedb_password"
#define CONFIG_SDB_WORKIP			"dong_storagedb_workip"
#define CONFIG_SDB_SWITCH			"dong_storagedb_switch"

// 云存储业务服务器
#define CONFIG_SB_DBIP				"dong_storagebusiness_dbip"
#define CONFIG_SB_SN				"dong_storagebusiness_sn"
#define CONFIG_SB_USERNAME			"dong_storagebusiness_username"
#define CONFIG_SB_PASSWORD			"dong_storagebusiness_password"
#define CONFIG_SB_WORKIP			"dong_storagebusiness_workip"
#define CONFIG_SB_DIP				"dong_storagebusiness_dip"
#define CONFIG_SB_SWITCH			"dong_storagebusiness_switch"
#define CONFIG_SB_HOST3				"dong_sb_host"
#define CONFIG_SB_SRCDB3				"dong_sb_srcdb"
#define CONFIG_SB_USERNAME3		"dong_sb_username"
#define CONFIG_SB_PASSWORD3		"dong_sb_password"

//容联云
#define CONFIG_SYSTEM_USER_ID		"dong_system_userid"
#define CONFIG_SYSTEM_ACCOUNT_SID	"dong_system_accountsid"
#define CONFIG_SYSTEM_AUTH_TOKEN	"dong_system_authtoken"
#define CONFIG_SYSTEM_APP_KEY		"dong_system_appkey"
#define CONFIG_SYSTEM_SWITCH		"dong_system_switch"

void CfgLineEncode(PUCHAR pKey, CSTRING& line)
{
	BYTE szEncBaseLine[256] = {0};
	BYTE szEncDesLine[256] = {0};
	int nEncDesLen = USDK_DESEncode(pKey, (PUCHAR)line.c_str(), line.size(), (PUCHAR)szEncDesLine);
	int nEncBaseLen = Base64EncVal((char*)szEncBaseLine, (const char*)szEncDesLine, nEncDesLen);
	line.assign((const char*)szEncBaseLine, nEncBaseLen);
}

void CfgLineDecode(PUCHAR pKey, CSTRING& line)
{
	//printf("key:%s line:%s\n", pKey, line.c_str());
	BYTE szDecBaseLine[256] = {0};
	BYTE szDecDesLine[256] = {0};
	int nDecBaseLen = Base64DecVal((char*)szDecBaseLine, line.c_str(), line.size());
	int nDecDesLen = USDK_DESDecode(pKey, (PUCHAR)szDecBaseLine, nDecBaseLen, (PUCHAR)szDecDesLine);
	line.assign((const char*)szDecDesLine, nDecDesLen);
	//printf("key:%s decline:%s\n", pKey, line.c_str());
}

bool GetDBServerCfg(ServerCfg_t& tCfgInfo)
{
	std::ifstream in(CONFIG_DONG_ENCODED_FILE);
	if (in.fail())
	{
		LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__); return false;
	}

	std::string line, key, value;
	while (getline(in, line))
	{
		CfgLineDecode((PUCHAR)_CFGENC_KEY_, line);

		if (-1 == DividePair(line, key, value, "=")) continue;

		if (0 == key.compare(CONFIG_DB_HOST))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szHost, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_DB_SRCDB))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSrcDB, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_DB_USERNAME))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szUserName, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_DB_PASSWORD))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szPassword, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_DB_WORKIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szWorkIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_DB_SWITCH))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSwitch, value.c_str(), nCpyLen);
		}
	}
	PrintServerCfg(tCfgInfo);
	std::string server_switch = "on";
	if (0 == server_switch.compare(tCfgInfo.szSwitch)) return true;
	return false;
}

bool GetDServerCfg(ServerCfg_t& tCfgInfo)
{
	std::ifstream in(CONFIG_DONG_ENCODED_FILE);
	if (in.fail())
	{
		LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__); return false;
	}

	memset(&tCfgInfo, 0, sizeof(ServerCfg_t));
	std::string line, key, value;
	while (getline(in, line))
	{
		CfgLineDecode((PUCHAR)_CFGENC_KEY_, line);

		if (-1 == DividePair(line, key, value, "=")) continue;

		if (0 == key.compare(CONFIG_D_DBIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szDBIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_D_SN))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSerialNO, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_D_USERNAME))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szUserName, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_D_PASSWORD))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szPassword, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_D_WORKIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szWorkIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_D_SWITCH))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSwitch, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_D_EXCHANGE))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szExchange, value.c_str(), nCpyLen);
		}
	}
	PrintServerCfg(tCfgInfo);
	std::string server_switch = "on";
	if (0 == server_switch.compare(tCfgInfo.szSwitch)) return true;
	return false;
}

void PrintSystemCfg(SystemCfg_t& tCfgInfo)
{
	printf("szUserID:(%s)\n",tCfgInfo.szUserID);
	printf("szAccountSid:(%s)\n",tCfgInfo.szAccountSid);
	printf("szAuthToken:(%s)\n",tCfgInfo.szAuthToken);
	printf("szAppID:(%s)\n",tCfgInfo.szAppKey);
	printf("szSwitch:(%s)\n",tCfgInfo.szSwitch);
	printf("\n");
}
bool GetSystemCfg(SystemCfg_t& tCfgInfo)
{
	LOG_DEBUG(LOG_MAIN,"%s | Nnmber:1 \n", __FUNCTION__);
	std::ifstream in(CONFIG_DONG_SYSTEM_FILE);
	if(in.fail())
	{
		LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__); return false;
	}
	LOG_DEBUG(LOG_MAIN,"%s | Nnmber:2 \n", __FUNCTION__);

	memset(&tCfgInfo, 0, sizeof(SystemCfg_t));
	std::string line, key, value;	
	while(getline(in, line))
	{
		if(-1 == DividePair(line, key, value, "=")) continue;

		if(0 == key.compare(CONFIG_SYSTEM_USER_ID))
		{
			int nCpyLen = value.size() < LEN_SYSTEM_ITEM ? value.size() : LEN_SYSTEM_ITEM;
			memcpy(tCfgInfo.szUserID, value.c_str(), nCpyLen);				
		}
		else if(0 == key.compare(CONFIG_SYSTEM_ACCOUNT_SID))
		{
			int nCpyLen = value.size() < LEN_SYSTEM_ITEM ? value.size() : LEN_SYSTEM_ITEM;
			memcpy(tCfgInfo.szAccountSid, value.c_str(), nCpyLen);				
		}
		else if(0 == key.compare(CONFIG_SYSTEM_AUTH_TOKEN))
		{
			int nCpyLen = value.size() < LEN_SYSTEM_ITEM ? value.size() : LEN_SYSTEM_ITEM;
			memcpy(tCfgInfo.szAuthToken, value.c_str(), nCpyLen);				
		}
		else if(0 == key.compare(CONFIG_SYSTEM_APP_KEY))
		{
			int nCpyLen = value.size() < LEN_SYSTEM_ITEM ? value.size() : LEN_SYSTEM_ITEM;
			memcpy(tCfgInfo.szAppKey, value.c_str(), nCpyLen);				
		}
		else if(0 == key.compare(CONFIG_SYSTEM_SWITCH))
		{
			int nCpyLen = value.size() < LEN_SYSTEM_ITEM ? value.size() : LEN_SYSTEM_ITEM;
			memcpy(tCfgInfo.szSwitch, value.c_str(), nCpyLen);				
		}
	}

	LOG_DEBUG(LOG_MAIN,"%s | Nnmber:3 | szSwitch:%s \n", __FUNCTION__, tCfgInfo.szSwitch);
	PrintSystemCfg(tCfgInfo);
	std::string server_switch = "on";
	if(0 == server_switch.compare(tCfgInfo.szSwitch))
	{
		LOG_DEBUG(LOG_MAIN,"%s | Nnmber:4 successful \n", __FUNCTION__);
		return true;
	}
	LOG_DEBUG(LOG_MAIN,"%s | Nnmber:4 failed \n", __FUNCTION__);
	return false;
}

bool GetRelayServerCfg(ServerCfg_t& tCfgInfo)
{
	std::ifstream in(CONFIG_DONG_ENCODED_FILE);
	if (in.fail())
	{
		LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__); return false;
	}
	std::string line, key, value;
	while (getline(in, line))
	{
		CfgLineDecode((PUCHAR)_CFGENC_KEY_, line);

		if (-1 == DividePair(line, key, value, "=")) continue;

		if (0 == key.compare(CONFIG_RELAY_DIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szDServerIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_RELAY_SN))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSerialNO, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_RELAY_USERNAME))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szUserName, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_RELAY_PASSWORD))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szPassword, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_RELAY_WORKIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szWorkIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_RELAY_SWITCH))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSwitch, value.c_str(), nCpyLen);
		}
	}
	PrintServerCfg(tCfgInfo);
	std::string server_switch = "on";
	if (0 == server_switch.compare(tCfgInfo.szSwitch)) return true;
	return false;
}

bool GetLgnServerCfg(ServerCfg_t& tCfgInfo)
{
	std::ifstream in(CONFIG_DONG_ENCODED_FILE);
	if (in.fail())
	{
		LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__); return false;
	}
	std::string line, key, value;
	while (getline(in, line))
	{
		CfgLineDecode((PUCHAR)_CFGENC_KEY_, line);

		if (-1 == DividePair(line, key, value, "=")) continue;

		if (0 == key.compare(CONFIG_LGN_DBIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szDBIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_LGN_SN))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSerialNO, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_LGN_USERNAME))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szUserName, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_LGN_PASSWORD))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szPassword, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_LGN_WORKIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szWorkIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_LGN_SWITCH))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSwitch, value.c_str(), nCpyLen);
		}
	}
	PrintServerCfg(tCfgInfo);
	std::string server_switch = "on";
	if (0 == server_switch.compare(tCfgInfo.szSwitch)) return true;
	return false;
}

bool GetNtyServerCfg(ServerCfg_t& tCfgInfo)
{
	std::ifstream in(CONFIG_DONG_ENCODED_FILE);
	if (in.fail())
	{
		LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__); return false;
	}
	std::string line, key, value;
	while (getline(in, line))
	{
		CfgLineDecode((PUCHAR)_CFGENC_KEY_, line);

		if (-1 == DividePair(line, key, value, "=")) continue;

		if (0 == key.compare(CONFIG_NTY_DBIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szDBIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_NTY_SN))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSerialNO, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_NTY_USERNAME))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szUserName, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_NTY_PASSWORD))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szPassword, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_NTY_WORKIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szWorkIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_NTY_SWITCH))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSwitch, value.c_str(), nCpyLen);
		}
	}
	PrintServerCfg(tCfgInfo);
	std::string server_switch = "on";
	if (0 == server_switch.compare(tCfgInfo.szSwitch)) return true;
	return false;
}

bool GetStatServerCfg(ServerCfg_t& tCfgInfo)
{
	std::ifstream in(CONFIG_DONG_ENCODED_FILE);
	if (in.fail())
	{
		LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__); return false;
	}
	std::string line, key, value;
	while (getline(in, line))
	{
		CfgLineDecode((PUCHAR)_CFGENC_KEY_, line);

		if (-1 == DividePair(line, key, value, "=")) continue;

		if (0 == key.compare(CONFIG_STATUS_DBIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szDBIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_STATUS_SN))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSerialNO, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_STATUS_USERNAME))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szUserName, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_STATUS_PASSWORD))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szPassword, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_STATUS_WORKIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szWorkIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_STATUS_SWITCH))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSwitch, value.c_str(), nCpyLen);
		}
	}
	PrintServerCfg(tCfgInfo);
	std::string server_switch = "on";
	if (0 == server_switch.compare(tCfgInfo.szSwitch)) return true;
	return false;
}

bool GetSDBServerCfg(ServerCfg_t& tCfgInfo)
{
	std::ifstream in(CONFIG_DONG_ENCODED_FILE);
	if (in.fail())
	{
		LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__); return false;
	}
	std::string line, key, value;
	while (getline(in, line))
	{
		CfgLineDecode((PUCHAR)_CFGENC_KEY_, line);

		if (-1 == DividePair(line, key, value, "=")) continue;

		if (0 == key.compare(CONFIG_SDB_HOST))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szHost, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SDB_SRCDB))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSrcDB, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SDB_SRCDB2))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSrcDB2, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SDB_USERNAME))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szUserName, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SDB_PASSWORD))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szPassword, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SDB_WORKIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szWorkIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SDB_SWITCH))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSwitch, value.c_str(), nCpyLen);
		}
	}
	PrintServerCfg(tCfgInfo);
	std::string server_switch = "on";
	if (0 == server_switch.compare(tCfgInfo.szSwitch)) return true;
	return false;
}

bool GetSBServerCfg(ServerCfg_t& tCfgInfo)
{
	std::ifstream in(CONFIG_DONG_ENCODED_FILE);
	if (in.fail())
	{
		LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__); return false;
	}
	std::string line, key, value;
	while (getline(in, line))
	{
		CfgLineDecode((PUCHAR)_CFGENC_KEY_, line);

		if (-1 == DividePair(line, key, value, "=")) continue;

		if (0 == key.compare(CONFIG_SB_DBIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szDBIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SB_DIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szDServerIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SB_SN))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSerialNO, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SB_USERNAME))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szUserName, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SB_PASSWORD))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szPassword, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SB_WORKIP))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szWorkIP, value.c_str(), nCpyLen);
		}
		else if (0 == key.compare(CONFIG_SB_SWITCH))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSwitch, value.c_str(), nCpyLen);
		}
		else if(0 == key.compare(CONFIG_SB_HOST3))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szHost3, value.c_str(), nCpyLen);
		}
		else if(0 == key.compare(CONFIG_SB_SRCDB3))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szSrcDB3, value.c_str(), nCpyLen);
		}
		else if(0 == key.compare(CONFIG_SB_USERNAME3))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szUserName3, value.c_str(), nCpyLen);
		}
		else if(0 == key.compare(CONFIG_SB_PASSWORD3))
		{
			int nCpyLen = value.size() < LEN_CFG_ITEM ? value.size() : LEN_CFG_ITEM;
			memcpy(tCfgInfo.szPassword3, value.c_str(), nCpyLen);
		}
	}
	PrintServerCfg(tCfgInfo);
	std::string server_switch = "on";
	if (0 == server_switch.compare(tCfgInfo.szSwitch)) return true;
	return false;
}
void PrintServerCfg(ServerCfg_t& tCfgInfo)
{
	printf("szDBIP:(%s)\n", tCfgInfo.szDBIP);
	printf("szSerialNO:(%s)\n", tCfgInfo.szSerialNO);
	printf("szHost:(%s)\n", tCfgInfo.szHost);
	printf("szSrcDB:(%s)\n", tCfgInfo.szSrcDB);
	printf("szSrcDB2:(%s)\n", tCfgInfo.szSrcDB2);
	printf("szUserName:(%s)\n", tCfgInfo.szUserName);
	printf("szPassword:(%s)\n", tCfgInfo.szPassword);
	printf("szWorkIP:(%s)\n", tCfgInfo.szWorkIP);
	printf("szDServerIP:(%s)\n", tCfgInfo.szDServerIP);
	printf("szHost3:(%s)\n",tCfgInfo.szHost3);
	printf("szSrcDB3:(%s)\n", tCfgInfo.szSrcDB3);
	printf("szUserName3:(%s)\n",tCfgInfo.szUserName3);
	printf("szPassword:(%s)", tCfgInfo.szPassword3);
	printf("szSwitch:(%s)\n", tCfgInfo.szSwitch);
	printf("\n");
}
DWORD g_dwSameIP = 0;
BYTE g_bSameSvrType = 0;
void RemoveSameServer( LIST_SERVERINFO& lstInfo )
{
	LIST_SERVERINFO lstTempInfo;
	LIST_SERVERINFO::iterator iter = lstInfo.begin();
	for (; iter != lstInfo.end(); iter++)
	{
		g_dwSameIP = iter->dwIP;
		g_bSameSvrType = iter->bServerType;
		LIST_SERVERINFO::iterator pos = std::find_if( lstTempInfo.begin(), lstTempInfo.end(), FindSameServer() );
		if (pos != lstTempInfo.end()) continue;
		lstTempInfo.push_back(*iter);
	}
	lstInfo.clear();
	lstInfo.insert(lstInfo.end(), lstTempInfo.begin(), lstTempInfo.end());
}

void AddMulPkt(DWORD dwDeviceID, MulPkt_e eMulPkt, PUCHAR pData, int nLen, DWORD dwCmdFlag, WORD wSegFlag)
{
	CDealMulPktMgr::Instance()->AddMulPkt(dwDeviceID, eMulPkt, pData, nLen, dwCmdFlag, wSegFlag);
}

void DelMulPkt( IDealMulPktSink* pSink )
{
	CDealMulPktMgr::Instance()->DelMulPkt(pSink);
}

void AddMulPktAck( DWORD dwDeviceID, MulPkt_e eMulPkt, DWORD dwCmdFlag )
{
	CDealMulPktMgr::Instance()->AddMulPktAck(dwDeviceID, eMulPkt, dwCmdFlag);
}

void StartSendMulPkt( DWORD dwDeviceID, MulPkt_e eMulPkt, IDealMulPktSink* pSink )
{
	CDealMulPktMgr::Instance()->StartSendMulPkt(dwDeviceID, eMulPkt, pSink);
}

bool IsValidMobilePhone( const char* pMobilePhone )
{
	if (strlen(pMobilePhone) != 11) return false;
	for (int i = 0; i < 11; i++)
	{
		if ( (pMobilePhone[i] < '0') && (pMobilePhone[i] > '9') ) return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool PermissionPass( int nPermission )
{
	if (nPermission == 0) return true;

	srand( (int)time(0) );
	int nRandNo = (int) (1.0*100*rand()/(RAND_MAX+1.0));
	if (nRandNo <= nPermission) return true;
	return false;
}

void GenerateTimeStamp( PUCHAR pTimeStamp )
{
	if (pTimeStamp == NULL) return;
	time_t tTime;
	struct tm tmTime;

	time(&tTime); 
	localtime_r(&tTime, &tmTime);
	snprintf((char*)pTimeStamp, LENGTH_TIMESTAMP+1, "%04d-%02d-%02d %02d:%02d:%02d",
		tmTime.tm_year+1900, tmTime.tm_mon+1, tmTime.tm_mday, tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec);
}

void GenerateDeviceStatusMsg( PUCHAR pDeviceName, PUCHAR pAlarmMsg, PUCHAR pTimeStamp, PUCHAR pOutMsg )
{
	// 消息格式: <device name> <alarm describe> <time>
	snprintf((char*)pOutMsg, LENGTH_MSGCONTENT, "<%s> <%s> <%s>", pDeviceName, pAlarmMsg, pTimeStamp);
}

DWORD ParseStoreIDStr( CSTRING& storeid )
{
	LIST_CSTRING lstStoreID;
	DivideStr(storeid, lstStoreID, ",");
	LIST_CSTRING::iterator iter = lstStoreID.begin();
	for (; iter != lstStoreID.end(); iter++)
	{
		DWORD dwStoreID = atoi(iter->c_str());
		if (dwStoreID) return dwStoreID;
	}
	return 0;
}

const BYTE STORE_TYPE_JPG = 1;
const BYTE STORE_TYPE_MP4 = 11;
const BYTE STORE_TYPE_AVI = 12;

void GenerateStoreKey( StoreKey_t& tKey, PUCHAR pKey )
{
	// "/devid/roomid/reason/timestamp-size-storeid.jpg"
	const char* pType = "jpg";
	if (tKey.bType == STORE_TYPE_JPG) pType = "jpg";
	else if (tKey.bType == STORE_TYPE_MP4) pType = "mp4";
	else if (tKey.bType == STORE_TYPE_AVI) pType = "avi";
	snprintf((char*)pKey, LENGTH_STOREKEY, "/%d/%d/%d/%s-%d-%d.%s", 
		tKey.dwDeviceID, tKey.dwRoomID, tKey.bRecReason, tKey.szTimeStamp, tKey.dwSize, tKey.dwStoreID, pType);
}

DWORD CalSendCount_ListStoreAccountKeys(LIST_STORE_ACCOUNTKEYS& listInfo, LIST_DWORD& listCount)
{
	LIST_STORE_ACCOUNTKEYS::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE
		- sizeof(BYTE) - 3*sizeof(DWORD) // length of StorageTag_t
		- sizeof(UINT); // Count
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		LIST_DWORD listCountTemp;
		DWORD dwLen = CalSendCount_StoreAccountKeys(nMaxLen, *iter, listCountTemp);
		if (dwLen > nMaxLen)
		{
			if (dwCount > 0) listCount.push_back(dwCount);

			LIST_DWORD::iterator pos = listCountTemp.begin();
			for (; pos != listCountTemp.end(); pos++)
			{
				listCount.push_back(*pos);
			}

			dwSendLen = 0;
			dwCount = 0;
		}
		else if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen;
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

DWORD CalSendCount_StoreAccountKeys( int nMaxLen, StoreAccountKeys_t& tInfo, LIST_DWORD& listCount )
{
	listCount.clear();
	DWORD dwSendLen = 0, dwCount = 0;

	// length of StorageAccount_t
	DWORD dwTotalLen = sizeof(DWORD) + 4*sizeof(BYTE)
		+ strlen((const char*)tInfo.tAccount.szAccessKey) + strlen((const char*)tInfo.tAccount.szSecretKey)
		+ strlen((const char*)tInfo.tAccount.szBucket) + strlen((const char*)tInfo.tAccount.szDomain);

	nMaxLen -= dwTotalLen;
	dwSendLen = sizeof(DWORD); 
	LIST_STOREKEY::iterator iter = tInfo.lstKey.begin();
	for (; tInfo.lstKey.end() != iter; ++iter)
	{
		DWORD dwLen = 4*sizeof(DWORD) + 2*sizeof(BYTE) + LENGTH_TIMESTAMP2; // length of StoreKey_t
		dwTotalLen += dwLen;
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen   = sizeof(DWORD); 
			dwSendLen  += dwLen;
			dwTotalLen += sizeof(DWORD);
			listCount.push_back((1<<16)|dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) {
		dwTotalLen += sizeof(DWORD);
		listCount.push_back((1<<16)|dwCount);
	}
	return dwTotalLen;
}

void Packet_StoreAccountKeys( CPutBuffer& buffer, StoreAccountKeys_t& tInfo )
{
	int nCount = tInfo.lstKey.size();
	buffer << tInfo.tAccount.dwStoreID;
	PutVariableStr(buffer, (PUCHAR)tInfo.tAccount.szAccessKey);
	PutVariableStr(buffer, (PUCHAR)tInfo.tAccount.szSecretKey);
	PutVariableStr(buffer, (PUCHAR)tInfo.tAccount.szBucket);
	PutVariableStr(buffer, (PUCHAR)tInfo.tAccount.szDomain);
	buffer << nCount;

	LIST_STOREKEY::iterator iter = tInfo.lstKey.begin();
	for (; iter != tInfo.lstKey.end(); iter++)
	{
		buffer << iter->dwDeviceID << iter->dwRoomID << iter->dwSize << iter->dwStoreID << iter->bType << iter->bRecReason;
		buffer << CByteArrayBuffer((PUCHAR)iter->szTimeStamp, LENGTH_TIMESTAMP2);
	}
}

DWORD CalSendCount_DownloadUrlsRep( LIST_STORE_KEYURL& listInfo, LIST_DWORD& listCount )
{
	LIST_STORE_KEYURL::iterator iter = listInfo.begin();
	int nMaxLen = MAX_PACKET_LEN - PACKET_HEADER_SIZE
		- sizeof(BYTE) - 3*sizeof(DWORD) // length of StorageTag_t
		- sizeof(UINT); // Count
	DWORD dwSendLen = 0, dwCount = 0;
	for (; listInfo.end() != iter; ++iter)
	{
		DWORD dwLen = 4*sizeof(DWORD) + 3*sizeof(BYTE) + strlen((const char*)iter->szUrl) + LENGTH_TIMESTAMP2;
		if (dwSendLen + dwLen > nMaxLen)
		{
			dwSendLen = dwLen; 
			listCount.push_back(dwCount);
			dwCount = 1;
		}
		else
		{
			dwSendLen += dwLen;
			dwCount++;
		}
	}
	if (dwCount > 0) listCount.push_back(dwCount);
	return listCount.size();
}

//数据库简单密码转换
void SelectPwdMark(std::string& pVall)
{
	int nCount = sizeof(S_ENCPWD_MARK)/sizeof(struct RoomEncPwdMark_t);
	for(int i = 0; i < nCount; i++)
	{
		if(0 == pVall.compare(S_ENCPWD_MARK[i].mark)){
			pVall = (std::string)S_ENCPWD_MARK[i].encpwd;
			break;
		}
	}
//	cout << "pVall1 = " << pVall << endl;
}
void InsertPwdMark(char* pVall)
{
	int nCount = sizeof(S_ENCPWD_MARK)/sizeof(struct RoomEncPwdMark_t);
	for(int i = 0; i < nCount; i++)
	{
		if(0 == strcmp(S_ENCPWD_MARK[i].encpwd,pVall)){
			memcpy(pVall,(char*)S_ENCPWD_MARK[i].mark,sizeof((char*)S_ENCPWD_MARK[i].mark));
			break;
		}
	}
	//	cout << "pVall1 = " << pVall << endl;
}
//
bool ProPhone(char strPhone[64])
{
	if(strlen(strPhone) != 19) return false;
	char  phone[64] = {0};
	strncpy(phone, strPhone + 7, strlen(strPhone)-8);
	memset(strPhone,0,strlen(strPhone));
	memcpy(strPhone, phone, strlen(phone));
	return true;
}

bool GetLocalFileList(const char* pLocalDir, LIST_LOCAL_FILE& lstFileInfo)
{
	char *path =NULL;
	path = getcwd(NULL, 0);
	LOG_DEBUG(LOG_UTIL, "CurrentPath = %s,  LocalListPath = %s\n", path, pLocalDir);

	DIR* pDir = NULL;
	pDir = opendir(pLocalDir);
	if(pDir == NULL) 
	{
		LOG_DEBUG(LOG_UTIL, "opendir failed\n");
		free(path);
		return false;
	}
	chdir(pLocalDir);
	lstFileInfo.clear();

	struct stat stFBuf;
	FileInfo_t tInfo;
	struct dirent *pDirent = NULL;
	while((pDirent = readdir(pDir)) != NULL)
	{
		//把当前目录.上一级目录..及隐藏文件都去掉,避免死循环遍历目录
		if(strncmp(pDirent->d_name, ".", 1) == 0) continue; 		
		//判断该文件是否是目录，及是否已搜索了3层(这里只搜索3层目录)
		if(-1 == stat(pDirent->d_name, &stFBuf) || S_ISDIR(stFBuf.st_mode))	continue;

		tInfo.strFileName.assign(pDirent->d_name);
		tInfo.cTime = stFBuf.st_atime;
		LOG_DEBUG(LOG_UTIL, "name %s\t atime %d\n", tInfo.strFileName.c_str(), tInfo.cTime);
		lstFileInfo.push_back(tInfo);
	}
	LOG_DEBUG(LOG_UTIL, "Total = %d\n", lstFileInfo.size());
	closedir(pDir);
	chdir(path);
	free(path);
	return true;
}

//获取磁盘使用情况
bool GetVstatdisk(const char *dirName,UInt64& freeMegaBytes,UInt64& totalMegaBytes)
{
	struct statvfs64 vfsInfo; 
	memset(&vfsInfo, 0, sizeof(struct statvfs64));
	int nRet = statvfs64(dirName, &vfsInfo);
	if (nRet != 0 || vfsInfo.f_bsize == 0) 
	{
		if(nRet) perror("GetVstatdisk");
		freeMegaBytes = 0;
		totalMegaBytes = 0;
		return false;
	}
	UInt64 sector = 1048576;  // 1024 * 1024 = 1048576
	UInt64 freeTmp = (UInt64)((UInt64)vfsInfo.f_bfree * (UInt64)vfsInfo.f_bsize); 
	freeMegaBytes =  freeTmp / sector; 
	UInt64 totalTmp = (UInt64)((UInt64)vfsInfo.f_blocks * (UInt64)vfsInfo.f_bsize); 
	totalMegaBytes = totalTmp / sector; 
	LOG_DEBUG(LOG_UTIL, "dirname: %s, free: %lld, total: %lld\n", dirName, freeMegaBytes, totalMegaBytes);
	return true;
}
//检查磁盘空间，判断磁盘剩余空间是否小于10%
bool CheckDiskSpace()
{
	const char *dirName = "/home/www/default/VResource_d/";
	UInt64 freeMegaBytes = 0;
	UInt64 totalMegaBytes = 0;
	if (false == GetVstatdisk(dirName, freeMegaBytes, totalMegaBytes)) return false;
	DWORD dwUsage = 100 * freeMegaBytes / totalMegaBytes;
	LOG_DEBUG(LOG_UTIL, "Usage = %d%\n", dwUsage);
	//如果剩余空间小于10%,启动删除机制
	if (dwUsage < 10)
	{
		GetDelFile(dirName);
		return false;
	}
	return true;
}
//获取本地列表，并找到最早下载过的文件
bool GetDelFile(const char* pLocalDir)
{
	LIST_LOCAL_FILE lstFileInfo;
	if (false == GetLocalFileList(pLocalDir,lstFileInfo)) return false;
	if (lstFileInfo.size() <= 10) return false;
	
	FileInfo_t tOldInfo;
	LIST_LOCAL_FILE::iterator iter = lstFileInfo.begin();
	tOldInfo.strFileName = iter->strFileName;
	tOldInfo.cTime = iter->cTime;
	for (; lstFileInfo.end() != iter; ++iter)
	{
		if (tOldInfo.cTime > iter->cTime)
		{
			tOldInfo.strFileName = iter->strFileName;
			tOldInfo.cTime = iter->cTime;
		}
	}
	LOG_DEBUG(LOG_UTIL,"OldFName =%s, OldTime = %d\n", tOldInfo.strFileName.c_str(), tOldInfo.cTime);
	char szDelFile[256] = {0};
	sprintf(szDelFile, (const char*)"rm -rf %s%s", pLocalDir, tOldInfo.strFileName.c_str());
	LOG_DEBUG(LOG_UTIL,"%s\n", szDelFile);
	LIST_CSTRING listStr;
	ExCommand(szDelFile, listStr, true);
	return true;
}

#define MAX_LINE_LEN 484
bool ExCommand( const char* pCommand, LIST_CSTRING& listStr, bool bPrint /*= false*/ )
{
	listStr.clear();
	FILE* fp = popen(pCommand, "r");
	if (!fp) {
		LOG_ERR(LOG_MAIN, "popen %s failed\n", pCommand);
		return false;
	}
	char szCommand[MAX_LINE_LEN+1] = {0};
	while( fgets(szCommand, sizeof(szCommand), fp) ) 
	{
		CSTRING tempStr; tempStr.assign((const char*)szCommand);
		listStr.push_back(tempStr);
		if (bPrint) LOG_DEBUG(LOG_MAIN, "%s", szCommand);
		memset(szCommand, 0, MAX_LINE_LEN);
	}
	if (-1 == pclose(fp))
	{
		LOG_DEBUG(LOG_MAIN, "pclose(err:%d:%s)\n", errno, strerror(errno)); return false;
	}
	return true;
}
