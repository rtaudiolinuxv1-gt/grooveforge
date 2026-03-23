#include "dsp/DrumVoice.h"

#include <cmath>

namespace groove {

namespace {

constexpr float kPi = 3.14159265359f;

float envelope(float time, float decay) {
    return std::exp(-time * decay);
}

}  // namespace

DrumVoice::DrumVoice(Type type) : type_(type) {}

void DrumVoice::trigger(float velocity) {
    active_ = true;
    velocity_ = velocity;
    time_ = 0.0f;
    phase_ = 0.0f;
}

float DrumVoice::render(float sampleRate) {
    if (!active_) {
        return 0.0f;
    }

    const float dt = 1.0f / sampleRate;
    float sample = 0.0f;
    float env = 0.0f;

    switch (type_) {
    case Type::Kick: {
        env = envelope(time_, 9.5f);
        const float frequency = 42.0f + 110.0f * std::exp(-time_ * 32.0f);
        phase_ += 2.0f * kPi * frequency * dt;
        sample = std::sin(phase_) * env;
        sample += 0.18f * noise() * envelope(time_, 28.0f);
        if (env < 0.0015f) {
            active_ = false;
        }
        break;
    }
    case Type::Snare: {
        env = envelope(time_, 16.0f);
        phase_ += 2.0f * kPi * 196.0f * dt;
        sample = 0.45f * std::sin(phase_) * env;
        sample += 0.80f * noise() * envelope(time_, 23.0f);
        if (env < 0.001f) {
            active_ = false;
        }
        break;
    }
    case Type::Hat: {
        env = envelope(time_, 52.0f);
        sample = noise() * env;
        sample -= 0.45f * std::sin(2.0f * kPi * 4800.0f * time_) * env;
        if (env < 0.0007f) {
            active_ = false;
        }
        break;
    }
    }

    time_ += dt;
    return sample * velocity_;
}

float DrumVoice::noise() {
    noiseState_ = noiseState_ * 1664525u + 1013904223u;
    const unsigned int masked = (noiseState_ >> 8) & 0x00ffffffu;
    return (static_cast<float>(masked) / static_cast<float>(0x00ffffffu)) * 2.0f - 1.0f;
}

}  // namespace groove
