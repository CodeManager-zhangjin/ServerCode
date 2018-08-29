#ifndef __VURL_H__
#define __VURL_H__
#include <stdio.h>
#include <string>
class CVUrl
{
public:
	CVUrl(){}
	~CVUrl(){}
	void UrlEncode(const std::string &strIn, std::string &strOut, bool isUpper = true);
	void UrlDecode(const std::string &strIn, std::string &strOut);
private:

	bool IsSafed(unsigned char in);
	char ToHexChar(unsigned char x, bool isUpper);
	unsigned char FromHexChar(char x);
};
#endif // __VURL_H__
