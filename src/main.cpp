#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <whisper.h>

#include "ConfigurationManager.h"
#include "Logger.h"

int main(int argc, char* argv[])
{
    if (argc > 2)
    {
        std::cerr
            << "Usage: transcription_app [config-file]\n";
        return 1;
    }

    std::shared_ptr<spdlog::logger> logger;

    try
    {
        const std::string configPath =
            argc == 2 ? argv[1] : "config/config.json";

        const AppConfig config =
            ConfigurationManager::load(configPath);

        logger = Logger::create(config.logging);
        logger->info("Configuration loaded successfully.");

        if (!std::filesystem::is_regular_file(config.modelPath))
        {
            throw std::runtime_error(
                "Model file not found: " + config.modelPath);
        }

        logger->info("Loading Whisper model...");

        auto parameters = whisper_context_default_params();
        parameters.use_gpu = false;

        // Automatically releases the model, including on exceptions.
        using WhisperContext =
            std::unique_ptr<whisper_context, decltype(&whisper_free)>;

        WhisperContext context(
            whisper_init_from_file_with_params(
                config.modelPath.c_str(), parameters),
            &whisper_free);

        if (!context)
        {
            throw std::runtime_error(
                "Whisper could not load the model.");
        }

        logger->info("Whisper model loaded successfully.");
        logger->info("C++ library integration check passed.");

        logger->flush();
        return 0;
    }
    catch (const std::exception& error)
    {
        if (logger)
        {
            logger->error("Application failed: {}", error.what());
            logger->flush();
        }
        else
        {
            std::cerr << "Startup failed: "
                      << error.what() << '\n';
        }

        return 1;
    }
}