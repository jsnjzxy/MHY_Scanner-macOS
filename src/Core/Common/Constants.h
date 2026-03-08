#pragma once

#include <string_view>

// B站游戏API URL
namespace GameBiliUrls
{
    constexpr const char* Userinfo = "https://line1-sdk-center-login-sh.biligame.net/api/client/user.info";
    constexpr const char* StartCaptcha = "https://line1-sdk-center-login-sh.biligame.net/api/client/start_captcha";
    constexpr const char* Login = "https://line1-sdk-center-login-sh.biligame.net/api/client/login";
    constexpr const char* Rsa = "https://line1-sdk-center-login-sh.biligame.net/api/client/rsa";
}

// 直播流API URL
namespace LiveStreamUrls
{
    constexpr const char* BiliRoomInit = "https://api.live.bilibili.com/room/v1/Room/room_init";
    constexpr const char* BiliV2PlayInfo = "https://api.live.bilibili.com/xlive/web-room/v2/index/getRoomPlayInfo";
    constexpr const char* DouyinRoom = "https://live.douyin.com/webcast/room/web/enter/?";
}

// 米哈游API URL
namespace MihoyoUrls
{
    // 崩坏3
    constexpr const char* Bh3V2Login = "https://api-sdk.mihoyo.com/bh3_cn/combo/granter/login/v2/login";
    constexpr const char* Bh3Version = "https://bh3-launcher-static.mihoyo.com/bh3_cn/mdk/launcher/api/resource?launcher_id=4";
    constexpr const char* Bh3QrcodeScan = "https://api-sdk.mihoyo.com/bh3_cn/combo/panda/qrcode/scan";
    constexpr const char* Bh3QrcodeConfirm = "https://api-sdk.mihoyo.com/bh3_cn/combo/panda/qrcode/confirm";

    // 原神
    constexpr const char* Hk4eQrcodeScan = "https://api-sdk.mihoyo.com/hk4e_cn/combo/panda/qrcode/scan";
    constexpr const char* Hk4eQrcodeConfirm = "https://api-sdk.mihoyo.com/hk4e_cn/combo/panda/qrcode/confirm";
    constexpr const char* Hk4eQrcodeFetch = "https://hk4e-sdk.mihoyo.com/hk4e_cn/combo/panda/qrcode/fetch";
    constexpr const char* Hk4eQrcodeQuery = "https://hk4e-sdk.mihoyo.com/hk4e_cn/combo/panda/qrcode/query";

    // 星穹铁道
    constexpr const char* HkrpgQrcodeScan = "https://api-sdk.mihoyo.com/hkrpg_cn/combo/panda/qrcode/scan";
    constexpr const char* HkrpgQrcodeConfirm = "https://api-sdk.mihoyo.com/hkrpg_cn/combo/panda/qrcode/confirm";

    // 绝区零
    constexpr const char* NapCnQrcodeScan = "https://api-sdk.mihoyo.com/nap_cn/combo/panda/qrcode/scan";
    constexpr const char* NapCnQrcodeConfirm = "https://api-sdk.mihoyo.com/nap_cn/combo/panda/qrcode/confirm";

    // 通用
    constexpr const char* TakumiMultiToken = "https://api-takumi.mihoyo.com/auth/api/getMultiTokenByLoginTicket";
    constexpr const char* TakumiGameToken = "https://api-takumi.mihoyo.com/auth/api/getGameToken";
    constexpr const char* TakumiGameTokenStoken = "https://api-takumi.mihoyo.com/account/ma-cn-session/app/getTokenByGameToken";
    constexpr const char* MysUserinfo = "https://bbs-api.miyoushe.com/user/api/getUserFullInfo";
    constexpr const char* PassportVerifier = "https://passport-api.mihoyo.com/account/ma-cn-verifier/verifier/createLoginCaptcha";
    constexpr const char* LoginByMobileCaptcha = "https://passport-api.mihoyo.com/account/ma-cn-passport/app/loginByMobileCaptcha";
}
