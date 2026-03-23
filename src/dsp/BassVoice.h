#pragma once

namespace groove {

class BassVoice {
public:
    void trigger(int midiNote, float velocity);
    float render(float sampleRate);
    void reset();

private:
    bool active_ = false;
    float frequency_ = 55.0f;
    float velocity_ = 0.0f;
    float phase_ = 0.0f;
    float filterState_ = 0.0f;
    float time_ = 0.0f;
};

}  // namespace groove
