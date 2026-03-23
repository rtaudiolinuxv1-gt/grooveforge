#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace groove {

class SampleBuffer {
public:
    bool loadFromFile(const std::string& path);

    bool isValid() const;
    double sampleRate() const;
    std::size_t frameCount() const;
    float sampleAt(double frameIndex) const;
    const std::string& path() const;

private:
    std::vector<float> frames_;
    double sampleRate_ = 48000.0;
    std::string path_;
};

}  // namespace groove
