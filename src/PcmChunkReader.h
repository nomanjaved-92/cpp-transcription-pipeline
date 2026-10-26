#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

class PcmChunkReader
{
public:
    explicit PcmChunkReader(const std::filesystem::path& path)
        : input_(path, std::ios::binary)
    {
        if (!input_)
        {
            throw std::runtime_error("Cannot open normalized PCM audio.");
        }

        const auto bytes = std::filesystem::file_size(path);

        if (bytes == 0 || bytes % 2 != 0)
        {
            throw std::runtime_error(
                "PCM audio is empty or contains an incomplete sample.");
        }

        totalSamples_ = bytes / 2;
    }

    double durationSeconds() const
    {
        return static_cast<double>(totalSamples_) / 16000.0;
    }

    std::vector<float> readNext(int seconds)
    {
        if (seconds < 5 || seconds > 300)
        {
            throw std::runtime_error("Invalid chunk duration.");
        }

        const auto requestedBytes =
            static_cast<std::size_t>(seconds) * 16000U * 2U;

        std::vector<unsigned char> bytes(requestedBytes);

        input_.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));

        const auto count = input_.gcount();

        if (input_.bad() || (input_.fail() && !input_.eof()))
        {
            throw std::runtime_error("Failed while reading PCM audio.");
        }

        if (count % 2 != 0)
        {
            throw std::runtime_error("Incomplete PCM sample.");
        }

        if (count == 0)
        {
            if (samplesRead_ != totalSamples_)
            {
                throw std::runtime_error(
                    "PCM file length changed during processing.");
            }

            return {};
        }

        const auto sampleCount =
            static_cast<std::size_t>(count) / 2;

        std::vector<float> samples(sampleCount);

        for (std::size_t i = 0; i < sampleCount; ++i)
        {
            const unsigned int raw =
                static_cast<unsigned int>(bytes[i * 2]) |
                (static_cast<unsigned int>(bytes[i * 2 + 1]) << 8);

            const int signedSample =
                raw < 32768 ? static_cast<int>(raw)
                            : static_cast<int>(raw) - 65536;

            samples[i] =
                static_cast<float>(signedSample) / 32768.0f;
        }

        samplesRead_ += sampleCount;

        if (samplesRead_ > totalSamples_)
        {
            throw std::runtime_error(
                "PCM file length changed during processing.");
        }

        return samples;
    }

private:
    std::ifstream input_;
    std::uintmax_t totalSamples_ = 0;
    std::uintmax_t samplesRead_ = 0;
};