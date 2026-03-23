#include "recording/AudioRecorder.h"

#include <algorithm>
#include <chrono>

namespace groove {

AudioRecorder::AudioRecorder() = default;

AudioRecorder::~AudioRecorder() {
    stop();
}

bool AudioRecorder::start(const std::string& path, AudioFileFormat format, int sampleRate) {
    stop();

    info_ = {};
    info_.samplerate = sampleRate;
    info_.channels = 2;
    info_.format = formatToSndfile(format);
    file_ = sf_open(path.c_str(), SFM_WRITE, &info_);
    if (!file_) {
        return false;
    }

    path_ = path;
    capacityFrames_ = static_cast<std::size_t>(sampleRate) * 30;
    ringBuffer_.assign(capacityFrames_ * 2, 0.0f);
    readFrame_.store(0);
    writeFrame_.store(0);
    stopRequested_.store(false);
    recording_.store(true);
    writerThread_ = std::thread(&AudioRecorder::writerLoop, this);
    return true;
}

void AudioRecorder::stop() {
    stopRequested_.store(true);
    recording_.store(false);

    if (writerThread_.joinable()) {
        writerThread_.join();
    }

    if (file_) {
        sf_write_sync(file_);
        sf_close(file_);
        file_ = nullptr;
    }
}

bool AudioRecorder::isRecording() const {
    return recording_.load();
}

std::string AudioRecorder::path() const {
    return path_;
}

void AudioRecorder::pushBlock(const float* left, const float* right, std::size_t frameCount) {
    if (!recording_.load() || !file_ || capacityFrames_ == 0) {
        return;
    }

    const std::size_t readFrame = readFrame_.load(std::memory_order_acquire);
    const std::size_t writeFrame = writeFrame_.load(std::memory_order_relaxed);
    const std::size_t usedFrames = writeFrame - readFrame;
    const std::size_t freeFrames = capacityFrames_ > usedFrames ? capacityFrames_ - usedFrames : 0;
    const std::size_t framesToWrite = std::min(frameCount, freeFrames);
    if (framesToWrite == 0) {
        return;
    }

    for (std::size_t frame = 0; frame < framesToWrite; ++frame) {
        const std::size_t target = (writeFrame + frame) % capacityFrames_;
        ringBuffer_[target * 2] = left[frame];
        ringBuffer_[target * 2 + 1] = right[frame];
    }

    writeFrame_.store(writeFrame + framesToWrite, std::memory_order_release);
}

void AudioRecorder::writerLoop() {
    std::vector<float> staging(4096 * 2, 0.0f);

    while (!stopRequested_.load() || readFrame_.load(std::memory_order_acquire) < writeFrame_.load(std::memory_order_acquire)) {
        const std::size_t readFrame = readFrame_.load(std::memory_order_relaxed);
        const std::size_t writeFrame = writeFrame_.load(std::memory_order_acquire);
        const std::size_t availableFrames = writeFrame - readFrame;
        if (availableFrames == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        const std::size_t chunkFrames = std::min<std::size_t>(availableFrames, 4096);
        for (std::size_t frame = 0; frame < chunkFrames; ++frame) {
            const std::size_t source = (readFrame + frame) % capacityFrames_;
            staging[frame * 2] = ringBuffer_[source * 2];
            staging[frame * 2 + 1] = ringBuffer_[source * 2 + 1];
        }

        sf_writef_float(file_, staging.data(), static_cast<sf_count_t>(chunkFrames));
        readFrame_.store(readFrame + chunkFrames, std::memory_order_release);
    }
}

int AudioRecorder::formatToSndfile(AudioFileFormat format) const {
    switch (format) {
    case AudioFileFormat::Wav:
        return SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    case AudioFileFormat::Flac:
        return SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
    }
    return SF_FORMAT_WAV | SF_FORMAT_PCM_16;
}

}  // namespace groove
