#pragma once

#include "DataStruct.h"
#include <map>

#define MAP_COUNT_VIEW 100

// 管理类模板
template <class Elem>
class CElemMapView
{
public:
	CElemMapView();
	virtual ~CElemMapView();

	// 管理连接客户端
	virtual int		AddElem(DWORD dwID, Elem* pElem);
	virtual int		DelElem(DWORD dwID);
	virtual Elem*	GetElem(DWORD dwID);
	virtual void	ClearElem();

protected:
	typedef std::map<DWORD, Elem*>	ELEM_MAP;

public:
	// 管理认证成功客户端
	virtual int		Template_AddViewSession(DWORD dwMainID, DWORD dwSubID, Elem* pElem);
	virtual int		Template_DelViewSession(DWORD dwMainID, DWORD dwSubID, Elem* pElem);
	virtual int		Template_GetViewSession(DWORD dwMainID, ELEM_MAP& mapElem);
	virtual Elem*	Template_GetViewSession(DWORD dwMainID, DWORD dwSubID);
protected:
	ELEM_MAP m_mapElement[MAP_COUNT_VIEW];

	typedef std::map<DWORD, ELEM_MAP> MAP_VIEW_SESSION; // UserID, MAP_VIEW
	MAP_VIEW_SESSION m_mapViewSession[MAP_COUNT_VIEW];
};

template <class Elem>
CElemMapView<Elem>::CElemMapView()
{
}

template <class Elem>
CElemMapView<Elem>::~CElemMapView()
{
	ClearElem();
}

template <class Elem>
void CElemMapView<Elem>::ClearElem()
{
	for (int i = 0; i < MAP_COUNT_VIEW; i++)
	{
		typename ELEM_MAP::iterator iter = m_mapElement[i].begin();
		for (; iter != m_mapElement[i].end(); iter++) {
			delete iter->second;
		}
		m_mapElement[i].clear();
	}
}

template <class Elem>
int CElemMapView<Elem>::AddElem(DWORD dwID, Elem* pElem)
{
	if (0 == dwID) return -1;
	if (NULL == pElem) return -1;
	printf("this %p AddElem %ld %p\n", this, dwID, pElem);
	int nIndex = dwID % MAP_COUNT_VIEW;
	m_mapElement[nIndex].insert( std::make_pair(dwID, pElem) );
	return 0;
}

template <class Elem>
int CElemMapView<Elem>::DelElem(DWORD dwID)
{
	if (0 == dwID) return -1;
	int nIndex = dwID % MAP_COUNT_VIEW;
	typename ELEM_MAP::iterator iter = m_mapElement[nIndex].find(dwID);
	if (iter != m_mapElement[nIndex].end()) {
		printf("this %p DelElem %ld %p\n", this, iter->first, iter->second);
		delete iter->second; m_mapElement[nIndex].erase(iter);
	}
	return 0;
}

template <class Elem>
Elem* CElemMapView<Elem>::GetElem(DWORD dwID)
{
	if (0 == dwID) return NULL;
	int nIndex = dwID % MAP_COUNT_VIEW;
	typename ELEM_MAP::iterator iter = m_mapElement[nIndex].find(dwID);
	if (iter != m_mapElement[nIndex].end()) {
		return iter->second;
	}
	printf("Can't Find (%ld)\n", dwID);
	return NULL;
}

template <class Elem>
int CElemMapView<Elem>::Template_AddViewSession(DWORD dwMainID, DWORD dwSubID, Elem* pElem)
{
	if (0 == dwMainID) return -1;
	if (0 == dwSubID) return -1;
	if (NULL == pElem) return -1;
	printf("this %p AddElem %ld %ld %p\n", this, dwMainID, dwSubID, pElem);
	int nIndex = dwMainID % MAP_COUNT_VIEW;
	typename MAP_VIEW_SESSION::iterator iter = m_mapViewSession[nIndex].find(dwMainID);
	if (iter != m_mapViewSession[nIndex].end())
	{
		typename ELEM_MAP::iterator pos = iter->second.find(dwSubID);
		if (pos != iter->second.end()) pos->second = pElem;
		else iter->second.insert(std::make_pair(dwSubID, pElem));
	}
	else
	{
		ELEM_MAP tMap; tMap.insert(std::make_pair(dwSubID, pElem));
		m_mapViewSession[nIndex].insert(std::make_pair(dwMainID, tMap));
	}
	return 0;
}

template <class Elem>
int CElemMapView<Elem>::Template_DelViewSession(DWORD dwMainID, DWORD dwSubID, Elem* pElem)
{
	if (0 == dwMainID) return -1;
	if (0 == dwSubID) return -1;
	int nIndex = dwMainID % MAP_COUNT_VIEW;
	typename MAP_VIEW_SESSION::iterator iter = m_mapViewSession[nIndex].find(dwMainID);
	if (iter != m_mapViewSession[nIndex].end())
	{
		printf("this %p DelElem %ld %ld %p\n", this, dwMainID, dwSubID, pElem);
		iter->second.erase(dwSubID);
		if (iter->second.empty()) m_mapViewSession[nIndex].erase(iter);
	}
	return 0;
}

template <class Elem>
Elem* CElemMapView<Elem>::Template_GetViewSession(DWORD dwMainID, DWORD dwSubID)
{
	if (0 == dwMainID) return NULL;
	if (0 == dwSubID) return NULL;
	int nIndex = dwMainID % MAP_COUNT_VIEW;
	typename MAP_VIEW_SESSION::iterator iter = m_mapViewSession[nIndex].find(dwMainID);
	if (iter != m_mapViewSession[nIndex].end())
	{
		typename ELEM_MAP::iterator pos = iter->second.find(dwSubID);
		if (pos != iter->second.end()) return pos->second;
	}

	printf("Can't Find (%ld %ld)\n", dwMainID, dwSubID);
	return NULL;
}

template <class Elem>
int CElemMapView<Elem>::Template_GetViewSession(DWORD dwMainID, ELEM_MAP& mapElem)
{
	if (0 == dwMainID) return -1;
	int nIndex = dwMainID % MAP_COUNT_VIEW;
	typename MAP_VIEW_SESSION::iterator iter = m_mapViewSession[nIndex].find(dwMainID);
	if (iter != m_mapViewSession[nIndex].end()) mapElem = iter->second;

	printf("Can't Find (%ld)\n", dwMainID);
	return -1;
}
