cmake_minimum_required(VERSION 3.16)
project(AudioCapture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific setup
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_definitions(-DPLATFORM_LINUX)
endif()

# Include directories
include_directories(include)

set(SOURCES
    src/main.cpp
    src/AudioCapture.cpp
    src/LoopbackCapture.cpp
    src/MicCapture.cpp
    src/WavWriter.cpp
    src/Utils.cpp
)

set(HEADERS
    include/AudioCapture.h
    include/LoopbackCapture.h
    include/MicCapture.h
    include/WavWriter.h
    include/Utils.h
)

add_executable(audio-capture ${SOURCES} ${HEADERS})

if(WIN32)
    target_link_libraries(audio-capture 
        ole32 
        oleaut32
        uuid
        winmm
    )
endif()