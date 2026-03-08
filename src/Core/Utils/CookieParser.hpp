#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

class CookieParser
{
public:
    explicit CookieParser(const std::string& cookieString)
    {
        parse(cookieString);
    }

    std::optional<std::string> operator[](std::string_view key) const
    {
        auto it = m_cookies.find(std::string(key));
        if (it != m_cookies.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

private:
    void parse(const std::string& cookieString)
    {
        size_t start = 0;
        while (start < cookieString.length())
        {
            // Skip leading whitespace
            while (start < cookieString.length() && std::isspace(cookieString[start]))
            {
                start++;
            }

            // Find semicolon or end
            size_t end = cookieString.find(';', start);
            if (end == std::string::npos)
            {
                end = cookieString.length();
            }

            // Extract key-value pair
            std::string_view pair(cookieString.data() + start, end - start);

            // Find equals sign
            size_t eqPos = pair.find('=');
            if (eqPos != std::string::npos)
            {
                std::string key(pair.substr(0, eqPos));
                std::string value(pair.substr(eqPos + 1));

                // Trim whitespace
                trim(key);
                trim(value);

                if (!key.empty())
                {
                    m_cookies[std::move(key)] = std::move(value);
                }
            }

            start = end + 1;
        }
    }

    static void trim(std::string& str)
    {
        size_t start = 0;
        while (start < str.length() && std::isspace(str[start]))
        {
            start++;
        }

        size_t end = str.length();
        while (end > start && std::isspace(str[end - 1]))
        {
            end--;
        }

        if (start > 0 || end < str.length())
        {
            str = str.substr(start, end - start);
        }
    }

    std::unordered_map<std::string, std::string> m_cookies;
};
