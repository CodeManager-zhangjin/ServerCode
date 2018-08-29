#ifndef UTILITY_DATA_STRUCT_H
#define UTILITY_DATA_STRUCT_H

#include "DataStruct.h"


const DWORD LEN_CHALLENGESTRING			= 16;
const DWORD LEN_BASE64PWD				= 60;
const DWORD MAXLEN_FILE_PATH			= 260;

enum TimerReason_e
{
	TIMER_NORMAL,
	TIMER_DELETEUNLOCKRECORD,
	TIMER_INSERTUNLOCKRECORD,
	TIMER_SMS_LIMIT,
	TIMER_SPECIALCROWD,
	TIMER_ADVERT,
};

// ˝æ›ø‚ºÚµ•√‹¬ÎÃÊªª
const static struct RoomEncPwdMark_t
{
	const char *encpwd;
	const char *mark;
}S_ENCPWD_MARK[]=
{
	{"13bbf54a6850c393fb8d1b2b3bba997b",  "#1" }, // Œ¥…Ë÷√√‹¬Î£¨√‹¬ÎŒ™ø’
	{"81dc9bdb52d04dc20036dbd8313ed055",  "#2" }, // 1234
	{"e10adc3949ba59abbe56e057f20f883e",  "#3" }, // 123456
	{"c33367701511b4f6020ec61ded352059",  "#4" }
};


#endif
