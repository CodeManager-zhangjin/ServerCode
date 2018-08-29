#include "vurl.h"
#include <ctype.h>
#include <assert.h>

bool CVUrl::IsSafed(unsigned char uch)
{
	if( isalnum(uch) )
		return true;

	if( (uch == '-')
		|| (uch == '_')
		|| (uch == '.')
		|| (uch == '~') )
		return true;

	return false;
}

char CVUrl::ToHexChar(unsigned char uch, bool isUpper)
{
	if(uch <= 9)
		return (uch + '0');

	if( isUpper )
		return (uch - 10 + 'A');
	return (uch - 10 + 'a');
}

unsigned char CVUrl::FromHexChar(char uch)
{
	if(uch >= 'A' && uch <= 'Z')
		return (uch - 'A' + 10);

	if(uch >= 'a' && uch <= 'z')
		return (uch - 'a' + 10);

	if(uch >= '0' && uch <= '9')
		return (uch - '0');

	return 0;
}

void CVUrl::UrlEncode(const std::string &strIn,
	std::string &strOut, bool isUpper)
{
	size_t len = strIn.length();
	for (size_t i = 0; i < len; ++i)
	{
		unsigned char uch = strIn[i];
		if( IsSafed(uch) )
			strOut += uch;
		else if(uch == ' ')
			strOut += "+";
		else
		{
			strOut += '%';
			strOut += ToHexChar(uch >> 4, isUpper);
			strOut += ToHexChar(uch & 0x0f, isUpper);
		}
	}
}

void CVUrl::UrlDecode(const std::string &strIn,
	std::string &strOut)
{
	size_t len = strIn.length();
	for (size_t i = 0; i < len; ++i)
	{
		if (strIn[i] == '+')
			strOut += ' ';
		else if (strIn[i] == '%' && i + 2 < len)
		{
			unsigned char left4Bit = FromHexChar(strIn[++i]);
			unsigned char right4Bit = FromHexChar(strIn[++i]);
			strOut += (left4Bit << 4) + right4Bit;
		}
		else
			strOut += strIn[i];
	}
}
