#ifndef __VIGO_LOG_H
#define __VIGO_LOG_H
#include <stdarg.h>

#ifdef WIN32
#   define LIBVIGOLOG_DLL_EXPORTS
#	ifdef LIBVIGOLOG_DLL_EXPORTS
#		define LIBVIGOLOG_EXPORT __declspec(dllexport)
#	else
#		define LIBVIGOLOG_EXPORT __declspec(dllimport)
#	endif 
#else
#	define LIBVIGOLOG_EXPORT 
#endif

#if defined(UNIX) || defined(_LINUX_NVR_)
#define DEFAULT_LOG_PATH				"/var/log/"
#else
#define DEFAULT_LOG_PATH				"./"
#endif

#define VIGO_LOG_LEVEL_NONE			0
#define	VIGO_LOG_LEVEL_EMERG		1
#define	VIGO_LOG_LEVEL_ALERT		2
#define	VIGO_LOG_LEVEL_CRIT			3
#define	VIGO_LOG_LEVEL_ERR			4
#define	VIGO_LOG_LEVEL_WARNING		5
#define	VIGO_LOG_LEVEL_NOTICE		6
#define VIGO_LOG_LEVEL_INFO			7
#define VIGO_LOG_LEVEL_DEBUG		8
#define VIGO_LOG_LEVEL_DETAIL		9

#define DEFAULT_LOG_CONFIG_FILE			"trace.cfg"
#define DEFAULT_MAX_FILE_SIZE			10240		/*10M*/
#define DEFAULT_LOG_INTERVAL			720			/*30days*/
#define DEFAULT_LOG_LEVEL				VIGO_LOG_LEVEL_ERR


#ifdef __cplusplus
extern "C" {
#endif
	LIBVIGOLOG_EXPORT void VigoLog( unsigned char nTraceLevel, char *szPrefix, 
		                            char* szFormat, va_list var_arg );
	LIBVIGOLOG_EXPORT void VigoLogInit(char* strConfigFile);
	LIBVIGOLOG_EXPORT void VigoLogFinish();
#ifdef __cplusplus
}
#endif

#endif //__VIGO_LOG_H

