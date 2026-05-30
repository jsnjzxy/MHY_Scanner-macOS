#include "HttpClient.h"

#include <sstream>
#include <cstdlib>
#include <iostream>
#include <cpr/api.h>

#ifdef ENABLE_PROXY
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#endif

HttpClient::HttpClient()
{
    // 配置默认选项
    session.SetVerifySsl(false);  // 对应原来的 SSL_VERIFYPEER=false
    session.SetRedirect(cpr::Redirect(true));  // 对应 FOLLOWLOCATION=true
    session.SetConnectTimeout(cpr::ConnectTimeout(std::chrono::milliseconds(10000)));
    session.SetTimeout(cpr::Timeout(std::chrono::milliseconds(10000)));

#ifdef ENABLE_PROXY
    // 开发模式：自动检测本地代理（Charles/Surge/Clash 等）
    auto checkLocalProxy = [](int port) -> bool {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        // 非阻塞连接测试
        fcntl(sock, F_SETFL, O_NONBLOCK);
        connect(sock, (struct sockaddr*)&addr, sizeof(addr));

        fd_set fdset;
        struct timeval tv;
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms 超时

        bool available = select(sock + 1, nullptr, &fdset, nullptr, &tv) > 0;
        close(sock);
        return available;
    };

    // 检测常用代理端口: Charles(8888), Surge(6152), Clash(7890), Whistle(8899)
    std::string proxyUrl;
    for (int port : {8888, 6152, 7890, 8899}) {
        if (checkLocalProxy(port)) {
            proxyUrl = "http://127.0.0.1:" + std::to_string(port);
            break;
        }
    }

    if (!proxyUrl.empty()) {
        session.SetProxies(cpr::Proxies{{"http", proxyUrl}, {"https", proxyUrl}});
        // 同时设置环境变量，确保系统 libcurl 也使用代理
        setenv("http_proxy", proxyUrl.c_str(), 1);
        setenv("https_proxy", proxyUrl.c_str(), 1);
        setenv("HTTP_PROXY", proxyUrl.c_str(), 1);
        setenv("HTTPS_PROXY", proxyUrl.c_str(), 1);
        // 保存代理 URL 供请求使用
        this->proxyUrl = proxyUrl;
        // 输出调试信息
        std::cout << "[HttpClient] Detected proxy: " << proxyUrl << std::endl;
    } else {
        std::cout << "[HttpClient] No local proxy detected" << std::endl;
    }
#endif
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

#ifdef ENABLE_PROXY
        if (!proxyUrl.empty()) {
            cpr::Proxies proxies{
                {"http", proxyUrl},
                {"https", proxyUrl}
            };
            cpr::Response r = cpr::Get(
                cpr::Url{url},
                cprHeaders,
                cpr::AcceptEncoding{cpr::AcceptEncodingMethods::gzip},
                proxies
            );
            response = r.text;
            return r.status_code >= 200 && r.status_code < 300;
        }
#endif

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

#ifdef ENABLE_PROXY
        if (!proxyUrl.empty()) {
            cpr::Proxies proxies{
                {"http", proxyUrl},
                {"https", proxyUrl}
            };
            cpr::Response r = cpr::Post(
                cpr::Url{url},
                cprHeaders,
                cpr::Body{postParams},
                proxies
            );

            if (header)
            {
                std::ostringstream oss;
                for (const auto& [key, value] : r.header)
                {
                    oss << key << ": " << value << "\r\n";
                }
                oss << "\r\n" << r.text;  // 添加空行和 body
                response = oss.str();
            }
            else
            {
                response = r.text;
            }

            return r.status_code >= 200 && r.status_code < 300;
        }
#endif

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
