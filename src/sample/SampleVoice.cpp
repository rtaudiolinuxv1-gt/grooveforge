#include "sample/SampleVoice.h"

#include <algorithm>
#include <cmath>

namespace groove {

void SampleVoice::trigger(std::shared_ptr<const SampleBuffer> sample, const Step& step, float gateSeconds, double playbackRate) {
    sample_ = std::move(sample);
    velocity_ = std::clamp(step.velocity * step.volume, 0.0f, 1.5f);
    position_ = 0.0;
    increment_ = playbackRate;
    time_ = 0.0f;
    attackSeconds_ = std::clamp(step.attack, 0.0005f, 2.0f);
    decaySeconds_ = std::clamp(step.decay, 0.0005f, 4.0f);
    sustainLevel_ = std::clamp(step.sustain, 0.0f, 1.0f);
    releaseSeconds_ = std::clamp(step.release, 0.0005f, 4.0f);
    holdSeconds_ = std::max(0.0f, gateSeconds);
    active_ = sample_ && sample_->isValid();
}

float SampleVoice::render() {
    if (!active_ || !sample_) {
        return 0.0f;
    }

    const float sample = sample_->sampleAt(position_) * velocity_ * envelopeLevel();
    position_ += increment_;
    time_ += static_cast<float>(increment_ / static_cast<double>(sample_->sampleRate()));
    if (position_ >= static_cast<double>(sample_->frameCount())) {
        active_ = false;
    }
    if (envelopeLevel() < 0.001f) {
        active_ = false;
    }
    return sample;
}

void SampleVoice::reset() {
    sample_.reset();
    position_ = 0.0;
    increment_ = 1.0;
    velocity_ = 0.0f;
    time_ = 0.0f;
    attackSeconds_ = 0.002f;
    decaySeconds_ = 0.120f;
    sustainLevel_ = 0.0f;
    releaseSeconds_ = 0.080f;
    holdSeconds_ = 0.0f;
    active_ = false;
}

float SampleVoice::envelopeLevel() const {
    const float attack = std::max(attackSeconds_, 0.0005f);
    const float decay = std::max(decaySeconds_, 0.0005f);
    const float release = std::max(releaseSeconds_, 0.0005f);
    const float sustain = std::clamp(sustainLevel_, 0.0f, 1.0f);
    const float hold = std::max(holdSeconds_, 0.0f);

    float localTime = time_;
    if (localTime < attack) {
        return localTime / attack;
    }

    localTime -= attack;
    if (localTime < decay) {
        const float progress = localTime / decay;
        return 1.0f + (sustain - 1.0f) * progress;
    }

    localTime -= decay;
    if (localTime < hold) {
        return sustain;
    }

    localTime -= hold;
    return sustain * std::exp(-5.0f * (localTime / release));
}

}  // namespace groove
