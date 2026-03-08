#include "HttpClient.h"

#include <sstream>
#include <cpr/api.h>

HttpClient::HttpClient()
{
    // 配置默认选项
    session.SetVerifySsl(false);  // 对应原来的 SSL_VERIFYPEER=false
    session.SetRedirect(cpr::Redirect(true));  // 对应 FOLLOWLOCATION=true
    session.SetConnectTimeout(cpr::ConnectTimeout(std::chrono::milliseconds(10000)));
    session.SetTimeout(cpr::Timeout(std::chrono::milliseconds(10000)));
}

std::string HttpClient::MapToQueryString(const std::map<std::string, std::string>& params)
{
    std::ostringstream paramsTemp;
    bool first = true;
    for (const auto& kv : params)
    {
        if (!first)
        {
            paramsTemp << "&";
        }
        first = false;
        paramsTemp << kv.first << "=" << kv.second;
    }
    return paramsTemp.str();
}

std::map<std::string, std::string> HttpClient::QueryStringToMap(const std::string& queryString)
{
    std::map<std::string, std::string> params;

    size_t startPos = 0;
    size_t endPos;

    while (startPos < queryString.length())
    {
        endPos = queryString.find('&', startPos);
        if (endPos == std::string::npos)
        {
            endPos = queryString.length();
        }

        std::string param = queryString.substr(startPos, endPos - startPos);

        size_t equalPos = param.find('=');
        if (equalPos != std::string::npos)
        {
            std::string key = param.substr(0, equalPos);
            std::string value = param.substr(equalPos + 1);
            params[key] = value;
        }

        startPos = endPos + 1;
    }

    return params;
}

bool HttpClient::GetRequest(std::string& response, const char* url, std::map<std::string, std::string> headers)
{
    try
    {
        cpr::Header cprHeaders;
        for (const auto& kv : headers)
        {
            cprHeaders.insert({kv.first, kv.second});
        }

        cpr::Response r = cpr::Get(
            cpr::Url{url},
            cprHeaders,
            cpr::AcceptEncoding{cpr::AcceptEncodingMethods::gzip}
        );

        response = r.text;
        return r.status_code >= 200 && r.status_code < 300;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool HttpClient::PostRequest(std::string& response, const char* url, const std::string& postParams,
                              std::map<std::string, std::string> headers, bool header)
{
    try
    {
        cpr::Header cprHeaders;
        for (const auto& kv : headers)
        {
            cprHeaders.insert({kv.first, kv.second});
        }

        cpr::Response r = cpr::Post(
            cpr::Url{url},
            cprHeaders,
            cpr::Body{postParams}
        );

        if (header)
        {
            // 如果需要返回 header，手动构建
            std::ostringstream oss;
            for (const auto& [key, value] : r.header)
            {
                oss << key << ": " << value << "\r\n";
            }
            response = oss.str();
        }
        else
        {
            response = r.text;
        }

        return r.status_code >= 200 && r.status_code < 300;
    }
    catch (const std::exception&)
    {
        return false;
    }
}
