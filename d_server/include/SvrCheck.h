#ifndef SVR_CHECK_H
#define SVR_CHECK_H

bool CheckExchange(char* pInVal);
int  GetCpuVal();

//NetCard Control Begin
void  SetCardIP(unsigned long dwIP = 0);
unsigned long GetCardWidth();
unsigned long GetCardSpeedUsedMbWidth();
//NetCard Control End

unsigned long GetMemInfo(int& nFreeMem, int& nTotalMem);

#endif
