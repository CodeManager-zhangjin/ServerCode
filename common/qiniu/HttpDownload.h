#ifndef CHTTPDOWNLOAD_H
#define CHTTPDOWNLOAD_H
#include <pthread.h>
#include <curl/curl.h>
#include <arpa/inet.h>
#include "qiniuInterface.h"
#include "NetworkInterface.h"
#include "singleton.h"
#include "Log.h"


#define DOWNLOAD_LISTEN_PORT 5555
#define DOWNLOAD_SEND_PORT 5556

#define HTTP_URL_LENGTH					128
#define HTTP_LOCAL_FILE_LENGTH		128
#define HTTP_BUFFER_LENGTH				1400
#define HTTP_SELECT_INTERVAL			100*1000

#define LOCAL_HOST					"127.0.0.1"
#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         50*1024*1024//最大的文件大小
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     1 //1秒回掉一次进度

#define THREAD_CMD_DOWNLOAD			0x01
#define THREAD_CMD_DOWNLOAD_REP     0x02

struct myprogress {
    double lastruntime;
    CURL *curl;
};

typedef enum
{
	DS_Unknown,
	DS_Downloading,
	DS_Finished,
}DownloadStep_e;

class CHttpDownload : public IHttpDownload, public INetConnectionSink
{
    DECLARE_SINGLETON(CHttpDownload)
public:
    CHttpDownload();
    virtual ~CHttpDownload();
    static void* HttpThread(void* pParam);
    static CHttpDownload* m_pThis;

	virtual int OnConnect(int nReason, INetConnection *pCon) {return 0;}
	virtual int OnDisconnect(int nReason, INetConnection *pCon) {return 0;}
    virtual int OnReceive(unsigned char *pData, int nLen, INetConnection *pCon);
	virtual int OnSend(INetConnection *pCon) {return 0;}

public:
    //IHttpDownload
    bool Download(char* pUrl, const char* pLocalFile, IHttpDownloadSink* pSink);
    void Destroy();
		
private:
    bool Init();
    void Clear();
    void Fini();
    int BindSocket(WORD wPort);
	int DownloadFile();	
	void ThreadLoop();
	void OnThreadCommand ();    
    int OnDownloadPercent(DWORD dwPercent, BYTE cResult);
    void SetThis(CHttpDownload* pThis);

private:
	    static size_t WriteData(void *ptr, size_t size, size_t nmemb, FILE *stream);
		//curl 进度回掉函数，做版本兼容用
         static int xferinfo (void * p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
         static int older_progress (void * p, double dltotal, double dlnow, double ultotal, double ulnow);

private:
    INetConnection* m_pMainCon;
    struct sockaddr_in m_tMainAddr;
    int m_nLocalAddr;
    int m_nSubSocket;

    pthread_t m_nThreadID;
	pthread_mutex_t m_tMutex;

   CURL* m_pCurl;
   FILE* m_pFile;
   IHttpDownloadSink* m_pSink;
   BYTE m_szDownloadUrl [HTTP_URL_LENGTH + 1];
   BYTE m_szLocalFile[HTTP_LOCAL_FILE_LENGTH + 1];
   BYTE m_szBuff[HTTP_BUFFER_LENGTH + 1];
  static  BYTE m_cDownloadStep;// 0，未知，1，正在下载，2，下载完成
};

#endif // CHTTPDOWNLOAD_H
