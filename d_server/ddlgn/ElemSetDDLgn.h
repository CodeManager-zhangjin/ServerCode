#pragma once

#include "DataStruct.h"
#include <set>

template <class Elem>
class CElemSetDDLgn  
{
public:
	CElemSetDDLgn();
	virtual ~CElemSetDDLgn();

	int AddElem(Elem* pElem);
	int DelElem(Elem* pElem);
protected:
	typedef std::set<Elem*>	ELEM_SET;
	ELEM_SET	m_setElement;
};

template <class Elem>
CElemSetDDLgn<Elem>::CElemSetDDLgn()
{
}

template <class Elem>
CElemSetDDLgn<Elem>::~CElemSetDDLgn()
{
	typename ELEM_SET::iterator iter = m_setElement.begin();
	for (; iter != m_setElement.end(); iter++) {
		delete *iter;
	}
	m_setElement.clear();
}

template <class Elem>
int CElemSetDDLgn<Elem>::AddElem(Elem* pElem)
{
	if (NULL == pElem) return -1;
	m_setElement.insert(pElem);
	return 0;
}

template <class Elem>
int CElemSetDDLgn<Elem>::DelElem(Elem* pElem)
{
	if (NULL == pElem) return -1;
	typename ELEM_SET::iterator iter = m_setElement.find(pElem);
	if (iter != m_setElement.end()) {
		m_setElement.erase(iter);
		delete pElem;
	}
	return 0;
}
