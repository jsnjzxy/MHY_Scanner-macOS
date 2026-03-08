#include "Main/MainWindow.h"
#include "Widgets/StyleManager.h"

#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <csignal>
#include <unistd.h>
#include <sys/file.h>
#include <locale.h>
#include <cstring>

#include <QMessageBox>
#include <QStyleHints>
#include <QFile>
#include <QDir>
#include <opencv2/core/utils/logger.hpp>

static std::mutex mtx;

// macOS 使用临时目录的锁文件
static std::string getMutexFilePath()
{
    QString path = QDir::tempPath() + "/MHY_Scanner.lock";
    return path.toStdString();
}

static bool isOpen()
{
    // 使用文件锁实现单实例
    static int lockFile = -1;
    std::string lockPath = getMutexFilePath();

    lockFile = open(lockPath.c_str(), O_CREAT | O_RDWR, 0666);
    if (lockFile == -1) {
        return false;
    }

    // 尝试获取排他锁
    int result = flock(lockFile, LOCK_EX | LOCK_NB);
    return result == -1; // 如果锁失败，说明已有实例在运行
}

// macOS 崩溃处理
static void signalHandler(int signum)
{
    // Signal handler 中只能使用 async-signal-safe 函数
    // 不能使用 mutex, Qt 类, malloc, Exit 等
    const char* msg = "MHY_Scanner crashed with signal: ";
    write(STDERR_FILENO, msg, strlen(msg));

    // 将信号编号转为字符串
    char sigStr[16];
    int len = 0;
    int s = signum;
    if (s == 0) {
        sigStr[len++] = '0';
    } else {
        char tmp[16];
        int tmpLen = 0;
        while (s > 0) { tmp[tmpLen++] = '0' + (s % 10); s /= 10; }
        for (int i = tmpLen - 1; i >= 0; i--) sigStr[len++] = tmp[i];
    }
    sigStr[len++] = '\n';
    write(STDERR_FILENO, sigStr, len);

    // 重置为默认处理并重新触发，避免死锁
    signal(signum, SIG_DFL);
    raise(signum);
}

int main(int argc, char* argv[])
{
    // 设置 UTF-8 编码
    setlocale(LC_ALL, "en_US.UTF-8");

    // 设置崩溃信号处理
    signal(SIGABRT, signalHandler);
    signal(SIGSEGV, signalHandler);
    signal(SIGBUS, signalHandler);
    signal(SIGFPE, signalHandler);

    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

    QApplication a(argc, argv);

    // 应用统一样式
    a.setStyleSheet(StyleManager::instance().getGlobalStyleSheet());

    QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);

    // 设置应用属性
    a.setApplicationName("MHY_Scanner");
    a.setApplicationVersion("1.1.15");
    a.setOrganizationName("MHY");

    if (isOpen())
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("警告");
        msgBox.setText("MHY_Scanner 已经在运行中。\n\n是否启动新的实例？");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);

        if (msgBox.exec() != QMessageBox::Yes) {
            return 0;
        }
    }

    MainWindow w;
    w.show();

    return a.exec();
}
