#ifndef _DBSDK_H_
#define _DBSDK_H_
#include "dbdef.h"
#include "DataStruct.h"


class IVisitor
{
public:
	virtual ~IVisitor() {}
	bool string2DataTime(const char *str, DateTime_t& tInfo);
	bool InsertVisitorTabName(const char *str, char* pName);
	void GetDateTime(DateTime_t& tInfo);
	bool SelectVisitorTabName(char* pName);
	void GetYearMon(char* strYM);
	void GetNextYM(char* strYM);
	void GetNameYM(char* pName,char* strYM);
	bool GetNextNameYM(char* tName,char* strYM);
};
class IUnlock
{
public:
	virtual ~IUnlock() {}
	int GetDateTime(TimeStampTransDate_t& tInfo);
	int GetDateTime(TimeInfo_t& timeInfo);
	int GetDateTime(DateInfo_t& tDateInfo);
	int GetTimeStampTransDate(TimeStampTransDate_t tInfo,time_t time_s);
	int PrintDateTime(DateInfo_t& tInfo);
	int PrintDateTime(TimeInfo_t& tInfo);
	int Days(DateInfo_t tInfo);
	time_t strtotime(DateInfo_t& tInfo,char *const date, int tt);
	int	TableName(DateInfo_t& tInfo,char tableName[LENGTH_TABLENAME]);
	int YN_Del(TimeInfo_t tInfo);
	int delDate(DateInfo_t& tInfo);
	int RecordsDel(DelTabNameTimeStamp_t& tab, int iFlag);
	bool GetTabName(char tabName[LENGTH_TABLENAME]);
	bool GetOnTabName(char tabName[LENGTH_TABLENAME]);
	bool IsSmsUser(time_t dwTimestamp);
};
class IAlarm
{
public:
	virtual ~IAlarm() {}
	bool GetAlarmTabName(CSTRING& tabName, AlarmStatus_t& tInfo);
	bool GetAlarmTabName(AlarmStatus_t tInfo, CSTRING& tabName);
	bool IsCheckTime(char szTimeStamp[20]);
};
#endif
