#include "ViewInterface.h"
#include "View.h"
#include "Log.h"

bool ViewInit()
{
	return CViewMgr::Instance()->Start();
}

void ViewFinish()
{
	CViewMgr::DeleteInstance();
	LOG_DEBUG(LOG_MAIN, "%s\n", __FUNCTION__);
}

IViewHandle* View_GetHandle()
{
	CViewMgr::Instance();
}

void View_SetSink( IViewHandleSink* pSink )
{
	CViewMgr::Instance()->SetSink(pSink);
}
