﻿#include "stdafx.h"
#include "UpdateHelper.h"
#include "SimpleXML.h"
#include "InternetCommon.h"


CUpdateHelper::CUpdateHelper()
{
}


CUpdateHelper::~CUpdateHelper()
{
}

void CUpdateHelper::SetUpdateSource(UpdateSource update_source)
{
    m_update_source = update_source;
}

bool CUpdateHelper::CheckForUpdate()
{
    wstring version_info;
    //使用GitHub更新源
    if (m_update_source == UpdateSource::GitHubSource)
    {
        if (CInternetCommon::GetURL(L"https://raw.githubusercontent.com/zhongyang219/MusicPlayer2/master/version.info", version_info))		//获取版本信息
        {
            m_row_data = true;
        }
        else if (CInternetCommon::GetURL(L"https://github.com/zhongyang219/MusicPlayer2/blob/master/version.info", version_info))		//获取版本信息
        {
            m_row_data = false;
        }
        else
        {
            return false;
        }

        if (!m_row_data)
        {
            size_t index = version_info.find(L"﻿&lt;version&gt;");
            if (index != std::wstring::npos)
                version_info = version_info.substr(index);

            CString str_version_info = version_info.c_str();
            str_version_info.Replace(L"&lt;", L"<");
            str_version_info.Replace(L"&gt;", L">");

            version_info = str_version_info;
        }
    }
    //使用Gitee更新源
    else
    {
        if (!CInternetCommon::GetURL(L"https://gitee.com/zhongyang219/MusicPlayer2/raw/master/version.info", version_info))     //获取版本信息
            return false;
    }

    ParseUpdateInfo(version_info);

    return true;
}

void CUpdateHelper::ParseUpdateInfo(wstring version_info)
{
    CSimpleXML version_xml;
    version_xml.LoadXMLContentDirect(version_info);

    m_version = version_xml.GetNode(L"version");
    wstring str_source_tag = (m_update_source == UpdateSource::GitHubSource ? L"GitHub" : L"Gitee");
    m_link64 = version_xml.GetNode(L"link_x64", str_source_tag.c_str());
    m_link = version_xml.GetNode(L"link", str_source_tag.c_str());
    CString contents_zh_cn = version_xml.GetNode(L"contents_zh_cn", L"update_contents").c_str();
    CString contents_en = version_xml.GetNode(L"contents_en", L"update_contents").c_str();
    contents_zh_cn.Replace(L"\\n", L"\r\n");
    contents_en.Replace(L"\\n", L"\r\n");
    m_contents_zh_cn = contents_zh_cn;
    m_contents_en = contents_en;
}

const std::wstring& CUpdateHelper::GetVersion() const
{
    return m_version;
}

const std::wstring& CUpdateHelper::GetLink() const
{
    return m_link;
}

const std::wstring& CUpdateHelper::GetLink64() const
{
    return m_link64;
}

const std::wstring& CUpdateHelper::GetContentsEn() const
{
    return m_contents_en;
}

const std::wstring& CUpdateHelper::GetContentsZhCn() const
{
    return m_contents_zh_cn;
}

bool CUpdateHelper::IsRowData()
{
    return m_row_data;
}
