#pragma once

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>

class AudioReader
{
public:
    static std::vector<float> readWav(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary | std::ios::ate);

        if (!input)
        {
            throw std::runtime_error("Cannot open audio: " + path.string());
        }

        const auto length = input.tellg();

        // Temporary limit for this whole-file reader.
        constexpr std::streamoff maxBytes = 32 * 1024 * 1024;

        if (length < 12 || length > maxBytes)
        {
            throw std::runtime_error(
                "WAV file is too small or exceeds the 32 MiB limit.");
        }

        std::vector<unsigned char> bytes(
            static_cast<std::size_t>(length));

        input.seekg(0);

        if (!input.read(
                reinterpret_cast<char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size())))
        {
            throw std::runtime_error("Cannot read complete audio file.");
        }

        const auto matches = [&](std::size_t position,
                                 const std::string& text)
        {
            return std::string(
                reinterpret_cast<const char*>(bytes.data() + position),
                text.size()) == text;
        };

        const auto read16 = [&](std::size_t position) -> std::uint16_t
        {
            return static_cast<std::uint16_t>(
                bytes[position] |
                (static_cast<unsigned int>(bytes[position + 1]) << 8));
        };

        const auto read32 = [&](std::size_t position) -> std::uint32_t
        {
            return static_cast<std::uint32_t>(bytes[position]) |
                (static_cast<std::uint32_t>(bytes[position + 1]) << 8) |
                (static_cast<std::uint32_t>(bytes[position + 2]) << 16) |
                (static_cast<std::uint32_t>(bytes[position + 3]) << 24);
        };

        if (!matches(0, "RIFF") || !matches(8, "WAVE"))
        {
            throw std::runtime_error("Input is not a RIFF WAV file.");
        }

        const auto riffEnd =
            static_cast<std::uint64_t>(read32(4)) + 8;

        if (riffEnd < 12 || riffEnd > bytes.size())
        {
            throw std::runtime_error("Invalid or truncated WAV container.");
        }

        const auto end = static_cast<std::size_t>(riffEnd);

        bool formatFound = false;
        bool dataFound = false;
        std::size_t dataOffset = 0;
        std::size_t dataSize = 0;
        std::size_t position = 12;

        // WAV files may contain metadata chunks before the audio.
        while (position < end)
        {
            if (end - position < 8)
            {
                throw std::runtime_error("Incomplete WAV chunk header.");
            }

            const auto size =
                static_cast<std::size_t>(read32(position + 4));

            const auto payload = position + 8;

            if (size > end - payload)
            {
                throw std::runtime_error("Truncated WAV chunk.");
            }

            if (matches(position, "fmt "))
            {
                if (formatFound || size < 16)
                {
                    throw std::runtime_error("Invalid WAV format chunk.");
                }

                if (read16(payload) != 1 ||
                    read16(payload + 2) != 1 ||
                    read32(payload + 4) != 16000 ||
                    read32(payload + 8) != 32000 ||
                    read16(payload + 12) != 2 ||
                    read16(payload + 14) != 16)
                {
                    throw std::runtime_error(
                        "Expected 16 kHz mono 16-bit PCM WAV. "
                        "Convert the input with FFmpeg first.");
                }

                formatFound = true;
            }
            else if (matches(position, "data"))
            {
                if (dataFound)
                {
                    throw std::runtime_error(
                        "Multiple WAV data chunks are not supported.");
                }

                dataFound = true;
                dataOffset = payload;
                dataSize = size;
            }

            position = payload + size;

            // RIFF chunks with odd lengths have one padding byte.
            if (size % 2 != 0)
            {
                if (position >= end)
                {
                    throw std::runtime_error("Missing WAV padding byte.");
                }

                ++position;
            }
        }

        if (!formatFound || !dataFound ||
            dataSize == 0 || dataSize % 2 != 0)
        {
            throw std::runtime_error("Missing or invalid WAV audio data.");
        }

        std::vector<float> samples(dataSize / 2);

        for (std::size_t i = 0; i < samples.size(); ++i)
        {
            const auto raw = read16(dataOffset + i * 2);

            const int signedSample =
                raw < 32768 ? static_cast<int>(raw)
                            : static_cast<int>(raw) - 65536;

            samples[i] = static_cast<float>(signedSample) / 32768.0f;
        }

        return samples;
    }
};