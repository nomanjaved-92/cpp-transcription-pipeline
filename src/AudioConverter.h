#pragma once

#include <cerrno>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <process.h>

class AudioConverter
{
public:
    AudioConverter()
    {
        const auto root = std::filesystem::temp_directory_path();
        std::random_device random;

        for (int attempt = 0; attempt < 20; ++attempt)
        {
            const auto candidate =
                root / ("transcription-" + std::to_string(random()) +
                        "-" + std::to_string(random()));

            if (std::filesystem::create_directory(candidate))
            {
                directory_ = candidate;
                return;
            }
        }

        throw std::runtime_error(
            "Could not create a temporary audio directory.");
    }

    ~AudioConverter()
    {
        // Cleanup must not throw during exception handling.
        std::error_code ignored;
        std::filesystem::remove_all(directory_, ignored);
    }

    AudioConverter(const AudioConverter&) = delete;
    AudioConverter& operator=(const AudioConverter&) = delete;

    std::filesystem::path convert(
        const std::filesystem::path& executable,
        const std::filesystem::path& input) const
    {
        if (!std::filesystem::is_regular_file(executable))
        {
            throw std::runtime_error(
                "FFmpeg executable not found: " + executable.string());
        }

        if (!std::filesystem::is_regular_file(input))
        {
            throw std::runtime_error(
                "Audio file not found: " + input.string());
        }

        const auto executablePath =
            std::filesystem::absolute(executable);

        const auto inputPath =
            std::filesystem::absolute(input);

        const auto outputPath = directory_ / "normalized.wav";

        // _wspawnv launches the executable directly.
        // File arguments are quoted to preserve spaces in paths.
        const std::vector<std::wstring> arguments = {
            quotePath(executablePath),
            L"-hide_banner",
            L"-loglevel", L"error",
            L"-nostdin",
            L"-n",
            L"-i", quotePath(inputPath),
            L"-map", L"0:a:0",
            L"-ac", L"1",
            L"-ar", L"16000",
            L"-c:a", L"pcm_s16le",
            L"-f", L"wav",
            quotePath(outputPath)
        };

        std::vector<const wchar_t*> pointers;

        for (const auto& argument : arguments)
        {
            pointers.push_back(argument.c_str());
        }

        pointers.push_back(nullptr);

        const auto status = _wspawnv(
            _P_WAIT,
            executablePath.c_str(),
            pointers.data());

        if (status == -1)
        {
            const int error = errno;

            throw std::runtime_error(
                "Could not start FFmpeg: " +
                std::error_code(error, std::generic_category()).message());
        }

        if (status != 0)
        {
            throw std::runtime_error(
                "FFmpeg conversion failed with exit code: " +
                std::to_string(status));
        }

        if (!std::filesystem::is_regular_file(outputPath))
        {
            throw std::runtime_error(
                "FFmpeg did not create the converted audio.");
        }

        return outputPath;
    }

private:
    static std::wstring quotePath(const std::filesystem::path& path)
    {
        const auto value = path.wstring();

        if (value.empty() ||
            value.find(L'"') != std::wstring::npos ||
            value.back() == L'\\')
        {
            throw std::runtime_error("Invalid file path.");
        }

        return L"\"" + value + L"\"";
    }

    std::filesystem::path directory_;
};