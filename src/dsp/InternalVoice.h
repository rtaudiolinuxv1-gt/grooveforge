#pragma once

#include "core/GrooveTypes.h"

namespace groove {

class InternalVoice {
public:
    explicit InternalVoice(InstrumentRole role = InstrumentRole::Kick);

    void setRole(InstrumentRole role);
    void trigger(int midiNote, float velocity);
    float render(float sampleRate);
    void reset();

private:
    float noise();
    float midiToFrequency(int midiNote) const;

    InstrumentRole role_ = InstrumentRole::Kick;
    bool active_ = false;
    float velocity_ = 0.0f;
    float frequency_ = 110.0f;
    float time_ = 0.0f;
    float phaseA_ = 0.0f;
    float phaseB_ = 0.0f;
    float filterState_ = 0.0f;
    unsigned int noiseState_ = 0x12345678u;
};

}  // namespace groove
