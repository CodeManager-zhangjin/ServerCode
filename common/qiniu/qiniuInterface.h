#ifndef _QINIU_INTERFACE_H_
#define _QINIU_INTERFACE_H_
#include "DataStruct.h"

class IHttpDownloadSink
{
public:
	virtual ~IHttpDownloadSink(){}
	// nPesent: 1~100; bFinish: 下载是否完成
	virtual void OnDownloadStatus(char* pUrl, int nPesent, bool bFinish = false) = 0;
};
class IHttpDownload
{
public:
	virtual ~IHttpDownload(){}
	// 下载文件  nPesent: 1~100; bFinish: 下载是否完成
	virtual bool Download(char* pUrl, const char* localFile, IHttpDownloadSink* pSink) = 0;
	virtual void Destroy() = 0;
};
IHttpDownload* Reg_HttpDownload();

bool Qiniu_Init();
void Qiniu_Finish();

// 创建上传凭证
void Qiniu_CreateUploadToken(const char* pAccessKey, const char* pSecretKey, const char* pBucket, char* pUploadToken);

// 获取下载外链
void Qiniu_GetDowndloadUrls(const char* pAccessKey, const char* pSecretKey, const char* pDomain, const char* key, char* pUrl);

bool Qiniu_DeleteFile(const char* pAccessKey, const char* pSecretKey, const char* pBucket, const char* key);

bool Qiniu_GetDownloadToken(const char* accesskey, const char* secretkey, char* szToken);


#endif
