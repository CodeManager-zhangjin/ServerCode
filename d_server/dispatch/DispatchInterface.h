#ifndef _DISPATCH_INTERFACE_H_
#define _DISPATCH_INTERFACE_H_

#include "DataStruct.h"

// 该模块本身不接收任何消息
// 该模块负责Device、View、ServerApp模块之间协调工作
// 当Device、View、ServerApp每个模块依靠自身不能完成某项工作时，
// 由Dispatch模块综合三个模块一起完成该项工作，并将结果通知工作发起模块

bool DispatchInit();
void DispatchFinish();

#endif
