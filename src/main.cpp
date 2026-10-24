#include <exception>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

int main()
{
    try
    {
        spdlog::set_pattern("[%H:%M:%S] [%l] %v");

        spdlog::info("Transcription pipeline started.");

        // Temporary in-memory configuration to verify JSON support.
        const nlohmann::json config = {
            {"application_name", "TranscriptionPipeline"},
            {"language", "en"},
            {"threads", 4}
        };

        const auto applicationName =
            config.at("application_name").get<std::string>();

        const auto language =
            config.at("language").get<std::string>();

        const auto threads =
            config.at("threads").get<int>();

        spdlog::info("Application: {}", applicationName);
        spdlog::info("Language: {}", language);
        spdlog::info("Threads: {}", threads);
        spdlog::info("JSON and logging dependencies are working.");

        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Startup failed: " << error.what() << '\n';
        return 1;
    }
}