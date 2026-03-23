#include "app/GrooveController.h"

#include <algorithm>

namespace groove {

GrooveController::GrooveController() {
    scene_ = normalizedScene(makeDefaultScene());
    scene_ = generator_.createScene(scene_);
}

bool GrooveController::initialize() {
    audioReady_ = engine_.start("groove_forge");
    pushScene();
    return audioReady_;
}

GrooveScene GrooveController::scene() const {
    return scene_;
}

void GrooveController::regenerateScene() {
    scene_ = generator_.createScene(scene_);
    completedPatternRepeats_ = 0;
    pushScene();
}

void GrooveController::mutateScene() {
    if (scene_.mutationEnabled == false) {
        return;
    }
    scene_ = generator_.mutateScene(scene_);
    pushScene();
}

void GrooveController::toggleStep(int instrumentIndex, int stepIndex) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    const int totalSteps = totalStepCount(scene_);
    if ((stepIndex < 0) || (stepIndex >= totalSteps)) {
        return;
    }

    auto& step = scene_.instruments[static_cast<std::size_t>(instrumentIndex)].steps[static_cast<std::size_t>(stepIndex)];
    step.active = (step.active == false);
    if (step.active) {
        if (step.velocity <= 0.0f) {
            step.velocity = 0.72f;
        }
        if (step.note <= 0) {
            step.note = scene_.instruments[static_cast<std::size_t>(instrumentIndex)].rootNote;
        }
    } else {
        step.velocity = 0.0f;
    }
    pushScene();
}

void GrooveController::addInstrument(const std::string& name, InstrumentRole role) {
    if (static_cast<int>(scene_.instruments.size()) >= kMaxInstrumentCount) {
        return;
    }

    auto instrument = makeInstrument(role, name);
    if (instrument.name.empty()) {
        instrument.name = "Instrument " + std::to_string(scene_.instruments.size() + 1);
    }
    resizeInstrumentSteps(instrument, totalStepCount(scene_));
    scene_.instruments.push_back(instrument);
    normalizeScene();
    pushScene();
}

void GrooveController::setInstrumentName(int instrumentIndex, const std::string& name) {
    if ((validInstrumentIndex(instrumentIndex) == false) || name.empty()) {
        return;
    }
    scene_.instruments[static_cast<std::size_t>(instrumentIndex)].name = name;
    pushScene();
}

void GrooveController::setInstrumentRole(int instrumentIndex, InstrumentRole role) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    auto& instrument = scene_.instruments[static_cast<std::size_t>(instrumentIndex)];
    instrument.role = role;
    instrument.rootNote = defaultMidiNoteForRole(role);
    instrument.layers.midiChannel = defaultMidiChannelForRole(role);
    instrument.layers.sampleRootMidiNote = instrument.rootNote;
    instrument.layers.soundfontChannel = defaultMidiChannelForRole(role);
    instrument.layers.soundfontProgram = defaultSoundfontProgramForRole(role);
    regenerateScene();
}

void GrooveController::setBpm(int bpm) {
    scene_.bpm = std::clamp(bpm, 40, 220);
    pushScene();
}

void GrooveController::setPatternBars(int bars) {
    scene_.patternBars = clampPatternBars(bars);
    normalizeScene();
    pushScene();
}

void GrooveController::setStepsPerBar(int stepsPerBar) {
    scene_.stepsPerBar = clampStepsPerBar(stepsPerBar);
    normalizeScene();
    pushScene();
}

void GrooveController::setRepeatsBeforeMutation(int repeats) {
    scene_.repeatsBeforeMutation = std::clamp(repeats, 1, 64);
}

void GrooveController::setSwing(float swing) {
    scene_.swing = std::clamp(swing, 0.0f, 0.45f);
    pushScene();
}

void GrooveController::setMutationEnabled(bool enabled) {
    scene_.mutationEnabled = enabled;
    pushScene();
}

void GrooveController::setMutationAmount(float mutationAmount) {
    scene_.mutationAmount = std::clamp(mutationAmount, 0.0f, 1.0f);
}

void GrooveController::setInstrumentDensity(int instrumentIndex, float density) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    scene_.instruments[static_cast<std::size_t>(instrumentIndex)].density = std::clamp(density, 0.0f, 1.0f);
}

void GrooveController::setInstrumentSynthEnabled(int instrumentIndex, bool enabled) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    scene_.instruments[static_cast<std::size_t>(instrumentIndex)].layers.synthEnabled = enabled;
    pushScene();
}

void GrooveController::setInstrumentSampleEnabled(int instrumentIndex, bool enabled) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    auto& layers = scene_.instruments[static_cast<std::size_t>(instrumentIndex)].layers;
    layers.sampleEnabled = enabled && (layers.samplePath.empty() == false);
    pushScene();
}

void GrooveController::setInstrumentSoundfontEnabled(int instrumentIndex, bool enabled) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    auto& layers = scene_.instruments[static_cast<std::size_t>(instrumentIndex)].layers;
    layers.soundfontEnabled = enabled && (scene_.soundfontPath.empty() == false);
    pushScene();
}

void GrooveController::setInstrumentMidiEnabled(int instrumentIndex, bool enabled) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    scene_.instruments[static_cast<std::size_t>(instrumentIndex)].layers.midiEnabled = enabled;
    pushScene();
}

void GrooveController::setInstrumentMidiChannel(int instrumentIndex, int channel) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    scene_.instruments[static_cast<std::size_t>(instrumentIndex)].layers.midiChannel = std::clamp(channel, 1, 16);
    pushScene();
}

void GrooveController::setInstrumentSoundfontChannel(int instrumentIndex, int channel) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    scene_.instruments[static_cast<std::size_t>(instrumentIndex)].layers.soundfontChannel = std::clamp(channel, 1, 16);
    pushScene();
}

void GrooveController::setInstrumentSoundfontBank(int instrumentIndex, int bank) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    scene_.instruments[static_cast<std::size_t>(instrumentIndex)].layers.soundfontBank = std::clamp(bank, 0, 16383);
    pushScene();
}

void GrooveController::setInstrumentSoundfontProgram(int instrumentIndex, int program) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    scene_.instruments[static_cast<std::size_t>(instrumentIndex)].layers.soundfontProgram = std::clamp(program, 0, 127);
    pushScene();
}

bool GrooveController::loadSample(int instrumentIndex, const std::string& path) {
    if ((validInstrumentIndex(instrumentIndex) == false) || path.empty()) {
        return false;
    }
    if (engine_.loadSample(instrumentIndex, path) == false) {
        return false;
    }
    auto& layers = scene_.instruments[static_cast<std::size_t>(instrumentIndex)].layers;
    layers.samplePath = path;
    layers.sampleEnabled = true;
    layers.sampleRootMidiNote = scene_.instruments[static_cast<std::size_t>(instrumentIndex)].rootNote;
    pushScene();
    return true;
}

void GrooveController::clearSample(int instrumentIndex) {
    if (validInstrumentIndex(instrumentIndex) == false) {
        return;
    }
    engine_.clearSample(instrumentIndex);
    auto& layers = scene_.instruments[static_cast<std::size_t>(instrumentIndex)].layers;
    layers.samplePath.clear();
    layers.sampleEnabled = false;
    pushScene();
}

bool GrooveController::loadSoundfont(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (engine_.loadSoundfont(path) == false) {
        return false;
    }
    scene_.soundfontPath = path;
    pushScene();
    return true;
}

void GrooveController::clearSoundfont() {
    engine_.clearSoundfont();
    scene_.soundfontPath.clear();
    for (auto& instrument : scene_.instruments) {
        instrument.layers.soundfontEnabled = false;
    }
    pushScene();
}

bool GrooveController::exportWavBars(const std::string& path, int bars) const {
    OfflineRenderRequest request;
    request.format = AudioFileFormat::Wav;
    request.mode = OfflineRenderMode::Bars;
    request.bars = std::max(1, bars);
    return offlineRenderer_.renderToFile(scene_, path, request);
}

bool GrooveController::exportWavSeconds(const std::string& path, double seconds) const {
    OfflineRenderRequest request;
    request.format = AudioFileFormat::Wav;
    request.mode = OfflineRenderMode::Seconds;
    request.seconds = std::max(0.1, seconds);
    return offlineRenderer_.renderToFile(scene_, path, request);
}

bool GrooveController::startRecording(const std::string& path, AudioFileFormat format) {
    if (audioReady_ == false) {
        return false;
    }
    return engine_.startRecording(path, format);
}

void GrooveController::stopRecording() {
    engine_.stopRecording();
}

bool GrooveController::isRecording() const {
    return engine_.isRecording();
}

std::string GrooveController::recordingPath() const {
    return engine_.recordingPath();
}

void GrooveController::setPlaying(bool playing) {
    engine_.setPlaying(playing);
    if (playing == false) {
        completedPatternRepeats_ = 0;
    }
}

bool GrooveController::isPlaying() const {
    return engine_.isPlaying();
}

int GrooveController::currentStep() const {
    return engine_.currentStep();
}

bool GrooveController::audioReady() const {
    return audioReady_;
}

void GrooveController::tickAutomation() {
    if (scene_.mutationEnabled == false) {
        engine_.consumeCompletedBars();
        return;
    }

    const int completedRepeats = engine_.consumeCompletedBars();
    if (completedRepeats <= 0) {
        return;
    }

    completedPatternRepeats_ += completedRepeats;
    if (completedPatternRepeats_ >= scene_.repeatsBeforeMutation) {
        completedPatternRepeats_ = 0;
        mutateScene();
    }
}

bool GrooveController::validInstrumentIndex(int instrumentIndex) const {
    return (instrumentIndex >= 0) && (instrumentIndex < static_cast<int>(scene_.instruments.size()));
}

void GrooveController::pushScene() {
    normalizeScene();
    engine_.setScene(scene_);
}

void GrooveController::normalizeScene() {
    scene_ = normalizedScene(scene_);
}

}  // namespace groove
