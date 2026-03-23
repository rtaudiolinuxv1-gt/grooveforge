#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include <sndfile.h>

#include "core/GrooveTypes.h"

namespace groove {

class AudioRecorder {
public:
    AudioRecorder();
    ~AudioRecorder();

    bool start(const std::string& path, AudioFileFormat format, int sampleRate);
    void stop();

    bool isRecording() const;
    std::string path() const;
    void pushBlock(const float* left, const float* right, std::size_t frameCount);

private:
    void writerLoop();
    int formatToSndfile(AudioFileFormat format) const;

    SNDFILE* file_ = nullptr;
    SF_INFO info_ {};
    std::string path_;

    std::vector<float> ringBuffer_;
    std::atomic<std::size_t> readFrame_ {0};
    std::atomic<std::size_t> writeFrame_ {0};
    std::size_t capacityFrames_ = 0;

    std::atomic<bool> recording_ {false};
    std::atomic<bool> stopRequested_ {false};
    std::thread writerThread_;
};

}  // namespace groove
