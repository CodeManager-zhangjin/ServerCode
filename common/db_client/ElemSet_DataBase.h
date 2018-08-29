// ElemSet_Sms.h: interface for the CElemSetDataBase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ELEMSET_DB_H__CD86C670_C4B0_4A51_B49F_4F6470219DB5__INCLUDED_)
#define AFX_ELEMSET_DB_H__CD86C670_C4B0_4A51_B49F_4F6470219DB5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DataStruct.h"
#include <set>

template <class Elem>
class CElemSetDataBase  
{
public:
	CElemSetDataBase();
	virtual ~CElemSetDataBase();

 	int AddElem(Elem* pElem);
	int DelElem(Elem* pElem);
protected:
	typedef std::set<Elem*>	ELEM_SET;
	ELEM_SET	m_setElement;
};

template <class Elem>
CElemSetDataBase<Elem>::CElemSetDataBase()
{
}

template <class Elem>
CElemSetDataBase<Elem>::~CElemSetDataBase()
{
	typename ELEM_SET::iterator iter = m_setElement.begin();
	for (; iter != m_setElement.end(); iter++) {
		delete *iter;
	}
	m_setElement.clear();
}

template <class Elem>
int CElemSetDataBase<Elem>::AddElem(Elem* pElem)
{
	if (NULL == pElem) return -1;
	m_setElement.insert(pElem);
	return 0;
}

template <class Elem>
int CElemSetDataBase<Elem>::DelElem(Elem* pElem)
{
	if (NULL == pElem) return -1;
	typename ELEM_SET::iterator iter = m_setElement.find(pElem);
	if (iter != m_setElement.end()) {
		m_setElement.erase(iter);
		delete pElem;
	}
	return 0;
}

#endif // !defined(AFX_ELEMSET_DB_H__CD86C670_C4B0_4A51_B49F_4F6470219DB5__INCLUDED_)
