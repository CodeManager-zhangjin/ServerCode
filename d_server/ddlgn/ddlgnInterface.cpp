#include "ddlgnInterface.h"
#include "ddLogin.h"

void DDLogin_SetSink(IDDLoginSink* pSink)
{
	CDDLoginMgr::Instance()->SetSink(pSink);
}

bool DDLgnInit(PUCHAR pDServerSN)
{
	return CDDLoginMgr::Instance()->Start(pDServerSN);
}

void DDLgnFinish()
{
	CDDLoginMgr::DeleteInstance();
}
