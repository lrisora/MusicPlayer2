/************************************************************************/
/*                                                                      */
/* c4-lib                                                               */
/* c4-lib is A Common Codes Converting Context library.                 */
/*                                                                      */
/* Version: 0.1                                                         */
/* Author:  wei_w (weiwl07@gmail.com)                                   */
/* Published under LGPL-3.0                                             */
/* https://www.gnu.org/licenses/lgpl-3.0.en.html                        */
/*                                                                      */
/* Copyright 2016 Project c4                                            */
/* https://github.com/kuyur/c4                                          */
/*                                                                      */
/************************************************************************/

#include "c4encode.h"

const char CC4Encode::LITTLEENDIAN_BOM[2]  = {'\xFF', '\xFE'};
const char CC4Encode::BIGENDIAN_BOM[2]     = {'\xFE', '\xFF'};
const char CC4Encode::UTF_8_BOM[3]         = {'\xEF', '\xBB', '\xBF'};
CC4EncodeUTF16* CC4EncodeUTF16::s_instance = NULL;
CC4EncodeUTF8*  CC4EncodeUTF8::s_instance  = NULL;
CC4EncodeUTF16::CGarbo CC4EncodeUTF16::garbo;
CC4EncodeUTF8::CGarbo  CC4EncodeUTF8::garbo;

CC4Encode::CC4Encode(const std::wstring& name, const std::wstring& version, const std::wstring& description, encode_features features, bool is_auto_check)
	:m_name(name),m_version(version),m_description(description),m_autoCheck(is_auto_check),m_encodeFeatures(features)
{}

std::wstring CC4Encode::toString() const
{
	std::wstring str;
	str.append(m_name);
	str.append(L"(");
	str.append(m_version);
	str.append(L")");
	return str;
}

bool CC4Encode::isAutoCheck() const
{
	return m_autoCheck;
}

void CC4Encode::setAutoCheck(bool is_auto_check)
{
	m_autoCheck = is_auto_check;
}

std::wstring CC4Encode::getName() const
{
	return m_name;
}

std::wstring CC4Encode::getVersion() const
{
	return m_version;
}

std::wstring CC4Encode::getDescription() const
{
	return m_description;
}

encode_features CC4Encode::getEncodeFeatures() const
{
	return m_encodeFeatures;
}

bool CC4Encode::hasFeature(CC4Encode::encodeFeature encode_feature) const
{
	return ((m_encodeFeatures&encode_feature) != 0);
}

CC4EncodeBase::CC4EncodeBase(const std::wstring& name, const std::wstring& version, const std::wstring& description, encode_features features, bool is_auto_check, const unsigned char * buffer, unsigned int buffer_length)
	:CC4Encode(name, version, description, features|typeExternal, is_auto_check),m_mapBufferLength(buffer_length)
{
	m_mapBuffer = buffer;
	m_policies  = NULL;
	m_segments  = NULL;
}

bool CC4EncodeBase::match(const char *src, unsigned int src_length) const
{
	if (hasFeature(typeBaseOnUnicode))
	{
		if ((src_length&1) != 0)
			return false;
		else
			// treat this string as unicode string
			return wmatch((wchar_t*)src, src_length>>1);
	}

	if (!hasFeature(typeBaseOnAnsi))
		return false;
	if (NULL == src)
		return false;
	if (0 == src_length)
		return false;
	if (NULL == m_mapBuffer)
		return false;
	if (NULL == m_policies)
		return false;
	if (NULL == m_segments)
		return false;

	if (!m_policies->checkValid())
		return false;
	if (!m_segments->checkValid())
		return false;

	unsigned char low=0, high=0;
	wchar_t ansiChar = 0;
	for (unsigned int i=0; i<src_length;)
	{
		memcpy(&high, src+i, 1); // reading first byte
		i++;
		if (m_policies->isContinueReadNextChar(high))
		{
			memcpy(&low, src+i, 1);
			i++;
		} else {
			low = high;
			high = 0;
		}
		// ansiChar = low + high*256;
		ansiChar = high<<8 | low;
		// TODO A2A
		if (UNKNOWN_CHAR == convertChar_A2U(ansiChar))
			return false;
	}

	return true;
}

std::wstring CC4EncodeBase::wconvertText(const char *src, unsigned int src_length) const
{
	if (hasFeature(typeBaseOnUnicode))
	{
		if ((src_length&1) != 0)
			return std::wstring();
		else
			// treat this string as unicode string
			return wconvertWideText((wchar_t*)src, src_length>>1);
	}

	if (!hasFeature(typeBaseOnAnsi))
		return std::wstring();
	if (!hasFeature(typeResultUnicode))
		return std::wstring();

	if (NULL == src)
		return std::wstring();
	if (0 == src_length)
		return std::wstring();
	if (NULL == m_mapBuffer)
		return std::wstring();
	if (NULL == m_policies)
		return std::wstring();
	if (NULL == m_segments)
		return std::wstring();

	if (!m_policies->checkValid())
		return std::wstring();
	if (!m_segments->checkValid())
		return std::wstring();
	
	unsigned int need_length = calcUnicodeStringLength(src, src_length);
	std::wstring unicodeStr(need_length, 0);
	unsigned int offset = 0;

	unsigned char low=0, high=0;
	wchar_t ansiChar = 0;
	for (unsigned int i=0; i<src_length;)
	{
		memcpy(&high, src+i, 1); // reading first byte
		i++;
		if (m_policies->isContinueReadNextChar(high))
		{
			memcpy(&low, src+i, 1);
			i++;
		} else {
			low = high;
			high = 0;
		}
		ansiChar = high<<8 | low;
		unicodeStr[offset++] = convertChar_A2U(ansiChar);
	}

	return unicodeStr;
}

std::wstring CC4EncodeBase::wconvertString(const char *src) const
{
	return wconvertText(src, (NULL!=src) ? strlen(src) : 0);
}

wchar_t CC4EncodeBase::convertChar_A2U(char high_byte, char low_byte) const
{
	wchar_t ansiChar;
	ansiChar = ((unsigned char)low_byte) + ((unsigned char)high_byte)*256;
	return convertChar_A2U(ansiChar);
}

wchar_t CC4EncodeBase::convertChar_A2U(wchar_t ansi_char) const
{
	if (!hasFeature(typeBaseOnAnsi))
		return UNKNOWN_CHAR;
	if (!hasFeature(typeResultUnicode))
		return UNKNOWN_CHAR;

	const CC4Segment* seg = m_segments->findMatchedSegment(ansi_char);
	if (NULL == seg)
		return UNKNOWN_CHAR;

	if (CC4Segment::refASCII == seg->m_reference)
		return ansi_char&0x007F;
	else if (CC4Segment::refOxFFFD == seg->m_reference)
		return UNKNOWN_CHAR;
	else if ((CC4Segment::refBUFFER == seg->m_reference))
	{
		wchar_t unicodeChar = UNKNOWN_CHAR;
		int offset = ansi_char - seg->m_begin + seg->m_offset;
		memcpy((void*)&unicodeChar, m_mapBuffer+offset*sizeof(wchar_t), sizeof(wchar_t));
		return unicodeChar;
	} else if ((CC4Segment::refSelf == seg->m_reference))
		return ansi_char;
	else
		return UNKNOWN_CHAR;
}

bool CC4EncodeBase::wmatch(const wchar_t *src, unsigned int src_str_length) const
{
	if (!hasFeature(typeBaseOnUnicode))
			return false;

	if (NULL == src)
		return false;
	if (0 == src_str_length)
		return false;
	if (NULL == m_mapBuffer)
		return false;
	/*
	if (NULL == m_policies)
		return false;
	*/
	if (NULL == m_segments)
		return false;

	/*
	if (!m_policies->checkValid())
		return false;
	*/
	if (!m_segments->checkValid())
		return false;

	for (unsigned int i=0; i<src_str_length;)
	{
		// TODO U2A
		if (UNKNOWN_CHAR == convertChar_U2U(*(src+i)))
			return false;
	}

	return true;
}

std::wstring CC4EncodeBase::wconvertWideText(const wchar_t *src, unsigned int src_str_length) const
{
	if (!hasFeature(typeBaseOnUnicode) || !hasFeature(typeResultUnicode))
		return std::wstring();

	if (!src)
		return std::wstring();
	if (src_str_length == 0)
		return std::wstring();
	if (NULL == m_mapBuffer)
		return std::wstring();
	if (NULL == m_segments)
		return std::wstring();

	std::wstring resultStr(src_str_length, 0);
	for (unsigned int i=0; i<src_str_length; ++i)
	{
		resultStr[i] = convertChar_U2U(*(src+i));
	}

	return resultStr;
}

std::wstring CC4EncodeBase::wconvertWideString(const wchar_t *src) const
{
	return wconvertWideText(src, (src) ? wcslen(src) : 0);
}

wchar_t CC4EncodeBase::convertChar_U2U(wchar_t unicode_char) const
{
	if (!hasFeature(typeBaseOnUnicode) || !hasFeature(typeResultUnicode))
		return UNKNOWN_CHAR;

	const CC4Segment* seg = m_segments->findMatchedSegment(unicode_char);
	if (NULL == seg)
		return UNKNOWN_CHAR;

	if (CC4Segment::refASCII == seg->m_reference)
		return unicode_char&0x007F;
	else if (CC4Segment::refOxFFFD == seg->m_reference)
		return UNKNOWN_CHAR;
	else if ((CC4Segment::refBUFFER == seg->m_reference))
	{
		wchar_t resultChr = UNKNOWN_CHAR;
		int offset = unicode_char - seg->m_begin + seg->m_offset;
		memcpy((void*)&resultChr, m_mapBuffer+offset*sizeof(wchar_t), sizeof(wchar_t));
		return resultChr;
	} else if ((CC4Segment::refSelf == seg->m_reference))
		return unicode_char;
	else
		return UNKNOWN_CHAR;
}

unsigned int CC4EncodeBase::calcUnicodeStringLength(const char *src, unsigned int src_length) const
{
	if (!hasFeature(typeBaseOnAnsi))
		return 0;
	if (!hasFeature(typeResultUnicode))
		return 0;

	if (NULL == src)
		return 0;
	if (0 == src_length)
		return 0;
	if (NULL == m_mapBuffer)
		return 0;
	if (NULL == m_policies)
		return 0;
	if (NULL == m_segments)
		return 0;

	if (!m_policies->checkValid())
		return 0;
	if (!m_segments->checkValid())
		return 0;

	unsigned int dest_len = 0;
	for (unsigned int i=0; i<src_length;)
	{
		if (m_policies->isContinueReadNextChar((unsigned char)*(src+i)))
		{
			i+=2;
		} else {
			i++;
		}
		dest_len++;
	}

	return dest_len;
}

bool CC4EncodeBase::convertAnsi2Unicode(const char *src, unsigned int src_length, char *dest, unsigned int dest_length, bool check_dest_length) const
{
	if (!hasFeature(typeBaseOnAnsi))
		return false;
	if (!hasFeature(typeResultUnicode))
		return false;

	if (NULL == src || NULL == dest)
		return false;
	if (!(dest>src+src_length || src>dest+dest_length))
		return false;
	if ((dest_length&1) != 0)
		return false;
	if (check_dest_length)
	{
		if (dest_length < calcUnicodeStringLength(src, src_length)*sizeof(wchar_t))
			return false;
	}

	if (!m_policies->checkValid())
		return false;
	if (!m_segments->checkValid())
		return false;

	unsigned int offset = 0;
	unsigned char low=0, high=0;
	wchar_t ansiChar = 0;
	for (unsigned int i=0; i<src_length;)
	{
		memcpy(&high, src+i, 1); // reading first byte
		i++;
		if (m_policies->isContinueReadNextChar(high))
		{
			memcpy(&low, src+i, 1);
			i++;
		} else {
			low = high;
			high = 0;
		}
		ansiChar = high<<8 | low;
		*((wchar_t*)dest+offset) = convertChar_A2U(ansiChar);
		offset++;
	}
	return true;
}

bool CC4EncodeBase::convertAnsi2Unicode(const char *src, unsigned int src_length, wchar_t *dest, unsigned int dest_str_length, bool check_dest_length) const
{
	return convertAnsi2Unicode(src, src_length, (char*)dest, dest_str_length*sizeof(wchar_t), check_dest_length);
}

bool CC4EncodeBase::convertUnicode2Unicode(const wchar_t *src, unsigned int src_str_length, wchar_t *dest, unsigned int dest_str_length) const
{
	if (!hasFeature(typeBaseOnUnicode))
		return false;
	if (!hasFeature(typeResultUnicode))
		return false;
	if (NULL == src || NULL == dest)
		return false;
	if (src_str_length != dest_str_length)
		return false;
	if (!(dest>src+src_str_length || src>dest+dest_str_length))
		return false;

	if (!m_segments->checkValid())
		return false;

	for (unsigned int i=0; i<src_str_length;)
	{
		*(dest+i) = convertChar_U2U(*(src+i));
	}
	return true;
}

bool CC4EncodeBase::setPolicies(const CC4Policies* ptr_policies)
{
	if (NULL == ptr_policies)
		return false;
	
	m_policies = ptr_policies;
	return true;
}

bool CC4EncodeBase::setSegments(const CC4Segments* ptr_segments)
{
	if (NULL == ptr_segments)
		return false;

	m_segments = ptr_segments;
	return true;
}

const CC4Policies* CC4EncodeBase::getPolicies() const
{
	return m_policies;
}

const CC4Segments* CC4EncodeBase::getSegments() const
{
	return m_segments;
}

CC4EncodeUTF16::CC4EncodeUTF16(const std::wstring& name, const std::wstring& version, const std::wstring& description, bool is_auto_check)
	:CC4Encode(name, version, description, CC4EncodeUTF16::_getEncodeFeatures(), is_auto_check)
{}

std::wstring CC4EncodeUTF16::_getName()
{
	return L"UTF-16";
}

std::wstring CC4EncodeUTF16::_getVersion()
{
	return L"Unicode";
}

std::wstring CC4EncodeUTF16::_getDescription()
{
	return L"Supporting character range: Unicode BMP(UCS-2), from 0x0000 to 0xFFFF.";
}

encode_features CC4EncodeUTF16::_getEncodeFeatures()
{
	return typeUTF16;
}

const CC4EncodeUTF16* CC4EncodeUTF16::getInstance()
{
	if (NULL == s_instance)
		s_instance = new CC4EncodeUTF16(CC4EncodeUTF16::_getName(), CC4EncodeUTF16::_getVersion(), CC4EncodeUTF16::_getDescription(), false);
	return s_instance;
}

bool CC4EncodeUTF16::match(const char *src, unsigned int src_length) const
{
	return CC4EncodeUTF16::_match(src, src_length);
}

bool CC4EncodeUTF16::wmatch(const wchar_t *src, unsigned int src_str_length) const
{
	if (NULL!=src && src_str_length>0)
		return true;
	return false;
}

std::string CC4EncodeUTF16::convertText(const char *src, unsigned int src_length) const
{
	return CC4EncodeUTF16::convert2utf8(src, src_length, true);
}

std::string CC4EncodeUTF16::convertString(const char *src) const
{
	return CC4EncodeUTF16::convert2utf8(src, (src) ? strlen(src): 0, true);
}

std::wstring CC4EncodeUTF16::wconvertText(const char *src, unsigned int src_length) const
{
	if (NULL == src)
		return std::wstring();
	if (src_length == 0)
		return std::wstring();
	// if src_length is an odd number
	if ((src_length&1) != 0)
		return std::wstring();

	const wchar_t *unicodeSrc;
	unicodeSrc = (wchar_t *)src;
	unsigned int need_length = src_length>>1;
	std::wstring unicodeStr(need_length, 0);
	for (unsigned int i=0; i<need_length; ++i)
		unicodeStr[i] = *(unicodeSrc+i);
	return unicodeStr;

	// if source is a string ended with 0x0000, a better method to return wconvertText result:
	//return std::wstring((wchar_t*)src);
}

std::wstring CC4EncodeUTF16::wconvertString(const char *src) const
{
	if (NULL == src)
		return std::wstring();
	// if src_length is an odd number
	if ((strlen(src)&1) != 0)
		return std::wstring();

	return std::wstring((wchar_t*)src);
}

std::string CC4EncodeUTF16::convertWideText(const wchar_t *src, unsigned int src_str_length) const
{
	return CC4EncodeUTF16::convert2utf8((char*)src, src_str_length*sizeof(wchar_t), true);
}

std::string CC4EncodeUTF16::convertWideString(const wchar_t *src) const
{
	return CC4EncodeUTF16::convert2utf8((char*)src, (src)? wcslen(src)*sizeof(wchar_t) : 0);
}

std::wstring CC4EncodeUTF16::wconvertWideText(const wchar_t *src, unsigned int src_str_length) const
{
	return wconvertText((char*)src, src_str_length*sizeof(wchar_t));
}

std::wstring CC4EncodeUTF16::wconvertWideString(const wchar_t *src) const
{
	if (NULL == src)
		return std::wstring();

	return std::wstring(src);
}

bool CC4EncodeUTF16::_match(const char *src, unsigned int src_length)
{
	if (NULL == src)
		return false;
	if (src_length == 0)
		return false;
	if ((src_length&1) != 0)
		return false;

	return true;
}

std::string CC4EncodeUTF16::convert2utf8(const char *src, unsigned int src_length, bool is_little_endian)
{
	if (NULL == src)
		return std::string();
	if (src_length == 0)
		return std::string();
	// if src_length is an odd number
	if ((src_length&1) != 0)
		return std::string();

	unsigned int need_length = calcUtf8StringLength(src, src_length, is_little_endian);
	std::string utf8str(need_length, 0);
	unsigned int offset = 0;

	const unsigned char *unsignedSrc = (unsigned char *)src;
	wchar_t chr = 0;
	char dest_chars[3];
	memset((void*)dest_chars, 0, 3);
	unsigned int dest_len;
	for (unsigned int i=0;i<src_length;)
	{
		dest_len = 0;
		if (is_little_endian)
			chr = *(unsignedSrc+i) + (*(unsignedSrc+i+1))*256;
		else
			chr = *(unsignedSrc+i+1) + (*(unsignedSrc+i))*256;
		i+=2;

		if (chr <= 0x007F)  // U-0000 - U-007F
		{
			dest_len = 1;
			dest_chars[0] = (char)chr;
		} else if (chr <= 0x07FF) {  // U-0080 - U-07FF
			dest_len = 2;
			dest_chars[0] = (char)(0xC0 | (chr>>6));
			dest_chars[1] = (char)(0x80 | (chr&0x003F));
		} else if (chr >= 0xD800 && chr <= 0xDFFF) {  // U-D800 - U-DFFF
			// ignore this unicode character
			dest_len = 0;
		} else {  // U-0800 - UD7FF and UE000 - UFFFF
			dest_len = 3;
			dest_chars[0] = (char)(0xE0 | (chr>>12));
			dest_chars[1] = (char)(0x80 | ((chr>>6) & 0x003F));
			dest_chars[2] = (char)(0x80 | (chr & 0x003F));
		}
		for (unsigned int j=0;j<dest_len;j++)
			utf8str[offset++] = dest_chars[j];
	}

	return utf8str;
}

std::string CC4EncodeUTF16::convert2utf8(const wchar_t *src, unsigned int src_str_length)
{
	return convert2utf8((char*)src, src_str_length*2, true);
}

bool CC4EncodeUTF16::convert2utf8(const char *src, unsigned int src_length, char *dest, unsigned int dest_length, bool is_little_endian, bool check_dest_length)
{
	if (NULL == src || NULL == dest)
		return false;
	if (0 == src_length || 0 == dest_length)
		return false;
	if (!(dest>src+src_length || src>dest+dest_length))
		return false;
	if ((src_length&1) != 0)
		return false;
	if (check_dest_length)
	{
		if (dest_length < calcUtf8StringLength(src, src_length, is_little_endian))
			return false;
	}

	unsigned int offset = 0;
	const unsigned char *unsignedSrc = (unsigned char *)src;
	wchar_t chr = 0;
	memset((void*)dest, 0, dest_length);
	for (unsigned int i=0;i<src_length;)
	{
		if (is_little_endian)
			chr = *(unsignedSrc+i) + (*(unsignedSrc+i+1))*256;
		else
			chr = *(unsignedSrc+i+1) + (*(unsignedSrc+i))*256;
		i+=2;

		if (chr <= 0x007F)  // U-0000 - U-007F
		{
			*(dest + offset++) = (char)chr;
		} else if (chr <= 0x07FF) {  // U-0080 - U-07FF
			*(dest + offset++) = (char)(0xC0 | (chr>>6));
			*(dest + offset++) = (char)(0x80 | (chr&0x003F));
		} else if (chr >= 0xD800 && chr <= 0xDFFF) {  // U-D800 - U-DFFF
			// ignore this unicode character
		} else {  // U-0800 - UD7FF and UE000 - UFFFF
			*(dest + offset++) = (char)(0xE0 | (chr>>12));
			*(dest + offset++) = (char)(0x80 | ((chr>>6) & 0x003F));
			*(dest + offset++) = (char)(0x80 | (chr & 0x003F));
		}
	}

	return true;
}

unsigned int CC4EncodeUTF16::calcUtf8StringLength(const char *src, unsigned int src_length, bool is_little_endian)
{
	if (NULL == src)
		return 0;
	if (src_length == 0)
		return 0;
	// if src_length is an odd number
	if ((src_length&1) != 0)
		return 0;

	const unsigned char *unsignedSrc = (unsigned char *)src;
	wchar_t chr = 0;
	unsigned int dest_len = 0;
	for (unsigned int i=0;i<src_length;)
	{
		if (is_little_endian)
			chr = *(unsignedSrc+i) + (*(unsignedSrc+i+1))*256;
		else
			chr = *(unsignedSrc+i+1) + (*(unsignedSrc+i))*256;
		i+=2;

		if (chr <= 0x007F)  // U-0000 - U-007F
		{
			dest_len += 1;
		} else if (chr <= 0x07FF) {  // U-0080 - U-07FF
			dest_len += 2;
		} else if (chr >= 0xD800 && chr <= 0xDFFF) {  // U-D800 - U-DFFF
			dest_len += 0;
		} else {  // U-0800 - UD7FF and UE000 - UFFFF
			dest_len += 3;
		}
	}

	return dest_len;
}

CC4EncodeUTF8::CC4EncodeUTF8(const std::wstring& name, const std::wstring& version, const std::wstring& description, bool is_auto_check)
	:CC4Encode(name, version, description, CC4EncodeUTF8::_getEncodeFeatures(), is_auto_check)
{}

std::wstring CC4EncodeUTF8::_getName()
{
	return L"UTF-8";
}

std::wstring CC4EncodeUTF8::_getVersion()
{
	return L"Unicode";
}

std::wstring CC4EncodeUTF8::_getDescription()
{
	return L"Supporting character range: Unicode BMP(UCS-2), from 0x0000 to 0xFFFF.";
}

encode_features CC4EncodeUTF8::_getEncodeFeatures()
{
	return typeUTF8;
}

const CC4EncodeUTF8* CC4EncodeUTF8::getInstance()
{
	if (NULL == s_instance)
		s_instance = new CC4EncodeUTF8(CC4EncodeUTF8::_getName(), CC4EncodeUTF8::_getVersion(), CC4EncodeUTF8::_getDescription(), true);
	return s_instance;
}

bool CC4EncodeUTF8::match(const char *src, unsigned int src_length) const
{
	return CC4EncodeUTF8::_match(src, src_length);
}

bool CC4EncodeUTF8::wmatch(const wchar_t *src, unsigned int src_str_length) const
{
	return false;
}

std::string CC4EncodeUTF8::convertText(const char *src, unsigned int src_length) const
{
	// check is utf-8 string or not
	if (!_match(src, src_length))
		return std::string();
	
	std::string buildUtf8Str(src_length, 0);
	for (unsigned int i=0; i<src_length; ++i)
	{
		buildUtf8Str[i] = *(src+i);
	}
	return buildUtf8Str;
}

std::string CC4EncodeUTF8::convertString(const char *src) const
{
	// check is utf-8 string or not
	if (!_match(src, (src)?strlen(src):0) || src == nullptr)
		return std::string();

	return std::string(src);
}

std::wstring CC4EncodeUTF8::wconvertText(const char *src, unsigned int src_length) const
{
	return CC4EncodeUTF8::convert2unicode(src, src_length);
}

std::wstring CC4EncodeUTF8::wconvertString(const char *src) const
{
	return CC4EncodeUTF8::convert2unicode(src, (NULL!=src) ? strlen(src) : 0);
}

bool CC4EncodeUTF8::_match(const char *src, unsigned int src_length)
{
	if (NULL == src)
		return false;
	if (src_length == 0)
		return false;

	unsigned int i = 0;
	bool isMatched = true;
	while (i < src_length)
	{
		if ((0x80 & *(src+i)) == 0) // ASCII
		{
			i++;
			continue;
		}
		else if((0xE0 & *(src+i)) == 0xC0) // 110xxxxx
		{
			if (*(src+i+1) == '\0')
			{
				isMatched=false;
				break;
			}
			if ((0xC0 & *(src+i+1)) == 0x80) // 10xxxxxx
			{
				i+=2;
				continue;
			}
			else
			{
				isMatched=false;
				break;
			}
		}
		else if((0xF0 & *(src+i)) == 0xE0) // 1110xxxx
		{
			if (*(src+i+1) == '\0')
			{
				isMatched=false;
				break;
			}
			if (*(src+i+2) == '\0')
			{
				isMatched=false;
				break;
			}
			if (((0xC0 & *(src+i+1)) == 0x80) && ((0xC0 & *(src+i+2)) == 0x80)) // 10xxxxxx 10xxxxxx
			{
				i+=3;
				continue;
			}
			else
			{
				isMatched=false;
				break;
			}
		}
		else // not matched
		{
			isMatched=false;
			break;
		}
	}
	return isMatched;
}

std::wstring CC4EncodeUTF8::convert2unicode(const char *src, unsigned int src_length)
{
	if (NULL == src)
		return std::wstring();
	if (0 == src_length)
		return std::wstring();

	unsigned int need_length = calcUnicodeStringLength(src, src_length);
	if ((0 == need_length) || ((unsigned int) -1 == need_length))
		return std::wstring();

	unsigned int offset = 0;
	std::wstring unicodeStr(need_length, 0);
	wchar_t chr=0;
	for (unsigned int i=0;i<src_length;)
	{
		if ((0x80 & *(src+i)) == 0) // ASCII
		{
			chr=*(src+i);
			i++;
		}
		else if((0xE0 & *(src+i)) == 0xC0) // 110xxxxx 10xxxxxx
		{
			chr =(*(src+i) & 0x3F)<<6;
			chr|=(*(src+i+1) & 0x3F);
			i+=2;
		}
		else if((0xF0 & *(src+i)) == 0xE0) // 1110xxxx 10xxxxxx 10xxxxxx
		{
			chr =(*(src+i) & 0x1F)<<12;
			chr|=(*(src+i+1) & 0x3F)<<6;
			chr|=(*(src+i+2) & 0x3F);
			i+=3;
		}
		/*
		else if() // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		{}
		else if() // 111110xx 10xxxxxx 10xxxxxx 10xxxxxx  10xxxxxx
		{}
		else if() // 1111110x 10xxxxxx 10xxxxxx 10xxxxxx  10xxxxxx 10xxxxxx 
		{}
		*/
		else // not utf-8 string
		{
			return unicodeStr;
		}
		unicodeStr[offset++] = chr;
	}

	return unicodeStr;
}

bool CC4EncodeUTF8::convert2unicode(const char *src, unsigned int src_length, char *dest, unsigned int dest_length, bool check_dest_length)
{
	if (NULL == src || NULL == dest)
		return false;
	if (!(dest>src+src_length || src>dest+dest_length))
		return false;
	if ((dest_length&1) != 0)
		return false;
	if (check_dest_length)
	{
		unsigned int need_length = calcUnicodeStringLength(src, src_length);
		if ((unsigned int)-1 == need_length)
			return false;
		if (dest_length < need_length*sizeof(wchar_t))
			return false;
	}

	unsigned int offset = 0;
	wchar_t chr=0;
	for (unsigned int i=0; i<src_length;)
	{
		if ((0x80 & *(src+i)) == 0) // ASCII
		{
			chr = *(src+i);
			i++;
		}
		else if((0xE0 & *(src+i)) == 0xC0) // 110xxxxx 10xxxxxx
		{
			chr =(*(src+i) & 0x3F)<<6;
			chr|=(*(src+i+1) & 0x3F);
			i+=2;
		}
		else if((0xF0 & *(src+i)) == 0xE0) // 1110xxxx 10xxxxxx 10xxxxxx
		{
			chr =(*(src+i) & 0x1F)<<12;
			chr|=(*(src+i+1) & 0x3F)<<6;
			chr|=(*(src+i+2) & 0x3F);
			i+=3;
		}
		/*
		else if() // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		{}
		else if() // 111110xx 10xxxxxx 10xxxxxx 10xxxxxx  10xxxxxx
		{}
		else if() // 1111110x 10xxxxxx 10xxxxxx 10xxxxxx  10xxxxxx 10xxxxxx 
		{}
		*/
		else // not utf-8 string
		{
			return false;
		}
		*((wchar_t*)dest+offset) = chr;
		offset++;
	}
	return true;
}

bool CC4EncodeUTF8::convert2unicode(const char *src, unsigned int src_length, wchar_t *dest, unsigned int dest_str_length, bool check_dest_length)
{
	return convert2unicode(src, src_length, (char*)dest, dest_str_length*sizeof(wchar_t), check_dest_length);
}

unsigned int CC4EncodeUTF8::calcUnicodeStringLength(const char *src, unsigned int src_length)
{
	if (NULL == src)
		return -1;
	if (0 == src_length)
		return 0;

	unsigned int dest_length=0;
	for (unsigned int i=0; i<src_length;)
	{
		// Only _match first byte of the utf-8 character
		if ((0x80 & *(src+i)) == 0) // ASCII
		{
			i++;
			dest_length++;
		}
		else if((0xE0 & *(src+i)) == 0xC0) // 110xxxxx 10xxxxxx
		{
			i+=2;
			dest_length++;
		}
		else if((0xF0 & *(src+i)) == 0xE0) // 1110xxxx 10xxxxxx 10xxxxxx
		{
			i+=3;
			dest_length++;
		}
		/*
		else if() // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		{}
		else if() // 111110xx 10xxxxxx 10xxxxxx 10xxxxxx  10xxxxxx
		{}
		else if() // 1111110x 10xxxxxx 10xxxxxx 10xxxxxx  10xxxxxx 10xxxxxx 
		{}
		*/
		else // not utf-8 string
		{
			return -1; // should return -1
		}
	}

	return dest_length;
}