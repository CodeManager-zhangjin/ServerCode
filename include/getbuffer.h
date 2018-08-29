#if !defined(GET_BUFFER_H_0001)
#define GET_BUFFER_H_0001

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <netinet/in.h> //for ntohl and ntohs
#include "bufferparam.h"

class CGetBuffer
{
public:
    
    inline CGetBuffer( BYTE* pbBuffer, UINT nBufferLen );
    inline CGetBuffer( char* pbBuffer, UINT nBufferLen );
    virtual ~CGetBuffer(){}
    // extraction operations
    CGetBuffer& operator>>(char& ch)    { Get( &ch, 1 ); return *this; }
    CGetBuffer& operator>>(BYTE& by)    { Get( &by, 1 ); return *this; }
    CGetBuffer& operator>>(short& w)    { Get( &w, 1 ); w = ntohs( w ); return *this; }
    CGetBuffer& operator>>(WORD& us)    { Get( &us, 1 ); us = ntohs( us ); return *this; }
    CGetBuffer& operator>>(int&    n)   { Get( &n, 1 ); n = ntohl( n ); return *this; }
    CGetBuffer& operator>>(DWORD& dw)   { Get( &dw, 1 ); dw = ntohl( dw ); return *this; }
    CGetBuffer& operator>>(LONG&  l)    { Get( &l, 1 ); l = ntohl( l ); return *this; }
    CGetBuffer& operator>>(UINT& ul)    { Get( &ul, 1 ); ul = ntohl( ul ); return *this; }
    CGetBuffer& operator>>(float& f)    { Get( &f, 1 ); return *this; }
    CGetBuffer& operator>>(double& d)   { Get( &d, 1 ); return *this; }


    template<class T> size_t Get ( T* item, size_t count )
    {    
    if ( m_nOffset + count*sizeof( T ) > m_nBufferLen )
    {
        throw CParserException();
    }
        
    memcpy( item, m_pbBuffer + m_nOffset , count*sizeof( T ) );
    m_nOffset += count*sizeof( T );
    return count*sizeof( T );
    }
    
    size_t GetString( char* lpszItem, size_t count )
    {
        if ( m_nOffset >= m_nBufferLen ) //there should be at least a '\0'
        {
        throw CParserException();
        }
        size_t nStrLen = strlen((LPCTSTR)( m_pbBuffer + m_nOffset ));
        size_t nCopy = nStrLen > count ? count : nStrLen; //be careful not to overflow the given buffer
        strncpy( lpszItem, (LPCTSTR)( m_pbBuffer + m_nOffset ), nCopy );
        m_nOffset += nStrLen + 1; //the NULL is the end of a string, independent of how many chars get
        return nCopy;
    }

    void Skip( UINT nOffset );
	void SetOffset( UINT nOffset );
    const UINT GetCurrentOffset() const { return m_nOffset; }
    LPCTSTR GetCurrentBuffer() const { return (LPCTSTR)( m_pbBuffer + m_nOffset ); }
    bool TestSkip( UINT nOffset ) const { return ( m_nOffset + nOffset ) > m_nBufferLen; }
    const UINT Size() const { return m_nBufferLen; }
private:

    BYTE*  m_pbBuffer;
    UINT   m_nBufferLen;
    UINT   m_nOffset;
};

inline CGetBuffer::CGetBuffer(BYTE* pbBuffer, UINT nBufferLen)
:m_pbBuffer(pbBuffer), m_nBufferLen(nBufferLen), m_nOffset(0)
{
}

inline CGetBuffer::CGetBuffer( char* pbBuffer, UINT nBufferLen )
:m_pbBuffer( (BYTE*)pbBuffer ), m_nBufferLen( nBufferLen ), m_nOffset( 0 )
{
}

inline void CGetBuffer::Skip( UINT nOffset )
{
    if ( m_nOffset + nOffset > m_nBufferLen )
    {
    throw CParserException();
    }
    m_nOffset += nOffset;
}

inline void CGetBuffer::SetOffset( UINT nOffset )
{
    if ( nOffset < 0 || nOffset > m_nBufferLen )
    {
        throw CParserException();
    }
    m_nOffset = nOffset;
}

inline CGetBuffer& operator >>(CGetBuffer& getBuffer, CByteArrayBuffer ba)
{
    getBuffer.Get( ba.GetBuffer(), ba.Size() );
    return getBuffer;
}

inline CGetBuffer& operator >>(CGetBuffer& getBuffer, CStringBuffer str)
{
    size_t copyed = getBuffer.GetString( str.GetBuffer(), str.Size() - 1 );
    *( str.GetBuffer() + copyed ) = '\0';
    return getBuffer;
}

inline CGetBuffer& operator >>(CGetBuffer& getBuffer, CVarString str)
{
    if ( getBuffer.TestSkip( 1 ) ) //there should at least a NULL
    {
        throw CParserException();
    }

    size_t nStringLen = strlen( getBuffer.GetCurrentBuffer() );
    size_t nAssigned = ( str.Size() > nStringLen ) ? nStringLen : str.Size();
    str.string()->assign( getBuffer.GetCurrentBuffer(), nAssigned );
    getBuffer.Skip( strlen( getBuffer.GetCurrentBuffer() ) + 1 );
    return getBuffer;
}

inline CGetBuffer& operator >>(CGetBuffer& getBuffer, CFixedString str)
{
    if ( getBuffer.TestSkip( 1 ) ) //there should at least a NULL
    {
        throw CParserException();
    }

    size_t nStringLen = strlen( getBuffer.GetCurrentBuffer() );
    size_t nAssigned = ( str.Size() > nStringLen ) ? nStringLen : str.Size();
    str.string()->assign( getBuffer.GetCurrentBuffer(), nAssigned );
    getBuffer.Skip( str.Size() );
    return getBuffer;
}

#endif //if defined(GET_BUFFER_H_0001)
