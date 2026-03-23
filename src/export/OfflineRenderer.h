#pragma once

#include <string>

#include "core/GrooveTypes.h"

namespace groove {

enum class OfflineRenderMode {
    Bars,
    Seconds,
};

struct OfflineRenderRequest {
    AudioFileFormat format = AudioFileFormat::Wav;
    OfflineRenderMode mode = OfflineRenderMode::Bars;
    int bars = 4;
    double seconds = 8.0;
    int sampleRate = 48000;
};

class OfflineRenderer {
public:
    bool renderToFile(const GrooveScene& scene, const std::string& path, const OfflineRenderRequest& request) const;

private:
    double nextStepDuration(const GrooveScene& scene, double sampleRate, int transportStep) const;
    double samplePlaybackRate(const Step& step, const InstrumentLayerSettings& settings, const SampleBuffer& sample, double sampleRate) const;
    int formatToSndfile(AudioFileFormat format) const;
};

}  // namespace groove
