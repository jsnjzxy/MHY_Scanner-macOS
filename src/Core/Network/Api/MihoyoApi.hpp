#pragma once

#include <string>
#include <string_view>
#include <format>
#include <random>
#include <sstream>
#include <optional>
#include <iostream>

#include <nlohmann/json.hpp>

#include "Common/Types.h"
#include "Common/Constants.h"
#include "../HttpClient.h"
#include "Utils/StringUtil.hpp"
#include "Utils/CryptoUtils.h"
#include "Utils/TimeUtil.hpp"
#include "Utils/UUIDUtil.hpp"

enum class QRCodeState : uint8_t
{
    Init = 0,
    Scanned = 1,
    Confirmed = 2,
    Expired = 3,
    Cancelled = 4
};

constinit const std::string_view mihoyobbs_salt{ "oqrJbPCoFhWhFBNDvVRuldbrbiVxyWsP" };
constinit const std::string_view mihoyobbs_salt_web{ "zZDfHqEcwTqvvKDmqRcHyqqurxGgLfBV" };

constinit const std::string_view mihoyobbs_salt_x4{ "xV8v4Qu54lUKrEYFZkJhB8cuOh9Asafs" };
constinit const std::string_view mihoyobbs_salt_x6{ "t0qEgfub6cvueAPgR5m9aQWWVciEer7v" };

constinit const std::string_view mihoyobbs_version{ "2.76.1" };

static const std::string device_id{ CreateUUID::CreateUUID4() };

[[nodiscard]] inline std::string DataSignAlgorithmVersionGen1()
{
    return "";
}

[[nodiscard]] inline std::string DataSignAlgorithmVersionGen2(const std::string_view body, const std::string_view query)
{
    const std::string time_now{ std::to_string(GetUnixTimeStampSeconds()) };
    std::random_device rd{};
    std::mt19937 gen{ rd() };
    int lower_bound{ 100001 };
    int upper_bound{ 200000 };
    std::uniform_int_distribution<int> dist(lower_bound, upper_bound);
    const std::string rand{ std::to_string(dist(gen)) };
    std::string m{ "salt=" + std::string(mihoyobbs_salt_x6) + "&t=" + time_now + "&r=" + rand + "&b=" + std::string(body) + "&q=" + std::string(query) };
    return time_now + "," + rand + "," + Md5(m);
}

inline std::map<std::string, std::string> GetRequestHeader()
{
    static const std::map<std::string, std::string> header{
        { "User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) miHoYoBBS/2.76.1" },
        { "Accept", "application/json" },
        { "x-rpc-app_id", "bll8iq97cem8" },
        { "x-rpc-app_version", "2.76.1" },
        { "x-rpc-client_type", "2" },
        { "x-rpc-device_id", device_id },
        { "x-rpc-device_name", "" },
        { "x-rpc-game_biz", "bbs_cn" },
        { "x-rpc-sdk_version", "2.16.0" }
    };
    return header;
}

static GameType loginType{ GameType::TearsOfThemis };

// ============================================
// 米游社 Passport 登录 API
// ============================================

// 创建二维码登录
inline auto CreateQrcodeLogin()
{
    struct
    {
        int retcode{};
        std::string url{};
        std::string ticket{};
    } result;

    HttpClient h;
    std::string res;
    auto headers = GetRequestHeader();
    h.PostRequest(res, MihoyoUrls::PassportQrcodeCreate, "{}", headers);

    if (res.empty())
    {
        result.retcode = -9999;
        return result;
    }

    try
    {
        nlohmann::json j = nlohmann::json::parse(res);
        result.retcode = j.value("retcode", -9998);
        if (result.retcode == 0 && j.contains("data"))
        {
            result.url = j["data"].value("url", "");
            result.ticket = j["data"].value("ticket", "");
            replace0026WithAmpersand(result.url);
        }
    }
    catch (...)
    {
        result.retcode = -9997;
    }
    return result;
}

// 查询二维码扫码状态
inline auto QueryQrcodeStatus(const std::string_view ticket)
{
    struct
    {
        int retcode{};
        enum { Created, Scanned, Confirmed, Expired, Cancelled } status{};
        std::string aid{};
        std::string mid{};
        std::string stoken{};
    } result;

    HttpClient h;
    std::string res;
    auto headers = GetRequestHeader();
    h.PostRequest(res, MihoyoUrls::PassportQrcodeQuery,
                  "{\"ticket\":\"" + std::string(ticket) + "\"}", headers);

    if (res.empty())
    {
        result.retcode = -9999;
        return result;
    }

    try
    {
        nlohmann::json j = nlohmann::json::parse(res);
        result.retcode = j.value("retcode", -9998);

        // 错误码处理
        if (result.retcode == -3501)
        {
            result.status = result.Expired;
            return result;
        }
        if (result.retcode == -3505)
        {
            result.status = result.Cancelled;
            return result;
        }

        if (j.contains("data") && j["data"].is_object())
        {
            std::string statusStr = j["data"].value("status", "");
            if (statusStr == "Created")
            {
                result.status = result.Created;
            }
            else if (statusStr == "Scanned")
            {
                result.status = result.Scanned;
            }
            else if (statusStr == "Confirmed")
            {
                result.status = result.Confirmed;

                // 从 user_info 中获取用户信息
                if (j["data"].contains("user_info") && j["data"]["user_info"].is_object())
                {
                    result.aid = j["data"]["user_info"].value("aid", "");
                    result.mid = j["data"]["user_info"].value("mid", "");
                }

                // 获取 stoken - 在 tokens 数组中
                if (j["data"].contains("tokens") && j["data"]["tokens"].is_array() && !j["data"]["tokens"].empty())
                {
                    result.stoken = j["data"]["tokens"][0].value("token", "");
                }
            }
        }
    }
    catch (...)
    {
        result.retcode = -9997;
    }
    return result;
}

// 扫描游戏二维码
inline bool ScanGameQrcode(const std::string_view ticket, const std::string_view stoken, const std::string_view mid)
{
    HttpClient client;
    auto headers = GetRequestHeader();
    headers["Content-Type"] = "application/json";
    headers["Cookie"] = "stoken=" + std::string(stoken) + "; mid=" + std::string(mid);

    std::string requestBody = "{\"token_types\":[\"1\"],\"ticket\":\"" + std::string(ticket) + "\"}";
    std::string response;

    std::cout << "[ScanGameQrcode] Requesting scan..." << std::endl;
    std::cout << "[ScanGameQrcode] ticket: " << ticket << std::endl;

    bool success = client.PostRequest(response, MihoyoUrls::PassportQrcodeScan, requestBody, headers);

    std::cout << "[ScanGameQrcode] Response: " << (response.empty() ? "(empty)" : response.substr(0, 200)) << std::endl;

    if (!success || response.empty())
    {
        std::cerr << "[ScanGameQrcode] Request failed or empty response" << std::endl;
        return false;
    }

    try
    {
        nlohmann::json j = nlohmann::json::parse(response);
        int retcode = j.value("retcode", -1);
        std::cout << "[ScanGameQrcode] retcode: " << retcode << std::endl;
        return retcode == 0;
    }
    catch (...)
    {
        std::cerr << "[ScanGameQrcode] JSON parse failed" << std::endl;
        return false;
    }
}

// 确认游戏二维码登录
inline bool ConfirmGameQrcode(const std::string_view ticket, const std::string_view stoken, const std::string_view mid)
{
    HttpClient client;
    auto headers = GetRequestHeader();
    headers["Content-Type"] = "application/json";
    headers["Cookie"] = "stoken=" + std::string(stoken) + "; mid=" + std::string(mid);

    std::string requestBody = "{\"ticket\":\"" + std::string(ticket) + "\",\"token_types\":[\"1\"]}";
    std::string response;

    bool success = client.PostRequest(response, MihoyoUrls::PassportQrcodeConfirm, requestBody, headers);

    if (!success || response.empty())
    {
        return false;
    }

    try
    {
        nlohmann::json j = nlohmann::json::parse(response);
        return j["retcode"] == 0;
    }
    catch (...)
    {
        return false;
    }
}

// ============================================
// 用户信息 API
// ============================================

inline std::string getMysUserName(const std::string_view uid)
{
    std::string re;
    HttpClient h;
    std::string url = std::string(MihoyoUrls::MysUserinfo) + "?uid=" + std::string(uid);
    h.GetRequest(re, url.c_str());

    try
    {
        nlohmann::json j = nlohmann::json::parse(re);
        if (j.value("retcode", -1) == 0 && j.contains("data"))
        {
            return j["data"]["user_info"].value("nickname", "未知用户");
        }
    }
    catch (...) {}
    return "未知用户";
}

// 获取 Cookie 账号信息
inline auto GetCookieAccountInfo(const std::string_view stoken, const std::string_view mid)
{
    struct
    {
        int retcode{};
        std::string uid{};
        std::string cookie_token{};
    } result;

    HttpClient client;
    std::map<std::string, std::string> headers{
        { "Cookie", "stoken=" + std::string(stoken) + "; mid=" + std::string(mid) },
        { "x-rpc-app_id", "bll8iq97cem8" }
    };

    std::string response;
    client.GetRequest(response, MihoyoUrls::GetCookieAccountInfo, headers);

    std::cout << "[GetCookieAccountInfo] Response: " << (response.empty() ? "(empty)" : response) << std::endl;

    if (response.empty())
    {
        result.retcode = -9999;
        return result;
    }

    try
    {
        nlohmann::json j = nlohmann::json::parse(response);
        result.retcode = j.value("retcode", -9998);
        if (result.retcode == 0 && j.contains("data"))
        {
            result.uid = j["data"].value("uid", "");
            result.cookie_token = j["data"].value("cookie_token", "");
        }
    }
    catch (...)
    {
        result.retcode = -9997;
    }
    return result;
}

// ============================================
// Token 转换 API
// ============================================

inline auto getStokenByGameToken(const std::string_view uid, const std::string_view game_token)
    -> std::optional<std::tuple<std::string, std::string>>
{
    static std::map<std::string, std::string> headers = {
        { "x-rpc-app_id", "bll8iq97cem8" },
        { "Referer", "https://app.mihoyo.com" },
        { "User-Agent", "Mozilla/5.0 (Linux; Android 12; LIO-AN00 Build/TKQ1.220829.002; wv) AppleWebKit/537.36 (KHTML, like Gecko) "
                        "Version/4.0 Chrome/103.0.5060.129 Mobile Safari/537.36 miHoYoBBS/2.76.1" }
    };
    std::string re;
    HttpClient h;
    h.PostRequest(re, MihoyoUrls::TakumiGameTokenStoken,
                  "{\"account_id\":" + std::string(uid) + ",\"game_token\":\"" + std::string(game_token) + "\"}",
                  headers);
    re = UTF8_To_string(re);
    nlohmann::json j;
    j = nlohmann::json::parse(re);
    if (static_cast<int>(j["retcode"]) == 0)
    {
        return std::make_optional(std::make_tuple(std::move(static_cast<std::string>(j["data"]["user_info"]["mid"])),
                                                  std::move(static_cast<std::string>(j["data"]["token"]["token"]))));
    }
    else
    {
        return std::nullopt;
    }
}

inline auto GetGameTokenByStoken(const std::string_view stoken, const std::string_view mid)
    -> std::optional<std::string>
{
    HttpClient client;
    std::map<std::string, std::string> headers{
        { "Cookie", "stoken=" + std::string(stoken) + "; mid=" + std::string(mid) },
        { "x-rpc-app_id", "bll8iq97cem8" },
        { "Content-Type", "application/json" },
        { "DS", DataSignAlgorithmVersionGen2("{}", "") }
    };
    std::string s;
    client.PostRequest(s, MihoyoUrls::TakumiGameToken, "{}", headers);

    std::cout << "[GetGameTokenByStoken] Response: " << (s.empty() ? "(empty)" : s) << std::endl;

    if (s.empty())
    {
        return std::nullopt;
    }

    const std::string& data = UTF8_To_string(s);
    try
    {
        nlohmann::json j = nlohmann::json::parse(data);
        int retcode = j.value("retcode", -1);
        std::cout << "[GetGameTokenByStoken] retcode: " << retcode << std::endl;
        if (retcode == 0 && j.contains("data") && j["data"].contains("game_token"))
        {
            return j["data"]["game_token"].get<std::string>();
        }
    }
    catch (...)
    {
        std::cerr << "[GetGameTokenByStoken] JSON parse failed" << std::endl;
    }
    return std::nullopt;
}

// ============================================
// 加密工具
// ============================================

inline std::string Encrypt(const std::string_view source)
{
    static constinit const char* PublicKey{
        "-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDDvekdPMHN3AYhm/vktJT+YJr7"
        "cI5DcsNKqdsx5DZX0gDuWFuIjzdwButrIYPNmRJ1G8ybDIF7oDW2eEpm5sMbL9zs"
        "9ExXCdvqrn51qELbqj0XxtMTIpaCHFSI50PfPpTFV9Xt/hmyVwokoOXFlAEgCn+Q"
        "CgGs52bFoYMtyi+xEQIDAQAB\n"
        "-----END PUBLIC KEY-----"
    };
    return rsaEncrypt(source.data(), PublicKey);
}

// ============================================
// 手机验证码登录 API
// ============================================

inline auto CreateLoginCaptcha(const std::string_view mobile, const std::string_view aigis = "")
{
    struct
    {
        int retcode{};
        std::string action_type{};
        std::string session_id{};
        int mmt_type{};
        std::string gt{};
        std::string challenge{};
        bool use_v4{false};
        std::string risk_type{};
    } GeetestData;

    const std::string RequestBody{ "{\"area_code\":\"" + Encrypt("+86") + "\",\"mobile\":\"" + Encrypt(mobile) + "\"}" };
    std::map<std::string, std::string> headers{ GetRequestHeader() };
    headers["DS"] = DataSignAlgorithmVersionGen2(RequestBody, "");
    if (!aigis.empty())
    {
        headers["X-Rpc-Aigis"] = aigis;
    }

    HttpClient h;
    std::string s;
    h.PostRequest(s, MihoyoUrls::PassportVerifier, RequestBody, headers, true);

    // 解析响应
    std::string bodystr{};
    size_t bodyStart = s.find("\r\n\r\n");
    if (bodyStart != std::string::npos)
    {
        bodystr = s.substr(bodyStart + 4);
    }
    else
    {
        bodyStart = s.find("\n\n");
        if (bodyStart != std::string::npos)
        {
            bodystr = s.substr(bodyStart + 2);
        }
        else if (size_t startPos = s.find_last_of("\n"); startPos != std::string::npos)
        {
            bodystr = s.substr(startPos + 1);
        }
        else
        {
            bodystr = s;
        }
    }

    if (bodystr.empty())
    {
        GeetestData.retcode = -9999;
        return GeetestData;
    }

    try
    {
        nlohmann::json body = nlohmann::json::parse(bodystr);
        GeetestData.retcode = body.value("retcode", -9997);

        if (GeetestData.retcode == 0)
        {
            GeetestData.action_type = body["data"].value("action_type", "");
        }
        else if (GeetestData.retcode == -3101)
        {
            // 需要极验验证
            constexpr std::string_view headerKey{ "X-Rpc-Aigis: " };
            std::string Aigis;
            if (size_t startPos = s.find(headerKey); startPos != std::string::npos)
            {
                startPos += headerKey.length();
                size_t endPos = s.find("\n", startPos);
                Aigis = s.substr(startPos, endPos - startPos);
            }

            if (!Aigis.empty())
            {
                try
                {
                    nlohmann::json j1 = nlohmann::json::parse(Aigis);
                    if (j1.contains("data") && j1.contains("session_id"))
                    {
                        std::string data = j1["data"].get<std::string>();
                        data = unescapeString(data);
                        nlohmann::json j2 = nlohmann::json::parse(data);

                        GeetestData.session_id = j1["session_id"].get<std::string>();
                        GeetestData.mmt_type = j1.value("mmt_type", 0);
                        GeetestData.use_v4 = j2.value("use_v4", false);

                        if (j2.contains("gt"))
                        {
                            GeetestData.gt = j2["gt"].get<std::string>();
                        }
                        if (j2.contains("challenge"))
                        {
                            GeetestData.challenge = j2["challenge"].get<std::string>();
                        }
                        if (j2.contains("risk_type"))
                        {
                            GeetestData.risk_type = j2["risk_type"].get<std::string>();
                        }
                    }
                }
                catch (...) {}
            }
        }
    }
    catch (...)
    {
        GeetestData.retcode = -9998;
    }

    return GeetestData;
}

inline auto LoginByMobileCaptcha(const std::string_view actionType, const std::string_view mobile, const std::string_view captcha, const std::string_view aigis = "")
{
    struct
    {
        int retcode{};
        struct
        {
            std::string V2Token{};
            std::string aid{};
            std::string mid{};
        } data;
    } result;

    const std::string RequestBody{ "{\"area_code\":\"" + Encrypt("+86") + "\",\"action_type\":\"" + std::string(actionType) + "\",\"captcha\":\"" + std::string(captcha) + "\",\"mobile\":\"" + Encrypt(mobile) + "\"}" };
    std::map<std::string, std::string> headers{ GetRequestHeader() };
    headers["DS"] = DataSignAlgorithmVersionGen2(RequestBody, "");
    if (!aigis.empty())
    {
        headers["X-Rpc-Aigis"] = aigis;
    }

    HttpClient h;
    std::string s;
    h.PostRequest(s, MihoyoUrls::LoginByMobileCaptcha, RequestBody, headers);

    nlohmann::json j = nlohmann::json::parse(s);
    result.retcode = j["retcode"];
    if (result.retcode == -3205)
    {
        return result;
    }
    else if (result.retcode == 0)
    {
        result.data.V2Token = j["data"]["token"]["token"].get<std::string>();
        result.data.aid = j["data"]["user_info"]["aid"].get<std::string>();
        result.data.mid = j["data"]["user_info"]["mid"].get<std::string>();
    }
    return result;
}

// ============================================
// 游戏扫码 API（游戏特定 API）
// ============================================

// 扫描游戏二维码
inline std::string ScanQRLogin(const std::string_view url, const std::string_view ticket, GameType gameType)
{
    std::string response;
    HttpClient client;
    std::string requestBody = "{\"app_id\":\"" + std::to_string(static_cast<int>(gameType)) +
        "\",\"device\":\"" + device_id + "\"" +
        ",\"passport_app_id\":\"bll8iq97cem8\"" +
        ",\"ticket\":\"" + std::string(ticket) + "\"}";

    auto headers = GetRequestHeader();
    headers["Content-Type"] = "application/json";
    client.PostRequest(response, url.data(), requestBody, headers);

    if (response.empty())
    {
        return "";
    }

    try
    {
        nlohmann::json j = nlohmann::json::parse(response);
        if (j.value("retcode", -1) == 0 && j.contains("data") && j["data"].contains("passport_qr_url"))
        {
            // 从 passport_qr_url 中提取 tk 参数
            std::string passportUrl = j["data"]["passport_qr_url"].get<std::string>();
            size_t tkPos = passportUrl.find("tk=");
            if (tkPos != std::string::npos)
            {
                size_t start = tkPos + 3;
                size_t end = passportUrl.find("&", start);
                if (end == std::string::npos)
                {
                    end = passportUrl.find("#", start);
                }
                if (end == std::string::npos)
                {
                    end = passportUrl.length();
                }
                return passportUrl.substr(start, end - start);
            }
        }
    }
    catch (...)
    {
        // JSON parse failed
    }
    return "";
}

// 确认游戏二维码登录
inline bool ConfirmQRLogin(const std::string_view url, const std::string_view uid, const std::string_view cookieToken, const std::string_view ticket, GameType gameType)
{
    std::string s;
    std::string requestBody = "{\"app_id\":" + std::to_string(static_cast<int>(gameType)) +
        ",\"device\":\"" + device_id + "\"" +
        ",\"payload\":{\"proto\":\"Account\",\"raw\":\"{\\\"uid\\\":\\\"" + std::string(uid) +
        "\\\",\\\"token\\\":\\\"" + std::string(cookieToken) + "\\\"}\"}" +
        ",\"ticket\":\"" + std::string(ticket) + "\"}";

    HttpClient client;
    auto headers = GetRequestHeader();
    headers["Content-Type"] = "application/json";
    client.PostRequest(s, url.data(), requestBody, headers);

    try
    {
        nlohmann::json j = nlohmann::json::parse(s);
        return j.value("retcode", -1) == 0;
    }
    catch (...)
    {
        return false;
    }
}
