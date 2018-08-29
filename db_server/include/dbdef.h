#ifndef _DBDEF_H_
#define _DBDEF_H_
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>

typedef std::string		CSTRING;

//1.时间结构体
struct DateTime_t
{
	unsigned int Year;
	unsigned int Mon;
	unsigned int Day;
	unsigned int Hour;
	unsigned int Min;
	unsigned int Sec;
};

#define LENGTH_TABLENAME 20

const unsigned int LENGTH_DATE = 4;
struct DateInfo_t
{
	char Year[LENGTH_DATE+1];
	char Month[LENGTH_DATE+1];
	char Day[LENGTH_DATE+1];
};
const unsigned int LENGTH_TIME = 2;
struct TimeInfo_t
{
	char Hour[LENGTH_TIME+1];
	char Min[LENGTH_TIME+1];
	char Sec[LENGTH_TIME+1];
};
struct TimeStampTransDate_t
{
	DateInfo_t dateInfo;
	TimeInfo_t timeInfo;
};
struct DelTabNameTimeStamp_t
{
	char Name[12];
	int start, end;
	char Name1[12];
	int start1, end1;
	char Name2[12];
	int start2, end2;
	char Name3[12];
	int start3, end3;
};
//////////////////////////////////////////////////////////////////////////
struct PushTokenType_t
{
	CSTRING setGtToken;
	CSTRING setBaiduToken;
	CSTRING setHWToken;
	CSTRING setMIToken; 
	CSTRING setJGToken;
	CSTRING setMZToken;
};
#endif

