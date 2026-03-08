#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <codecvt>
#include <locale>

inline std::string urlEncode(const std::string& str)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (auto itr = str.cbegin(), end = str.cend(); itr != end; ++itr)
    {
        const unsigned char c = *itr;

        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '!' || c == '*' ||
            c == '\'' || c == '(' || c == ')' || c == ';' || c == ':' || c == '@' || c == '&' ||
            c == '=' || c == '$' || c == ',' || c == '/' || c == '?' || c == '#' || c == '[' || c == ']')
        {
            escaped << c;
            continue;
        }

        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }
    return escaped.str();
}

inline std::string urlDecode(const std::string& str)
{
    std::string str_decode;
    int i;
    const char* cd = str.c_str();
    char p[2];
    for (i = 0; i < static_cast<int>(strlen(cd)); i++)
    {
        memset(p, '\0', 2);
        if (cd[i] != '%')
        {
            str_decode += cd[i];
            continue;
        }
        p[0] = cd[++i];
        p[1] = cd[++i];
        p[0] = p[0] - 48 - ((p[0] >= 'A') ? 7 : 0) - ((p[0] >= 'a') ? 32 : 0);
        p[1] = p[1] - 48 - ((p[1] >= 'A') ? 7 : 0) - ((p[1] >= 'a') ? 32 : 0);
        str_decode += static_cast<unsigned char>(p[0] * 16 + p[1]);
    }
    return str_decode;
}

inline std::string replaceQuotes(const std::string& str)
{
    std::string newStr;
    for (int i = 0; i < static_cast<int>(str.length()); i++)
    {
        if (str[i] == '\"')
        {
            newStr += '\\';
        }
        newStr += str[i];
    }
    return newStr;
}

inline std::string string_To_UTF8(const std::string& str)
{
    // 使用标准 C++ codecvt 进行编码转换
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wideStr = converter.from_bytes(str);
        return converter.to_bytes(wideStr);
    } catch (const std::exception&) {
        return str;
    }
}

inline std::string UTF8_To_string(const std::string& str)
{
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wideStr = converter.from_bytes(str);
        return converter.to_bytes(wideStr);
    } catch (const std::exception&) {
        return str;
    }
}

inline std::string escapeString(const std::string& input)
{
    std::string escaped;
    for (char c : input)
    {
        if (c == '\\')
        {
            escaped += "\\\\";
        }
        else if (c == '\"')
        {
            escaped += "\\\"";
        }
        else if (c == '\n')
        {
            escaped += "\\n";
        }
        else if (c == '\t')
        {
            escaped += "\\t";
        }
        else if (c == '\r')
        {
            escaped += "\\r";
        }
        else
        {
            escaped += c;
        }
    }
    return escaped;
}

inline std::string unescapeString(const std::string& input)
{
    std::string unescaped;
    bool isEscape = false;
    for (char c : input)
    {
        if (isEscape)
        {
            if (c == 'n')
            {
                unescaped += '\n';
            }
            else if (c == 't')
            {
                unescaped += '\t';
            }
            else if (c == 'r')
            {
                unescaped += '\r';
            }
            else
            {
                unescaped += c;
            }
            isEscape = false;
        }
        else
        {
            if (c == '\\')
            {
                isEscape = true;
            }
            else
            {
                unescaped += c;
            }
        }
    }
    return unescaped;
}

inline void replace0026WithAmpersand(std::string& str)
{
    std::string searchString = "0026";
    std::string replacement = "&";
    size_t pos = 0;

    while ((pos = str.find(searchString, pos)) != std::string::npos)
    {
        str.replace(pos, searchString.length(), replacement);
        pos += replacement.length();
    }
}

inline std::wstring stringTowstring(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

inline std::string wstringTostring(const std::wstring& wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}
