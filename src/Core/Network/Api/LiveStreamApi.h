#pragma once

#include <string>

#include "../HttpClient.h"

enum class LiveStreamStatus : uint8_t
{
    Normal = 0,
    Absent = 1,
    NotLive = 2,
    Error = 3
};

class LiveBili : public HttpClient
{
public:
    LiveBili(const std::string& roomID);
    LiveStreamStatus GetLiveStreamStatus();
    std::string GetLiveStreamLink();

private:
    //获取B站直播流地址
    std::string GetLinkByRealRoomID(const std::string& realRoomID);
    std::string GetStreamUrl(const std::string& url, const std::map<std::string, std::string>& param);
    std::string m_roomID;
    std::string m_realRoomID;
    //获取直播间信息接口
    //https://api.live.bilibili.com/room/v1/Room/get_info?room_id=6&from=room
};

class LiveDouyin : public HttpClient
{
public:
    LiveDouyin(const std::string& roomID);
    LiveStreamStatus GetLiveStreamStatus();
    std::string GetLiveStreamLink() const;
    ~LiveDouyin();

private:
    std::string m_roomID;
    std::string m_flvUrl;
};