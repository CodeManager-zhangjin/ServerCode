#pragma once

#include "DataStruct.h"
#include <map>

// 管理类模板
template <class Elem>
class CElemMap_ServerApp  
{
public:
	CElemMap_ServerApp();
	virtual ~CElemMap_ServerApp();

	virtual int		AddElem(DWORD dwID, Elem* pElem);
	virtual int		DelElem(            Elem* pElem);
	virtual int		DelElem(DWORD dwID             );
	virtual Elem*	GetElem(DWORD dwID			   );
	virtual void	ClearElem();
protected:
	typedef std::map<DWORD, Elem*>	ELEM_MAP;
	ELEM_MAP m_mapElement;
};

template <class Elem>
CElemMap_ServerApp<Elem>::CElemMap_ServerApp()
{
}

template <class Elem>
CElemMap_ServerApp<Elem>::~CElemMap_ServerApp()
{
	ClearElem();
}

template <class Elem>
void CElemMap_ServerApp<Elem>::ClearElem()
{
	typename ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++) {
		delete iter->second;
	}
	m_mapElement.clear();
}

template <class Elem>
int CElemMap_ServerApp<Elem>::AddElem(DWORD dwID, Elem* pElem)
{
	if (0 == dwID) return -1;
	if (NULL == pElem) return -1;
	printf("this %p AddElem %ld %p\n", this, dwID, pElem);
	m_mapElement.insert( std::make_pair(dwID, pElem) );
	return 0;
}

template <class Elem>
int CElemMap_ServerApp<Elem>::DelElem(Elem* pElem)
{
	if (NULL == pElem) return -1;
	typename ELEM_MAP::iterator iter = m_mapElement.begin();
	for (; iter != m_mapElement.end(); iter++) {
		if (pElem == iter->second) {
			printf("this %p DelElem %ld %p\n", this, iter->first, iter->second);
			delete pElem; m_mapElement.erase(iter);
			return 0;
		}
	}
	return 0;
}

template <class Elem>
int CElemMap_ServerApp<Elem>::DelElem(DWORD dwID)
{
	if (0 == dwID) return -1;
	typename ELEM_MAP::iterator iter = m_mapElement.find(dwID);
	if (iter != m_mapElement.end()) {
		printf("this %p DelElem %ld %p\n", this, iter->first, iter->second);
		delete iter->second; m_mapElement.erase(iter);
	}
	return 0;
}

template <class Elem>
Elem* CElemMap_ServerApp<Elem>::GetElem(DWORD dwID)
{
	if (0 == dwID) return NULL;
	typename ELEM_MAP::iterator iter = m_mapElement.find(dwID);
	if (iter != m_mapElement.end()) {
		return iter->second;
	}
	printf("Can't Find (%ld)\n", dwID);
	return NULL;
}
