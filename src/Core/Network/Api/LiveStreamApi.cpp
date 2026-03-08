#include "LiveStreamApi.h"

#include <format>
#include <fstream>
#include <regex>
#include <random>
#include <chrono>

#include <nlohmann/json.hpp>

#include "Utils/CryptoUtils.h"
#include "Utils/StringUtil.hpp"
#include "Utils/TimeUtil.hpp"
#include "abogus.h"
#include "Common/Constants.h"

LiveBili::LiveBili(const std::string& roomID) :
    m_roomID(roomID)
{
}

LiveStreamStatus LiveBili::GetLiveStreamStatus()
{
    std::string result;
    std::string url = std::string(LiveStreamUrls::BiliRoomInit) + "?id=" + m_roomID;
    std::cout << "[Bili] Room ID: " << m_roomID << std::endl;
    std::cout << "[Bili] Request URL: " << url << std::endl;

    GetRequest(result, url.c_str());
    std::cout << "[Bili] Response length: " << result.length() << std::endl;

    try
    {
        auto roomInfo = nlohmann::json::parse(result);
        std::cout << "[Bili] JSON parsed successfully" << std::endl;

        if (!roomInfo.contains("code"))
        {
            std::cerr << "[Bili] Missing 'code' in response" << std::endl;
            return LiveStreamStatus::Error;
        }

        int code = roomInfo["code"];
        std::cout << "[Bili] Response code: " << code << std::endl;

        if (code == 60004)
        {
            std::cout << "[Bili] Room does not exist" << std::endl;
            return LiveStreamStatus::Absent;
        }

        if (code == 0)
        {
            if (!roomInfo.contains("data"))
            {
                std::cerr << "[Bili] Missing 'data' in response" << std::endl;
                return LiveStreamStatus::Error;
            }

            int liveStatus = roomInfo["data"]["live_status"];
            std::cout << "[Bili] Live status: " << liveStatus << std::endl;

            if (liveStatus != 1)
            {
                std::cout << "[Bili] Room is not streaming" << std::endl;
                return LiveStreamStatus::NotLive;
            }
            else
            {
                m_realRoomID = std::to_string(roomInfo["data"]["room_id"].get<int>());
                std::cout << "[Bili] Real room ID: " << m_realRoomID << std::endl;
                return LiveStreamStatus::Normal;
            }
        }

        std::cerr << "[Bili] Unknown response code: " << code << std::endl;
        if (roomInfo.contains("message"))
        {
            std::cerr << "[Bili] Message: " << roomInfo["message"].get<std::string>() << std::endl;
        }
        return LiveStreamStatus::Error;
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "[Bili] JSON parse error: " << e.what() << std::endl;
        return LiveStreamStatus::Error;
    }
}

std::string LiveBili::GetLiveStreamLink()
{
    return GetLinkByRealRoomID(m_realRoomID);
}

std::string LiveBili::GetLinkByRealRoomID(const std::string& realRoomID)
{
    const std::map<std::string, std::string>& params = {
        //appkey:iVGUTjsxvpLeuDCf
        //build:6215200
        //c_locale:zh_CN
        { "codec", "0" },
        //device:web
        //device_name:VTR-AL00
        //dolby:1
        { "format", "0,2" },
        //free_type:0
        //http:1
        //mask:0
        //mobi_app:web
        //network:wifi
        //no_playurl:0
        { "only_audio", "0" },
        { "only_video", "0" },
        //TODO platform 会影响下载时使用的referer
        //{"platform", "h5" },
        //play_type:0
        { "protocol", "0,1" },
        { "qn", "10000" },
        { "room_id", realRoomID },
        //s_locale:zh_CN
        //statistics:{\"appId\":1,\"platform\":3,\"version\":\"6.21.5\",\"abtest\":\"\"}
    };
    return GetStreamUrl(LiveStreamUrls::BiliV2PlayInfo, params);
}

std::string LiveBili::GetStreamUrl(const std::string& url, const std::map<std::string, std::string>& param)
{
    std::string str;
    std::string fullUrl = url + "?" + MapToQueryString(param);
    std::cout << "[Bili] Stream URL request: " << fullUrl.substr(0, 100) << "..." << std::endl;

    GetRequest(str, fullUrl.c_str());
    std::cout << "[Bili] Stream response length: " << str.length() << std::endl;

    try
    {
        auto playInfo = nlohmann::json::parse(str);
        std::cout << "[Bili] Stream JSON parsed successfully" << std::endl;

        if (!playInfo.contains("data") || !playInfo["data"].contains("playurl_info"))
        {
            std::cerr << "[Bili] Missing 'data' or 'playurl_info' in response" << std::endl;
            if (playInfo.contains("code"))
            {
                std::cerr << "[Bili] Error code: " << playInfo["code"] << std::endl;
                if (playInfo.contains("message"))
                {
                    std::cerr << "[Bili] Error message: " << playInfo["message"].get<std::string>() << std::endl;
                }
            }
            return "";
        }

        auto& codec = playInfo["data"]["playurl_info"]["playurl"]["stream"][0]["format"][0]["codec"][0];
        const std::string& base_url{ codec["base_url"].get<std::string>() };
        const std::string& extra{ codec["url_info"][0]["extra"].get<std::string>() };
        const std::string& host{ codec["url_info"][0]["host"].get<std::string>() };

        std::string stream_url{ host + base_url + extra };
        std::cout << "[Bili] Final stream URL: " << stream_url.substr(0, 80) << "..." << std::endl;
        return stream_url;
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "[Bili] JSON parse error in GetStreamUrl: " << e.what() << std::endl;
        return "";
    }
}

LiveDouyin::LiveDouyin(const std::string& roomID) :
    m_roomID(roomID)
{
}

LiveStreamStatus LiveDouyin::GetLiveStreamStatus()
{
    std::cout << "[Douyin] Room ID: " << m_roomID << std::endl;

    std::string ret;
    std::string user_agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/92.0.4515.159 Safari/537.36";
    const std::map<std::string, std::string>& header = {
        { "User-Agent", user_agent },
        { "referer", "https://live.douyin.com/" },
        { "cookie", "ttwid=1%7CxaukhGxBEHUsUlCmIY4HRiGzUO3JIZtwThFAM26tJso%7C1676112432%7C7764fe35c2bb172955868ce911bca5aa8b1019e28e9fd9f8cd925bbb11a3ec1f; xgplayer_user_id=327735195191; odin_tt=7205a5bf96b9ae49071d11088256571edac5cfc1423f99b4d8b27ef9968e144b9ef6e70febb76032686d2f53b386b024dee75ebecccbfa2f68c2096dd7fdaaa887921bf20108bf274532d73f11ae74b2; pwa2=%220%7C0%7C3%7C1%22; bd_ticket_guard_client_data=eyJiZC10aWNrZXQtZ3VhcmQtdmVyc2lvbiI6MiwiYmQtdGlja2V0LWd1YXJkLWl0ZXJhdGlvbi12ZXJzaW9uIjoxLCJiZC10aWNrZXQtZ3VhcmQtcmVlLXB1YmxpYy1rZXkiOiJCTUoxOC9iWmV0QlF1OGxWMXFnbFpVTE5abXFIV2YzNDh5YWZLandxRXk0c3ZrdHRQaTNXcU9jYk0vSDBnVHc2bWZyWmxMYnI5MWhPTzJGWjFaaTJDbGc9IiwiYmQtdGlja2V0LWd1YXJkLXdlYi12ZXJzaW9uIjoxfQ==; s_v_web_id=verify_llt5j3pc_qHbwb43X_rsMK_4URB_8lX8_KGZk6AZuqKQW; passport_csrf_token=6ee312f524a087d2e0954e280195f093; passport_csrf_token_default=6ee312f524a087d2e0954e280195f093; FORCE_LOGIN=%7B%22videoConsumedRemainSeconds%22%3A180%7D; volume_info=%7B%22isUserMute%22%3Afalse%2C%22isMute%22%3Atrue%2C%22volume%22%3A0.6%7D; live_version=%221.1.1.3542%22; device_web_cpu_core=12; device_web_memory_size=8; webcast_local_quality=sd; csrf_session_id=c2d050ec6eca0581bd33676bb01efbcb; webcast_leading_last_show_time=1693840853743; webcast_leading_total_show_times=1; download_guide=%223%2F20230905%2F0%22; strategyABtestKey=%221693878032.975%22; stream_recommend_feed_params=%22%7B%5C%22cookie_enabled%5C%22%3Atrue%2C%5C%22screen_width%5C%22%3A1536%2C%5C%22screen_height%5C%22%3A960%2C%5C%22browser_online%5C%22%3Atrue%2C%5C%22cpu_core_num%5C%22%3A12%2C%5C%22device_memory%5C%22%3A8%2C%5C%22downlink%5C%22%3A10%2C%5C%22effective_type%5C%22%3A%5C%224g%5C%22%2C%5C%22round_trip_time%5C%22%3A150%7D%22; VIDEO_FILTER_MEMO_SELECT=%7B%22expireTime%22%3A1694482833541%2C%22type%22%3A1%7D; home_can_add_dy_2_desktop=%221%22; __ac_signature=_02B4Z6wo00f01HeJgRgAAIDBgvZrhVoJ1YB3qYWAAHkc9uZs7w58Na.-yxd4KVmlVgdxWyyGFH2FZ4Kcpu-LDEpV82QCjWieKfk06Z9B0S9K5LW49zI8AxPvqQFgkBRN9-xx8Aidv67iRFh00f; live_can_add_dy_2_desktop=%220%22; msToken=TJ5bDSCEeWCWz1nY9wrOhgQ4dr_SCBPPImY2G9Blj2VTSEwa7b4RfiqweW5RR0jaCEp-itcBYF8BispuGB-mlFCtslq_jJ5EU8fBjsLPginmPn39I5ZjIA==; tt_scid=NU1fQiBzvNTjAvXazULYjHoX7IQrdOmzz76IroqwcbcST8q5MiafUeBz6kCNZJIlcb47; msToken=MpALMGeJrcnDeVMS6FSENJ16Oz4BL2kXyknj5QXcXixQgcCBNF21bC0dE8mlrlHUZKbwHMN_4AG7a5V2yeSqLtePinRu-DHqZv1do92C6XGKEZVnxj4eug==; IsDouyinActive=false" }
    };
    std::string p = "aid=6383&app_name=douyin_web&live_id=1&device_platform=web&browser_language=zh-CN&browser_platform=Win32&browser_name=Edge&browser_version=139.0.0.0&is_need_double_stream=false&web_rid=" + m_roomID;
    char* ab = get_abogus(user_agent.c_str(), p.c_str());
    if (!ab) {
        std::cerr << "[Douyin] a_bogus 生成失败，请确保 abogus_py 已部署且已安装 Node.js 并在该目录执行 npm install" << std::endl;
        return LiveStreamStatus::Error;
    }
    std::string abogus(ab);
    free_abogus(ab);
    std::string url = std::string(LiveStreamUrls::DouyinRoom) + p + "&a_bogus=" + abogus;

    std::cout << "[Douyin] Request URL: " << url.substr(0, 100) << "..." << std::endl;

    GetRequest(ret, url.c_str(), header);
    std::cout << "[Douyin] Response length: " << ret.length() << std::endl;

    try
    {
        auto streamInfo = nlohmann::json::parse(ret);
        std::cout << "[Douyin] JSON parsed successfully" << std::endl;

        if (!streamInfo.contains("status_code"))
        {
            std::cerr << "[Douyin] Missing 'status_code' in response" << std::endl;
            return LiveStreamStatus::Error;
        }

        int statusCode = streamInfo["status_code"].get<int>();
        std::cout << "[Douyin] Status code: " << statusCode << std::endl;

        if (statusCode != 0)
        {
            std::cerr << "[Douyin] API returned error status: " << statusCode << std::endl;
            return LiveStreamStatus::Absent;
        }

        if (!streamInfo.contains("data"))
        {
            std::cerr << "[Douyin] Missing 'data' in response" << std::endl;
            return LiveStreamStatus::Absent;
        }

        if (!streamInfo["data"].contains("data"))
        {
            std::cerr << "[Douyin] Missing 'data.data' in response" << std::endl;
            return LiveStreamStatus::Absent;
        }

        if (!streamInfo["data"]["data"].is_array())
        {
            std::cerr << "[Douyin] 'data.data' is not an array" << std::endl;
            return LiveStreamStatus::Absent;
        }

        if (streamInfo["data"]["data"].size() == 0)
        {
            std::cerr << "[Douyin] 'data.data' array is empty" << std::endl;
            return LiveStreamStatus::Absent;
        }

        const nlohmann::json& data = streamInfo["data"]["data"][0];
        std::cout << "[Douyin] Got room data" << std::endl;

        if (!data.contains("status"))
        {
            std::cerr << "[Douyin] Missing 'status' in room data" << std::endl;
            return LiveStreamStatus::Error;
        }

        int status = data["status"].get<int>();
        std::cout << "[Douyin] Room status: " << status << std::endl;

        // 抖音 status == 2 代表是开播的状态
        if (status == 2)
        {
            if (!data.contains("stream_url"))
            {
                std::cerr << "[Douyin] Missing 'stream_url' in room data" << std::endl;
                return LiveStreamStatus::Error;
            }

            std::cout << "[Douyin] Room is streaming, extracting URL..." << std::endl;

            // 先尝试 pull_datas
            if (data["stream_url"].contains("pull_datas") && !data["stream_url"]["pull_datas"].empty())
            {
                std::cout << "[Douyin] Using pull_datas" << std::endl;
                nlohmann::json pullDatas = data["stream_url"]["pull_datas"];
                nlohmann::json doubleScreenStreams = pullDatas.begin().value();

                if (doubleScreenStreams.contains("stream_data"))
                {
                    nlohmann::json streamData = nlohmann::json::parse(doubleScreenStreams["stream_data"].get<std::string>());
                    if (streamData.contains("data") && streamData["data"].contains("origin"))
                    {
                        m_flvUrl = streamData["data"]["origin"]["main"]["flv"].get<std::string>();
                        std::cout << "[Douyin] Stream URL obtained: " << m_flvUrl.substr(0, 80) << "..." << std::endl;
                        return LiveStreamStatus::Normal;
                    }
                }
            }

            // 尝试 live_core_sdk_data
            std::cout << "[Douyin] Trying live_core_sdk_data" << std::endl;
            if (!data["stream_url"].contains("live_core_sdk_data"))
            {
                std::cerr << "[Douyin] Missing 'live_core_sdk_data'" << std::endl;
                return LiveStreamStatus::Error;
            }

            if (!data["stream_url"]["live_core_sdk_data"].contains("pull_data"))
            {
                std::cerr << "[Douyin] Missing 'pull_data'" << std::endl;
                return LiveStreamStatus::Error;
            }

            if (!data["stream_url"]["live_core_sdk_data"]["pull_data"].contains("stream_data"))
            {
                std::cerr << "[Douyin] Missing 'stream_data'" << std::endl;
                return LiveStreamStatus::Error;
            }

            if (!data["stream_url"]["live_core_sdk_data"]["pull_data"]["stream_data"].is_string())
            {
                std::cerr << "[Douyin] 'stream_data' is not a string" << std::endl;
                return LiveStreamStatus::Error;
            }

            std::string streamDataStr = data["stream_url"]["live_core_sdk_data"]["pull_data"]["stream_data"].get<std::string>();
            std::cout << "[Douyin] Stream data length: " << streamDataStr.length() << std::endl;

            nlohmann::json streamData = nlohmann::json::parse(streamDataStr);

            if (!streamData.contains("data") || !streamData["data"].contains("origin"))
            {
                std::cerr << "[Douyin] Missing 'data' or 'origin' in stream data" << std::endl;
                std::cout << "[Douyin] Available keys: ";
                for (auto it = streamData.begin(); it != streamData.end(); ++it)
                {
                    std::cout << it.key() << " ";
                }
                std::cout << std::endl;
                return LiveStreamStatus::Error;
            }

            m_flvUrl = streamData["data"]["origin"]["main"]["flv"].get<std::string>();
            std::cout << "[Douyin] Stream URL obtained: " << m_flvUrl.substr(0, 80) << "..." << std::endl;
            return LiveStreamStatus::Normal;
        }
        else if (status == 4)
        {
            std::cout << "[Douyin] Room is not streaming" << std::endl;
            return LiveStreamStatus::NotLive;
        }

        std::cerr << "[Douyin] Unknown room status: " << status << std::endl;
        return LiveStreamStatus::Error;
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "[Douyin] JSON parse error: " << e.what() << std::endl;
        return LiveStreamStatus::Error;
    }
}

std::string LiveDouyin::GetLiveStreamLink() const
{
    return m_flvUrl;
}

LiveDouyin::~LiveDouyin()
{
}