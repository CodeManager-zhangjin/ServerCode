#pragma once

#include "DataStruct.h"
#include <map>

#define MAP_COUNT_DEVICE 100

// 管理类模板
template <class Elem>
class CElemMapDevice  
{
public:
	CElemMapDevice();
	virtual ~CElemMapDevice();

	// 管理连接设备
	virtual int		AddElem(DWORD dwID, Elem* pElem);
	virtual int		DelElem(DWORD dwID             );
	virtual Elem*	GetElem(DWORD dwID			   );
	virtual void	ClearElem();

	// 管理注册成功设备
	virtual int		Template_AddDevice(DWORD dwID, Elem* pElem);
	virtual int		Template_ReplaceDevice(DWORD dwID, Elem* pElem);
	virtual int		Template_DelDevice(DWORD dwID);
	virtual Elem*	Template_GetDevice(DWORD dwID);
	virtual int		Template_GetSize();
	virtual Elem*	Template_GetBegin();
protected:
	typedef std::map<DWORD, Elem*>	ELEM_MAP;
	ELEM_MAP m_mapElement[MAP_COUNT_DEVICE];
	ELEM_MAP m_mapDevice[MAP_COUNT_DEVICE];
};

template <class Elem>
CElemMapDevice<Elem>::CElemMapDevice()
{
}

template <class Elem>
CElemMapDevice<Elem>::~CElemMapDevice()
{
	ClearElem();
}

template <class Elem>
void CElemMapDevice<Elem>::ClearElem()
{
	for (int i = 0; i < MAP_COUNT_DEVICE; i++)
	{
		typename ELEM_MAP::iterator iter = m_mapElement[i].begin();
		for (; iter != m_mapElement[i].end(); iter++) {
			delete iter->second;
		}
		m_mapElement[i].clear();
	}
}

template <class Elem>
int CElemMapDevice<Elem>::AddElem(DWORD dwID, Elem* pElem)
{
	if (0 == dwID) return -1;
	if (NULL == pElem) return -1;
	printf("this %p AddElem %ld %p\n", this, dwID, pElem);
	int nIndex = dwID % MAP_COUNT_DEVICE;
	m_mapElement[nIndex].insert( std::make_pair(dwID, pElem) );
	return 0;
}

template <class Elem>
int CElemMapDevice<Elem>::DelElem(DWORD dwID)
{
	if (0 == dwID) return -1;
	int nIndex = dwID % MAP_COUNT_DEVICE;
	typename ELEM_MAP::iterator iter = m_mapElement[nIndex].find(dwID);
	if (iter != m_mapElement[nIndex].end()) {
		printf("this %p DelElem %ld %p dwClientID %d\n", this, iter->first, iter->second, dwID);
		delete iter->second; m_mapElement[nIndex].erase(iter);
	}
	return 0;
}

template <class Elem>
Elem* CElemMapDevice<Elem>::GetElem(DWORD dwID)
{
	if (0 == dwID) return NULL;
	int nIndex = dwID % MAP_COUNT_DEVICE;
	typename ELEM_MAP::iterator iter = m_mapElement[nIndex].find(dwID);
	if (iter != m_mapElement[nIndex].end()) {
		return iter->second;
	}
	printf("Can't Find (%ld)\n", dwID);
	return NULL;
}

template <class Elem>
int CElemMapDevice<Elem>::Template_AddDevice(DWORD dwID, Elem* pElem)
{
	if (0 == dwID) return -1;
	if (NULL == pElem) return -1;
	int nIndex = dwID % MAP_COUNT_DEVICE;
	m_mapDevice[nIndex].insert( std::make_pair(dwID, pElem) );
	return 0;
}

template <class Elem>
int CElemMapDevice<Elem>::Template_ReplaceDevice(DWORD dwID, Elem* pElem)
{
	if (0 == dwID) return -1;
	if (NULL == pElem) return -1;
	int nIndex = dwID % MAP_COUNT_DEVICE;
	typename ELEM_MAP::iterator iter = m_mapDevice[nIndex].find(dwID);
	if (iter != m_mapDevice[nIndex].end()) {
		iter->second = pElem;
	}
	return 0;
}

template <class Elem>
int CElemMapDevice<Elem>::Template_DelDevice(DWORD dwID)
{
	if (0 == dwID) return -1;
	int nIndex = dwID % MAP_COUNT_DEVICE;
	typename ELEM_MAP::iterator iter = m_mapDevice[nIndex].find(dwID);
	if (iter != m_mapDevice[nIndex].end()) {
		m_mapDevice[nIndex].erase(iter);
	}
	return 0;
}

template <class Elem>
Elem* CElemMapDevice<Elem>::Template_GetDevice(DWORD dwID)
{
	if (0 == dwID) return NULL;
	int nIndex = dwID % MAP_COUNT_DEVICE;
	typename ELEM_MAP::iterator iter = m_mapDevice[nIndex].find(dwID);
	if (iter != m_mapDevice[nIndex].end()) {
		return iter->second;
	}
	return NULL;
}

template <class Elem>
int CElemMapDevice<Elem>::Template_GetSize()
{
	int nCount = 0;
	for (int i = 0; i < MAP_COUNT_DEVICE; i++)
	{
		nCount += m_mapDevice[i].size();
	}
	return nCount;
}

template <class Elem>
Elem* CElemMapDevice<Elem>::Template_GetBegin()
{
	for (int i = 0; i < MAP_COUNT_DEVICE; i++)
	{
		if (m_mapDevice[i].empty() == false) return m_mapDevice[i].begin()->second;
	}
	return NULL;
}
