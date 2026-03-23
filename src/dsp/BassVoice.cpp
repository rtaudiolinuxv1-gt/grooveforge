#include "dsp/BassVoice.h"

#include <cmath>

namespace groove {

namespace {

constexpr float kPi = 3.14159265359f;

float midiToFrequency(int midiNote) {
    return 440.0f * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);
}

}  // namespace

void BassVoice::trigger(int midiNote, float velocity) {
    active_ = true;
    frequency_ = midiToFrequency(midiNote);
    velocity_ = velocity;
    time_ = 0.0f;
}

float BassVoice::render(float sampleRate) {
    if (!active_) {
        return 0.0f;
    }

    const float dt = 1.0f / sampleRate;
    phase_ += frequency_ * dt;
    if (phase_ >= 1.0f) {
        phase_ -= 1.0f;
    }

    const float saw = (phase_ * 2.0f) - 1.0f;
    const float sub = std::sin(2.0f * kPi * (frequency_ * 0.5f) * time_);
    const float raw = 0.75f * saw + 0.25f * sub;
    const float cutoff = 0.12f;
    filterState_ += cutoff * (raw - filterState_);
    const float env = std::exp(-time_ * 4.2f);

    time_ += dt;
    if (env < 0.001f) {
        active_ = false;
    }

    return filterState_ * env * velocity_ * 0.55f;
}

void BassVoice::reset() {
    active_ = false;
    phase_ = 0.0f;
    filterState_ = 0.0f;
    time_ = 0.0f;
}

}  // namespace groove
