#include "DBSDK.h"

/////////////////////////////////IUnlock////////////////////////////////////////
//获取时间年月日时分秒
int IUnlock::GetDateTime(TimeStampTransDate_t& tInfo)
{
	time_t time_s = time(NULL);
	time_s += (8 * 3600);
	struct tm *pdatetime = gmtime((const time_t*)&time_s);
	snprintf(tInfo.dateInfo.Year, 5, "%04d", pdatetime->tm_year+1900);
	snprintf(tInfo.dateInfo.Month, 3, "%02d", pdatetime->tm_mon+1);
	snprintf(tInfo.dateInfo.Day, 3, "%02d", pdatetime->tm_mday);
	snprintf(tInfo.timeInfo.Hour, 3, "%02d", pdatetime->tm_hour);
	snprintf(tInfo.timeInfo.Min, 3, "%02d", pdatetime->tm_min);
	snprintf(tInfo.timeInfo.Sec, 3, "%02d", pdatetime->tm_sec);
	return 0;
}
//获取本地时间
int IUnlock::GetDateTime(TimeInfo_t& timeInfo)
{
	time_t time_s = time(NULL);
	time_s += (8 * 3600);
	const struct tm *pdatetime = gmtime((const time_t*)&time_s);
	snprintf(timeInfo.Hour, 3, "%02d", pdatetime->tm_hour);
	snprintf(timeInfo.Min, 3, "%02d", pdatetime->tm_min);
	snprintf(timeInfo.Sec, 3, "%02d", pdatetime->tm_sec);
	return 0;
}
//获取本地日期
int IUnlock::GetDateTime(DateInfo_t& tDateInfo)
{
	time_t time_s = time(NULL);
	time_s += (8 * 3600);
	struct tm *pdatetime = gmtime((const time_t*)&time_s);
	snprintf(tDateInfo.Year, 5, "%04d", pdatetime->tm_year+1900);
	snprintf(tDateInfo.Month, 3, "%02d", pdatetime->tm_mon+1);
	snprintf(tDateInfo.Day, 3, "%02d", pdatetime->tm_mday);
	return 0;
}
//通过时间戳获取转换后的日期
int IUnlock::GetTimeStampTransDate(TimeStampTransDate_t tInfo,time_t time_s)
{	//平台时间后一天
	time_t time_local = time(NULL) + (8 + 24) * 3600;
	struct tm *pdatetimelocal = gmtime((const time_t*)&time_local);
	//开门时间
	time_s = time_s + 8 * 3600;//转化为北京时区
	struct tm *pdatetime = gmtime((const time_t*)&time_s);

	if(time_s > time_local){
		snprintf(tInfo.dateInfo.Year, 5, "%04d", pdatetimelocal->tm_year+1900);
		snprintf(tInfo.dateInfo.Month, 3, "%02d", pdatetimelocal->tm_mon+1);
		snprintf(tInfo.dateInfo.Day, 3, "%02d", pdatetimelocal->tm_mday);
		snprintf(tInfo.timeInfo.Hour, 3, "%02d", pdatetimelocal->tm_hour);
		snprintf(tInfo.timeInfo.Min, 3, "%02d", pdatetimelocal->tm_min);
		snprintf(tInfo.timeInfo.Sec, 3, "%02d", pdatetimelocal->tm_sec);
		return -1;
	}
	snprintf(tInfo.dateInfo.Year, 5, "%04d", pdatetime->tm_year+1900);
	snprintf(tInfo.dateInfo.Month, 3, "%02d", pdatetime->tm_mon+1);
	snprintf(tInfo.dateInfo.Day, 3, "%02d", pdatetime->tm_mday);
	snprintf(tInfo.timeInfo.Hour, 3, "%02d", pdatetime->tm_hour);
	snprintf(tInfo.timeInfo.Min, 3, "%02d", pdatetime->tm_min);
	snprintf(tInfo.timeInfo.Sec, 3, "%02d", pdatetime->tm_sec);
	return 0;
}
//判断要删除月份的最大天数
int IUnlock::Days(DateInfo_t tInfo)
{
	int year = atoi(tInfo.Year);
	int month = atoi(tInfo.Month);
	switch (month)
	{
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		return 31;
		break;
	case 2:
		if(year%400==0 ||(year%100!=0 && year%4==0))
			return 29;
		else 
			return 28;
		break;
	case 4:
	case 6:
	case 9:
	case 11:
		return 30;
		break; 
	default:
		return 0;
		break;
	}       
} 
//通过日期获取转换后的时间戳，date格式为[%Y%m%d%H%M%S]
time_t IUnlock::strtotime(DateInfo_t& tInfo,char *const date, int tt)
{
	long int DayTime = 86400;
	time_t ft = 0;
	int days = Days(tInfo);
	char format[] = "%Y%m%d%H%M%S";
	struct tm tm;
	strptime(date, format, &tm);

	if((atoi(tInfo.Day) + tt) > days){
		return 0;
	}
	ft = mktime(&tm) + (tt * DayTime);
	return ft;
}
//打印日期
int IUnlock::PrintDateTime(DateInfo_t& tInfo)
{
	printf("%04s-%02s-%02s\n",tInfo.Year, tInfo.Month, tInfo.Day);
	return 0;
}
//打印时间
int IUnlock::PrintDateTime(TimeInfo_t& tInfo)
{
	printf("%02s:%02s:%02s\n",tInfo.Hour, tInfo.Min, tInfo.Sec);
	return 0;
}
//通过日期确定要添加/删除的表名称
int IUnlock::TableName(DateInfo_t& tInfo, char tableName[LENGTH_TABLENAME])
{
	int table = atoi(tInfo.Month);
	int sflag = atoi(tInfo.Day);
	if(atoi(tInfo.Year) < 1970) return false;
	if(table < 1 || table > 12) return false;
	if(sflag < 1 || sflag > 31) return false;
	if(table == 1 || table == 7){
		if(sflag <= 15)
			strcpy(tableName,"unlocklog11");
		else
			strcpy(tableName,"unlocklog12");
	}else if(table == 2 || table == 8){
		if(sflag <= 15)
			strcpy(tableName,"unlocklog21");
		else
			strcpy(tableName,"unlocklog22");
	}else if(table == 3 || table == 9){
		if(sflag <= 15)
			strcpy(tableName,"unlocklog31");
		else
			strcpy(tableName,"unlocklog32");
	}else if(table == 4 || table == 10){
		if(sflag <= 15)
			strcpy(tableName,"unlocklog41");
		else
			strcpy(tableName,"unlocklog42");
	}else if(table == 5 || table == 11){
		if(sflag <= 15)
			strcpy(tableName,"unlocklog51");
		else
			strcpy(tableName,"unlocklog52");
	}else if(table == 6 || table == 12){
		if(sflag <= 15)
			strcpy(tableName,"unlocklog61");
		else
			strcpy(tableName,"unlocklog62");
	}
	return true;
}
//通过时间判断是否可以删除
int IUnlock::YN_Del(TimeInfo_t tInfo)
{
	unsigned int tHour = atoi(tInfo.Hour);
	if((tHour >= 0) && (tHour < 2)){
		return 1;
	}
	return 0;
}
//通过当前日期判断删除日期
int IUnlock::delDate(DateInfo_t &tInfo)
{
	int month = atoi(tInfo.Month);
	int days   = Days(tInfo);
	if(atoi(tInfo.Day) > days)
		sprintf(tInfo.Day,"%02d",days);

	if(month <= 6){
		sprintf(tInfo.Year,"%04d",atoi(tInfo.Year)-1);
		sprintf(tInfo.Month,"%02d",atoi(tInfo.Month)+6);
	}else if(month > 6){
		sprintf(tInfo.Month,"%02d",atoi(tInfo.Month)-6);
	}
	return 0;
}
//通过当前日期判断删除日期的时间戳
int IUnlock::RecordsDel(DelTabNameTimeStamp_t& tab,int iFlag)
{
	//判断是否到删除的时间
	int tFlag = 0;
	TimeStampTransDate_t tInfo;
	GetDateTime(tInfo.dateInfo);//
	GetDateTime(tInfo.timeInfo);//
	if(iFlag == 1){
		tFlag = YN_Del(tInfo.timeInfo);//
		if(0 == tFlag)
			return false; 
	}
	else
	{
		tFlag = iFlag;
	}

	long int DayTime = 86400;//8小时
	char tBegin[20],tEnd[20];//要删除的开始结束时间戳


	delDate(tInfo.dateInfo);//通过当前日期判断要删除的日期
	if(atoi(tInfo.dateInfo.Day) > Days(tInfo.dateInfo)){
		sprintf(tInfo.dateInfo.Day,"%02d",Days(tInfo.dateInfo));
		printf("%s,%d\n",tInfo.dateInfo.Day,Days(tInfo.dateInfo));
	}
	sprintf(tBegin,"%s%s%s%s%s%s",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day, tInfo.timeInfo.Hour, tInfo.timeInfo.Min, tInfo.timeInfo.Sec);
	time_t tm_s = strtotime(tInfo.dateInfo,tBegin,0);
	printf("Del Date:%s,MaxDays:%d,timestamp:%d\n",tBegin,Days(tInfo.dateInfo),tm_s);
	if(tm_s == 0) {
		return false;
	}	

	GetTimeStampTransDate(tInfo,(tm_s - 3*DayTime));
	TableName(tInfo.dateInfo,tab.Name3);
	GetTimeStampTransDate(tInfo,(tm_s - 2*DayTime));
	TableName(tInfo.dateInfo,tab.Name2);
	GetTimeStampTransDate(tInfo,(tm_s - 1*DayTime));
	TableName(tInfo.dateInfo,tab.Name1);
	GetTimeStampTransDate(tInfo,(tm_s - 0*DayTime));
	TableName(tInfo.dateInfo,tab.Name);//获取要删除的表名称

	if(1 == tFlag){//0-6
		sprintf(tBegin,"%s%s%s000000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
		sprintf(tEnd,"%s%s%s060000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
	}else if(2 == tFlag){//6-8
		sprintf(tBegin,"%s%s%s060000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
		sprintf(tEnd,"%s%s%s080000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
	}else if(3 == tFlag){//8-12
		sprintf(tBegin,"%s%s%s080000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
		sprintf(tEnd,"%s%s%s120000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
	}else if(4 == tFlag){//12-16
		sprintf(tBegin,"%s%s%s120000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
		sprintf(tEnd,"%s%s%s160000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
	}else if(5 == tFlag){//16-18
		sprintf(tBegin,"%s%s%s160000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
		sprintf(tEnd,"%s%s%s180000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
	}else if(6 == tFlag){//18-24
		sprintf(tBegin,"%s%s%s180000",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
		sprintf(tEnd,"%s%s%s235959",tInfo.dateInfo.Year, tInfo.dateInfo.Month, tInfo.dateInfo.Day);
	}

	tab.start3 = strtotime(tInfo.dateInfo,tBegin,-3);
	tab.start2 = strtotime(tInfo.dateInfo,tBegin,-2);
	tab.start1 = strtotime(tInfo.dateInfo,tBegin,-1);
	tab.start  = strtotime(tInfo.dateInfo,tBegin,0);

	tab.end3 = strtotime(tInfo.dateInfo,tEnd,-3);
	tab.end2 = strtotime(tInfo.dateInfo,tEnd,-2);
	tab.end1 = strtotime(tInfo.dateInfo,tEnd,-1);
	tab.end  = strtotime(tInfo.dateInfo,tEnd,0);	
	printf("tBegin:%s, tEnd:%s\n",tBegin, tEnd);
	printf("Flag:%d, startTime:%d, endTime:%d\n",tFlag,tab.start3,tab.end3);
	printf("Flag:%d, startTime:%d, endTime:%d\n",tFlag,tab.start2,tab.end2);
	printf("Flag:%d, startTime:%d, endTime:%d\n",tFlag,tab.start1,tab.end1);
	printf("Flag:%d, startTime:%d, endTime:%d\n",tFlag,tab.start,tab.end);
	return true;
}

//根据时间戳获取表名
bool IUnlock::GetTabName(char tabName[LENGTH_TABLENAME])
{
	time_t time_local = time(NULL);// + 8  * 3600;
	struct tm *pdatetimelocal = gmtime((const time_t*)&time_local);
	int year = pdatetimelocal->tm_year+1900;
	int mon = pdatetimelocal->tm_mon + 1;
	int day  = pdatetimelocal->tm_mday;
	int Seq1 = (mon <= 6) ? mon : (mon - 6);
	int Seq2 = (day <= 15) ? 1 : 2;
	sprintf(tabName,"unlocklog%d%d",Seq1, Seq2);
//	printf("tabName = %s\n", tabName);
	return 0;
}
bool IUnlock::GetOnTabName(char tabName[LENGTH_TABLENAME])
{
	if(!strcmp(tabName,"unlocklog11")){strcpy(tabName,"unlocklog62");}
	else if(!strcmp(tabName,"unlocklog12")){strcpy(tabName,"unlocklog11");}
	else if(!strcmp(tabName,"unlocklog21")){strcpy(tabName,"unlocklog12");}
	else if(!strcmp(tabName,"unlocklog22")){strcpy(tabName,"unlocklog21");}
	else if(!strcmp(tabName,"unlocklog31")){strcpy(tabName,"unlocklog22");}
	else if(!strcmp(tabName,"unlocklog32")){strcpy(tabName,"unlocklog31");}
	else if(!strcmp(tabName,"unlocklog41")){strcpy(tabName,"unlocklog32");}
	else if(!strcmp(tabName,"unlocklog42")){strcpy(tabName,"unlocklog41");}
	else if(!strcmp(tabName,"unlocklog51")){strcpy(tabName,"unlocklog42");}
	else if(!strcmp(tabName,"unlocklog52")){strcpy(tabName,"unlocklog51");}
	else if(!strcmp(tabName,"unlocklog61")){strcpy(tabName,"unlocklog52");}
	else if(!strcmp(tabName,"unlocklog62")){strcpy(tabName,"unlocklog61");}
	else 	return false;	
	return false;
}
//判断是否发送短信
bool IUnlock::IsSmsUser(time_t dwTimestamp)
{
	const int date2016 = 1451577600;
	const int hour2  = 60 * 60 *2; //2小时
	const int days = 3600 * 24;//1天
	time_t time_local = time(NULL) - date2016;
	int timeDiff = abs(time_local - dwTimestamp);
	if( timeDiff > (1 * days) &&  timeDiff < (1 * days + hour2)){	return true;	}
	if( timeDiff > (2 * days) &&  timeDiff < (2 * days + hour2)){	return true;	}
	if( timeDiff > (3 * days) &&  timeDiff < (3 * days + hour2)){	return true;	}
	if( timeDiff > (4 * days) &&  timeDiff < (4 * days + hour2)){	return true;	}
	if( timeDiff > (5 * days) &&  timeDiff < (5 * days + hour2)){	return true;	}
	return false;
}


///////////////////////////////IVisitor//////////////////////////////////////////
//1.将字符串转换为时间结构体
bool IVisitor::string2DataTime(const char *str, DateTime_t& tInfo)
{
	memset(&tInfo,0,sizeof(DateTime_t));
	if(strlen(str) != 14) return false;
	char year[5],mon[3],day[3],hour[3],min[3],sec[3];
	for(int i = 0; i < 14; i++)
	{
		if(i < 4) year[i] = str[i];
		else if(i < 6) mon[i%2] = str[i];
		else if(i < 8) day[i%2] = str[i];
		else if(i < 10)hour[i%2] = str[i];
		else if(i < 12)min[i%2] = str[i];
		else if(i < 14)sec[i%2] = str[i];
	}
	tInfo.Year = atoi(year);
	tInfo.Mon  = atoi(mon);
	tInfo.Day  = atoi(day);
	tInfo.Hour = atoi(hour);
	tInfo.Min  = atoi(min);
	tInfo.Sec  = atoi(sec);

	//	printf("%04d/%02d/%02d %02d:%02d:%02d\n",tInfo.Year,tInfo.Mon,tInfo.Day,tInfo.Hour,tInfo.Min,tInfo.Sec);
	return true;
}

//2.根据时间字符串获得存入数据库时的表名
bool IVisitor::InsertVisitorTabName(const char *str, char* pName)
{
	DateTime_t tInfo;
	if(!string2DataTime(str,tInfo)) return false;
	const char* tabName = "storekey";
	int t1 = tInfo.Mon > 6  ? (tInfo.Mon - 6) : tInfo.Mon;
	int t2 = tInfo.Day > 15 ? 2 : 1;
	sprintf(pName,"%s%d%d",tabName,t1,t2);
	//	printf("%s\n",pName);
	return true;
}

//3.获取当前时间年月日时分秒
void IVisitor::GetDateTime(DateTime_t& tInfo)
{
	time_t time_s = time(NULL);
	time_s += (8 * 3600);
	struct tm *pdatetime = gmtime((const time_t*)&time_s);
	tInfo.Year = pdatetime->tm_year + 1900;
	tInfo.Mon  = pdatetime->tm_mon + 1;
	tInfo.Day  = pdatetime->tm_mday;
	tInfo.Hour = pdatetime->tm_hour;
	tInfo.Min  = pdatetime->tm_min;
	tInfo.Sec  = pdatetime->tm_sec;
	//	printf("%04d/%02d/%02d %02d:%02d:%02d\n",tInfo.Year,tInfo.Mon,tInfo.Day,tInfo.Hour,tInfo.Min,tInfo.Sec);
}

//4.根据当前时间获得查询访客表名
bool IVisitor::SelectVisitorTabName(char* pName)
{
	DateTime_t tInfo;
	GetDateTime(tInfo);
	const char* tabName = "storekey";
	int t1 = tInfo.Mon > 6  ? (tInfo.Mon - 6) : tInfo.Mon;
	int t2 = tInfo.Day > 15 ? 2 : 1;
	sprintf(pName,"%s%d%d",tabName,t1,t2);	
	//	printf("%s,%s\n",pName);
	return true;
}

//6.获取当前时间年月
void IVisitor::GetYearMon(char* strYM)
{
	time_t time_s = time(NULL);
	time_s += (8 * 3600);
	struct tm *pdatetime = gmtime((const time_t*)&time_s);
	int y = pdatetime->tm_year + 1900;
	int m  = pdatetime->tm_mon + 1;
	sprintf(strYM, "%04d%02d\0",y,m);
	//	printf("%s\n",strYM);
}

//7.根据当前年月获得上一个年月
void IVisitor::GetNextYM(char* strYM)
{
	int y = atoi(strYM)/100;
	int m = atoi(strYM)%100;
	if(m == 1){ m = 12; y -= 1;}
	else{ m -= 1;}
	sprintf(strYM,"%04d%02d\0",y,m);
	//	printf("%s\n",strYM);
}

//8.获取表名和年月
void IVisitor::GetNameYM(char* pName,char* strYM)
{
	SelectVisitorTabName(pName);
	GetYearMon(strYM);
}
//9.下一个表名和年月
bool IVisitor::GetNextNameYM(char* tName,char* strYM)
{
	if(!strcmp(tName,"storekey11")){strcpy(tName,"storekey62");GetNextYM(strYM);return true;}
	if(!strcmp(tName,"storekey12")){strcpy(tName,"storekey11");return true;}
	if(!strcmp(tName,"storekey21")){strcpy(tName,"storekey12");GetNextYM(strYM);return true;}
	if(!strcmp(tName,"storekey22")){strcpy(tName,"storekey21");return true;}
	if(!strcmp(tName,"storekey31")){strcpy(tName,"storekey22");GetNextYM(strYM);return true;}
	if(!strcmp(tName,"storekey32")){strcpy(tName,"storekey31");return true;}
	if(!strcmp(tName,"storekey41")){strcpy(tName,"storekey32");GetNextYM(strYM);return true;}
	if(!strcmp(tName,"storekey42")){strcpy(tName,"storekey41");return true;}
	if(!strcmp(tName,"storekey51")){strcpy(tName,"storekey42");GetNextYM(strYM);return true;}
	if(!strcmp(tName,"storekey52")){strcpy(tName,"storekey51");return true;}
	if(!strcmp(tName,"storekey61")){strcpy(tName,"storekey52");GetNextYM(strYM);return true;}
	if(!strcmp(tName,"storekey62")){strcpy(tName,"storekey61");return true;}
	return false;
}
//////////////////////////////////////////////////////////////////////////
//使用平台时间戳获取表名
bool IAlarm::GetAlarmTabName(CSTRING& tabName, AlarmStatus_t& tInfo)
{
	char tabFirstSeq[3] = {"\0"};
	std::string tableName("alarmrecord");
	char cTimeStamp[20];

	time_t time_local = time(NULL) +  (8 * 3600);
	struct tm *pdatetimelocal = gmtime((const time_t*)&time_local);
	int year = pdatetimelocal->tm_year+1900;
	int mon = pdatetimelocal->tm_mon + 1;
	int day  = pdatetimelocal->tm_mday;
	int hour = pdatetimelocal->tm_hour;
	int min = pdatetimelocal->tm_min;
	int sec = pdatetimelocal->tm_sec;
	int Seq1 = (mon <= 6) ? mon : (mon - 6);
	int Seq2 = (day <= 15) ? 1 : 2;
	sprintf(tabFirstSeq,"%d%d",Seq1, Seq2);
	tabName = tableName + tabFirstSeq;
//	snprintf(cTimeStamp, 20, "%04d-%02d-%02d %02d:%02d:%02d", 
//		year, mon, day, hour, min, sec);
//	memcpy(tInfo.szTimeStamp, cTimeStamp, LENGTH_TIMESTAMP);
	tInfo.dwTimeStamp = time_local;
	return true;
}

bool IAlarm::GetAlarmTabName(AlarmStatus_t tInfo, CSTRING& tabName)
{
	if( tInfo.szTimeStamp[0] == '\0' ){	return false;	}
	if( false == IsCheckTime((char*)tInfo.szTimeStamp)){	return false;	}

	char tabFirstSeq[3] = {"\0"};
	CSTRING tableName("alarmrecord");
	char mon[3] = {"\0"};
	char day[3] = {"\0"};
	sprintf(mon,"%c%c",tInfo.szTimeStamp[6], tInfo.szTimeStamp[7]);
	sprintf(day,"%c%c",tInfo.szTimeStamp[9], tInfo.szTimeStamp[10]);

	int m = atoi(mon);
	int d = atoi(day);
	int f1 = m > 6 ? m - 6 : m;
	int f2 = d > 15 ? 1 : 2;
	sprintf(tabFirstSeq,"%d%d", f1, f2);
	tabName = tableName + tabFirstSeq;
	return true;
}

bool IAlarm::IsCheckTime(char szTimeStamp[20])
{
	char cTimeStamp[20];
	time_t time_local = time(NULL) +  (8 * 3600);
	struct tm *pdatetimelocal = gmtime((const time_t*)&time_local);
	int year = pdatetimelocal->tm_year+1900;
	int mon = pdatetimelocal->tm_mon + 1;
	int day  = pdatetimelocal->tm_mday;
	int hour = pdatetimelocal->tm_hour;
	int min = pdatetimelocal->tm_min;
	int sec = pdatetimelocal->tm_sec;
	snprintf(cTimeStamp, 20, "%04d-%02d-%02d %02d:%02d:%02d", 
		year, mon, day, hour, min, sec);
	int res= strcmp(cTimeStamp, szTimeStamp);
	if(res == -1) return false;
	return true;
}