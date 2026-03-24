cmake_minimum_required(VERSION 3.16)

project(RTAudioSeq VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets Multimedia)
pkg_check_modules(JACK REQUIRED IMPORTED_TARGET jack)
pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)
pkg_check_modules(FLUIDSYNTH REQUIRED IMPORTED_TARGET fluidsynth)

enable_testing()

set(RTAUDIOSEQ_SOURCES
    src/main.cpp
    src/app/GrooveController.cpp
    src/audio/JackEngine.cpp
    src/export/OfflineRenderer.cpp
    src/generation/PatternGenerator.cpp
    src/recording/AudioRecorder.cpp
    src/sample/SampleBuffer.cpp
    src/sample/SampleVoice.cpp
    src/sf2/SoundFontSynth.cpp
    src/ui/MainWindow.cpp
)

add_executable(RTAudioSeq ${RTAUDIOSEQ_SOURCES})
target_include_directories(RTAudioSeq PRIVATE src)
target_link_libraries(RTAudioSeq PRIVATE Qt5::Widgets Qt5::Multimedia PkgConfig::JACK PkgConfig::SNDFILE PkgConfig::FLUIDSYNTH Threads::Threads)

add_executable(pattern_generator_tests
    tests/PatternGeneratorTests.cpp
    src/generation/PatternGenerator.cpp
)
target_include_directories(pattern_generator_tests PRIVATE src)

add_test(NAME pattern_generator_tests COMMAND pattern_generator_tests)
