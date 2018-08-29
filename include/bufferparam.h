#if !defined( BUFFER_PARAM_H_INCLUDE )
#define BUFFER_PARAM_H_INCLUDE

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <utility>
#include <string>
#include "DataStruct.h"
#include "Log.h"

class CParserException
{
public:
    CParserException(){}
    ~CParserException(){}

    void Descript() const
    {
        LOG_EMERG(LOG_UNKNOW, "Exception occured during parsering network data\n");
    }
};

template <class T>
class CPtrArrayBuffer
{
public:
    CPtrArrayBuffer( T* pBuffer, UINT nSize ) : m_pBuffer( pBuffer ), m_nSize( nSize ) {}
    CPtrArrayBuffer( const T* pBuffer, UINT nSize ) : m_pBuffer( (T*)pBuffer ), m_nSize( nSize ) {}

    ~CPtrArrayBuffer(){};

    UINT Size() const { return m_nSize; }
    T* GetBuffer() const { return m_pBuffer; }
private:
    T*   m_pBuffer;
    UINT m_nSize;
};

typedef CPtrArrayBuffer<BYTE> CByteArrayBuffer;
typedef CPtrArrayBuffer<char> CStringBuffer;

class CVarString
{
public:
    CVarString( std::string& str, size_t size ) : m_pString( &str ), m_size( size ) {}
    ~CVarString() {}

    std::string* string() const { return m_pString; }
    size_t Size() const { return m_size; }
private:
    std::string* m_pString;
    size_t       m_size;
};

class CFixedString : public CVarString
{
public:
    CFixedString( std::string& str, size_t size ) : CVarString( str, size ) {}
    ~CFixedString() {}
};

#endif //if !defined( BUFFER_PARAM_H_INCLUDE )
