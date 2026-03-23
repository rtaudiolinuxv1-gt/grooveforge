#pragma once

#include <memory>

#include "sample/SampleBuffer.h"

namespace groove {

class SampleVoice {
public:
    void trigger(std::shared_ptr<const SampleBuffer> sample, float velocity, double playbackRate = 1.0);
    float render();
    void reset();

private:
    std::shared_ptr<const SampleBuffer> sample_;
    double position_ = 0.0;
    double increment_ = 1.0;
    float velocity_ = 0.0f;
    bool active_ = false;
};

}  // namespace groove
