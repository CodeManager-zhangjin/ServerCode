#ifndef _DDLGN_INTERFACE_H_
#define _DDLGN_INTERFACE_H_

#include "DataStruct.h"

bool DDLgnInit(PUCHAR pDServerSN);
void DDLgnFinish();

class IDDLoginSink
{
public:
	virtual ~IDDLoginSink(){}
	// ��ȡע���������Ϣ
	virtual int OnGetDServerInfo(DServerInfo_t& tInfo) = 0;
};
void DDLogin_SetSink(IDDLoginSink* pSink);

#endif //_DDLGN_INTERFACE_H_
