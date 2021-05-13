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

#include <fstream>
#include "c4context.h"
#include "c4encode.h"
#include "tinyxml.h"

CC4Context::CC4Context(const std::wstring &charmap_name, const std::wstring &base_path)
	:m_bInitialized(false), m_charmapConfPath(base_path), m_basePath(base_path), m_errorMessage(L"no error.")
{
	m_charmapConfPath.append(charmap_name);
	m_constEncodes.push_back((const CC4Encode*)CC4EncodeUTF16::getInstance());
	m_constEncodes.push_back((const CC4Encode*)CC4EncodeUTF8::getInstance());
}

CC4Context::~CC4Context()
{
	finalize();
}

void CC4Context::setCharmapConfPath(const std::wstring &charmap_name, const std::wstring &base_path)
{
	m_basePath.clear();
	m_basePath.append(base_path);
	m_charmapConfPath.clear();
	m_charmapConfPath.append(base_path);
	m_charmapConfPath.append(charmap_name);
}

const wchar_t* CC4Context::getLastErrorMessage() const
{
	return m_errorMessage.c_str();
}

bool CC4Context::init()
{
	if (m_bInitialized) return m_bInitialized;

	// Because TiXml does not support wchar_t file name,
	// use stream to load xml file.
	std::wifstream file(m_charmapConfPath.c_str(), std::ios::in|std::ios::binary);
	if (!file)
	{
		m_errorMessage.clear();
		m_errorMessage.append(L"Error: Failed to open charmap config file. Pleae check config file existing or not:");
		m_errorMessage.append(m_charmapConfPath);
		return m_bInitialized;
	}
	
	file.seekg(0, std::ios::end);
	unsigned int fileLength = file.tellg();
	char *fileBuffer = new char[fileLength+1];
	memset((void*)fileBuffer, 0, fileLength+1);
	file.seekg(0, 0);
	wchar_t chr = 0;
	for (unsigned int i=0; i<fileLength; ++i)
	{
		file.get(chr);
		// Only need to copy high byte. The low byte is 0x00
		memcpy(fileBuffer+i, &chr, 1);
	}
	file.close();

	TiXmlDocument *doc = new TiXmlDocument;
	doc->Parse(fileBuffer, NULL, TIXML_ENCODING_UTF8);
	if (doc->Error())
	{
		delete []fileBuffer;
		delete doc;
		m_errorMessage.clear();
		m_errorMessage.append(L"Error: Failed to parse xml file. Check content of config file.");
		return m_bInitialized;
	}
	delete []fileBuffer;
	fileBuffer = NULL;

	m_bInitialized = loadCharmapConfig(doc);
	delete doc;

	return m_bInitialized;
}

bool CC4Context::loadCharmapConfig(const TiXmlDocument *xmlfile)
{
	if (!xmlfile)
	{
		m_errorMessage.clear();
		m_errorMessage.append(L"Error: null xml pointer.");
		return false;
	}
	
	// node of charmaps. Root node.
	const TiXmlElement *pRoot = xmlfile->RootElement();
	if (!pRoot)
	{
		m_errorMessage.clear();
		m_errorMessage.append(L"Error: Failed to get root node in xml file.");
		return false;
	}
	if (strcmp(pRoot->Value(), "charmaps") != 0)
	{
		m_errorMessage.clear();
		m_errorMessage.append(L"Error: Failed to get charmaps node in xml file. Wrong node name.");
		return false;
	}

	const TiXmlElement *pElem = pRoot;
	// iterate all charmap nodes.
	bool loadCharmapsResult = false;
	for (const TiXmlElement *sub_tag=pElem->FirstChildElement(); sub_tag; sub_tag=sub_tag->NextSiblingElement())
	{
		loadCharmapsResult = loadCharmap(sub_tag) || loadCharmapsResult;
	}
	
	return loadCharmapsResult;
}

bool CC4Context::loadCharmap(const TiXmlElement *charmap_node)
{
	bool loadCharmapResult = false;
	if (!charmap_node) return loadCharmapResult;
	if (strcmp(charmap_node->Value(), "charmap") != 0) return loadCharmapResult;
	
	const TiXmlElement *pElem;

	// name
	pElem = charmap_node->FirstChild("name")->ToElement();
	if (!pElem) return loadCharmapResult;
	if (!pElem->GetText()) return loadCharmapResult;
	const std::wstring& name = CC4EncodeUTF8::convert2unicode(pElem->GetText(), strlen(pElem->GetText()));

	// version. Version is optional
	const char *ver = NULL;
	pElem = charmap_node->FirstChildElement("version");
	if (pElem)
		 ver = charmap_node->FirstChildElement("version")->GetText();
	const std::wstring& version = CC4EncodeUTF8::convert2unicode(ver, (NULL!=ver) ? strlen(ver) : 0);

	// description. Description is optional
	const char *desc = NULL;
	pElem = charmap_node->FirstChildElement("description");
	if (pElem)
		desc = charmap_node->FirstChildElement("description")->GetText();
	const std::wstring& description = CC4EncodeUTF8::convert2unicode(desc, (NULL!=desc) ? strlen(desc) : 0);

	// isAutoCheck
	bool isAutoCheck = false;
	pElem = charmap_node->FirstChild("useinautocheck")->ToElement();
	if (!pElem) return loadCharmapResult;
	if (!pElem->GetText()) return loadCharmapResult;
	if (_stricmp(pElem->GetText(),"true") == 0)
		isAutoCheck = true;
	else
		isAutoCheck = false;

	// path
	pElem = charmap_node->FirstChild("path")->ToElement();
	if (!pElem) return loadCharmapResult;
	if (!pElem->GetText()) return loadCharmapResult;
	std::wstring mapPath(m_basePath);
	mapPath.append(CC4EncodeUTF8::convert2unicode(pElem->GetText(), strlen(pElem->GetText())));

	// feature
	encode_features features = CC4Encode::typeNoFeature;
	for (const TiXmlElement *sub_tag=charmap_node->FirstChildElement("feature"); sub_tag; sub_tag=sub_tag->NextSiblingElement("feature"))
	{
		if (!sub_tag->GetText()) continue;
		features |= CC4Encode::toEncodeFeature(sub_tag->GetText());
	}
	if (!CC4Encode::checkFeatureValid(features)) return loadCharmapResult;

	// readingpolicy
	int num = 0;
	for (const TiXmlElement *sub_tag=charmap_node->FirstChildElement("readingpolicy"); sub_tag; sub_tag=sub_tag->NextSiblingElement("readingpolicy"))
	{
		num++;
	}
	if (0 == num) return loadCharmapResult;
	CC4Policies *policies = new CC4Policies(num);
	num = 0;
	loadCharmapResult = true;
	for (const TiXmlElement *sub_tag=charmap_node->FirstChildElement("readingpolicy"); sub_tag; sub_tag=sub_tag->NextSiblingElement("readingpolicy"))
	{
		/*
		wchar_t policy_begin;
		if (!toHexValue(sub_tag->Attribute("begin"), policy_begin))
		{
			loadResult = false;
			break;
		}
		*/
		// begin attribute
		if (!sub_tag->Attribute("begin"))
		{
			loadCharmapResult = false;
			break;
		}
		unsigned int begin;
		if (sscanf_s(sub_tag->Attribute("begin"), "%x", &begin) == -1)
		{
			loadCharmapResult = false;
			break;
		}
		if (begin > 0xFF)
		{
			loadCharmapResult = false;
			break;
		}
		unsigned char policy_begin = (unsigned char)begin;

		// end attribute
		if (!sub_tag->Attribute("end"))
		{
			loadCharmapResult = false;
			break;
		}
		unsigned int end;
		if (sscanf_s(sub_tag->Attribute("end"), "%x", &end) == -1)
		{
			loadCharmapResult = false;
			break;
		}
		if (end > 0xFF)
		{
			loadCharmapResult = false;
			break;
		}
		unsigned char policy_end = (unsigned char)end;

		// readnext attribute
		bool policy_readnext=false;
		if (!sub_tag->Attribute("readnext"))
		{
			loadCharmapResult = false;
			break;
		}
		if (_stricmp(sub_tag->Attribute("readnext"),"true") == 0)
			policy_readnext = true;
		else
			policy_readnext = false;
		
		CC4Policy policy;
		policy.m_begin    = policy_begin;
		policy.m_end      = policy_end;
		policy.m_readnext = policy_readnext;
		policies->setPolicy(num, policy);
		num++;
	}
	if (!loadCharmapResult)
	{
		if (policies != NULL)
			delete policies;
		m_errorMessage.clear();
		m_errorMessage.append(L"Error: Failed to load policies. Charmap name:");
		m_errorMessage.append(name);
		return loadCharmapResult;
	}

	// segment
	num = 0;
	for (const TiXmlElement *sub_tag=charmap_node->FirstChildElement("segment"); sub_tag; sub_tag=sub_tag->NextSiblingElement("segment"))
	{
		num++;
	}
	if (0 == num)
	{
		loadCharmapResult = false;
		if (policies != NULL)
			delete policies;
		return loadCharmapResult;
	}
	CC4Segments *segments = new CC4Segments(num);
	num = 0;
	for (const TiXmlElement *sub_tag=charmap_node->FirstChildElement("segment"); sub_tag; sub_tag=sub_tag->NextSiblingElement("segment"))
	{
		/*
		wchar_t segment_begin;
		if (!toHexValue(sub_tag->Attribute("begin"), segment_begin))
		{
			loadResult = false;
			break;
		}
		*/
		// begin attribute
		if (!sub_tag->Attribute("begin"))
		{
			loadCharmapResult = false;
			break;
		}
		unsigned int begin;
		if (sscanf_s(sub_tag->Attribute("begin"), "%x", &begin) == -1)
		{
			loadCharmapResult = false;
			break;
		}
		if (begin > 0xFFFF)
		{
			loadCharmapResult = false;
			break;
		}
		wchar_t segment_begin = (wchar_t)begin;

		// end attribute
		if (!sub_tag->Attribute("end"))
		{
			loadCharmapResult = false;
			break;
		}
		unsigned int end;
		if (sscanf_s(sub_tag->Attribute("end"), "%x", &end) == -1)
		{
			loadCharmapResult = false;
			break;
		}
		if (end > 0xFFFF)
		{
			loadCharmapResult = false;
			break;
		}
		wchar_t segment_end = (wchar_t)end;

		// reference attribute
		CC4Segment::segmentRef segment_refType = CC4Segment::toSegmentRef(sub_tag->Attribute("reference"));
		int segment_offset = -1;
		if (CC4Segment::refBUFFER == segment_refType)
		{
			// offset attribute
			if (!sub_tag->Attribute("offset"))
			{
				loadCharmapResult = false;
				break;
			}
			segment_offset = atoi(sub_tag->Attribute("offset"));
			if (segment_offset < 0)
			{
				loadCharmapResult = false;
				break;
			}
		}
		
		CC4Segment segment;
		segment.m_begin     = segment_begin;
		segment.m_end       = segment_end;
		segment.m_reference = segment_refType;
		segment.m_offset    = segment_offset;
		segments->setSegment(num, segment);
		num++;
	}
	if (!loadCharmapResult)
	{
		if (policies != NULL)
			delete policies;
		if (segments != NULL)
			delete segments;
		m_errorMessage.clear();
		m_errorMessage.append(L"Error: Failed to load segments. Charmap name:");
		m_errorMessage.append(name);
		return loadCharmapResult;
	}

	// Loading map buffer
	std::wifstream mapFile(mapPath.c_str(), std::ios::in|std::ios::binary);
	if (!mapFile)
	{
		loadCharmapResult = false;
		if (policies != NULL)
			delete policies;
		if (segments != NULL)
			delete segments;
		m_errorMessage.clear();
		m_errorMessage.append(L"Error: Failed to open charmap. Check charmap existing or not:");
		m_errorMessage.append(mapPath);
		return loadCharmapResult;
	}
	mapFile.seekg(0, std::ios::end);
	unsigned int mapBufferLength = mapFile.tellg();
	unsigned char *mapBuffer = new unsigned char[mapBufferLength];
	memset((void*)mapBuffer, 0 ,mapBufferLength);
	mapFile.seekg(0, 0);
	wchar_t chr = 0;
	for (unsigned int i=0; i<mapBufferLength; ++i)
	{
		mapFile.get(chr);
		memcpy(mapBuffer+i, &chr, 1);
	}
	mapFile.close();

	// create CC4Encode in memory
	CC4EncodeBase *encode = new CC4EncodeBase(name, version, description, features, isAutoCheck, mapBuffer, mapBufferLength);
	encode->setPolicies(policies);
	encode->setSegments(segments);

	m_mapBuffers.push_back(mapBuffer);
	m_policies.push_back(policies);
	m_segments.push_back(segments);
	m_encodes.push_back((CC4Encode*)encode);

	return loadCharmapResult;
}

void CC4Context::finalize()
{
	if (m_bInitialized)
	{
		// delete encode
		std::vector<CC4Encode*>::iterator encode_iter;
		encode_iter = m_encodes.begin();
		for (; encode_iter != m_encodes.end(); ++encode_iter)
			delete *encode_iter;
		m_encodes.erase(m_encodes.begin(), m_encodes.end());

		// delete policies
		std::vector<CC4Policies*>::iterator policies_iter;
		for (policies_iter = m_policies.begin(); policies_iter != m_policies.end(); ++policies_iter)
			delete *policies_iter;
		m_policies.clear();

		// delete segments
		std::vector<CC4Segments*>::iterator segment_iter;
		for (segment_iter = m_segments.begin(); segment_iter != m_segments.end(); ++segment_iter)
			delete *segment_iter;
		m_segments.clear();

		// delete map buffer
		std::vector<unsigned char*>::iterator buffer_iter;
		for (buffer_iter = m_mapBuffers.begin(); buffer_iter != m_mapBuffers.end(); ++buffer_iter)
		{
			delete *buffer_iter;
		}
		m_mapBuffers.clear();
	}
	m_bInitialized = false;
}

const CC4Encode* CC4Context::getEncode(const std::wstring& encode_name) const
{
	for (unsigned int i = 0; i < m_constEncodes.size(); ++i)
	{
		const CC4Encode *encode = m_constEncodes[i];
		if (encode->getName().compare(encode_name) == 0)
			return encode;
	}
	for (unsigned int i = 0; i < m_encodes.size(); ++i)
	{
		CC4Encode *encode = m_encodes[i];
		if (encode->getName().compare(encode_name) == 0)
			return encode;
	}
	return NULL;
}

unsigned int CC4Context::getEncodeAmount() const
{
	return m_encodes.size() + m_constEncodes.size();
}

const CC4Encode* CC4Context::getMostPossibleEncode(const std::string& text) const
{
	for (unsigned int i = 0; i < m_constEncodes.size(); ++i)
	{
		const CC4Encode *encode = m_constEncodes[i];
		if (!encode->isAutoCheck())
			continue;
		if (encode->match(text.c_str(), text.size()))
			return encode;
	}
	for (unsigned int i = 0; i< m_encodes.size(); ++i)
	{
		CC4Encode *encode = m_encodes[i];
		if (!encode->isAutoCheck())
			continue;
		if (encode->match(text.c_str(), text.size()))
			return encode;
	}
	return NULL;
}

const CC4Encode* CC4Context::getMostPossibleEncode(const char* text) const
{
	if (NULL == text)
		return NULL;
	for (unsigned int i = 0; i < m_constEncodes.size(); ++i)
	{
		const CC4Encode *encode = m_constEncodes[i];
		if (!encode->isAutoCheck())
			continue;
		if (encode->match(text, strlen(text)))
			return encode;
	}
	for (unsigned int i = 0; i< m_encodes.size(); ++i)
	{
		CC4Encode *encode = m_encodes[i];
		if (!encode->isAutoCheck())
			continue;
		if (encode->match(text, strlen(text)))
			return encode;
	}
	return NULL;
}

std::list<const CC4Encode*> CC4Context::getEncodesList() const
{
	std::list<const CC4Encode*> encodes_list;
	for (unsigned int i = 0; i < m_constEncodes.size(); ++i)
	{
		const CC4Encode *encode = m_constEncodes[i];
		encodes_list.push_back(encode);
	}
	for (unsigned int i=0; i< m_encodes.size(); ++i)
	{
		const CC4Encode *encode = m_encodes[i];
		encodes_list.push_back(encode);
	}
	return encodes_list;
}

std::list<std::wstring> CC4Context::getEncodesNameList() const
{
	std::list<std::wstring> name_list;
	for (unsigned int i = 0; i < m_constEncodes.size(); ++i)
	{
		const CC4Encode *encode = m_constEncodes[i];
		name_list.push_back(encode->getName());
	}
	for (unsigned int i=0; i< m_encodes.size(); ++i)
	{
		const CC4Encode *encode = m_encodes[i];
		name_list.push_back(encode->getName());
	}
	return name_list;
}