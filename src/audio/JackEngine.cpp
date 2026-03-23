#include "audio/JackEngine.h"

#include <algorithm>
#include <cstring>

namespace groove {

namespace {

std::uint8_t clampMidiValue(int value) {
    return static_cast<std::uint8_t>(std::clamp(value, 0, 127));
}

}  // namespace

JackEngine::JackEngine() = default;

JackEngine::~JackEngine() {
    stop();
}

bool JackEngine::start(const std::string& clientName) {
    jack_status_t status = JackStatus(0);
    client_ = jack_client_open(clientName.c_str(), JackNullOption, &status);
    if (client_ == nullptr) {
        return false;
    }

    jack_set_process_callback(client_, &JackEngine::processShim, this);
    sampleRate_ = static_cast<double>(jack_get_sample_rate(client_));
    soundfont_.configure(sampleRate_);

    leftPort_ = jack_port_register(client_, "out_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    rightPort_ = jack_port_register(client_, "out_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    midiOutPort_ = jack_port_register(client_, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
    if ((leftPort_ == nullptr) || (rightPort_ == nullptr) || (midiOutPort_ == nullptr)) {
        stop();
        return false;
    }

    if (jack_activate(client_) != 0) {
        stop();
        return false;
    }

    return true;
}

void JackEngine::stop() {
    stopRecording();
    if (client_ == nullptr) {
        return;
    }

    jack_deactivate(client_);
    jack_client_close(client_);
    client_ = nullptr;
    leftPort_ = nullptr;
    rightPort_ = nullptr;
    midiOutPort_ = nullptr;
}

void JackEngine::setScene(const GrooveScene& scene) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    scene_ = normalizedScene(scene);
    ensureRuntimeSize(scene_.instruments.size());
    for (std::size_t index = 0; index < scene_.instruments.size(); ++index) {
        internalVoices_[index].setRole(scene_.instruments[index].role);
    }

    if (scene_.soundfontPath.empty()) {
        soundfont_.clear();
    } else if ((soundfont_.path() == scene_.soundfontPath) == false) {
        soundfont_.load(scene_.soundfontPath);
    }
}

GrooveScene JackEngine::sceneSnapshot() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return scene_;
}

bool JackEngine::loadSample(int instrumentIndex, const std::string& path) {
    auto buffer = std::make_shared<SampleBuffer>();
    if (buffer->loadFromFile(path) == false) {
        return false;
    }

    std::lock_guard<std::mutex> lock(stateMutex_);
    ensureRuntimeSize(static_cast<std::size_t>(std::max(instrumentIndex + 1, 0)));
    sampleBuffers_[static_cast<std::size_t>(instrumentIndex)] = std::shared_ptr<const SampleBuffer>(buffer);
    return true;
}

void JackEngine::clearSample(int instrumentIndex) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    if ((instrumentIndex < 0) || (instrumentIndex >= static_cast<int>(sampleBuffers_.size()))) {
        return;
    }
    sampleBuffers_[static_cast<std::size_t>(instrumentIndex)].reset();
    sampleVoices_[static_cast<std::size_t>(instrumentIndex)].reset();
}

bool JackEngine::loadSoundfont(const std::string& path) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    soundfont_.configure(sampleRate_);
    return soundfont_.load(path);
}

void JackEngine::clearSoundfont() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    soundfont_.clear();
    for (auto& noteState : soundfontNotes_) {
        noteState.active = false;
        noteState.samplesRemaining = 0;
    }
}

bool JackEngine::startRecording(const std::string& path, AudioFileFormat format) {
    return recorder_.start(path, format, static_cast<int>(sampleRate_));
}

void JackEngine::stopRecording() {
    recorder_.stop();
}

bool JackEngine::isRecording() const {
    return recorder_.isRecording();
}

std::string JackEngine::recordingPath() const {
    return recorder_.path();
}

void JackEngine::setPlaying(bool playing) {
    const bool wasPlaying = playing_.exchange(playing);
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (playing && (wasPlaying == false)) {
        pendingMidiStart_.store(true);
        currentStep_.store(-1);
        stepSamplesRemaining_ = 0.0;
        return;
    }

    if ((playing == false) && wasPlaying) {
        pendingMidiStop_.store(true);
        resetTransportLocked();
    }
}

bool JackEngine::isPlaying() const {
    return playing_.load();
}

int JackEngine::currentStep() const {
    return currentStep_.load();
}

int JackEngine::consumeCompletedBars() {
    return completedBars_.exchange(0);
}

int JackEngine::processShim(jack_nframes_t nframes, void* arg) {
    return static_cast<JackEngine*>(arg)->process(nframes);
}

int JackEngine::process(jack_nframes_t nframes) {
    auto* left = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(leftPort_, nframes));
    auto* right = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(rightPort_, nframes));
    auto* midiBuffer = jack_port_get_buffer(midiOutPort_, nframes);
    if ((left == nullptr) || (right == nullptr) || (midiBuffer == nullptr)) {
        return 0;
    }

    std::memset(left, 0, sizeof(jack_default_audio_sample_t) * nframes);
    std::memset(right, 0, sizeof(jack_default_audio_sample_t) * nframes);
    jack_midi_clear_buffer(midiBuffer);

    std::lock_guard<std::mutex> lock(stateMutex_);
    const GrooveScene snapshot = scene_;
    ensureRuntimeSize(snapshot.instruments.size());
    writePendingTransportLocked(midiBuffer);

    if (playing_.load() == false) {
        if (recorder_.isRecording()) {
            recorder_.pushBlock(left, right, static_cast<std::size_t>(nframes));
        }
        return 0;
    }

    if (stepSamplesRemaining_ <= 0.0) {
        const double stepDuration = nextStepDuration(snapshot);
        renderStepLocked(snapshot, midiBuffer, 0, stepDuration);
        stepSamplesRemaining_ = stepDuration;
    }

    for (jack_nframes_t frame = 0; frame < nframes; ++frame) {
        if (stepSamplesRemaining_ <= 0.0) {
            const double stepDuration = nextStepDuration(snapshot);
            renderStepLocked(snapshot, midiBuffer, frame, stepDuration);
            stepSamplesRemaining_ += stepDuration;
        }

        float mono = 0.0f;
        for (std::size_t index = 0; index < internalVoices_.size(); ++index) {
            mono += internalVoices_[index].render(static_cast<float>(sampleRate_));
            mono += sampleVoices_[index].render();
        }

        float sfLeft = 0.0f;
        float sfRight = 0.0f;
        soundfont_.renderFrame(sfLeft, sfRight);
        const float leftSample = std::clamp((mono * 0.32f) + (sfLeft * 0.60f), -1.0f, 1.0f);
        const float rightSample = std::clamp((mono * 0.32f) + (sfRight * 0.60f), -1.0f, 1.0f);

        left[frame] = leftSample;
        right[frame] = rightSample;
        advanceMidiNotesLocked(midiBuffer, frame);
        stepSamplesRemaining_ -= 1.0;
    }

    if (recorder_.isRecording()) {
        recorder_.pushBlock(left, right, static_cast<std::size_t>(nframes));
    }
    return 0;
}

void JackEngine::ensureRuntimeSize(std::size_t count) {
    if (internalVoices_.size() < count) {
        internalVoices_.resize(count);
    }
    if (sampleVoices_.size() < count) {
        sampleVoices_.resize(count);
    }
    if (sampleBuffers_.size() < count) {
        sampleBuffers_.resize(count);
    }
    if (midiNotes_.size() < count) {
        midiNotes_.resize(count);
    }
    if (soundfontNotes_.size() < count) {
        soundfontNotes_.resize(count);
    }
    if (internalVoices_.size() > count) {
        internalVoices_.resize(count);
        sampleVoices_.resize(count);
        sampleBuffers_.resize(count);
        midiNotes_.resize(count);
        soundfontNotes_.resize(count);
    }
}

void JackEngine::renderStepLocked(const GrooveScene& snapshot, void* midiBuffer, jack_nframes_t frameOffset, double stepDuration) {
    const int totalSteps = std::max(1, totalStepCount(snapshot));
    currentStep_.store(transportStep_);
    const int gateSamples = std::max(1, static_cast<int>(stepDuration * 0.82));

    for (std::size_t index = 0; index < snapshot.instruments.size(); ++index) {
        const auto& instrument = snapshot.instruments[index];
        const auto& step = instrument.steps[static_cast<std::size_t>(transportStep_)];
        if (step.active == false) {
            continue;
        }

        if (instrument.layers.synthEnabled) {
            internalVoices_[index].setRole(instrument.role);
            internalVoices_[index].trigger(step.note, step.velocity);
        }

        if (instrument.layers.sampleEnabled) {
            auto sample = sampleBuffers_[index];
            if ((sample == nullptr) == false) {
                if (sample->isValid()) {
                    sampleVoices_[index].trigger(sample, step.velocity, samplePlaybackRate(step, instrument.layers, *sample));
                }
            }
        }

        if (instrument.layers.midiEnabled) {
            auto& midiState = midiNotes_[index];
            if (midiState.active) {
                writeMidiNoteOff(midiBuffer, frameOffset, midiState.channel, midiState.note);
            }
            midiState.active = true;
            midiState.channel = static_cast<std::uint8_t>(std::clamp(instrument.layers.midiChannel - 1, 0, 15));
            midiState.note = clampMidiValue(step.note);
            midiState.samplesRemaining = gateSamples;
            writeMidiNoteOn(midiBuffer, frameOffset, midiState.channel, midiState.note, clampMidiValue(static_cast<int>(step.velocity * 127.0f)));
        }

        if (instrument.layers.soundfontEnabled && soundfont_.isLoaded()) {
            auto& noteState = soundfontNotes_[index];
            if (noteState.active) {
                soundfont_.noteOff(noteState.channel, noteState.note);
            }
            noteState.active = true;
            noteState.channel = static_cast<std::uint8_t>(std::clamp(instrument.layers.soundfontChannel - 1, 0, 15));
            noteState.note = clampMidiValue(step.note);
            noteState.samplesRemaining = gateSamples;
            soundfont_.selectPreset(noteState.channel, instrument.layers.soundfontBank, instrument.layers.soundfontProgram);
            soundfont_.noteOn(noteState.channel, noteState.note, std::clamp(static_cast<int>(step.velocity * 127.0f), 1, 127));
        }
    }

    transportStep_ = (transportStep_ + 1) % totalSteps;
    if (transportStep_ == 0) {
        completedBars_.fetch_add(1);
    }
}

double JackEngine::nextStepDuration(const GrooveScene& snapshot) const {
    const double quarter = sampleRate_ * 60.0 / static_cast<double>(snapshot.bpm);
    const double baseStep = (quarter * 4.0) / static_cast<double>(snapshot.stepsPerBar);
    const double swing = std::clamp(static_cast<double>(snapshot.swing), 0.0, 0.45);
    const bool evenStep = (transportStep_ % 2) == 0;
    return evenStep ? baseStep * (1.0 + swing) : baseStep * (1.0 - swing);
}

void JackEngine::resetTransportLocked() {
    transportStep_ = 0;
    currentStep_.store(-1);
    completedBars_.store(0);
    stepSamplesRemaining_ = 0.0;
    for (auto& voice : internalVoices_) {
        voice.reset();
    }
    for (auto& voice : sampleVoices_) {
        voice.reset();
    }
    soundfont_.allNotesOff();
    for (auto& noteState : soundfontNotes_) {
        noteState.active = false;
        noteState.samplesRemaining = 0;
    }
}

double JackEngine::samplePlaybackRate(const Step& step, const InstrumentLayerSettings& settings, const SampleBuffer& sample) const {
    double rate = sample.sampleRate() / sampleRate_;
    const int semitoneOffset = step.note - settings.sampleRootMidiNote;
    rate *= std::pow(2.0, static_cast<double>(semitoneOffset) / 12.0);
    return rate;
}

void JackEngine::writePendingTransportLocked(void* midiBuffer) {
    if (pendingMidiStop_.exchange(false)) {
        for (auto& midiState : midiNotes_) {
            if (midiState.active) {
                writeMidiNoteOff(midiBuffer, 0, midiState.channel, midiState.note);
                midiState.active = false;
                midiState.samplesRemaining = 0;
            }
        }
        for (auto& noteState : soundfontNotes_) {
            if (noteState.active) {
                soundfont_.noteOff(noteState.channel, noteState.note);
                noteState.active = false;
                noteState.samplesRemaining = 0;
            }
        }
        const jack_midi_data_t stopMessage[1] = {0xfc};
        jack_midi_event_write(midiBuffer, 0, stopMessage, 1);
    }

    if (pendingMidiStart_.exchange(false)) {
        const jack_midi_data_t startMessage[1] = {0xfa};
        jack_midi_event_write(midiBuffer, 0, startMessage, 1);
    }
}

void JackEngine::advanceMidiNotesLocked(void* midiBuffer, jack_nframes_t frameOffset) {
    for (auto& midiState : midiNotes_) {
        if (midiState.active == false) {
            continue;
        }
        midiState.samplesRemaining -= 1;
        if (midiState.samplesRemaining <= 0) {
            writeMidiNoteOff(midiBuffer, frameOffset, midiState.channel, midiState.note);
            midiState.active = false;
        }
    }

    for (auto& noteState : soundfontNotes_) {
        if (noteState.active == false) {
            continue;
        }
        noteState.samplesRemaining -= 1;
        if (noteState.samplesRemaining <= 0) {
            soundfont_.noteOff(noteState.channel, noteState.note);
            noteState.active = false;
        }
    }
}

void JackEngine::writeMidiNoteOn(void* midiBuffer, jack_nframes_t frameOffset, std::uint8_t channel, std::uint8_t note, std::uint8_t velocity) {
    const jack_midi_data_t message[3] = {
        static_cast<jack_midi_data_t>(0x90 | (channel & 0x0f)),
        static_cast<jack_midi_data_t>(note),
        static_cast<jack_midi_data_t>(std::max<std::uint8_t>(velocity, 1)),
    };
    jack_midi_event_write(midiBuffer, frameOffset, message, 3);
}

void JackEngine::writeMidiNoteOff(void* midiBuffer, jack_nframes_t frameOffset, std::uint8_t channel, std::uint8_t note) {
    const jack_midi_data_t message[3] = {
        static_cast<jack_midi_data_t>(0x80 | (channel & 0x0f)),
        static_cast<jack_midi_data_t>(note),
        0,
    };
    jack_midi_event_write(midiBuffer, frameOffset, message, 3);
}

}  // namespace groove
