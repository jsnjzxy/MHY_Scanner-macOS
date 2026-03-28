#include "abogus.h"

#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <dispatch/dispatch.h>
#include <pthread.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/wait.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

// nlohmann/json 由 CMake 配置 include 路径
#include <nlohmann/json.hpp>

namespace {

// ============================================================================
// 平台抽象层
// ============================================================================

#if defined(_WIN32)
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

// 获取可执行文件所在目录
std::string get_executable_dir() {
#if defined(__APPLE__)
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        return std::filesystem::path(buf).parent_path().string();
    }
#elif defined(__linux__)
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        return std::filesystem::path(buf).parent_path().string();
    }
#elif defined(_WIN32)
    char buf[MAX_PATH];
    if (GetModuleFileNameA(nullptr, buf, MAX_PATH) > 0) {
        return std::filesystem::path(buf).parent_path().string();
    }
#endif
    return "";
}

// 查找 abogus 可执行文件目录（含 get_abogus 可执行文件）
std::string find_abogus_dir() {
    // 1. 优先使用环境变量
    if (const char* env = std::getenv("MHY_ABOGUS_JS_PATH")) {
        std::string p(env);
        if (std::filesystem::exists(p + "/get_abogus")) {
            return p;
        }
    }
    // 兼容旧环境变量名
    if (const char* env = std::getenv("MHY_ABOGUS_PY_PATH")) {
        std::string p(env);
        if (std::filesystem::exists(p + "/get_abogus")) {
            return p;
        }
    }

    // 2. 相对于当前工作目录搜索
    std::filesystem::path cwd = std::filesystem::current_path();
    const char* cwd_rels[] = {
        "abogus_js",
        "abogus_cpp/abogus_js",
        "3rdparty/abogus_cpp/abogus_js",
        "../abogus_js",
        "../abogus_cpp/abogus_js",
        "../../3rdparty/abogus_cpp/abogus_js",
        "../../../3rdparty/abogus_cpp/abogus_js"
    };
    for (const auto* rel : cwd_rels) {
        auto p = (cwd / rel).string();
        if (std::filesystem::exists(p + "/get_abogus")) {
            return p;
        }
    }

    // 3. 相对于可执行文件目录搜索
    std::string exe_dir = get_executable_dir();
    if (!exe_dir.empty()) {
        const char* exe_rels[] = {
            "abogus_js",
            "abogus_cpp/abogus_js",
            "../Resources/abogus_js",  // macOS app: MacOS -> Contents/Resources/abogus_js
            "../Resources/abogus_cpp/abogus_js"
        };
        for (const auto* rel : exe_rels) {
            auto p = (std::filesystem::path(exe_dir) / rel).string();
            if (std::filesystem::exists(p + "/get_abogus")) {
                return p;
            }
        }
    }

    return "";
}

// ============================================================================
// 可执行文件调用（使用 stdin/stdout 管道通信）
// ============================================================================

#if defined(_WIN32)

// Windows 实现：使用 popen 通过管道传递 stdin
std::string run_executable_with_stdin(const std::string& execPath,
                                       const std::string& jsonInput) {
    // 写入临时文件
    std::string tmpPath = std::filesystem::temp_directory_path().string() +
                          "\\mhy_abogus_" +
                          std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                          ".json";

    {
        std::ofstream tmpFile(tmpPath, std::ios::binary);
        if (!tmpFile) return "";
        tmpFile << jsonInput;
    }

    // 执行可执行文件
    std::string resultCmd = "\"" + execPath + "\" \"" + tmpPath + "\" 2>nul";
    FILE* resultPipe = _popen(resultCmd.c_str(), "r");
    if (!resultPipe) {
        std::remove(tmpPath.c_str());
        return "";
    }

    std::string result;
    char buf[256];
    while (fgets(buf, sizeof(buf), resultPipe)) {
        result += buf;
    }
    _pclose(resultPipe);
    std::remove(tmpPath.c_str());

    // 去除尾部换行
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

#else

// Unix 实现：使用 pipe + fork + execvp，完全避免 shell
std::string run_executable_with_stdin(const std::string& execPath,
                                       const std::string& jsonInput) {
    int stdin_pipe[2];
    int stdout_pipe[2];

    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0) {
        return "";
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return "";
    }

    if (pid == 0) {
        // 子进程
        close(stdin_pipe[1]);   // 关闭写端
        close(stdout_pipe[0]);  // 关闭读端

        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);

        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        // 直接执行可执行文件
        execl(execPath.c_str(), execPath.c_str(), nullptr);

        // exec 失败
        _exit(127);
    }

    // 父进程
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    // 写入 JSON 数据到子进程 stdin
    write(stdin_pipe[1], jsonInput.c_str(), jsonInput.size());
    close(stdin_pipe[1]);

    // 读取子进程 stdout
    std::string result;
    char buf[256];
    ssize_t n;
    while ((n = read(stdout_pipe[0], buf, sizeof(buf))) > 0) {
        result.append(buf, n);
    }
    close(stdout_pipe[0]);

    // 等待子进程结束
    int status;
    waitpid(pid, &status, 0);

    // 去除尾部换行
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    return result;
}

#endif

// ============================================================================
// 高层封装
// ============================================================================

void print_abogus_hint() {
    std::filesystem::path cwd = std::filesystem::current_path();
    std::cerr << "[Douyin] abogus 查找提示:\n"
              << "  当前目录: " << cwd.string() << "\n";

    std::string exe_dir = get_executable_dir();
    if (!exe_dir.empty()) {
        std::cerr << "  可执行文件目录: " << exe_dir << "\n";
    }

    std::cerr << "解决方案:\n"
              << "  1. 将 3rdparty/abogus_cpp/abogus_js/get_abogus 复制到上述目录下\n"
              << "  2. 或设置环境变量: export MHY_ABOGUS_JS_PATH=/path/to/abogus_js\n";
}

// 通过可执行文件获取 a_bogus（无需 Node.js）
std::string get_abogus_via_executable(const char* userAgent, const char* params) {
    std::string dir = find_abogus_dir();
    if (dir.empty()) {
        print_abogus_hint();
        return "";
    }

    std::string execPath = dir + "/get_abogus";
    if (!std::filesystem::exists(execPath)) {
        std::cerr << "[Douyin] 错误: 找不到 " << execPath << "\n";
        return "";
    }

    // 构建 JSON 输入
    nlohmann::json j;
    j["params"] = params ? std::string(params) : "";
    j["user_agent"] = userAgent ? std::string(userAgent) : "";
    std::string jsonStr = j.dump();

    // 直接调用可执行文件（无需 node）
    std::string result = run_executable_with_stdin(execPath, jsonStr);

    if (result.empty() || result.substr(0, 5) == "ERROR") {
        std::cerr << "[Douyin] get_abogus 执行失败: " << result << "\n";
        return "";
    }

    return result;
}

// 单例模式：预编译的 JS 上下文缓存（未来可扩展）
class AbogusCache {
public:
    static AbogusCache& instance() {
        static AbogusCache inst;
        return inst;
    }

    // 预留：可用于缓存编译后的 JS 上下文
    // 目前每次调用仍启动新进程，但结构上支持未来优化

private:
    AbogusCache() = default;
    std::mutex mutex_;
};

} // anonymous namespace

// ============================================================================
// C 接口实现
// ============================================================================

static char* get_abogus_impl(const char* userAgent, const char* params) {
    try {
        std::string nodeResult = get_abogus_via_executable(userAgent, params);
        if (nodeResult.empty()) {
            return nullptr;
        }

        // 分配内存并复制结果
        char* result = new char[nodeResult.length() + 1];
        std::strcpy(result, nodeResult.c_str());
        return result;
    } catch (const std::exception& e) {
        std::cerr << "[Douyin] get_abogus 异常: " << e.what() << "\n";
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

extern "C" {

char* get_abogus(const char* userAgent, const char* params) {
    if (!userAgent || !params) {
        return nullptr;
    }

#if defined(__APPLE__)
    // macOS: popen/fork 在非主线程调用可能导致崩溃，派发到主线程
    if (!pthread_main_np()) {
        __block char* result = nullptr;
        dispatch_sync(dispatch_get_main_queue(), ^{
            result = get_abogus_impl(userAgent, params);
        });
        return result;
    }
#endif

    return get_abogus_impl(userAgent, params);
}

void free_abogus(char* ptr) {
    delete[] ptr;
}

} // extern "C"
