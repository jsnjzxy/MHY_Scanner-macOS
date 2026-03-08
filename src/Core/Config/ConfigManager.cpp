#include "ConfigManager.h"

#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <limits.h>
#endif

std::string ConfigManager::getConfigFilePath()
{
#ifdef __APPLE__
    // macOS: 获取可执行文件所在目录
    // 可执行文件在 MHY_Scanner.app/Contents/MacOS/MHY_Scanner
    // Config 放在 Contents/Resources 目录下
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0)
    {
        std::filesystem::path exePath(path);
        // 从 MacOS 目录回到 Contents，然后进入 Resources
        std::filesystem::path contentsDir = exePath.parent_path().parent_path();
        return (contentsDir / "Resources" / "Config" / "userinfo.json").string();
    }
#endif
    // 其他平台使用相对路径
    return "./Config/userinfo.json";
}

ConfigManager::ConfigManager()
{
    m_config = loadConfig();
}

ConfigManager::~ConfigManager()
{
}

ConfigManager& ConfigManager::getInstance()
{
    static ConfigManager instance;
    return instance;
}

void ConfigManager::updateConfig(const std::string& config)
{
    m_config = config;
    std::ofstream outFile(getConfigFilePath());
    outFile << nlohmann::json::parse(config).dump(4);
    outFile.close();
}

std::string ConfigManager::getConfig() const
{
    return m_config;
}

std::string ConfigManager::loadConfig()
{
    std::string configPath = getConfigFilePath();
    if (std::filesystem::exists(configPath))
    {
        const std::string& configContent = readConfigFile(configPath);
        return configContent;
    }
    else
    {
        return defaultConfig();
    }
}

std::string ConfigManager::readConfigFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (file.is_open())
    {
        const std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        std::cout << "读取配置文件成功。\n";
        return content;
    }
    else
    {
        std::cout << "无法读取配置文件。\n";
        return "";
    }
}

void ConfigManager::createDefaultConfigFile(const std::string& filePath, const std::string& Config)
{
    std::filesystem::path directory = std::filesystem::path(filePath).parent_path();
    if (!std::filesystem::exists(directory))
    {
        if (!std::filesystem::create_directories(directory))
        {
            std::cout << "无法创建配置文件夹。\n";
            exit(0);
        }
    }
    std::ofstream file(filePath);
    if (file.is_open())
    {
        file << Config;
        file.close();
        std::cout << "创建并写入默认配置文件成功。\n";
    }
    else
    {
        std::cout << "无法创建配置文件。\n";
    }
}

std::string ConfigManager::defaultConfig()
{
    const static char* defaultConfig = R"({"auto_exit": false,"auto_login":false,"auto_start": false,"continuous_scan":false,"account":[],"last_account":0,"num":0,"live_platform":0})";
    m_config = defaultConfig;
    createDefaultConfigFile(getConfigFilePath(), defaultConfig);
    return defaultConfig;
}
