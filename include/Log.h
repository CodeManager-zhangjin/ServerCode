#ifndef _NOTIFY_LOG_H_
#define _NOTIFY_LOG_H_

#include "VigoLog.h"

enum LogTagIndex_e
{
	LOG_DB_CLIENT,
	LOG_DB_SERVER,
	LOG_PUSH_IOS,
	LOG_PUSH_BAIDUYUN,
	LOG_OPENSSL,
	LOG_MAIN,
	LOG_UTIL,
	LOG_NETLISTEN,
	LOG_UNKNOW,
};

static const char* g_szModuleTag[] =
{
	"DBClient",
	"DBServer",
	"Pushios",
	"PushBaiduyun",
	"Openssl",
	"Main",
	"Util",
	"NetListen",
	"Unknow",
};

inline void LOG_EMERG(LogTagIndex_e eIndex, const char *szFormat, ...)
{
	va_list var_arg;
	va_start(var_arg,szFormat);
	VigoLog(VIGO_LOG_LEVEL_EMERG, (char*)g_szModuleTag[eIndex], (char*)szFormat, var_arg);
	va_end (var_arg);
}

inline void LOG_ALERT(LogTagIndex_e eIndex, const char *szFormat, ...)
{
	va_list var_arg;
	va_start(var_arg,szFormat);
	VigoLog(VIGO_LOG_LEVEL_ALERT, (char*)g_szModuleTag[eIndex], (char*)szFormat, var_arg);
	va_end (var_arg);
}

inline void LOG_CRIT(LogTagIndex_e eIndex, const char *szFormat, ...)
{
	va_list var_arg;
	va_start(var_arg,szFormat);
	VigoLog(VIGO_LOG_LEVEL_CRIT, (char*)g_szModuleTag[eIndex], (char*)szFormat, var_arg);
	va_end (var_arg);
}

inline void LOG_ERR(LogTagIndex_e eIndex, const char *szFormat, ...)
{
	va_list var_arg;
	va_start(var_arg,szFormat);
	VigoLog(VIGO_LOG_LEVEL_ERR, (char*)g_szModuleTag[eIndex], (char*)szFormat, var_arg);
	va_end (var_arg);
}

inline void LOG_WARN(LogTagIndex_e eIndex, const char *szFormat, ...)
{
	va_list var_arg;
	va_start(var_arg,szFormat);
	VigoLog(VIGO_LOG_LEVEL_WARNING, (char*)g_szModuleTag[eIndex], (char*)szFormat, var_arg);
	va_end (var_arg);
}
inline void LOG_NOTICE(LogTagIndex_e eIndex, const char *szFormat, ...)
{
	va_list var_arg;
	va_start(var_arg,szFormat);
	VigoLog(VIGO_LOG_LEVEL_NOTICE, (char*)g_szModuleTag[eIndex], (char*)szFormat, var_arg);
	va_end (var_arg);
}
inline void LOG_INFO(LogTagIndex_e eIndex, const char *szFormat, ...)
{
	va_list var_arg;
	va_start(var_arg,szFormat);
	VigoLog(VIGO_LOG_LEVEL_INFO, (char*)g_szModuleTag[eIndex], (char*)szFormat, var_arg);
	va_end (var_arg);
}
inline void LOG_DEBUG(LogTagIndex_e eIndex, const char *szFormat, ...)
{
	va_list var_arg;
	va_start(var_arg,szFormat);
	VigoLog(VIGO_LOG_LEVEL_DEBUG, (char*)g_szModuleTag[eIndex], (char*)szFormat, var_arg);
	va_end (var_arg);
}
inline void LOG_DETAIL(LogTagIndex_e eIndex, const char *szFormat, ...)
{
	va_list var_arg;
	va_start(var_arg,szFormat);
	VigoLog(VIGO_LOG_LEVEL_DETAIL, (char*)g_szModuleTag[eIndex], (char*)szFormat, var_arg);
	va_end (var_arg);
}

#define LOG_ASSERT_RETVOID(index, expr) \
	do { \
		if (!(expr)) { \
			LOG_CRIT(index, "Assert failed: file=%s line=%d expr=%s\n",__FILE__,__LINE__,#expr); \
			return; \
		} \
	}while(0);
	
#define LOG_ASSERT_RET(index, expr, rv) \
	do { \
		if (!(expr)) { \
			LOG_CRIT(index, "Assert failed: file=%s line=%d expr=%s\n",__FILE__,__LINE__,#expr); \
			return rv;\
		} \
	}while(0);

#define LOG_ASSERT(index, expr) \
	do { \
		if (!(expr)) { \
			LOG_CRIT(index, "Assert failed: file=%s line=%d expr=%s\n",__FILE__,__LINE__,#expr); \
		} \
	}while(0);

#endif // _NOTIFY_LOG_H_
