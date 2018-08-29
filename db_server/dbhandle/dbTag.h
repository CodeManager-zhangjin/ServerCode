#ifndef _DBTAG_H_
#define _DBTAG_H_

//////////////////////////////////////////////////////////////////////////
// table server
#define TABLE_SERVER		"server"
#define T_SERVER_ID			"svrid"
#define T_SERVER_VENDORID	"vendorid"
#define T_SERVER_SN			"svrsn"
#define T_SERVER_TYPE		"svrtype"
#define T_SERVER_NAME		"svrname"
#define T_SERVER_USERNAME	"svrusername"
#define T_SERVER_PASSWORD	"svrpassword"
#define T_SERVER_IP			"svrip"
#define T_SERVER_NETID		"svrnetid"
#define T_SERVER_POSITION	"svrposition"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table user
#define TABLE_USER			"user"
#define T_USER_ID			"userid"
#define T_USER_CONFIGUREINDEX "configureindex"
#define T_USER_NAME			"username"
#define T_USER_LANGUAGE		"language"
#define T_USER_PASSWORD		"userpassword"
#define T_USER_MOBILEPHONE	"mobilephone"
#define T_USER_EMAIL		"email"
#define T_USER_QUESTION		"question"
#define T_USER_ANSWER		"answer"
#define T_USER_REGISTERDATE	"registerdate"
#define T_USER_REGISTERIP	"registerip"
#define T_USER_ROLEID		"roleid"
#define T_USER_VENDORID		"vendorid"
#define T_USER_STOREID		"storeid"
#define T_USER_STORELIMIT	"storelimit"
#define T_USER_TYPE			"usertype"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table device
#define TABLE_DEVICE		"device" // 该表格的configureindex字段已经无效
#define T_DEVICE_ID			"deviceid"
#define T_DEVICE_VENDORID	"vendorid"
#define T_DEVICE_CONFIGUREINDEX "configureindex"
#define T_DEVICE_SN			"devicesn"
#define T_DEVICE_NAME		"devicename"
#define T_DEVICE_OWNERID	"ownerid"
#define T_DEVICE_AUTORELAY	"autorelay"  // 0-不转发 1-自动转发 2-转发
#define T_DEVICE_GROUPID	"groupid"
#define T_DEVICE_ROOMID	"roomid"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table userdevice
#define TABLE_USERDEVICE		"userdevice"
#define T_USERDEVICE_USERID		"userid"
#define T_USERDEVICE_DEVICEID	"deviceid"
#define T_USERDEVICE_ROOMID		"roomid"
#define T_USERDEVICE_DEVICENAME	"devicename"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table group
#define TABLE_GROUP			"groups"
#define T_GROUP_ID			"groupid"
#define T_GROUP_NAME		"groupname"
#define T_GROUP_PARENTID	"parentid"
#define T_GROUP_SEQUENCE	"sequence"
#define T_GROUP_USERID		"userid"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table pushinfo
#define TABLE_PUSHINFO			"pushinfo"
#define T_PUSHINFO_ID			"pushinfoid"
#define T_PUSHINFO_GT_TOKEN		"token"
#define T_PUSHINFO_USERID		"userid"
#define T_PUSHINFO_OS			"os"
#define T_PUSHINFO_LANGUAGE		"language"
#define T_PUSHINFO_CREATED		"created"
#define T_PUSHINFO_VENDORID		"vendorid"
#define T_PUSHINFO_BAIDU_TOKEN	"baidutoken"
#define T_PUSHINFO_HW_TOKEN		"hwtoken"
#define T_PUSHINFO_MI_TOKEN		"mitoken"
#define T_PUSHINFO_JG_TOKEN		"jgtoken"
#define T_PUSHINFO_MZ_TOKEN		"mztoken"
#define T_PUSHINFO_SWITCH		"switch"
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// table dserverindex
#define TABLE_DSERVERINDEX		"dserverindex"
#define T_DSERVERINDEX_VENDORID	"vendorid"
#define T_DSERVERINDEX_INDEX	"configureindex"
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// table pieceofserialno
#define TABLE_PIECEOFSERIALNO		"pieceofserialno"
#define T_PIECEOFSERIALNO_BEGIN		"begin"
#define T_PIECEOFSERIALNO_END		"end"
#define T_PIECEOFSERIALNO_VENDORID	"vendorid"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table vendor
#define TABLE_VENDOR				"vendor"
#define T_VENDOR_ID					"vendorid"
#define T_VENDOR_STOREID			"storeid"
#define T_VENDOR_TYPE				"vendortype"
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// table vendorurl
#define TABLE_VENDORURL				"vendorurl"
#define T_VENDORURL_VENDORID		"vendorid"
#define T_VENDORURL_URL				"url"
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// table deviceroom
#define TABLE_DEVICEROOM			"deviceroom"
#define T_DEVICEROOM_ROOMID			"roomid"
#define T_DEVICEROOM_DEVICEID		"deviceid"
#define T_DEVICEROOM_ROOM			"room"
#define T_DEVICEROOM_PUSHINDEX		"pushindex"
#define T_DEVICEROOM_USERINDEX		"userindex"
#define T_DEVICEROOM_CARDINDEX		"cardindex"
#define T_DEVICEROOM_PASSWORD		"password"
#define T_DEVICEROOM_INDOORINDEX	"indoorindex"
#define T_DEVICEROOM_PUSHSWITCHINDEX		"pushswitchindex"
#define T_DEVICEROOM_CATEGORY		"category"
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// table roomcard
#define TABLE_ROOMCARD			"roomcard"
#define T_ROOMCARD_ROOMID		"roomid"
#define T_ROOMCARD_CARDID		"cardid"
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// table card
#define TABLE_CARD			"card"
#define T_CARD_ID			"cardid"
#define T_CARD_NUMBER		"cardnumber"
#define T_CARD_TYPE			"cardtype"
#define T_CARD_TIMELIMIT	"timelimit"
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// table dserver 其他客户独立平台的注册服务器
#define TABLE_DSERVER			"dserver"  // 用于暗中配置和控制其他独立平台
#define T_DSERVER_CUSTOMER		"customer"
#define T_DSERVER_SVRSN			"svrsn"
#define T_DSERVER_PERMISSION	"permission"
#define T_DSERVER_CAPACITY		"capacity"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table storage
#define TABLE_STORAGE			"storage"
#define T_STORAGE_STOREID		"storeid"
#define T_STORAGE_USERNAME		"username"
#define T_STORAGE_ACCESSKEY		"accesskey"
#define T_STORAGE_SECRETKEY		"secretkey"
#define T_STORAGE_BUCKET		"bucket"
#define T_STORAGE_DOMAIN		"domain"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table storekey or storekeybak
#define TABLE_STOREKEY			"storekey"
#define TABLE_STOREKEYBAK		"storekeybak"
#define T_STOREKEY_DEVICEID		"deviceid"
#define T_STOREKEY_ROOMID		"roomid"
#define T_STOREKEY_TYPE			"type"
#define T_STOREKEY_REASON		"reason"
#define T_STOREKEY_TIMESTAMP	"timestamp"
#define T_STOREKEY_SIZE			"size"
#define T_STOREKEY_STOREID		"storeid"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table unlocklog
#define TABLE_UNLOCKLOG			"unlocklog"
#define T_UNLOCKLOG_DEVICEID	"deviceid"
#define T_UNLOCKLOG_ROOMID		"roomid"
#define T_UNLOCKLOG_USERID		"userid"
#define T_UNLOCKLOG_CARDID		"cardid"
#define T_UNLOCKLOG_CARDNUM		"comnumber"
#define T_UNLOCKLOG_TYPE		"reason"
#define T_UNLOCKLOG_TIMESTAMP	"timestamp"
//////////////////////////////////////////////////////////////////////////
// table alarmrecord
#define TABLE_ALARM				"alarmrecord"
#define T_ALARM_DEVICEID		"deviceid"
#define T_ALARM_ROOMID		"roomid"
#define T_ALARM_TYPE			"type"
#define T_ALARM_SUBTYPE		"subtype"
#define T_ALARM_TIMESTAMP	"timestamp"
//////////////////////////////////////////////////////////////////////////
// table ucpaas
#define TABLE_UCPAAS			"ucpaas"
#define T_UCPAAS_USERNAME		"username"
#define T_UCPAAS_PASSWORD		"password"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table vendor
#define TABLE_VENDOR			"vendor"
#define T_VENDOR_PHONENUMBER	"vendorphonenumber"
#define T_VENDOR_VENDORID		"vendorid"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// table ucpaasapp
#define TABLE_UCPAASAPP			"ucpaasapp"
#define T_UCPAASAPP_USERID		"userid"
#define T_UCPAASAPP_APPID		"appid"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// 查询数据库截至日期
#define T_DEVICEID          "deviceid"
#define T_DEADLINE          "calldateline"
#define T_DEVICE            "callquery"
#define T_CALLDATE			"calldate"
//////////////////////////////////////////////////////////////////////////
// 查詢物業公告信息
#define T_DEVICE_GROUPID 	"groupid"
#define T_VILLAGEID			"villageid"
#define T_NOTICEINDEX       "noticeindex"
#define T_ADVERTINDEX		"adindex"
#define T_VISITORCFG			"visitorcfg"
#define TABLE_PUBLICINDEX   "publicindex"
//////////////////////////////////////////////////////////////////////////
//可以打电话设备的屏幕尺寸
#define DEV_SCREEN_SIZE7	14
#define DEV_SCREEN_SIZE0	15
#define DEV_SCREEN_SIZE13	17
///////////////////////////////////////////////////////////////////////////
//室内机 
#define TABLE_INDOOR		"indoor"
#define T_INDOOR_SERIALNO	"serialno"
#define T_INDOOR_ID			"indoorid"
#define T_INDOOR_DEVICEID	"deviceid"
#define T_INDOOR_ROOMID		"roomid"
//////////////////////////////////////////////////////////////////////////
#define TABLE_DEVICEROOM	"deviceroom"
#define T_DEVICEROOM_ROOMID "roomid"
#define T_DEVICEROOM_ROOM	"room"
#define T_DEVICEROOM_INDOORINDEX	"indoorindex"
#define T_DEVICEROOM_CATEGORY	"category"
//////////////////////////////////////////////////////////////////////////

//operation_db
//////////////////////////////////////////////////////////////////////////
#define		TABLE_ADVERT				"advert"
#define		T_ADVERT_ADID			"adid"
#define		T_ADVERT_APPLYTYPE	"applytype"
#define		T_ADVERT_APPLYID		"applyid"
#define		T_ADVERT_ADTYPE		"adtype"
#define		T_ADVERT_SIZE				"size"
#define		T_ADVERT_STOREID		"storeid"
#define		T_ADVERT_STORETYPE	"storetype"
#define		T_ADVERT_FORMAT		"format"
#define		T_ADVERT_USETYPE		"usetype"
#define		T_ADVERT_TIMESTAMP			"timestamp"
#define		T_ADVERT_USEPOSITION		"useposition"
//////////////////////////////////////////////////////////////////////////
#define		TABLE_ADRELATION		"adrelation"
#define		T_ADRELATION_VILLAGEID		"villageid"
#define		T_ADRELATION_USETYPE			"usetype"
#define		T_ADRELATION_USEPOSITION	"useposition"
#define		T_ADRELATION_IMGID				"imgid"
#define		T_ADRELATION_VOICEID			"voiceid"
#define		T_ADRELATION_TEXTID				"textid"
#define		T_ADRELATION_VIDEOID			"videoid"
//////////////////////////////////////////////////////////////////////////
#define TABLE_STORAGE2					"storage"
#define T_STORAGE2_STOREID			"storeid"
#define T_STORAGE2_USERNAME		"username"
#define T_STORAGE2_ACCESSKEY		"accesskey"
#define T_STORAGE2_SECRETKEY		"secretkey"
#define T_STORAGE2_BUCKET			"bucket"
#define T_STORAGE2_DOMAIN			"domain"
//////////////////////////////////////////////////////////////////////////
#define		TABLE_VILLAGEAD		"villagead"
//////////////////////////////////////////////////////////////////////////
#define  TABLE_ADVERTMGR		"advertmgr"
#define  T_ADVERTMGR_ID			"advertid"
//////////////////////////////////////////////////////////////////////////
#endif	// _DBTAG_H_
