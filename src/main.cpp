#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <whisper.h>

#include <algorithm>
#include <cctype>

#include "ConfigurationManager.h"
#include "Logger.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "AudioReader.h"
#include "AudioConverter.h"

int main(int argc, char* argv[])
{

   if (argc < 3 || argc > 4)
   {
       std::cerr
           << "Usage: transcription_app <audio-file> <output.json> "
           << "[config-file]\n";
        return 1;
   }

    std::shared_ptr<spdlog::logger> logger;

    try
    {
     const std::string audioPath = argv[1];
     const std::filesystem::path outputPath = argv[2];

     const std::string configPath =
     argc == 4 ? argv[3] : "config/config.json";

     if (std::filesystem::exists(outputPath))
     {
         throw std::runtime_error(
             "Output already exists. Choose a new output filename.");
     }

        const AppConfig config =
            ConfigurationManager::load(configPath);

        logger = Logger::create(config.logging);
        logger->info("Configuration loaded successfully.");

        if (!std::filesystem::is_regular_file(audioPath))
        {
            throw std::runtime_error(
                "Audio file not found: " + audioPath);
        }

        if (std::filesystem::file_size(audioPath) == 0)
        {
            throw std::runtime_error("Audio file is empty.");
        }

        auto extension =
            std::filesystem::path(audioPath).extension().string();

        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            });

        const auto found = std::find(
            config.allowedExtensions.begin(),
            config.allowedExtensions.end(),
            extension);

        if (found == config.allowedExtensions.end())
        {
            throw std::runtime_error(
                "Audio extension is not allowed by configuration: " +
                extension);
        }

        logger->info("Input validation passed.");

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

        if (whisper_lang_id(config.language.c_str()) < 0)
        {
            throw std::runtime_error(
                "Unsupported language: " + config.language);
        }

        if (!whisper_is_multilingual(context.get()) &&
            config.language != "en")
        {
            throw std::runtime_error(
                "This model supports English only.");
        }

        logger->info("Converting input audio to normalized WAV.");

        AudioConverter converter;

        const auto normalizedPath =
            converter.convert(config.ffmpegPath, audioPath);

        const auto samples = AudioReader::readWav(normalizedPath);

        logger->info("Audio conversion completed.");

        logger->info(
            "Audio loaded: {:.2f} seconds.",
            static_cast<double>(samples.size()) / 16000.0);

        auto options =
            whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

        options.n_threads = config.threads;
        options.language = config.language.c_str();
        options.translate = false;
        options.no_context = true;
        options.no_timestamps = false;
        options.print_realtime = false;
        options.print_progress = false;
        options.print_timestamps = false;
        options.print_special = false;

        logger->info("Transcription started.");

        const int status = whisper_full(
            context.get(),
            options,
            samples.data(),
            static_cast<int>(samples.size()));

        if (status != 0)
        {
            throw std::runtime_error(
                "Whisper transcription failed with code: " +
                std::to_string(status));
        }

        nlohmann::json segments = nlohmann::json::array();
        std::string fullText;

        const int segmentCount =
            whisper_full_n_segments(context.get());

        for (int i = 0; i < segmentCount; ++i)
        {
            const char* segmentText =
                whisper_full_get_segment_text(context.get(), i);

            if (!segmentText)
            {
                throw std::runtime_error("Whisper returned null segment text.");
            }

            std::string text(segmentText);

            // Trim surrounding whitespace without changing recognized words.
            const auto first = text.find_first_not_of(" \t\r\n");

            if (first == std::string::npos)
            {
                continue;
            }

            const auto last = text.find_last_not_of(" \t\r\n");
            text = text.substr(first, last - first + 1);

            // Whisper timestamps use units of 10 milliseconds.
            const double start =
                static_cast<double>(
                    whisper_full_get_segment_t0(context.get(), i)) / 100.0;

            const double end =
                static_cast<double>(
                    whisper_full_get_segment_t1(context.get(), i)) / 100.0;

            segments.push_back({
                {"start", start},
                {"end", end},
                {"text", text}
            });

            if (!fullText.empty())
            {
                fullText += ' ';
            }

            fullText += text;
        }

        const nlohmann::json result = {
            {"language", config.language},
            {"duration_seconds",
                static_cast<double>(samples.size()) / 16000.0},
            {"text", fullText},
            {"segments", segments}
        };

        if (outputPath.has_parent_path())
        {
            std::filesystem::create_directories(
                outputPath.parent_path());
        }

        std::ofstream output(outputPath, std::ios::binary);

        if (!output)
        {
            throw std::runtime_error("Cannot open output file.");
        }

        output << result.dump(2) << '\n';
        output.close();

        if (!output)
        {
            throw std::runtime_error("Failed to write complete transcript.");
        }

        logger->info(
            "Transcription complete: {} segments saved.",
            segments.size());

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