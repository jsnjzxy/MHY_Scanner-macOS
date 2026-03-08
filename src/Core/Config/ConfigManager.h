#pragma once

#include <string>

class ConfigManager final
{
public:
    static ConfigManager& getInstance();
    void updateConfig(const std::string& config);
    std::string getConfig() const;
    std::string defaultConfig();

private:
    ConfigManager();
    ~ConfigManager();
    std::string m_config;
    ConfigManager(const ConfigManager& config) = delete;
    const ConfigManager& operator=(const ConfigManager& config) = delete;
    std::string loadConfig();
    std::string readConfigFile(const std::string& filePath);
    void createDefaultConfigFile(const std::string& filePath, const std::string& Config);
    static std::string getConfigFilePath();
};