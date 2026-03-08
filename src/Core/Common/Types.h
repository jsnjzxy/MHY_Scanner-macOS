#pragma once

#include <cstdint>

enum class ServerType : uint8_t
{
    Official = 1,
    BH3_BiliBili = 2
};

enum class GameType
{
    UNKNOW = 0,
    Honkai3 = 1,
    TearsOfThemis = 2,
    Genshin = 4,
    PlatformApp = 5,
    Honkai2 = 7,
    HonkaiStarRail = 8,
    CloudGame = 9,
    _3NNN = 10,
    PJSH = 11,
    ZenlessZoneZero = 12,
    HYG = 13,
    Honkai3_BiliBili = 10000
};

enum class ScanRet
{
    UNKNOW = 0,
    SUCCESS = 1,
    FAILURE_1 = 3,
    FAILURE_2 = 4,
    LIVESTOP = 5,
    STREAMERROR = 6
};
