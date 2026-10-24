#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "ConfigurationManager.h"

class Logger
{
public:
    static std::shared_ptr<spdlog::logger> create(
        const LoggingConfig& config)
    {
        const std::filesystem::path logPath(config.file);

        if (logPath.has_parent_path())
        {
            std::filesystem::create_directories(
                logPath.parent_path());
        }

        auto consoleSink =
            std::make_shared<spdlog::sinks::stderr_color_sink_mt>();

        const auto maxBytes =
            static_cast<std::size_t>(config.maxSizeMb)
            * 1024U * 1024U;

        auto fileSink =
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                config.file,
                maxBytes,
                static_cast<std::size_t>(config.maxFiles));

        std::vector<spdlog::sink_ptr> sinks{
            consoleSink,
            fileSink
        };

        auto logger = std::make_shared<spdlog::logger>(
            "transcription", sinks.begin(), sinks.end());

        logger->set_level(
            spdlog::level::from_str(config.level));

        logger->set_pattern(
            "[%Y-%m-%d %H:%M:%S] [%l] %v");

        logger->flush_on(spdlog::level::err);

        return logger;
    }
};