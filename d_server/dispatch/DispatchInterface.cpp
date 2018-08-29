#include "DispatchInterface.h"
#include "Dispatch.h"
#include "Log.h"

bool DispatchInit()
{
	return CDispatch::Instance()->Start();
}

void DispatchFinish()
{
	LOG_DEBUG(LOG_MAIN, "%s 1\n", __FUNCTION__);
	CDispatch::DeleteInstance();
	LOG_DEBUG(LOG_MAIN, "%s 2\n", __FUNCTION__);
}
