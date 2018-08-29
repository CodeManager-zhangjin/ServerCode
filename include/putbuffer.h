#if !defined(PUT_BUFFER_H_0001)
#define PUT_BUFFER_H_0001

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include <string.h>
#include <netinet/in.h> //for the htonl and htons
#include "bufferparam.h"
///this class servers for the buffer filling sent to socket by stream themod
class CPutBuffer
{
public:
    CPutBuffer();
    CPutBuffer( BYTE* pbBuffer, UINT nBufferLen );
    virtual ~CPutBuffer(){}
    /// put operations for basic type, you can extend it by write a sophisticated globle operator <<
    CPutBuffer& operator<<(BYTE by)   { return Put( &by, 1 ); }
    CPutBuffer& operator<<(char ch)   { return Put( &ch, 1 );}
    CPutBuffer& operator<<(short w)   { short wn = htons( w ); return Put( &wn, 1 ); }
    CPutBuffer& operator<<(WORD us)   { WORD usn = htons( us ); return Put( &usn, 1); }
    CPutBuffer& operator<<(int n)     { int nn = htonl( n ); return Put( &nn, 1 ); }
    CPutBuffer& operator<<(DWORD dw)  { DWORD dwn = htonl( dw ); return Put( &dwn, 1 ); }
    CPutBuffer& operator<<(long l)    { long ln = htonl( l ); return Put( &ln, 1 ); }
    CPutBuffer& operator<<(UINT ul)   { UINT uln = htonl( ul ); return Put( &uln, 1 ); }
    CPutBuffer& operator<<(float f)   { return Put( &f, 1 ); }
    CPutBuffer& operator<<(double d)  { return Put( &d, 1 ); }
    
    operator BYTE* () const { return m_pbBuffer; }
    operator char* () const { return (char*)m_pbBuffer; }
    operator void* () const { return (void*)m_pbBuffer; }
    UINT  GetFilledSize() const { return m_nOffset; }
    UINT  Size() const { return m_nBufferLen; }
	BYTE* GetCurrentBuffer() { return m_pbBuffer + m_nOffset; }
    void SetOffset( UINT nOffset );
	void SetBuffer(BYTE* pbBuffer, UINT nBufferLen);
    void Skip( UINT nOffset );


    ///don't call this method directly, call operator << instead
    ///this method is called when you are implementing operator <<
    template<class T> CPutBuffer& Put ( const T* item, size_t count )
    {
        if ( m_nOffset + count*sizeof( T ) > m_nBufferLen )
        {
			printf("bufflen:%d offset:%d new:%d\n", m_nBufferLen, m_nOffset, count*sizeof(T));
            throw CParserException();
        }
        
        memcpy( m_pbBuffer + m_nOffset, item, count*sizeof(T) );
        m_nOffset += count*sizeof( T );
        return *this;
    }

private:
    BYTE*  m_pbBuffer;
    UINT   m_nBufferLen;
    UINT   m_nOffset;
};

inline CPutBuffer::CPutBuffer(BYTE* pbBuffer, UINT nBufferLen)
    :m_pbBuffer(pbBuffer), m_nBufferLen(nBufferLen), m_nOffset(0)
{
}

inline CPutBuffer::CPutBuffer()
    :m_pbBuffer(NULL), m_nBufferLen(0), m_nOffset(0)
{
}

inline void CPutBuffer::SetBuffer(BYTE* pbBuffer, UINT nBufferLen)
{
	if(m_pbBuffer) return;
	m_pbBuffer = pbBuffer;
	m_nBufferLen = nBufferLen;
	m_nOffset = 0;
}

inline void CPutBuffer::Skip( UINT nOffset )
{
    if ( m_nOffset + nOffset > m_nBufferLen )
    {
        throw CParserException();
    }
    m_nOffset += nOffset;
}

inline void CPutBuffer::SetOffset( UINT nOffset )
{
    if ( nOffset < 0 || nOffset > m_nBufferLen )
    {
        throw CParserException();
    }
    m_nOffset = nOffset;
}

inline CPutBuffer& operator<<( CPutBuffer& putBuffer, const CByteArrayBuffer& ba )
{
    return putBuffer.Put( ba.GetBuffer(), ba.Size() );
}

inline CPutBuffer& operator <<( CPutBuffer& putBuffer, const CStringBuffer& str )
{
    size_t nStrLen = strlen( str.GetBuffer() );
    //str.Size() < nStrLen means just copy the first str.Size() chars instead of whole string
    size_t nCopy = str.Size() < nStrLen ? str.Size() : nStrLen;
    putBuffer.Put( str.GetBuffer(), nCopy );
    //don't forget put a NULL in the end of the string
    char chNULL = '\0';
    return putBuffer << chNULL;
}

inline CPutBuffer& operator<< ( CPutBuffer& putBuffer, const std::string& str )
{
    putBuffer.Put( str.data(), str.size() );
    //don't forget put a NULL in the end of the string
    char chNULL = '\0';
    return putBuffer << chNULL;
}

inline CPutBuffer& operator<< (CPutBuffer& putBuffer, const CVarString& str)
{
    std::string* pString = str.string();
    int nStrLen = pString->size();
    if ( (UINT)nStrLen > str.Size() - 1 ) //leave one space for NULL
    {
        nStrLen = str.Size() - 1;
    }
    putBuffer.Put( pString->data(), nStrLen );
    //don't forget put a NULL in the end of the string
    char chNULL = '\0';
    putBuffer << chNULL;
    return putBuffer;
}

inline CPutBuffer& operator<< (CPutBuffer& putBuffer, const CFixedString& str)
{
    std::string* pString = str.string();
    int nStrLen = pString->size();
    if ( (UINT)nStrLen > str.Size() - 1 ) //leave one space for NULL
    {
        nStrLen = str.Size() - 1;
    }
    putBuffer.Put( pString->data(), nStrLen );
    //don't forget put a NULL in the end of the string
    char chNULL = '\0';
    putBuffer << chNULL;
    //padding the left space
    putBuffer.Skip( str.Size() - nStrLen - 1 );
    return putBuffer;
}

#endif //if defined(PUT_BUFFER_H_0001)
