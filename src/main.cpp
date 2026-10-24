#include <exception>
#include <iostream>
#include <memory>
#include <string>

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
        logger->info("Language: {}", config.language);
        logger->info("Threads: {}", config.threads);
        logger->debug("Debug logging is enabled.");
        logger->info("Configuration and logging setup complete.");

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
            // Configuration or logger initialization failed.
            std::cerr << "Startup failed: "
                      << error.what() << '\n';
        }

        return 1;
    }
}