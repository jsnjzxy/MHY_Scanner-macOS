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
    static std::string MapToQueryString(const std::map<std::string, std::string>& params);
    std::map<std::string, std::string> QueryStringToMap(const std::string& str);

private:
    cpr::Session session;
};
