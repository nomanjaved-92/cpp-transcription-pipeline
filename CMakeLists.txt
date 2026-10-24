cmake_minimum_required(VERSION 3.20)

project(
    TranscriptionPipeline
    VERSION 0.1.0
    LANGUAGES CXX
)

add_executable(transcription_app
    src/main.cpp
)

set_target_properties(transcription_app PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if(MSVC)
    target_compile_options(transcription_app PRIVATE /W4 /permissive-)
else()
    target_compile_options(transcription_app PRIVATE -Wall -Wextra -Wpedantic)
endif()