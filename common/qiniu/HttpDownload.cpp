#include "HttpDownload.h"


IMPLEMENT_SINGLETON(CHttpDownload)
CHttpDownload* CHttpDownload::m_pThis = NULL;
BYTE CHttpDownload::m_cDownloadStep = DS_Unknown;

CHttpDownload::CHttpDownload()
{
    m_nThreadID = 0;
    m_pCurl = NULL;
    m_pSink = NULL;
    memset(m_szDownloadUrl, 0, HTTP_URL_LENGTH + 1);
    memset(m_szLocalFile, 0, HTTP_LOCAL_FILE_LENGTH + 1);
    memset(m_szBuff, 0, HTTP_BUFFER_LENGTH + 1);
    Init();
}

CHttpDownload::~CHttpDownload()
{
    Clear();
    Fini();
}

bool CHttpDownload::Init()
{
	LOG_DEBUG(LOG_MAIN,"CHttpDownload::%s\n", __FUNCTION__);
    curl_global_init(CURL_GLOBAL_ALL);
    m_pMainCon = CreateRawUdpCon(this, 0, DOWNLOAD_LISTEN_PORT);//5555
    if(!m_pMainCon) {
        LOG_WARN(LOG_MAIN, "Fail to CreateRawUdpCon on port: %d\n", DOWNLOAD_LISTEN_PORT);
        return false;
    }

    m_tMainAddr.sin_family = AF_INET;
    m_tMainAddr.sin_port = htons(DOWNLOAD_LISTEN_PORT);
    m_tMainAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    m_nLocalAddr = ntohl(inet_addr(LOCAL_HOST));

    m_nSubSocket = BindSocket(DOWNLOAD_SEND_PORT);
    if(m_nSubSocket <= 0) {
        LOG_WARN(LOG_MAIN,"HttpDownload creat socket on port: %d error\n", DOWNLOAD_SEND_PORT);
        return false;
    }
    SetThis(this);
    pthread_create( &m_nThreadID, NULL, CHttpDownload::HttpThread, NULL );
    return true;
}

void CHttpDownload::Clear()
{
	LOG_DEBUG(LOG_MAIN,"CHttpDownload::%s\n", __FUNCTION__);
    if (m_pFile)
    {
        fclose(m_pFile);
        m_pFile = NULL;
    }
    if (m_pCurl)
    {
        curl_easy_cleanup(m_pCurl);
        m_pCurl = NULL;
        curl_global_cleanup();
    }
    memset(m_szDownloadUrl, 0, HTTP_URL_LENGTH + 1);
    memset(m_szLocalFile, 0, HTTP_LOCAL_FILE_LENGTH + 1);
}

void CHttpDownload::Fini()
{
	LOG_DEBUG(LOG_MAIN,"CHttpDownload::%s\n", __FUNCTION__);
	curl_global_cleanup();
	if(m_nThreadID > 0) pthread_join (m_nThreadID, NULL);    
    if(m_pMainCon != NULL)
    {
        NetworkDestroyConnection(m_pMainCon);
        m_pMainCon = NULL;
    }
}
void* CHttpDownload::HttpThread(void* pParam)
{
    m_pThis->ThreadLoop();
    return NULL;
}

int CHttpDownload::OnReceive(unsigned char *pData, int nLen, INetConnection *pCon)
{
    int nIndex = 0;
    if(pData[0] != THREAD_CMD_DOWNLOAD_REP) return -1;
    nIndex ++;
    DWORD dwPercent = 0;
    BYTE cResult = 0;//等于1表示下载完成
    memcpy(&dwPercent, pData + nIndex, sizeof(int)); nIndex += sizeof(int);
    memcpy(&cResult, pData + nIndex, sizeof(BYTE)); nIndex += sizeof(BYTE);
    if(m_pSink != NULL) m_pSink->OnDownloadStatus((char*)m_szDownloadUrl, (int)dwPercent, cResult != 0 ? true : false);
}
void CHttpDownload::ThreadLoop ()
{
	LOG_DEBUG(LOG_MAIN, "CHttpDownload::%s\n", __FUNCTION__);
	fd_set fds; 
	while(1)
	{
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = HTTP_SELECT_INTERVAL;
		FD_ZERO(&fds);
		if(m_nSubSocket > 0)  FD_SET(m_nSubSocket, &fds);
		int rt = select (m_nSubSocket + 1, &fds, NULL, NULL, &timeout);
		if(rt == 0)   continue;
        if(FD_ISSET (m_nSubSocket, &fds))    OnThreadCommand ();
	}
}
void CHttpDownload::OnThreadCommand ()
{	
	LOG_DEBUG(LOG_MAIN,"CHttpDownload::%s\n", __FUNCTION__);
	struct sockaddr_in sin;
	socklen_t tLen = sizeof (sin);
	int nReadLen = recvfrom (m_nSubSocket, m_szBuff, HTTP_BUFFER_LENGTH, 0, (struct sockaddr*)&sin, &tLen);
	WORD wCmd = m_szBuff[0];
    switch(wCmd)
    {
    case THREAD_CMD_DOWNLOAD:
        DownloadFile();
        break;
    default:
        break;
    }
}

int CHttpDownload::OnDownloadPercent(DWORD dwPercent, BYTE cResult)
{
    memset(m_szBuff, 0, sizeof(m_szBuff));
    int nIndex = 0;
    m_szBuff[nIndex] = THREAD_CMD_DOWNLOAD_REP; nIndex++;
    memcpy(m_szBuff + nIndex, &dwPercent, sizeof(DWORD)); nIndex += sizeof(DWORD);
    memcpy(m_szBuff + nIndex, &cResult, sizeof(BYTE)); nIndex += sizeof(BYTE);
	return sendto( m_nSubSocket, m_szBuff, nIndex, 0,
                  (const struct sockaddr*)&m_tMainAddr,
                   sizeof(struct sockaddr) );
}

void CHttpDownload::SetThis(CHttpDownload *pThis)
{
    m_pThis = pThis;
}
bool CHttpDownload::Download(char *pUrl, const char *pLocalFile, IHttpDownloadSink *pSink)
{	
	LOG_DEBUG(LOG_MAIN,"CHttpDownload::%s\n", __FUNCTION__);
    if(m_cDownloadStep == DS_Downloading) return false;
	if(m_cDownloadStep == DS_Unknown) m_cDownloadStep = DS_Downloading;
    if(pUrl && pLocalFile && pSink) 
    {
        memcpy(m_szDownloadUrl, pUrl, (int)strlen(pUrl));
		memcpy (m_szLocalFile, pLocalFile, (int)strlen (pLocalFile));
        m_pSink = pSink;
		BYTE cCmd = THREAD_CMD_DOWNLOAD;
		m_pMainCon->SendTo (&cCmd, 1, m_nLocalAddr, DOWNLOAD_SEND_PORT);
		LOG_DEBUG(LOG_MAIN,"CHttpDownload::%s 2\n", __FUNCTION__);
		return true;
	}
	return false;
}

void CHttpDownload::Destroy()
{
	LOG_DEBUG(LOG_MAIN,"CHttpDownload::%s\n", __FUNCTION__);
    Clear();
}

int CHttpDownload::BindSocket(WORD wPort)
{
	LOG_DEBUG(LOG_MAIN,"CHttpDownload::%s\n", __FUNCTION__);
    int sock = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( sock == -1 )
    {
        LOG_DEBUG(LOG_MAIN, "Fail to socket for sync on port: %d\n", wPort);
        return -1;
    }
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons( wPort );
    sin.sin_addr.s_addr = htonl(INADDR_ANY); // inet_addr( LOCAL_HOST );
    socklen_t nLen = sizeof( sin );
    if (bind(sock, (struct sockaddr*)&sin, nLen) != 0 )
    {
        LOG_DEBUG(LOG_MAIN,"Bind sync failed on port: %d\n", wPort);
        close( sock );
        sock = -1;
    }
    return sock;
}

int CHttpDownload::DownloadFile ()
{
	LOG_DEBUG(LOG_MAIN,"CHttpDownload::%s\n", __FUNCTION__);
    m_pCurl = curl_easy_init();
    CURLcode res = CURLE_OK;
    m_pFile = fopen ((const char*)m_szLocalFile, "wb");
    curl_easy_setopt(m_pCurl, CURLOPT_URL, (const char*)m_szDownloadUrl);

    struct myprogress prog;
    prog.lastruntime = 0;
    prog.curl = m_pCurl;

    curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSFUNCTION, &older_progress);
    curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, &prog);
#if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(m_pCurl, CURLOPT_XFERINFOFUNCTION, &xferinfo);
    curl_easy_setopt(m_pCurl, CURLOPT_XFERINFODATA, &prog);
#endif

    curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1);
    // 设置重定向的最大次数
    curl_easy_setopt(m_pCurl, CURLOPT_MAXREDIRS, 5);
    curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &WriteData);
    curl_easy_setopt (m_pCurl, CURLOPT_WRITEDATA, m_pFile);

    res = curl_easy_perform(m_pCurl);

    if (res != CURLE_OK)
    {
        LOG_DEBUG(LOG_MAIN, "!!! Response code: %d\n", res);
    }
    Clear();
    return res;
}


size_t CHttpDownload::WriteData (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written = fwrite (ptr, size, nmemb, stream);
	return written;
}

/* this is how the CURLOPT_XFERINFOFUNCTION callback works */
 int CHttpDownload::xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    struct myprogress *myp = (struct myprogress *)p;
    CURL *curl = myp->curl;
    double curtime = 0;

    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

    /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */

	if(dlnow == dltotal && dlnow > 0 && m_cDownloadStep == DS_Downloading) m_cDownloadStep = DS_Finished;
	if( m_cDownloadStep == DS_Finished)
	{
		m_pThis->OnDownloadPercent(100, 1);
		m_cDownloadStep = DS_Unknown;
	}
	
    if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) 
	{
        myp->lastruntime = curtime;
		LOG_DEBUG (LOG_MAIN, "TOTAL TIME: %f\n", curtime);
		LOG_DEBUG (LOG_MAIN, " DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T"\n", dlnow, dltotal);
		DWORD dwPercent = dlnow * 100 / dltotal;
		LOG_DEBUG(LOG_MAIN, "Percent = %d\n", dwPercent);
        m_pThis->OnDownloadPercent((int)dwPercent, dlnow != dltotal ? 0 : 1);
	}

    if(dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
        return 1;
    return 0;
}

/* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */
int CHttpDownload::older_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow)
{
    return xferinfo(p, (curl_off_t)dltotal, (curl_off_t)dlnow, (curl_off_t)ultotal, (curl_off_t)ulnow);
}

