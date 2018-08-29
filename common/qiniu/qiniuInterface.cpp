#include "qiniuInterface.h"
#include "rs.h"
#include "http.h"
#include "Log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "HttpDownload.h"

Qiniu_Client g_clsClient;

extern char* QINIU_ACCESS_KEY;
extern char* QINIU_SECRET_KEY;

IHttpDownload* Reg_HttpDownload()
{
	return CHttpDownload::Instance();
}

bool Qiniu_Init()
{
	Qiniu_Servend_Init(-1);
	Qiniu_Client_InitMacAuth(&g_clsClient, 1024, NULL);
	return true;
}

void Qiniu_Finish()
{
	Reg_HttpDownload()->Destroy();
	Qiniu_Client_Cleanup(&g_clsClient);
	Qiniu_Servend_Cleanup();
}

void Qiniu_CreateUploadToken(const char* pAccessKey, const char* pSecretKey, const char* pBucket, char* pUploadToken)
{
	Qiniu_Mac mac;
	mac.accessKey = pAccessKey;
	mac.secretKey = pSecretKey;

	Qiniu_RS_PutPolicy putPolicy;
	Qiniu_Zero(putPolicy);
	putPolicy.scope = pBucket;
	char* pUptoken = Qiniu_RS_PutPolicy_Token(&putPolicy, &mac);//服务器获取上传凭证

	//printf("Qiniu_CreateUploadToken:\n%s\n", pUptoken);
	memcpy(pUploadToken, pUptoken, strlen((const char*)pUptoken));
	Qiniu_Free(pUptoken);
}

void Qiniu_GetDowndloadUrls( const char* pAccessKey, const char* pSecretKey, const char* pDomain, const char* key, char* pUrl )
{
	Qiniu_Mac mac;
	mac.accessKey = pAccessKey;
	mac.secretKey = pSecretKey;

	Qiniu_RS_GetPolicy getPolicy;
	Qiniu_Zero(getPolicy);

	char* baseUrl = Qiniu_RS_MakeBaseUrl(pDomain, key);
	char* url = Qiniu_RS_GetPolicy_MakeRequest(&getPolicy, baseUrl, &mac);

	//printf("DowndloadUrl:\n%s\n", url);
	memcpy(pUrl, url, strlen((const char*)url));
	Qiniu_Free(baseUrl);
	Qiniu_Free(url);
}

bool Qiniu_DeleteFile(const char* pAccessKey, const char* pSecretKey, const char* pBucket, const char* key)
{
	QINIU_ACCESS_KEY = (char*)pAccessKey;
	QINIU_SECRET_KEY = (char*)pSecretKey;
	Qiniu_Error error = Qiniu_RS_Delete(&g_clsClient, pBucket, key);
	if (error.code != 200)
	{
		LOG_DEBUG(LOG_DB_SERVER, "Qiniu_DeleteFile:%d:%s\n", error.code, error.message);
		return false;
	}
	return true;
}

bool Qiniu_GetDownloadToken(const char* pAccessKey, const char* pSecretKey, char* szToken)
{
	LOG_DEBUG(LOG_DB_SERVER,"%s\n",__FUNCTION__);
	Qiniu_Mac mac;
	mac.accessKey = pAccessKey;
	mac.secretKey = pSecretKey;
	Qiniu_RS_GetPolicy getPolicy;
	Qiniu_Zero(getPolicy);
	char* token = Qiniu_RS_GetPolicy_Token(&getPolicy, &mac);
	memcpy(szToken, token, strlen((const char*)token));
	LOG_DEBUG(LOG_DB_SERVER,"szToken=%s\n", token);
	return true;
}