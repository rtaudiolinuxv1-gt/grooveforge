#include "sample/SampleVoice.h"

namespace groove {

void SampleVoice::trigger(std::shared_ptr<const SampleBuffer> sample, float velocity, double playbackRate) {
    sample_ = std::move(sample);
    velocity_ = velocity;
    position_ = 0.0;
    increment_ = playbackRate;
    active_ = sample_ && sample_->isValid();
}

float SampleVoice::render() {
    if (!active_ || !sample_) {
        return 0.0f;
    }

    const float sample = sample_->sampleAt(position_) * velocity_;
    position_ += increment_;
    if (position_ >= static_cast<double>(sample_->frameCount())) {
        active_ = false;
    }
    return sample;
}

void SampleVoice::reset() {
    sample_.reset();
    position_ = 0.0;
    increment_ = 1.0;
    velocity_ = 0.0f;
    active_ = false;
}

}  // namespace groove
