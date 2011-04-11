
#ifndef __view_framework_clipboard_util_win_h__
#define __view_framework_clipboard_util_win_h__

#pragma once

#include <shlobj.h>

#include <string>
#include <vector>

class ClipboardUtil
{
public:
    /////////////////////////////////////////////////////////////////////////////
    // 剪贴板格式.
    static FORMATETC* GetUrlFormat();
    static FORMATETC* GetUrlWFormat();
    static FORMATETC* GetMozUrlFormat();
    static FORMATETC* GetPlainTextFormat();
    static FORMATETC* GetPlainTextWFormat();
    static FORMATETC* GetFilenameFormat();
    static FORMATETC* GetFilenameWFormat();
    // MS HTML格式.
    static FORMATETC* GetHtmlFormat();
    // Firefox text/html
    static FORMATETC* GetTextHtmlFormat();
    static FORMATETC* GetCFHDropFormat();
    static FORMATETC* GetFileDescriptorFormat();
    static FORMATETC* GetFileContentFormatZero();
    static FORMATETC* GetWebKitSmartPasteFormat();

    /////////////////////////////////////////////////////////////////////////////
    // 检查|data_object|是否有请求的类型. 如果有则返回true.
    static bool HasUrl(IDataObject* data_object);
    static bool HasFilenames(IDataObject* data_object);
    static bool HasPlainText(IDataObject* data_object);
    static bool HasFileContents(IDataObject* data_object);
    static bool HasHtml(IDataObject* data_object);

    /////////////////////////////////////////////////////////////////////////////
    // 提取IDataObject信息的辅助函数. 如果在|data_object|中找到请求的数据则返回
    // true.
    static bool GetUrl(IDataObject* data_object,
        std::wstring* url, std::wstring* title, bool convert_filenames);
    static bool GetFilenames(IDataObject* data_object,
        std::vector<std::wstring>* filenames);
    static bool GetPlainText(IDataObject* data_object, std::wstring* plain_text);
    static bool GetHtml(IDataObject* data_object, std::wstring* text_html,
        std::string* base_url);
    static bool GetFileContents(IDataObject* data_object,
        std::wstring* filename, std::string* file_contents);

    // MS CF_HTML格式和普通text/html之间转换的辅助方法.
    static std::string HtmlToCFHtml(const std::string& html,
        const std::string& base_url);
    static void CFHtmlToHtml(const std::string& cf_html, std::string* html,
        std::string* base_url);
};

#endif //__view_framework_clipboard_util_win_h__