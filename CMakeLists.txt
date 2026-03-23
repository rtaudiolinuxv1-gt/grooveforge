cmake_minimum_required(VERSION 3.16)

project(groove_forge VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)
pkg_check_modules(JACK REQUIRED IMPORTED_TARGET jack)
pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)

enable_testing()

set(GROOVE_FORGE_SOURCES
    src/main.cpp
    src/app/GrooveController.cpp
    src/audio/JackEngine.cpp
    src/dsp/BassVoice.cpp
    src/dsp/DrumVoice.cpp
    src/generation/PatternGenerator.cpp
    src/recording/AudioRecorder.cpp
    src/sample/SampleBuffer.cpp
    src/sample/SampleVoice.cpp
    src/ui/MainWindow.cpp
)

add_executable(groove_forge ${GROOVE_FORGE_SOURCES})
target_include_directories(groove_forge PRIVATE src)
target_link_libraries(groove_forge PRIVATE Qt5::Widgets PkgConfig::JACK PkgConfig::SNDFILE Threads::Threads)

add_executable(pattern_generator_tests
    tests/PatternGeneratorTests.cpp
    src/generation/PatternGenerator.cpp
)
target_include_directories(pattern_generator_tests PRIVATE src)

add_test(NAME pattern_generator_tests COMMAND pattern_generator_tests)
