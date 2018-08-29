#include "DeviceInterface.h"
#include "Device.h"

bool DeviceInit()
{
	return CDeviceMgr::Instance()->Start();
}

void DeviceFinish()
{
	LOG_DEBUG(LOG_MAIN, "%s 1\n", __FUNCTION__);
	CDeviceMgr::DeleteInstance();
	LOG_DEBUG(LOG_MAIN, "%s 2\n", __FUNCTION__);
}

IDeviceHandle* Device_GetHandle()
{
	CDeviceMgr::Instance();
}

void Device_SetSink( IDeviceHandleSink* pSink )
{
	CDeviceMgr::Instance()->SetSink(pSink);
}

void TEST_YSC_ONOFF_LINE(char cOnOff)
{
	PUCHAR pSN = (PUCHAR)"OJOQL2WB4JV5Q56K090E";
	CDeviceMgr::Instance()->AddNotify_OnOff(pSN, cOnOff);
}