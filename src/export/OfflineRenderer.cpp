#include "export/OfflineRenderer.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <sndfile.h>

#include "dsp/InternalVoice.h"
#include "sample/SampleBuffer.h"
#include "sample/SampleVoice.h"
#include "sf2/SoundFontSynth.h"

namespace groove {

namespace {

struct NoteState {
    bool active = false;
    int note = 0;
    int channel = 0;
    int samplesRemaining = 0;
};

double framesPerBar(const GrooveScene& scene, double sampleRate) {
    const double quarter = sampleRate * 60.0 / static_cast<double>(scene.bpm);
    return quarter * 4.0;
}

}  // namespace

bool OfflineRenderer::renderToFile(const GrooveScene& inputScene, const std::string& path, const OfflineRenderRequest& request) const {
    GrooveScene scene = normalizedScene(inputScene);
    if (path.empty()) {
        return false;
    }

    const int sampleRate = std::clamp(request.sampleRate, 8000, 192000);
    long long totalFrames = 0;
    if (request.mode == OfflineRenderMode::Bars) {
        totalFrames = static_cast<long long>(std::llround(framesPerBar(scene, static_cast<double>(sampleRate)) * static_cast<double>(std::max(1, request.bars))));
    } else {
        totalFrames = static_cast<long long>(std::llround(std::max(0.1, request.seconds) * static_cast<double>(sampleRate)));
    }
    if (totalFrames <= 0) {
        return false;
    }

    SF_INFO info {};
    info.samplerate = sampleRate;
    info.channels = 2;
    info.format = formatToSndfile(request.format);
    SNDFILE* file = sf_open(path.c_str(), SFM_WRITE, &info);
    if (file == nullptr) {
        return false;
    }

    std::vector<InternalVoice> internalVoices(scene.instruments.size());
    std::vector<SampleVoice> sampleVoices(scene.instruments.size());
    std::vector<std::shared_ptr<const SampleBuffer>> sampleBuffers(scene.instruments.size());
    std::vector<NoteState> soundfontNotes(scene.instruments.size());
    SoundFontSynth soundfont;
    soundfont.configure(static_cast<double>(sampleRate));
    if (scene.soundfontPath.empty() == false) {
        soundfont.load(scene.soundfontPath);
    }

    for (std::size_t index = 0; index < scene.instruments.size(); ++index) {
        internalVoices[index].setRole(scene.instruments[index].role);
        if (scene.instruments[index].layers.samplePath.empty() == false) {
            auto buffer = std::make_shared<SampleBuffer>();
            if (buffer->loadFromFile(scene.instruments[index].layers.samplePath)) {
                sampleBuffers[index] = std::shared_ptr<const SampleBuffer>(buffer);
            }
        }
    }

    std::vector<float> interleaved(4096 * 2, 0.0f);
    double stepSamplesRemaining = 0.0;
    int transportStep = 0;
    const int totalSteps = std::max(1, totalStepCount(scene));
    long long framesRendered = 0;

    while (framesRendered < totalFrames) {
        const int blockFrames = static_cast<int>(std::min<long long>(4096, totalFrames - framesRendered));
        for (int frame = 0; frame < blockFrames; ++frame) {
            if (stepSamplesRemaining <= 0.0) {
                const double stepDuration = nextStepDuration(scene, static_cast<double>(sampleRate), transportStep);
                const int gateSamples = std::max(1, static_cast<int>(stepDuration * 0.82));
                for (std::size_t instrumentIndex = 0; instrumentIndex < scene.instruments.size(); ++instrumentIndex) {
                    const auto& instrument = scene.instruments[instrumentIndex];
                    const auto& step = instrument.steps[static_cast<std::size_t>(transportStep)];
                    if (step.active == false) {
                        continue;
                    }

                    if (instrument.layers.synthEnabled) {
                        internalVoices[instrumentIndex].setRole(instrument.role);
                        internalVoices[instrumentIndex].trigger(step.note, step.velocity);
                    }

                    if (instrument.layers.sampleEnabled) {
                        auto sample = sampleBuffers[instrumentIndex];
                        if ((sample != nullptr) && sample->isValid()) {
                            sampleVoices[instrumentIndex].trigger(sample, step.velocity, samplePlaybackRate(step, instrument.layers, *sample, static_cast<double>(sampleRate)));
                        }
                    }

                    if (instrument.layers.soundfontEnabled && (soundfont.isLoaded())) {
                        auto& noteState = soundfontNotes[instrumentIndex];
                        if (noteState.active) {
                            soundfont.noteOff(noteState.channel, noteState.note);
                        }
                        noteState.active = true;
                        noteState.channel = std::clamp(instrument.layers.soundfontChannel - 1, 0, 15);
                        noteState.note = std::clamp(step.note, 0, 127);
                        noteState.samplesRemaining = gateSamples;
                        soundfont.selectPreset(noteState.channel, instrument.layers.soundfontBank, instrument.layers.soundfontProgram);
                        soundfont.noteOn(noteState.channel, noteState.note, std::clamp(static_cast<int>(step.velocity * 127.0f), 1, 127));
                    }
                }
                transportStep = (transportStep + 1) % totalSteps;
                stepSamplesRemaining += stepDuration;
            }

            float mono = 0.0f;
            for (std::size_t instrumentIndex = 0; instrumentIndex < internalVoices.size(); ++instrumentIndex) {
                mono += internalVoices[instrumentIndex].render(static_cast<float>(sampleRate));
                mono += sampleVoices[instrumentIndex].render();
            }

            for (auto& noteState : soundfontNotes) {
                if (noteState.active == false) {
                    continue;
                }
                noteState.samplesRemaining -= 1;
                if (noteState.samplesRemaining <= 0) {
                    soundfont.noteOff(noteState.channel, noteState.note);
                    noteState.active = false;
                }
            }

            float sfLeft = 0.0f;
            float sfRight = 0.0f;
            soundfont.renderFrame(sfLeft, sfRight);
            const float left = std::clamp((mono * 0.32f) + (sfLeft * 0.60f), -1.0f, 1.0f);
            const float right = std::clamp((mono * 0.32f) + (sfRight * 0.60f), -1.0f, 1.0f);
            interleaved[static_cast<std::size_t>(frame) * 2] = left;
            interleaved[static_cast<std::size_t>(frame) * 2 + 1] = right;
            stepSamplesRemaining -= 1.0;
        }

        sf_writef_float(file, interleaved.data(), blockFrames);
        framesRendered += blockFrames;
    }

    sf_write_sync(file);
    sf_close(file);
    return true;
}

double OfflineRenderer::nextStepDuration(const GrooveScene& scene, double sampleRate, int transportStep) const {
    const double quarter = sampleRate * 60.0 / static_cast<double>(scene.bpm);
    const double baseStep = (quarter * 4.0) / static_cast<double>(scene.stepsPerBar);
    const double swing = std::clamp(static_cast<double>(scene.swing), 0.0, 0.45);
    const bool evenStep = (transportStep % 2) == 0;
    return evenStep ? baseStep * (1.0 + swing) : baseStep * (1.0 - swing);
}

double OfflineRenderer::samplePlaybackRate(const Step& step, const InstrumentLayerSettings& settings, const SampleBuffer& sample, double sampleRate) const {
    double rate = sample.sampleRate() / sampleRate;
    const int semitoneOffset = step.note - settings.sampleRootMidiNote;
    rate *= std::pow(2.0, static_cast<double>(semitoneOffset) / 12.0);
    return rate;
}

int OfflineRenderer::formatToSndfile(AudioFileFormat format) const {
    switch (format) {
    case AudioFileFormat::Wav:
        return SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    case AudioFileFormat::Flac:
        return SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
    }
    return SF_FORMAT_WAV | SF_FORMAT_PCM_16;
}

}  // namespace groove
