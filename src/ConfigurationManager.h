#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <vector>

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
    std::string modelPath = "models/ggml-base.en.bin";
    std::string ffmpegPath = "D:/Tools/ffmpeg/bin/ffmpeg.exe";
    std::vector<std::string> allowedExtensions{
    ".wav", ".mp3", ".m4a", ".flac"
    };
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
        config.modelPath = json.value("model_path", config.modelPath);

        if (config.modelPath.empty())
        {
           throw std::runtime_error(
               "'model_path' must not be empty.");
        }
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
        config.ffmpegPath = json.value("ffmpeg_path", config.ffmpegPath);

        if (config.ffmpegPath.empty())
        {
            throw std::runtime_error("'ffmpeg_path' must not be empty.");
        }

        if (json.contains("allowed_extensions"))
        {
            const auto& extensions = json.at("allowed_extensions");

            if (!extensions.is_array() || extensions.empty())
            {
                throw std::runtime_error(
                    "'allowed_extensions' must be a non-empty array.");
            }

            config.allowedExtensions.clear();

            for (const auto& item : extensions)
            {
                if (!item.is_string())
                {
                    throw std::runtime_error(
                        "Each allowed extension must be a string.");
                }

                auto extension = item.get<std::string>();

                std::transform(
                    extension.begin(),
                    extension.end(),
                    extension.begin(),
                    [](unsigned char character)
                    {
                        return static_cast<char>(std::tolower(character));
                    });

                if (extension.size() < 2 || extension.front() != '.')
                {
                    throw std::runtime_error(
                        "Extensions must start with a dot, for example '.mp3'.");
                }

                const bool valid = std::all_of(
                    extension.begin() + 1,
                    extension.end(),
                    [](unsigned char character)
                    {
                        return (character >= 'a' && character <= 'z') ||
                            (character >= '0' && character <= '9');
                    });

                if (!valid)
                {
                    throw std::runtime_error(
                        "Invalid audio extension: " + extension);
                }

                if (std::find(
                    config.allowedExtensions.begin(),
                    config.allowedExtensions.end(),
                    extension) == config.allowedExtensions.end())
                {
                    config.allowedExtensions.push_back(extension);
                }
            }
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