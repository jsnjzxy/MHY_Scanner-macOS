#pragma once

#include <string>
#include <string_view>
#include <iostream>
#include <map>

#include <nlohmann/json.hpp>

#include "../HttpClient.h"
#include "Utils/TimeUtil.hpp"
#include "Utils/StringUtil.hpp"
#include "Utils/CryptoUtils.h"
#include "Common/Constants.h"

using json = nlohmann::json;

namespace BH3API
{
namespace BILI
{
namespace detail
{
constinit const std::string_view userinfoParam{ R"({"cur_buvid":"XZA2FA4AC240F665E2F27F603ABF98C615C29","client_timestamp":"1667057013442","sdk_type":"1","isRoot":"0","merchant_id":"590","dp":"1280 * 720",
"mac":"08:00 : 27 : 53 : DD : 12","uid":"437470182","support_abis":"x86, armeabi - v7a, armeabi","apk_sign":"4502a02a00395dec05a4134ad593224d","platform_type":"3","old_buvid":"XZA2FA4AC240F665E2F27F603ABF98C615C29",
"operators":"5","fingerprint":"","model":"MuMu","udid":"XXA31CBAB6CBA63E432E087B58411A213BFB7","net":"5","app_id":"180","brand":"Android","oaid":"","game_id":"180","timestamp":"1667057013275","ver":"6.1.0","c":"1",
"version_code":"510","server_id":"378","version":"1","domain_switch_count":"0","pf_ver":"12","access_key":"","domain":"line1 - sdk - center - login - sh.biligame.net","original_domain":"","imei":"","sdk_log_type":"1",
"sdk_ver":"3.4.2","android_id":"84567e2dda72d1d4","channel_id":1})" };

constinit const std::string_view rsaParam{ R"({"operators":"5", "merchant_id":"590","isRoot":"0","domain_switch_count":"0","sdk_type": "1","sdk_log_type":"1","timestamp":"1613035485639","support_abis":"x86,armeabi-v7a,armeabi",
"access_key":"","sdk_ver":"3.4.2","oaid":"","dp":"1280*720","original_domain":"","imei":"","version":"1","udid":"KREhESMUIhUjFnJKNko2TDQFYlZkB3cdeQ==","apk_sign":"4502a02a00395dec05a4134ad593224d","platform_type":"3",
"old_buvid":"XZA2FA4AC240F665E2F27F603ABF98C615C29","android_id":"84567e2dda72d1d4","fingerprint":"","mac":"08:00:27:53:DD:12","server_id":"378","domain":"line1-sdk-center-login-sh.biligame.net","app_id":"180",
"version_code":"510","net":"4","pf_ver":"12","cur_buvid":"XZA2FA4AC240F665E2F27F603ABF98C615C29","c":"1","brand":"Android","client_timestamp":"1613035486888","channel_id":"1","uid":"","game_id":"180","ver":"6.1.0",
"model":"MuMu"})" };

constinit const std::string_view loginParam{ R"({"operators":"5","merchant_id":"590","isRoot":"0","domain_switch_count":"0","sdk_type":"1","sdk_log_type":"1","timestamp":"1613035508188","support_abis":"x86,armeabi-v7a,armeabi",
"access_key":"","sdk_ver":"3.4.2","oaid":"","dp":"1280*720","original_domain":"","imei":"227656364311444","gt_user_id":"fac83ce4326d47e1ac277a4d552bd2af","seccode":"","version":"1",
"udid":"KREhESMUIhUjFnJKNko2TDQFYlZkB3cdeQ==","apk_sign":"4502a02a00395dec05a4134ad593224d","platform_type":"3","old_buvid":"XZA2FA4AC240F665E2F27F603ABF98C615C29","android_id":"84567e2dda72d1d4","fingerprint":"",
"validate":"84ec07cff0d9c30acb9fe46b8745e8df","mac":"08:00:27:53:DD:12","server_id":"378","domain":"line1-sdk-center-login-sh.biligame.net","app_id":"180",
"pwd":"rxwA8J+GcVdqa3qlvXFppusRg4Ss83tH6HqxcciVsTdwxSpsoz2WuAFFGgQKWM1GtFovrLkpeMieEwOmQdzvDiLTtHeQNBOiqHDfJEKtLj7h1nvKZ1Op6vOgs6hxM6fPqFGQC2ncbAR5NNkESpSWeYTO4IT58ZIJcC0DdWQqh4=",
"version_code":"510","net":"4","pf_ver":"12","cur_buvid":"XZA2FA4AC240F665E2F27F603ABF98C615C29","c":"1","brand":"Android","client_timestamp":"1613035509437","channel_id":"1","uid":"",
"captcha_type":"1","game_id":"180","challenge":"efc825eaaef2405c954a91ad9faf29a2","user_id":"doo349","ver":"6.1.0","model":"MuMu"})" };

constinit const std::string_view captchaParam{ R"({"operators":"5","merchant_id":"590","isRoot":"0","domain_switch_count":"0","sdk_type":"1","sdk_log_type":"1","timestamp":"1613035486182","support_abis":"x86,armeabi-v7a,armeabi",
"access_key":"","sdk_ver":"3.4.2","oaid":"","dp":"1280*720","original_domain":"","imei":"227656364311444","version":"1","udid":"KREhESMUIhUjFnJKNko2TDQFYlZkB3cdeQ==","apk_sign":"4502a02a00395dec05a4134ad593224d",
"platform_type":"3","old_buvid":"XZA2FA4AC240F665E2F27F603ABF98C615C29","android_id":"84567e2dda72d1d4","fingerprint":"","mac":"08:00:27:53:DD:12","server_id":"378","domain":"line1-sdk-center-login-sh.biligame.net",
"app_id":"180","version_code":"510","net":"4","pf_ver":"12","cur_buvid":"XZA2FA4AC240F665E2F27F603ABF98C615C29","c":"1","brand":"Android","client_timestamp":"1613035487431","channel_id":"1","uid":"","game_id":"180",
"ver":"6.1.0","model":"MuMu"})" };

static const std::map<std::string, std::string> headers{
    { "User-Agent", "Mozilla/5.0 BSGameSDK" },
    { "Content-Type", "application/x-www-form-urlencoded" },
    { "Host", "line1-sdk-center-login-sh.biligame.net" }
};

inline std::string SetSign(std::map<std::string, std::string> data)
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        std::string key = it->first;
        std::string value = it->second;
        key.erase(std::remove(key.begin(), key.end(), '\"'), key.end());
        value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
        it->second = value;
    }

    data["timestamp"] = std::to_string(GetUnixTimeStampSeconds());
    data["client_timestamp"] = std::to_string(GetUnixTimeStampSeconds());
    std::string sign;
    std::string data2;
    for (std::pair<std::string, std::string> c : data)
    {
        if (c.first == "pwd")
        {
            std::string pwd = urlEncode(c.second);
            data2 += c.first + "=" + pwd + "&";
        }
        data2 += c.first + "=" + c.second + "&";
    }
    for (std::pair<std::string, std::string> c : data)
    {
        sign += c.second;
    }
    sign += "dbf8f1b4496f430b8a3c0f436a35b931";
    return data2 + "sign=" + Md5(sign);
}
}
inline auto GetUserInfo(const std::string& uid, const std::string& accessKey)
{
    nlohmann::json userinfoParamj;
    userinfoParamj = nlohmann::json::parse(detail::userinfoParam.data());
    userinfoParamj["uid"] = uid;
    userinfoParamj["access_key"] = accessKey;
    std::map<std::string, std::string> m;
    for (auto it = userinfoParamj.begin(); it != userinfoParamj.end(); ++it)
    {
        if (it.value().is_string())
        {
            m[it.key()] = it.value().get<std::string>();
        }
    }
    std::string s{ detail::SetSign(m) };
    std::string t;
    HttpClient h{};
    h.PostRequest(t, GameBiliUrls::Userinfo, s, detail::headers);
#ifdef _DEBUG
    std::cout << "BiliBili用户信息：" << t << std::endl;
#endif // _DEBUG
    struct
    {
        int code{};
        std::string uname{};
    } result;
    json j{};
    j = nlohmann::json::parse(t);
    result.code = j["code"];
    if (result.code != 0)
    {
        return result;
    }
    result.uname = j["uname"];
    return result;
}

inline auto LoginByPassWord(
    const std::string_view biliAccount,
    const std::string_view biliPwd,
    const std::string_view gt_user = "",
    const std::string_view challenge = "",
    const std::string_view validate = "")
{
    json data;
    data= nlohmann::json::parse(detail::rsaParam.data());
    std::map<std::string, std::string> dataM;
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        if (it.value().is_string())
        {
            dataM[it.key()] = it.value().get<std::string>();
        }
    }
    std::string p1 = detail::SetSign(dataM);
    std::string re;
    HttpClient h;
    h.PostRequest(re, GameBiliUrls::Rsa, p1, detail::headers);
#ifdef _DEBUG
    std::cout << re << std::endl;
#endif // _DEBUG
    data= nlohmann::json::parse(detail::loginParam.data());
    json re1J;
    re1J= nlohmann::json::parse(re.data());
    std::string publicKey = re1J["rsa_key"];
    FormatRsaPublicKey(publicKey);
    data["access_key"] = "";
    data["gt_user_id"] = gt_user.data();
    data["uid"] = "";
    data["challenge"] = challenge.data();
    data["user_id"] = biliAccount.data();
    data["validate"] = validate.data();
    if (!validate.empty())
    {
        data["seccode"] = validate.data() + std::string{ "|jordan" };
    }
    std::string hash1 = re1J["hash"];
    std::string rekit = rsaEncrypt(hash1 + biliPwd.data(), publicKey);
    data["pwd"] = rekit;
#ifdef _DEBUG
    std::cout << data.dump() << std::endl;
#endif // _DEBUG
    std::map<std::string, std::string> dataR;
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        if (it.value().is_string())
        {
            dataR[it.key()] = it.value().get<std::string>();
        }
    }
    std::string p2 = detail::SetSign(dataR);
    re.clear();
    data.clear();
    re1J.clear();
    h.PostRequest(re, GameBiliUrls::Login, p2, detail::headers);
    json j1;
    j1= nlohmann::json::parse(re);
    struct
    {
        int code{};
        std::string message{};
        std::string uid{};
        std::string access_key{};
        std::string uname{};
    } result;
    result.code = j1["code"];
    if (result.code == 20000 || result.code != 0)
    {
        try
        {
            result.message = static_cast<std::string>(j1["message"]);
        }
        catch (const std::exception&)
        {
            result.message = "意外登录错误";
        }
        return result;
    }
    result.uid = std::to_string((int)j1["uid"]);
    result.access_key = static_cast<std::string>(j1["access_key"]);
    auto UserInforesult{ GetUserInfo(result.uid, result.access_key) };
    result.uname = UserInforesult.uname;
    return result;
}

inline auto CaptchaCaptcha()
{
    json data;
    data= nlohmann::json::parse(detail::captchaParam.data());
    std::map<std::string, std::string> info;
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        if (it.value().is_string())
        {
            info[it.key()] = it.value().get<std::string>();
        }
    }
    std::string data1 = detail::SetSign(info);
    std::string data2;
    HttpClient h;
    h.PostRequest(data2, GameBiliUrls::StartCaptcha, data1, detail::headers);
    json captchaJ;
    captchaJ= nlohmann::json::parse(data2);
    struct
    {
        std::string gt{};
        std::string challenge{};
        std::string gt_user_id{};
    } result;
    result.gt = captchaJ["gt"].get<std::string>();
    result.challenge = captchaJ["challenge"].get<std::string>();
    result.gt_user_id = captchaJ["gt_user_id"].get<std::string>();
    return result;
}

}
}