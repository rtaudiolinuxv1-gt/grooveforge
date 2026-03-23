#pragma once

namespace groove {

class DrumVoice {
public:
    enum class Type {
        Kick,
        Snare,
        Hat,
    };

    explicit DrumVoice(Type type);

    void trigger(float velocity);
    float render(float sampleRate);

private:
    Type type_;
    bool active_ = false;
    float velocity_ = 0.0f;
    float time_ = 0.0f;
    float phase_ = 0.0f;
    unsigned int noiseState_ = 0x12345678u;

    float noise();
};

}  // namespace groove
