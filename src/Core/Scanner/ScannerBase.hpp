#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <map>

#include "Common/Types.h"
#include "Common/Constants.h"

class ScannerBase
{
public:
    GameType gameType;
    std::string lastTicket;
    std::string stoken;      // 米游社 session token
    std::string mid;         // 米游社 mid

    // 游戏类型映射（从二维码内容识别）
    std::map<std::string_view, std::function<void()>> setGameType{
        { "8F3", [this]() { gameType = GameType::Honkai3; } },
        { "9E&", [this]() { gameType = GameType::Genshin; } },
        { "8F%", [this]() { gameType = GameType::HonkaiStarRail; } },
        { "%BA", [this]() { gameType = GameType::ZenlessZoneZero; } },
    };
};