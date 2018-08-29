#ifndef _PROTOCOL_ALARM_H_
#define _PROTOCOL_ALARM_H_

#include "Protocol.h"

const BYTE GROUPCODE_CA				= 0xCA;
const BYTE GROUPCODE_AC				= 0xAC;

const BYTE CA_SOURCE_TYPE_NONE		= 0x00;	// 默认, 未明确发送端
const BYTE CA_SOURCE_TYPE_DEVICE	= 0x01;	// 前端设备（如报警设备、报警联动设备、IPC等）
const BYTE CA_SOURCE_TYPE_SETUP		= 0x02;	// Setup软件
const BYTE CA_SOURCE_TYPE_ALCSVR	= 0x03;	// 报警服务器软件
const BYTE CA_SOURCE_TYPE_ANDRIOD	= 0x04;	// 安卓手机客户端
const BYTE CA_SOURCE_TYPE_IOS		= 0x05;	// 苹果手机客户端

// 客户端登入连接验证
const WORD CA_Command_Authenticate_Request = 0x0001;
const WORD CA_Command_Challenge_Request = 0x0002;
const WORD CA_Command_Challenge_Result = 0x0003;
const WORD CA_Command_Authenticate_Result = 0x0004;

// 摄像机注册连接验证
const WORD CA_Command_Register_Request = 0x00F1;
const WORD CA_Command_Register_Result = 0x00F2;

// 摄像机搜索
const WORD CA_Command_Scan_Request = 0x0005;
const WORD CA_Command_Scan_Result = 0x0006;
const WORD CA_Command_ScanEx_Result = 0x000F;

// Image升级
const WORD CA_Command_UpdateFirmware_Request = 0x0007;
const WORD CA_Command_UpdateFirmware_Result = 0x0008;
const WORD CA_Command_SendFirmware_Request = 0x0009;
const WORD CA_Command_SendFirmware_Result = 0x000A;
const WORD CA_Command_WriteFirmware_Request = 0x000B;
const WORD CA_Command_WriteFirmware_Result = 0x000C;

// 音视频播放
const WORD CA_Command_Play_Request = 0x0011;
const WORD CA_Command_Play_Result = 0x0021;
const WORD CA_Command_Stop_Request = 0x0012;
const WORD CA_Command_Stop_Result = 0x0022;
const WORD CA_Command_Pause_Request = 0x0013;
const WORD CA_Command_Pause_Result = 0x0023;
const WORD CA_Command_Resume_Request = 0x0014;
const WORD CA_Command_Resume_Result = 0x0024;
const WORD CA_Command_Frame_Request = 0x0015;
const WORD CA_Command_Frame_Result = 0x0025;
const WORD CA_Command_PlayFinish_Result = 0x0026;
const WORD CA_Command_GetRecordTime_Request = 0x0017;
const WORD CA_Command_GetRecordTime_Result = 0x0027;
const WORD CA_Command_GetRecordHour_Request = 0x0018;
const WORD CA_Command_GetRecordHour_Result = 0x0028;
const WORD CA_Command_GetRecordMinu_Request = 0x0019;
const WORD CA_Command_GetRecordMinu_Result = 0x0029;
const WORD CA_Command_GetPlayPriority_Request = 0x001A;
const WORD CA_Command_GetPlayPriority_Result = 0x002A;
const WORD CA_Command_OpenDo_Request = 0x007A;
const WORD CA_Command_OpenDo_Result = 0x008A;

// 摄像机报警
const WORD CA_Command_StartAlarm_Request = 0x0101;
const WORD CA_Command_StartAlarm_Result = 0x0111;
const WORD CA_Command_StopAlarm_Request = 0x0102;
const WORD CA_Command_StopAlarm_Result = 0x0112;


// CA SessionType
const DWORD CA_SESSIONTYPE_NONE				= 0x00000000;	// 空值,无意义
const DWORD CA_SESSIONTYPE_COMMAND			= 0x00000001;	// 命令连接
const DWORD CA_SESSIONTYPE_VIDEO			= 0x00000002;	// 视频连接
const DWORD CA_SESSIONTYPE_AUDIO			= 0x00000004;	// 音频连接
const DWORD CA_SESSIONTYPE_VIDEOUSER		= 0x00000008;	// 视频连接,客户端回传
//const DWORD CA_SESSIONTYPE_TALK     		= 0x00000010;	// 音频连接,客户端回传
const DWORD CA_SESSIONTYPE_AUDIOUSER		= 0x00000010; // 音频连接,客户端回传

// CA ChannelType
const DWORD CA_CHANNELTYPE_DEFAULT			= 0x00000000;	// 默认channel
															// 非真实channel
															// 默认 实时默认channel
const DWORD CA_CHANNELTYPE_RTDEFAULT		= 0x00000001;	// 实时默认channel
															// 非真实实时channel
															// 由摄像机决定或用户设置
const DWORD CA_CHANNELTYPE_PBDEFAULT		= 0x00000002;	// 回放默认channel
															// 非真实回放channel
															// 由摄像机决定或用户设置
const DWORD CA_CHANNELTYPE_RTALARMCT		= 0x00000003;	// 预定义实时报警中心channel
															// 非真实实时channel
															// 由摄像机决定或用户设定
const DWORD CA_CHANNELTYPE_RTALARMCT2		= 0x00000004;	// 预定义Setup channel
															// 非真实实时channel
															// 由摄像机决定或用户设定
const DWORD CA_CHANNELTYPE_RTCHANNEL_PC		= 0x00000011;	// 第一路实时channel
															// 真实实时channel
const DWORD CA_CHANNELTYPE_RTCHANNEL_MOB	= 0x00000012;	// 第二路实时channel
															// 真实实时channel
const DWORD CA_CHANNELTYPE_RTCHANNEL3		= 0x00000013;	// 第三路实时channel
															// 真实实时channel
const DWORD CA_CHANNELTYPE_PBSDCARD			= 0x00000101;	// SD卡回放channel
															// 真实回放channel
const DWORD CA_CHANNELTYPE_PBNAS			= 0x00000102;	// NAS回放channel
															// 真实回放channel
const DWORD CA_CHANNELTYPE_PBALARMCT		= 0x00000103;	// 报警中心回放channel
															// 真实回放channel


// CA PlayTransFlag
const BYTE CA_PLAY_TRANSFLAG_NORMAL			= 0x00;	// 普通传输方式
const BYTE CA_PLAY_TRANSFLAG_DOWNLOAD		= 0x01;	// 下载传输方式

//观看视频时返回的错误码
const WORD CA_ERROR_PLAY_NOPRIORITY				= 0x0101;	// 没有播放权限
const WORD CA_ERROR_PLAY_HANGUP					= 0x0103;	// 对方已挂断
const WORD CA_ERROR_PLAY_DEVBUSY				= 0x0104;	// 设备占线，无法播放
const WORD CA_ERROR_PLAY_AUDIO_BUSY				= 0x0106;   // 音频播放已被其他用户占用
const WORD CA_ERROR_PLAY_AUDIO_OK	        	= 0x0107;	// 音频当前正常播放

// 长度常量
const int CA_LENGTH_SERIALNUMBER		= 32;
const int CA_LENGTH_USERNAME			= 32;
const int CA_LENGTH_PASSWORD			= 32;
const int CA_LENGTH_MD516				= 16;

const BYTE CA_MEDIA_TYPE_VIDEO = 0x91;
const BYTE CA_MEDIA_TYPE_AUDIO = 0x21;
const BYTE CA_MEDIA_TYPE_AUDIOUSER = 0x22;

#endif //_PROTOCOL_ALARM_H_
