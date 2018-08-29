#include "WebcmdInterface.h"
#include "Webcmd.h"

bool WebcmdInit()
{
	CCGICenter::Instance()->Start();
	return true;
}

void WebcmdFinish()
{
	CCGICenter::DeleteInstance();
}
