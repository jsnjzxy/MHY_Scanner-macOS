#include "GeetestDialog.h"

#ifdef __APPLE__

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <nlohmann/json.hpp>
#include <iostream>

// ========================================
// 私有实现类
// ========================================
class GeetestDialog::Private
{
public:
    WKWebView* webView{nullptr};
    NSObject* navigationDelegate{nullptr};
    NSObject* scriptMessageHandler{nullptr};
};

// ========================================
// WKNavigationDelegate 实现
// ========================================
@interface GeetestNavigationDelegate : NSObject <WKNavigationDelegate>
@property (nonatomic, assign) GeetestDialog* dialog;
@end

@implementation GeetestNavigationDelegate
- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
    std::cout << "[GeetestDialog] Page loaded" << std::endl;
}

- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error {
    std::cerr << "[GeetestDialog] Navigation failed: " << error.localizedDescription.UTF8String << std::endl;
}

- (void)webView:(WKWebView *)webView didFailProvisionalNavigation:(WKNavigation *)navigation withError:(NSError *)error {
    std::cerr << "[GeetestDialog] Provisional navigation failed: " << error.localizedDescription.UTF8String << std::endl;
}
@end

// ========================================
// Script Message Handler 实现
// ========================================
@interface GeetestScriptHandler : NSObject <WKScriptMessageHandler>
@property (nonatomic, assign) GeetestDialog* dialog;
@end

@implementation GeetestScriptHandler
- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message {
    if ([message.name isEqualToString:@"geetestResult"]) {
        NSString* jsonStr = message.body;
        QString result = QString::fromNSString(jsonStr);
        std::cout << "[GeetestDialog] Received full result: " << result.toStdString() << std::endl;
        self.dialog->parseVerifyResult(result);
    }
}
@end

// ========================================
// GeetestDialog 实现
// ========================================
GeetestDialog::GeetestDialog(QWidget* parent)
    : QDialog(parent)
    , m_private(std::make_unique<Private>())
{
    setupUI();
}

GeetestDialog::~GeetestDialog()
{
    if (m_private->webView) {
        [m_private->webView release];
    }
    if (m_private->navigationDelegate) {
        [m_private->navigationDelegate release];
    }
    if (m_private->scriptMessageHandler) {
        [m_private->scriptMessageHandler release];
    }
}

void GeetestDialog::setupUI()
{
    setWindowTitle(QStringLiteral("人机验证"));
    setMinimumSize(400, 500);
    resize(450, 550);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 创建 WKWebView 配置
    WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
    WKUserContentController* contentController = [[WKUserContentController alloc] init];

    // 注册脚本消息处理器
    m_private->scriptMessageHandler = [[GeetestScriptHandler alloc] init];
    static_cast<GeetestScriptHandler*>(m_private->scriptMessageHandler).dialog = this;
    [contentController addScriptMessageHandler:static_cast<GeetestScriptHandler*>(m_private->scriptMessageHandler)
                                         name:@"geetestResult"];

    config.userContentController = contentController;

    // 创建 WKWebView
    WKWebView* webView = [[WKWebView alloc] initWithFrame:CGRectZero configuration:config];
    webView.translatesAutoresizingMaskIntoConstraints = NO;
    webView.navigationDelegate = nil;  // 稍后设置

    // 设置 User-Agent 模拟米游社 iOS 客户端
    [webView setCustomUserAgent:@"Mozilla/5.0 (iPhone; CPU iPhone OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148 Hyperion/2.107.0"];

    m_private->webView = webView;

    // 设置导航代理
    m_private->navigationDelegate = [[GeetestNavigationDelegate alloc] init];
    static_cast<GeetestNavigationDelegate*>(m_private->navigationDelegate).dialog = this;
    webView.navigationDelegate = static_cast<GeetestNavigationDelegate*>(m_private->navigationDelegate);

    // 将 WebView 添加到 Qt 布局
    NSView* parentView = reinterpret_cast<NSView*>(winId());
    [parentView addSubview:webView];

    // 设置约束
    [webView.leadingAnchor constraintEqualToAnchor:parentView.leadingAnchor].active = YES;
    [webView.trailingAnchor constraintEqualToAnchor:parentView.trailingAnchor].active = YES;
    [webView.topAnchor constraintEqualToAnchor:parentView.topAnchor].active = YES;
    [webView.bottomAnchor constraintEqualToAnchor:parentView.bottomAnchor].active = YES;

    [config release];
    [contentController release];
}

void GeetestDialog::setParams(const QString& gt, const QString& challenge,
                               const QString& sessionId, bool useV4)
{
    m_gt = gt;
    m_challenge = challenge;
    m_sessionId = sessionId;
    m_useV4 = useV4;
}

void GeetestDialog::setResultCallback(ResultCallback callback)
{
    m_callback = std::move(callback);
}

void GeetestDialog::startVerify()
{
    loadVerifyPage();
}

void GeetestDialog::loadVerifyPage()
{
    if (!m_private->webView || m_gt.isEmpty()) {
        m_result.success = false;
        m_result.errorMessage = QStringLiteral("参数错误");
        Q_EMIT verifyCompleted(m_result);
        if (m_callback) m_callback(m_result);
        return;
    }

    // 构建极验 GT4 验证页面 HTML
    // GT4 标准 API: https://docs.geetest.com/gt4/apirefer/browser/web
    QString html = QStringLiteral(R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>人机验证</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            background: #f5f5f5;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
        }
        .container {
            text-align: center;
            padding: 40px;
            background: white;
            border-radius: 12px;
            box-shadow: 0 2px 12px rgba(0,0,0,0.1);
        }
        .title {
            font-size: 18px;
            color: #333;
            margin-bottom: 30px;
        }
        #captcha-box {
            margin: 20px 0;
        }
        .loading {
            color: #666;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="title">请完成安全验证</div>
        <div id="captcha-box">
            <p class="loading">正在加载验证组件...</p>
        </div>
    </div>

    <script src="https://static.geetest.com/v4/gt4.js"></script>
    <script>
        var captchaId = '%1';

        initGeetest4({
            captchaId: captchaId,
            product: 'bind',
            language: 'zho'
        }, function(captcha) {
            captcha.onReady(function() {
                document.querySelector('.loading').style.display = 'none';
                captcha.showCaptcha();
            });

            captcha.onSuccess(function() {
                var result = captcha.getValidate();
                console.log('Geetest result:', JSON.stringify(result));
                if (result) {
                    var data = {
                        success: true,
                        lot_number: result.lot_number,
                        pass_token: result.pass_token,
                        gen_time: result.gen_time,
                        captcha_output: result.captcha_output,
                        captcha_id: captchaId
                    };
                    window.webkit.messageHandlers.geetestResult.postMessage(JSON.stringify(data));
                }
            });

            captcha.onError(function(e) {
                console.log('Geetest error:', e);
                var data = {
                    success: false,
                    errorMessage: e.description || '验证失败'
                };
                window.webkit.messageHandlers.geetestResult.postMessage(JSON.stringify(data));
            });

            captcha.onClose(function() {
                var data = {
                    success: false,
                    errorMessage: '用户取消验证'
                };
                window.webkit.messageHandlers.geetestResult.postMessage(JSON.stringify(data));
            });

            // 保存 captcha 实例以便后续使用
            window.gtCaptcha = captcha;
        });
    </script>
</body>
</html>
)").arg(m_gt);

    // 加载 HTML
    NSURL* baseURL = [NSURL URLWithString:@"https://static.geetest.com/"];
    NSString* htmlStr = html.toNSString();
    [m_private->webView loadHTMLString:htmlStr baseURL:baseURL];

    std::cout << "[GeetestDialog] Loading verify page with gt: "
              << m_gt.left(20).toStdString() << "..." << std::endl;
}

void GeetestDialog::parseVerifyResult(const QString& json)
{
    try {
        nlohmann::json j = nlohmann::json::parse(json.toStdString());

        m_result.success = j.value("success", false);

        if (m_result.success) {
            m_result.lotNumber = QString::fromStdString(j.value("lot_number", ""));
            m_result.passToken = QString::fromStdString(j.value("pass_token", ""));
            m_result.genTime = QString::fromStdString(j.value("gen_time", ""));
            m_result.captchaOutput = QString::fromStdString(j.value("captcha_output", ""));
            m_result.captchaId = QString::fromStdString(j.value("captcha_id", ""));

            std::cout << "[GeetestDialog] Verify success!" << std::endl;
        } else {
            m_result.errorMessage = QString::fromStdString(j.value("errorMessage", "验证失败"));
            std::cout << "[GeetestDialog] Verify failed: "
                      << m_result.errorMessage.toStdString() << std::endl;
        }
    } catch (const std::exception& e) {
        m_result.success = false;
        m_result.errorMessage = QStringLiteral("解析结果失败: ") + QString::fromStdString(e.what());
        std::cerr << "[GeetestDialog] Parse error: " << e.what() << std::endl;
    }

    Q_EMIT verifyCompleted(m_result);
    if (m_callback) m_callback(m_result);

    // 关闭对话框
    QTimer::singleShot(100, this, &QDialog::accept);
}

#endif // __APPLE__
