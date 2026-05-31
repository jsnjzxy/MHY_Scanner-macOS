# 米哈游游戏二维码登录流程文档

## 概述

本文档描述了 MHY_Scanner 实现游戏二维码自动登录的正确流程。该流程通过逆向分析米游社 APP 的网络请求得出。

## 流程图

```
┌──────────┐     ┌──────────────────────────────────────────────────────┐
│ 游戏端   │     │                    MHY_Scanner                       │
│ 显示二维码│     │                                                      │
└────┬─────┘     └──────────────────────┬───────────────────────────────┘
     │                                  │
     │  二维码内容 (含 ticket)           │
     │◀─────────────────────────────────│
     │                                  │
     │                                  ▼
     │                    ┌─────────────────────────────┐
     │                    │  Step 1: 游戏特定 API 扫描   │
     │                    │  POST combo/panda/qrcode/scan│
     │                    │  Body: {                    │
     │                    │    app_id: "4",             │
     │                    │    device: "UUID",          │
     │                    │    passport_app_id: "bll...",│
     │                    │    ticket: "游戏二维码ticket" │
     │                    │  }                          │
     │                    └─────────────┬───────────────┘
     │                                  │
     │                                  │ Response: { passport_qr_url: "...tk=XXX..." }
     │                                  ▼
     │                    ┌─────────────────────────────┐
     │                    │  提取 tk 参数               │
     │                    │  tk = "51078210-9afb-..."   │
     │                    └─────────────┬───────────────┘
     │                                  │
     │                                  ▼
     │                    ┌─────────────────────────────┐
     │                    │  Step 2: Passport API 扫描   │
     │                    │  POST scanQRLogin           │
     │                    │  Cookie: stoken=...; mid=...│
     │                    │  Body: {                    │
     │                    │    ticket: "tk值",          │
     │                    │    token_types: ["1"]       │
     │                    │  }                          │
     │                    └─────────────┬───────────────┘
     │                                  │
     │                                  │ Response: { app_name: "原神", ... }
     │                                  ▼
     │                    ┌─────────────────────────────┐
     │                    │  用户确认登录？             │
     │                    │  (auto_login 或手动点击)    │
     │                    └─────────────┬───────────────┘
     │                                  │
     │                                  ▼
     │                    ┌─────────────────────────────┐
     │                    │  Step 3: Passport API 确认   │
     │                    │  POST confirmQRLogin        │
     │                    │  Cookie: stoken=...; mid=...│
     │                    │  Body: {                    │
     │                    │    ticket: "tk值",          │
     │                    │    token_types: ["1"]       │
     │                    │  }                          │
     │                    └─────────────┬───────────────┘
     │                                  │
     │                                  │ Response: { retcode: 0 }
     │                                  │
     ▼                                  ▼
┌──────────┐                 ┌──────────────────┐
│ 游戏端   │◀────────────────│ 登录成功！       │
│ 登录成功 │                 │                  │
└──────────┘                 └──────────────────┘
```

## API 详细说明

### Step 1: 游戏特定 API 扫描

**URL**: `https://api-sdk.mihoyo.com/{game}/combo/panda/qrcode/scan`

| 游戏 | URL 路径 |
|------|----------|
| 原神 | `hk4e_cn/combo/panda/qrcode/scan` |
| 崩坏：星穹铁道 | `hkrpg_cn/combo/panda/qrcode/scan` |
| 绝区零 | `nap_cn/combo/panda/qrcode/scan` |
| 崩坏3 | `bh3_cn/combo/panda/qrcode/scan` |

**请求头**:
```
Content-Type: application/json
x-rpc-app_id: bll8iq97cem8
x-rpc-device_id: {UUID}
x-rpc-app_version: 2.76.1
x-rpc-client_type: 2
```

**请求体**:
```json
{
  "app_id": "4",
  "device": "7151313B-7DA5-4192-9672-6D3700098654",
  "passport_app_id": "bll8iq97cem8",
  "ticket": "6a1b8ac5faaacc34229c55be"
}
```

**关键字段**:
- `app_id`: 游戏标识（原神=4, 星铁=5, 绝区零=8, 崩坏3=1）
- `passport_app_id`: **必须添加**，值为 `bll8iq97cem8`
- `ticket`: 从游戏二维码中提取的 24 位字符

**响应**:
```json
{
  "retcode": 0,
  "message": "OK",
  "data": {
    "passport_qr_url": "https://user.mihoyo.com/login-platform/mobile.html?expire=xxx&tk=51078210-9afb-42c2-9065-16e6d92c77ae&token_types=1#/login/qr"
  }
}
```

**关键操作**: 从 `passport_qr_url` 中提取 `tk` 参数值

---

### Step 2: Passport API 扫描

**URL**: `https://passport-api.mihoyo.com/account/ma-cn-passport/app/scanQRLogin`

**请求头**:
```
Content-Type: application/json
Cookie: stoken={stoken}; mid={mid}
x-rpc-app_id: bll8iq97cem8
x-rpc-device_id: {UUID}
```

**请求体**:
```json
{
  "token_types": ["1"],
  "ticket": "51078210-9afb-42c2-9065-16e6d92c77ae"
}
```

**关键字段**:
- `ticket`: **使用 Step 1 返回的 tk 值**，不是原始二维码 ticket
- `Cookie`: 必须包含有效的 `stoken` 和 `mid`

**响应**:
```json
{
  "retcode": 0,
  "message": "OK",
  "data": {
    "app_id": "c76ync6mutq8",
    "app_name": "原神",
    "account_disp_name": "132******90"
  }
}
```

---

### Step 3: Passport API 确认

**URL**: `https://passport-api.mihoyo.com/account/ma-cn-passport/app/confirmQRLogin`

**请求头**:
```
Content-Type: application/json
Cookie: stoken={stoken}; mid={mid}
x-rpc-app_id: bll8iq97cem8
x-rpc-device_id: {UUID}
```

**请求体**:
```json
{
  "ticket": "51078210-9afb-42c2-9065-16e6d92c77ae",
  "token_types": ["1"]
}
```

**响应**:
```json
{
  "retcode": 0,
  "message": "OK",
  "data": {}
}
```

---

## 关键概念

### 两种 Ticket

| 名称 | 来源 | 用途 |
|------|------|------|
| `ticket` | 游戏二维码（最后 24 位字符） | Step 1 请求参数 |
| `tk` | Step 1 响应中的 `passport_qr_url` | Step 2 & 3 请求参数 |

### 认证凭证

| 凭证 | 说明 | 获取方式 |
|------|------|----------|
| `stoken` | 米游社 session token | Passport API 登录后获取 |
| `mid` | 米游社账户标识 | Passport API 登录后获取 |

### 为什么不能用单一 API

1. **游戏特定 API 的 confirm**：
   - 需要 `uid` + `cookieToken`
   - 但 `cookieToken` 无法用于游戏登录确认
   - **结论**: 只能用于 scan，不能用于 confirm

2. **Passport API 直接使用**：
   - `scanQRLogin` 直接使用游戏二维码 ticket 会返回 `-3501`
   - Passport API 不识别游戏二维码格式
   - **结论**: 需要先通过游戏 API 转换 ticket

3. **正确做法**：
   - 游戏扫描 API 初始化流程并生成 `tk`
   - Passport API 使用 `tk` 完成认证和确认

---

## 常见错误码

| retcode | 说明 | 解决方案 |
|---------|------|----------|
| 0 | 成功 | - |
| -3501 | 二维码已失效 | 刷新游戏二维码重试 |
| -100 | 登录状态失效 | 重新登录米游社账户 |

---

## 代码实现要点

### 1. ScanQRLogin 函数返回 tk

```cpp
// 返回 tk 字符串，空字符串表示失败
inline std::string ScanQRLogin(const std::string_view url, const std::string_view ticket, GameType gameType)
{
    // 请求体必须包含 passport_app_id
    std::string requestBody = "{\"app_id\":\"" + std::to_string(static_cast<int>(gameType)) +
        "\",\"device\":\"" + device_id + "\"" +
        ",\"passport_app_id\":\"bll8iq97cem8\"" +
        ",\"ticket\":\"" + std::string(ticket) + "\"}";
    
    // 从响应中提取 tk
    // passport_qr_url: "...tk=XXX&..." → 提取 XXX
}
```

### 2. 存储必要凭证

```cpp
class ScannerBase
{
public:
    std::string stoken;  // Passport API 认证
    std::string mid;     // Passport API 认证
    std::string m_tk;    // 从游戏扫描 API 获取，用于 Passport API
};
```

### 3. 三步流程

```cpp
void handleQRCodeDetected(const std::string& code)
{
    // 1. 游戏扫描 → 获取 tk
    std::string tk = ScanQRLogin(scanUrl, ticket, gameType);
    
    // 2. Passport 扫描
    ScanGameQrcode(tk, stoken, mid);
    
    // 3. Passport 确认
    ConfirmGameQrcode(tk, stoken, mid);
}
```

---

## 参考链接

- Passport API 文档：`passport-api.mihoyo.com`
- 游戏扫描 API：`api-sdk.mihoyo.com/{game}/combo/panda/qrcode/scan`
