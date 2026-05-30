#pragma once

#include <string>
#include <map>
#include <cpr/cpr.h>

class HttpClient
{
public:
    HttpClient();
    ~HttpClient() = default;

    bool GetRequest(std::string& response, const char* url, std::map<std::string, std::string> headers = {});
    bool PostRequest(std::string& response, const char* url, const std::string& postParams,
                     std::map<std::string, std::string> headers = {}, bool header = false);
    // 新方法：返回响应头中的 Cookie
    bool PostRequestWithCookies(std::string& response, std::map<std::string, std::string>& cookies,
                                 const char* url, const std::string& postParams,
                                 std::map<std::string, std::string> headers = {});
    static std::string MapToQueryString(const std::map<std::string, std::string>& params);
    std::map<std::string, std::string> QueryStringToMap(const std::string& str);

private:
    cpr::Session session;
#ifdef ENABLE_PROXY
    std::string proxyUrl;
#endif
};
