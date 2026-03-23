#include "dsp/InternalVoice.h"

#include <algorithm>
#include <cmath>

namespace groove {

namespace {

constexpr float kPi = 3.14159265359f;

float envelope(float time, float decay) {
    return std::exp(-time * decay);
}

float softClip(float sample) {
    return std::tanh(sample);
}

}  // namespace

InternalVoice::InternalVoice(InstrumentRole role) : role_(role) {}

void InternalVoice::setRole(InstrumentRole role) {
    role_ = role;
}

void InternalVoice::trigger(int midiNote, float velocity) {
    active_ = true;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);
    frequency_ = midiToFrequency(midiNote);
    time_ = 0.0f;
    phaseA_ = 0.0f;
    if ((role_ == InstrumentRole::Bass) || (role_ == InstrumentRole::Lead)) {
        return;
    }
    phaseB_ = 0.0f;
}

float InternalVoice::render(float sampleRate) {
    if (active_ == false) {
        return 0.0f;
    }

    const float dt = 1.0f / sampleRate;
    float sample = 0.0f;
    float env = 0.0f;

    switch (role_) {
    case InstrumentRole::Kick: {
        env = envelope(time_, 9.5f);
        const float frequency = 40.0f + 120.0f * std::exp(-time_ * 34.0f);
        phaseA_ += 2.0f * kPi * frequency * dt;
        sample = std::sin(phaseA_) * env;
        sample += 0.16f * noise() * envelope(time_, 30.0f);
        if (env < 0.0015f) {
            active_ = false;
        }
        break;
    }
    case InstrumentRole::Snare: {
        env = envelope(time_, 15.0f);
        phaseA_ += 2.0f * kPi * 196.0f * dt;
        sample = 0.35f * std::sin(phaseA_) * env;
        sample += 0.85f * noise() * envelope(time_, 22.0f);
        if (env < 0.001f) {
            active_ = false;
        }
        break;
    }
    case InstrumentRole::ClosedHat: {
        env = envelope(time_, 60.0f);
        sample = 0.85f * noise() * env;
        sample -= 0.35f * std::sin(2.0f * kPi * 5600.0f * time_) * env;
        if (env < 0.0007f) {
            active_ = false;
        }
        break;
    }
    case InstrumentRole::OpenHat: {
        env = envelope(time_, 18.0f);
        sample = 0.75f * noise() * env;
        sample -= 0.25f * std::sin(2.0f * kPi * 4100.0f * time_) * env;
        if (env < 0.0008f) {
            active_ = false;
        }
        break;
    }
    case InstrumentRole::Clap: {
        const float burstA = envelope(time_, 20.0f);
        const float burstB = time_ > 0.018f ? envelope(time_ - 0.018f, 28.0f) : 0.0f;
        const float burstC = time_ > 0.032f ? envelope(time_ - 0.032f, 32.0f) : 0.0f;
        env = burstA + 0.8f * burstB + 0.6f * burstC;
        sample = noise() * env;
        if (env < 0.001f) {
            active_ = false;
        }
        break;
    }
    case InstrumentRole::Perc:
    case InstrumentRole::Custom: {
        env = envelope(time_, 10.0f);
        phaseA_ += 2.0f * kPi * std::max(frequency_, 90.0f) * dt;
        sample = std::sin(phaseA_) * env;
        sample += 0.10f * noise() * envelope(time_, 24.0f);
        if (env < 0.001f) {
            active_ = false;
        }
        break;
    }
    case InstrumentRole::Bass: {
        phaseA_ += frequency_ * dt;
        phaseB_ += (frequency_ * 0.5f) * dt;
        if (phaseA_ >= 1.0f) {
            phaseA_ -= 1.0f;
        }
        if (phaseB_ >= 1.0f) {
            phaseB_ -= 1.0f;
        }
        const float saw = (phaseA_ * 2.0f) - 1.0f;
        const float sub = std::sin(2.0f * kPi * phaseB_);
        const float raw = 0.70f * saw + 0.30f * sub;
        const float cutoff = 0.11f;
        filterState_ += cutoff * (raw - filterState_);
        env = envelope(time_, 4.0f);
        sample = filterState_ * env * 0.75f;
        if (env < 0.001f) {
            active_ = false;
        }
        break;
    }
    case InstrumentRole::Lead: {
        phaseA_ += frequency_ * dt;
        phaseB_ += frequency_ * 1.003f * dt;
        if (phaseA_ >= 1.0f) {
            phaseA_ -= 1.0f;
        }
        if (phaseB_ >= 1.0f) {
            phaseB_ -= 1.0f;
        }
        const float saw = (phaseA_ * 2.0f) - 1.0f;
        const float pulse = phaseB_ < 0.45f ? 1.0f : -1.0f;
        const float raw = 0.55f * saw + 0.45f * pulse;
        const float cutoff = 0.18f;
        filterState_ += cutoff * (raw - filterState_);
        env = envelope(time_, 2.8f);
        sample = softClip(filterState_ * 1.4f) * env * 0.45f;
        if (env < 0.001f) {
            active_ = false;
        }
        break;
    }
    }

    time_ += dt;
    return sample * velocity_;
}

void InternalVoice::reset() {
    active_ = false;
    velocity_ = 0.0f;
    time_ = 0.0f;
    phaseA_ = 0.0f;
    phaseB_ = 0.0f;
    filterState_ = 0.0f;
}

float InternalVoice::noise() {
    noiseState_ = noiseState_ * 1664525u + 1013904223u;
    const unsigned int masked = (noiseState_ >> 8) & 0x00ffffffu;
    return (static_cast<float>(masked) / static_cast<float>(0x00ffffffu)) * 2.0f - 1.0f;
}

float InternalVoice::midiToFrequency(int midiNote) const {
    return 440.0f * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);
}

}  // namespace groove
