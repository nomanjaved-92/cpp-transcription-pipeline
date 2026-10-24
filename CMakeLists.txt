cmake_minimum_required(VERSION 3.20)

project(
    TranscriptionPipeline
    VERSION 0.1.0
    LANGUAGES CXX
)

include(FetchContent)

# Fixed versions keep dependency selection consistent.
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(json spdlog)

add_executable(transcription_app
    src/main.cpp
)

set_target_properties(transcription_app PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(transcription_app PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

if(MSVC)
    target_compile_options(transcription_app PRIVATE /W4 /permissive-)
else()
    target_compile_options(transcription_app PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()