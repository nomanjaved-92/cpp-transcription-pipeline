#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

struct LoggingConfig
{
    std::string level = "info";
    std::string file = "logs/transcription.log";
    int maxSizeMb = 5;
    int maxFiles = 3;
};

struct AppConfig
{
    std::string language = "en";
    int threads = 4;
    LoggingConfig logging;
};

class ConfigurationManager
{
public:
    static AppConfig load(const std::string& path)
    {
        std::ifstream input(path);

        if (!input)
        {
            throw std::runtime_error(
                "Cannot open configuration file: " + path);
        }

        const auto json = nlohmann::json::parse(input);

        if (!json.is_object())
        {
            throw std::runtime_error(
                "Configuration must be a JSON object.");
        }

        AppConfig config;

        config.language = json.value("language", config.language);
        config.threads = readInteger(
            json, "threads", config.threads, 1, 64);

        if (json.contains("logging"))
        {
            const auto& logging = json.at("logging");

            if (!logging.is_object())
            {
                throw std::runtime_error(
                    "'logging' must be a JSON object.");
            }

            config.logging.level =
                logging.value("level", config.logging.level);

            config.logging.file =
                logging.value("file", config.logging.file);

            config.logging.maxSizeMb = readInteger(
                logging, "max_size_mb", 5, 1, 100);

            config.logging.maxFiles = readInteger(
                logging, "max_files", 3, 1, 20);
        }

        if (config.language.empty())
        {
            throw std::runtime_error(
                "'language' must not be empty.");
        }

        if (config.logging.file.empty())
        {
            throw std::runtime_error(
                "'logging.file' must not be empty.");
        }

        const auto& level = config.logging.level;

        if (level != "trace" && level != "debug" &&
            level != "info" && level != "warn" &&
            level != "error" && level != "critical" &&
            level != "off")
        {
            throw std::runtime_error(
                "Invalid logging level: " + level);
        }

        return config;
    }

private:
    static int readInteger(
        const nlohmann::json& object,
        const std::string& key,
        int defaultValue,
        int minimum,
        int maximum)
    {
        if (!object.contains(key))
        {
            return defaultValue;
        }

        const auto& value = object.at(key);

        if (!value.is_number_integer() ||
            value < minimum || value > maximum)
        {
            throw std::runtime_error(
                "'" + key + "' must be an integer between " +
                std::to_string(minimum) + " and " +
                std::to_string(maximum) + ".");
        }

        return value.get<int>();
    }
};