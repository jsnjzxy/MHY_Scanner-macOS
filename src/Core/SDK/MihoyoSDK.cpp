#include "MihoyoSDK.h"

#include <nlohmann/json.hpp>
#include <format>

#include "Utils/CryptoUtils.h"
#include "Utils/StringUtil.hpp"
#include "Utils/TimeUtil.hpp"
#include "Common/Constants.h"

using json = nlohmann::json;

MihoyoSDK::MihoyoSDK()
{
}

std::string MihoyoSDK::verify(const std::string& uid, const std::string& access_key)
{
#ifdef _DEBUG
    std::cout << "verify with uid = " << uid << std::endl;
#endif // _DEBUG
    verifyData["uid"] = uid;
    verifyData["access_key"] = access_key;
    const std::string bodyData = std::string(verifyData["access_key"]) + "\",\"uid\":" + std::string(verifyData["uid"]);
    json body;
    body = nlohmann::json::parse(verifyBody);
    body["data"] = bodyData;
    const std::string sBody = body.dump();
    body.clear();
    std::string s;
    PostRequest(s, MihoyoUrls::Bh3V2Login, makeSign(sBody));
#ifdef _DEBUG
    std::cout << "崩坏3验证完成 : " << s << std::endl;
#endif // _DEBUG
    return s;
}

void MihoyoSDK::setBHVer(const std::string& s)
{
    bh3Ver = s;
}

void MihoyoSDK::setOAServer()
{
    oaString = getOAServer();
}

bool MihoyoSDK::validityCheck(std::string_view ticket)
{
    if (ticket == m_ticket)
    {
        return true;
    }
    return false;
}

std::string MihoyoSDK::getOAServer()
{
    //const std::string bhVer = bh3Ver;
    //const std::string oaMainUrl = "https://dispatch.scanner.hellocraft.xyz/v1/query_dispatch/?version=";
    //std::string param = bhVer + "_gf_android_bilibili&t=" + std::to_string(getCurrentUnixTime());
    //std::string feedback;
    //GetRequest(feedback, oaMainUrl + param);

    std::string oaserver;
    GetRequest(oaserver, "https://mi-m-cpjgtouitx.cn-hangzhou.fcapp.run");
    if (oaserver.empty())
    {
        exit(0);
    }
    return oaserver;

    //	param = bhVer + "_gf_android_bilibili&t=" + std::to_string(getCurrentUnixTime());
    //	json j;
    //	j= nlohmann::json::parse(feedback);
    //	std::string dispatch_url = j["region_list"][0]["dispatch_url"];
    //	std::string dispatch;
    //	PostRequest(dispatch, dispatch_url + param, "");
    //	dispatch = UTF8_To_string(dispatch);
    //#ifdef _DEBUG
    //	std::cout << "获得OA服务器 : " << dispatch << std::endl;
    //#endif // DEBUG
    //	j.clear();
    //	return dispatch;
}

void MihoyoSDK::scanInit(const std::string& ticket, const std::string& bhInfo)
{
    m_ticket = ticket;
    m_bhInfo = bhInfo;
}

ScanRet MihoyoSDK::scanCheck()
{
    json check;
    check= nlohmann::json::parse(scanCheckS);
    check["ticket"] = m_ticket;
    check["ts"] = (int)GetUnixTimeStampSeconds();
    std::string postBody = makeSign(check.dump());
    std::string feedback;
    PostRequest(feedback, MihoyoUrls::Bh3QrcodeScan, postBody); //扫码请求
    check= nlohmann::json::parse(feedback);
    int retcode = (int)check["retcode"];
    check.clear();
    if (retcode != 0)
    {
        return ScanRet::FAILURE_1;
    }
    return ScanRet::SUCCESS;
}

ScanRet MihoyoSDK::scanConfirm()
{
    json bhInfoJ;
    bhInfoJ= nlohmann::json::parse(m_bhInfo);

    json bhInFo;
    bhInFo= nlohmann::json::parse(bhInfoJ["data"].dump());
    //std::cout << bhInFo.dump() << std::endl;
    json scanResultJ;
    scanResultJ= nlohmann::json::parse(scanResult);

    json scanDataJ;
    scanDataJ= nlohmann::json::parse(scanData);

    //json oa;
    //oa= nlohmann::json::parse(oaString);
    scanDataJ["dispatch"] = oaString;
    scanDataJ["c"] = bhInFo["open_id"];
    std::cout << std::string(bhInFo["open_id"]) << " \n";
    scanDataJ["accountToken"] = bhInFo["combo_token"];
    //std::cout << scanDataJ.dump() << std::endl;
    json scanExtJ;
    scanExtJ= nlohmann::json::parse(scanExtR);
    scanExtJ["data"] = scanDataJ;
    json scanRawJ;
    scanRawJ= nlohmann::json::parse(scanRawR);
    scanRawJ["open_id"] = bhInFo["open_id"];
    scanRawJ["combo_id"] = bhInFo["combo_id"];
    scanRawJ["combo_token"] = bhInFo["combo_token"];
    json scanPayLoadJ;
    scanPayLoadJ= nlohmann::json::parse(scanPayloadR);
    scanPayLoadJ["raw"] = scanRawJ;
    scanPayLoadJ["ext"] = scanExtJ;
    scanResultJ["payload"] = scanPayLoadJ;
    scanResultJ["ts"] = (int)GetUnixTimeStampSeconds();
    scanResultJ["ticket"] = m_ticket;
    std::string postBody = scanResultJ.dump();
    postBody = makeSign(postBody);
    json postBodyJ;
    postBodyJ= nlohmann::json::parse(postBody);
    std::string a1 = postBodyJ["payload"]["ext"].dump();
    a1 = replaceQuotes(a1);
    postBodyJ["payload"]["ext"] = a1;
    std::string a2 = postBodyJ["payload"]["raw"].dump();
    a2 = replaceQuotes(a2);
    postBodyJ["payload"]["raw"] = a2;
    postBody = postBodyJ.dump();
#ifdef _DEBUG
    std::cout << postBody << std::endl;
#endif // _DEBUG
    std::string response;
    PostRequest(response, MihoyoUrls::Bh3QrcodeConfirm, postBody);
    postBodyJ= nlohmann::json::parse(response);
    if ((int)postBodyJ["retcode"] == 0)
    {
        return ScanRet::SUCCESS;
    }
    else
    {
        return ScanRet::FAILURE_2;
    }
    return ScanRet::FAILURE_2;
}

void MihoyoSDK::setUserName(const std::string& name)
{
    json setName;
    setName= nlohmann::json::parse(scanRawR);
    setName["asterisk_name"] = name;
    scanRawR = setName.dump();
    setName.clear();
}

std::string MihoyoSDK::makeSign(const std::string data)
{
    std::string sign;
    std::string data2;
    json p;
    p= nlohmann::json::parse(data);
    std::map<std::string, std::string> m;
    for (auto it = p.begin(); it != p.end(); ++it)
    {
        if (it.value().is_string())
        {
            m[it.key()] = it.value().get<std::string>();
        }
    }
    for (auto& it : m)
    {
        if (it.first == "sign")
            continue;
        if (it.first == "data") //需要优化
        {
            if (it.second.front() == '\"')
            {
                it.second.erase(0, 1);
            }
            if (it.second.back() == '\"')
            {
                it.second.pop_back();
            }
        }
        if (it.first == "device")
        {
            if (it.second.front() == '\"')
            {
                it.second.erase(0, 1);
            }
            if (it.second.back() == '\"')
            {
                it.second.pop_back();
            }
        }
        data2 += it.first + "=" + it.second + "&";
    }
    data2.erase(data2.length() - 1);
#ifdef _DEBUG
    std::cout << "makeSign = " << data2 << std::endl;
#endif // DEBUG
    sign = bh3Sign(data2);
    p["sign"] = sign;
    sign = p.dump();
    p.clear();
    return sign;
}

std::string MihoyoSDK::bh3Sign(std::string data)
{
    const std::string key = "0ebc517adb1b62c6b408df153331f9aa";
    data.erase(std::remove(data.begin(), data.end(), '\\'), data.end());
    std::string sign = HmacSha256(data, key);
#ifdef _DEBUG
    std::cout << "Hmac_Sha256签名完成" << std::endl;
#endif // DEBUG
    return sign;
}