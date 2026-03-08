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
    std::string_view scanUrl{};
    std::string_view confirmUrl{};
    std::string lastTicket;
    std::string uid;
    std::string gameToken{};
    std::map<std::string_view, std::function<void()>> setGameType{
        { "8F3", [this]() {
             gameType = GameType::Honkai3;
             scanUrl = MihoyoUrls::Bh3QrcodeScan;
             confirmUrl = MihoyoUrls::Bh3QrcodeConfirm;
         } },
        { "9E&", [this]() {
             gameType = GameType::Genshin;
             scanUrl = MihoyoUrls::Hk4eQrcodeScan;
             confirmUrl = MihoyoUrls::Hk4eQrcodeConfirm;
         } },
        { "8F%", [this]() {
             gameType = GameType::HonkaiStarRail;
             scanUrl = MihoyoUrls::HkrpgQrcodeScan;
             confirmUrl = MihoyoUrls::HkrpgQrcodeConfirm;
         } },
        { "%BA", [this]() {
             gameType = GameType::ZenlessZoneZero;
             scanUrl = MihoyoUrls::NapCnQrcodeScan;
             confirmUrl = MihoyoUrls::NapCnQrcodeConfirm;
         } },
    };
};