#include "sample/SampleBuffer.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <sndfile.h>

namespace groove {

bool SampleBuffer::loadFromFile(const std::string& path) {
    SF_INFO info {};
    SNDFILE* file = sf_open(path.c_str(), SFM_READ, &info);
    if (!file || info.frames <= 0 || info.channels <= 0) {
        if (file) {
            sf_close(file);
        }
        return false;
    }

    std::vector<float> interleaved(static_cast<std::size_t>(info.frames) * static_cast<std::size_t>(info.channels));
    const sf_count_t framesRead = sf_readf_float(file, interleaved.data(), info.frames);
    sf_close(file);
    if (framesRead <= 0) {
        return false;
    }

    frames_.assign(static_cast<std::size_t>(framesRead), 0.0f);
    for (sf_count_t frame = 0; frame < framesRead; ++frame) {
        float mixed = 0.0f;
        for (int channel = 0; channel < info.channels; ++channel) {
            mixed += interleaved[static_cast<std::size_t>(frame) * static_cast<std::size_t>(info.channels) + static_cast<std::size_t>(channel)];
        }
        frames_[static_cast<std::size_t>(frame)] = mixed / static_cast<float>(info.channels);
    }

    sampleRate_ = static_cast<double>(info.samplerate);
    path_ = path;
    return true;
}

bool SampleBuffer::isValid() const {
    return !frames_.empty();
}

double SampleBuffer::sampleRate() const {
    return sampleRate_;
}

std::size_t SampleBuffer::frameCount() const {
    return frames_.size();
}

float SampleBuffer::sampleAt(double frameIndex) const {
    if (frames_.empty() || frameIndex < 0.0) {
        return 0.0f;
    }

    const auto lowerIndex = static_cast<std::size_t>(frameIndex);
    if (lowerIndex >= frames_.size()) {
        return 0.0f;
    }

    const auto upperIndex = std::min(lowerIndex + 1, frames_.size() - 1);
    const float fraction = static_cast<float>(frameIndex - static_cast<double>(lowerIndex));
    return frames_[lowerIndex] + (frames_[upperIndex] - frames_[lowerIndex]) * fraction;
}

const std::string& SampleBuffer::path() const {
    return path_;
}

}  // namespace groove
