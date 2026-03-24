#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace groove {

constexpr int kMinPatternBars = 1;
constexpr int kMaxPatternBars = 16;
constexpr int kMinStepsPerBar = 4;
constexpr int kMaxStepsPerBar = 32;
constexpr int kDefaultStepsPerBar = 16;
constexpr int kMaxInstrumentCount = 16;

enum class AudioFileFormat {
    Wav,
    Flac,
};

enum class InstrumentRole {
    Kick,
    Snare,
    ClosedHat,
    OpenHat,
    Clap,
    Perc,
    Bass,
    Lead,
    Custom,
};

struct Step {
    bool active = false;
    bool locked = false;
    float velocity = 0.0f;
    float volume = 1.0f;
    int note = 60;
    float attack = 0.002f;
    float decay = 0.120f;
    float sustain = 0.0f;
    float release = 0.080f;
    float gate = 0.82f;
};

struct StepParameterDefaults {
    float volume = 1.0f;
    float attack = 0.002f;
    float decay = 0.120f;
    float sustain = 0.0f;
    float release = 0.080f;
    float gate = 0.82f;
};

struct InstrumentLayerSettings {
    bool sampleEnabled = false;
    bool midiEnabled = false;
    bool soundfontEnabled = false;
    int midiChannel = 1;
    int sampleRootMidiNote = 60;
    int soundfontChannel = 1;
    int soundfontBank = 0;
    int soundfontProgram = 0;
    std::string samplePath;
};

struct InstrumentDefinition {
    std::string name;
    InstrumentRole role = InstrumentRole::Custom;
    float density = 0.70f;
    int rootNote = 60;
    StepParameterDefaults stepDefaults;
    InstrumentLayerSettings layers;
    std::vector<Step> steps;
};

struct GrooveScene {
    int bpm = 118;
    int patternBars = 2;
    int stepsPerBar = kDefaultStepsPerBar;
    int repeatsBeforeMutation = 4;
    float swing = 0.10f;
    float mutationAmount = 0.35f;
    bool mutationEnabled = true;
    std::string soundfontPath;
    std::vector<InstrumentDefinition> instruments;
    std::uint32_t seed = 1;
};

inline int clampPatternBars(int bars) {
    return std::clamp(bars, kMinPatternBars, kMaxPatternBars);
}

inline int clampStepsPerBar(int stepsPerBar) {
    stepsPerBar = std::clamp(stepsPerBar, kMinStepsPerBar, kMaxStepsPerBar);
    if ((stepsPerBar % 4) == 0) {
        return stepsPerBar;
    }
    stepsPerBar -= stepsPerBar % 4;
    if (stepsPerBar < kMinStepsPerBar) {
        stepsPerBar = kMinStepsPerBar;
    }
    return stepsPerBar;
}

inline int totalStepCount(int patternBars, int stepsPerBar) {
    return clampPatternBars(patternBars) * clampStepsPerBar(stepsPerBar);
}

inline int totalStepCount(const GrooveScene& scene) {
    return totalStepCount(scene.patternBars, scene.stepsPerBar);
}

inline int defaultMidiNoteForRole(InstrumentRole role) {
    switch (role) {
    case InstrumentRole::Kick:
        return 36;
    case InstrumentRole::Snare:
        return 38;
    case InstrumentRole::ClosedHat:
        return 42;
    case InstrumentRole::OpenHat:
        return 46;
    case InstrumentRole::Clap:
        return 39;
    case InstrumentRole::Perc:
        return 45;
    case InstrumentRole::Bass:
        return 36;
    case InstrumentRole::Lead:
        return 60;
    case InstrumentRole::Custom:
        return 60;
    }
    return 60;
}

inline int defaultMidiChannelForRole(InstrumentRole role) {
    switch (role) {
    case InstrumentRole::Kick:
    case InstrumentRole::Snare:
    case InstrumentRole::ClosedHat:
    case InstrumentRole::OpenHat:
    case InstrumentRole::Clap:
    case InstrumentRole::Perc:
        return 10;
    case InstrumentRole::Bass:
    case InstrumentRole::Lead:
    case InstrumentRole::Custom:
        return 1;
    }
    return 1;
}

inline int defaultSoundfontProgramForRole(InstrumentRole role) {
    switch (role) {
    case InstrumentRole::Kick:
        return 116;
    case InstrumentRole::Snare:
        return 115;
    case InstrumentRole::ClosedHat:
    case InstrumentRole::OpenHat:
        return 114;
    case InstrumentRole::Clap:
        return 115;
    case InstrumentRole::Perc:
        return 113;
    case InstrumentRole::Bass:
        return 33;
    case InstrumentRole::Lead:
        return 81;
    case InstrumentRole::Custom:
        return 0;
    }
    return 0;
}

inline const char* instrumentRoleName(InstrumentRole role) {
    switch (role) {
    case InstrumentRole::Kick:
        return "Kick";
    case InstrumentRole::Snare:
        return "Snare";
    case InstrumentRole::ClosedHat:
        return "Closed Hat";
    case InstrumentRole::OpenHat:
        return "Open Hat";
    case InstrumentRole::Clap:
        return "Clap";
    case InstrumentRole::Perc:
        return "Perc";
    case InstrumentRole::Bass:
        return "Bass";
    case InstrumentRole::Lead:
        return "Lead";
    case InstrumentRole::Custom:
        return "Custom";
    }
    return "Instrument";
}

inline const char* audioFileFormatName(AudioFileFormat format) {
    switch (format) {
    case AudioFileFormat::Wav:
        return "WAV";
    case AudioFileFormat::Flac:
        return "FLAC";
    }
    return "Audio";
}

inline Step makeStep(float velocity, int note) {
    Step step;
    step.active = true;
    step.velocity = velocity;
    step.note = note;
    return step;
}

inline void clampStepParameterDefaults(StepParameterDefaults& defaults) {
    defaults.volume = std::clamp(defaults.volume, 0.0f, 1.5f);
    defaults.attack = std::clamp(defaults.attack, 0.001f, 2.0f);
    defaults.decay = std::clamp(defaults.decay, 0.001f, 4.0f);
    defaults.sustain = std::clamp(defaults.sustain, 0.0f, 1.0f);
    defaults.release = std::clamp(defaults.release, 0.001f, 4.0f);
    defaults.gate = std::clamp(defaults.gate, 0.05f, 1.5f);
}

inline void applyStepParameterDefaults(Step& step, const StepParameterDefaults& defaults) {
    step.volume = defaults.volume;
    step.attack = defaults.attack;
    step.decay = defaults.decay;
    step.sustain = defaults.sustain;
    step.release = defaults.release;
    step.gate = defaults.gate;
}

inline void applyInstrumentDefaultsToUnlockedSteps(InstrumentDefinition& instrument) {
    clampStepParameterDefaults(instrument.stepDefaults);
    for (auto& step : instrument.steps) {
        if (step.locked) {
            continue;
        }
        applyStepParameterDefaults(step, instrument.stepDefaults);
    }
}

inline InstrumentDefinition makeInstrument(InstrumentRole role, const std::string& customName = std::string()) {
    InstrumentDefinition instrument;
    instrument.role = role;
    instrument.name = customName.empty() ? instrumentRoleName(role) : customName;
    instrument.rootNote = defaultMidiNoteForRole(role);
    instrument.layers.midiChannel = defaultMidiChannelForRole(role);
    instrument.layers.sampleRootMidiNote = instrument.rootNote;
    instrument.layers.soundfontChannel = defaultMidiChannelForRole(role);
    instrument.layers.soundfontProgram = defaultSoundfontProgramForRole(role);

    switch (role) {
    case InstrumentRole::Kick:
        instrument.density = 0.85f;
        break;
    case InstrumentRole::Snare:
        instrument.density = 0.68f;
        break;
    case InstrumentRole::ClosedHat:
        instrument.density = 0.82f;
        break;
    case InstrumentRole::OpenHat:
        instrument.density = 0.35f;
        break;
    case InstrumentRole::Clap:
        instrument.density = 0.42f;
        break;
    case InstrumentRole::Perc:
        instrument.density = 0.50f;
        break;
    case InstrumentRole::Bass:
        instrument.density = 0.56f;
        break;
    case InstrumentRole::Lead:
        instrument.density = 0.32f;
        break;
    case InstrumentRole::Custom:
        instrument.density = 0.45f;
        instrument.name = customName.empty() ? "Instrument" : customName;
        break;
    }

    return instrument;
}

inline GrooveScene makeDefaultScene() {
    GrooveScene scene;
    scene.instruments = {
        makeInstrument(InstrumentRole::Kick),
        makeInstrument(InstrumentRole::Snare),
        makeInstrument(InstrumentRole::ClosedHat),
        makeInstrument(InstrumentRole::OpenHat),
        makeInstrument(InstrumentRole::Clap),
        makeInstrument(InstrumentRole::Perc),
        makeInstrument(InstrumentRole::Bass),
        makeInstrument(InstrumentRole::Lead),
    };
    return scene;
}

inline void resizeInstrumentSteps(InstrumentDefinition& instrument, int stepCount) {
    if (stepCount < 0) {
        stepCount = 0;
    }

    Step defaultStep;
    defaultStep.active = false;
    defaultStep.locked = false;
    defaultStep.velocity = 0.0f;
    defaultStep.note = instrument.rootNote;
    applyStepParameterDefaults(defaultStep, instrument.stepDefaults);
    const std::size_t oldSize = instrument.steps.size();
    instrument.steps.resize(static_cast<std::size_t>(stepCount), defaultStep);
    for (std::size_t index = oldSize; index < instrument.steps.size(); ++index) {
        instrument.steps[index].note = instrument.rootNote;
    }
}

inline GrooveScene normalizedScene(GrooveScene scene) {
    scene.bpm = std::clamp(scene.bpm, 40, 220);
    scene.patternBars = clampPatternBars(scene.patternBars);
    scene.stepsPerBar = clampStepsPerBar(scene.stepsPerBar);
    scene.repeatsBeforeMutation = std::clamp(scene.repeatsBeforeMutation, 1, 64);
    scene.swing = std::clamp(scene.swing, 0.0f, 0.45f);
    scene.mutationAmount = std::clamp(scene.mutationAmount, 0.0f, 1.0f);

    if (scene.instruments.empty()) {
        scene.instruments = makeDefaultScene().instruments;
    }
    if (static_cast<int>(scene.instruments.size()) > kMaxInstrumentCount) {
        scene.instruments.resize(kMaxInstrumentCount);
    }

    const int stepCount = totalStepCount(scene);
    for (std::size_t index = 0; index < scene.instruments.size(); ++index) {
        auto& instrument = scene.instruments[index];
        if (instrument.name.empty()) {
            instrument.name = instrumentRoleName(instrument.role);
        }
        instrument.density = std::clamp(instrument.density, 0.0f, 1.0f);
        instrument.rootNote = std::clamp(instrument.rootNote, 0, 127);
        clampStepParameterDefaults(instrument.stepDefaults);
        instrument.layers.midiChannel = std::clamp(instrument.layers.midiChannel, 1, 16);
        instrument.layers.sampleRootMidiNote = std::clamp(instrument.layers.sampleRootMidiNote, 0, 127);
        instrument.layers.soundfontChannel = std::clamp(instrument.layers.soundfontChannel, 1, 16);
        instrument.layers.soundfontBank = std::clamp(instrument.layers.soundfontBank, 0, 16383);
        instrument.layers.soundfontProgram = std::clamp(instrument.layers.soundfontProgram, 0, 127);
        resizeInstrumentSteps(instrument, stepCount);
        for (auto& step : instrument.steps) {
            step.volume = std::clamp(step.volume, 0.0f, 1.5f);
            step.note = std::clamp(step.note, 0, 127);
            step.velocity = std::clamp(step.velocity, 0.0f, 1.0f);
            if (step.attack <= 0.0f) {
                step.attack = instrument.stepDefaults.attack;
            }
            if (step.decay <= 0.0f) {
                step.decay = instrument.stepDefaults.decay;
            }
            if (step.release <= 0.0f) {
                step.release = instrument.stepDefaults.release;
            }
            if (step.gate <= 0.0f) {
                step.gate = instrument.stepDefaults.gate;
            }
            step.sustain = std::clamp(step.sustain, 0.0f, 1.0f);
        }
    }

    return scene;
}

}  // namespace groove
