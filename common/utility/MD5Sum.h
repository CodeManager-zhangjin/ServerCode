/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */
#ifndef uint32
#define uint32 unsigned long int
#endif

class MD5Sum
{
public:
	MD5Sum();
	MD5Sum(const unsigned char* pachSource, uint32 nLen);

	const char* Calculate(const unsigned char* pachSource, uint32 nLen);
	const char* GetHash() const;
	const unsigned char* GetRawHash() const { return m_rawHash; }

private:
	char			m_sHash[64];
	unsigned char	m_rawHash[16];
};
