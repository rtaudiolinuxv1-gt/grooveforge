#include "sf2/SoundFontSynth.h"

#include <algorithm>

#include <fluidsynth.h>

namespace groove {

SoundFontSynth::SoundFontSynth() {
    recreate();
}

SoundFontSynth::~SoundFontSynth() {
    clear();
    if (synth_) {
        delete_fluid_synth(synth_);
        synth_ = nullptr;
    }
    if (settings_) {
        delete_fluid_settings(settings_);
        settings_ = nullptr;
    }
}

void SoundFontSynth::configure(double sampleRate) {
    if (sampleRate <= 0.0 || sampleRate == sampleRate_) {
        return;
    }
    sampleRate_ = sampleRate;
    const std::string reloadPath = path_;
    recreate();
    if (reloadPath.empty() == false) {
        load(reloadPath);
    }
}

bool SoundFontSynth::load(const std::string& path) {
    if ((synth_ == nullptr) || path.empty()) {
        return false;
    }

    if ((path == path_) && (soundFontId_ >= 0)) {
        return true;
    }

    clear();
    soundFontId_ = fluid_synth_sfload(synth_, path.c_str(), 1);
    if (soundFontId_ < 0) {
        soundFontId_ = -1;
        path_.clear();
        return false;
    }

    path_ = path;
    return true;
}

void SoundFontSynth::clear() {
    if (synth_ == nullptr) {
        path_.clear();
        soundFontId_ = -1;
        return;
    }

    allNotesOff();
    if (soundFontId_ >= 0) {
        fluid_synth_sfunload(synth_, soundFontId_, 1);
    }
    soundFontId_ = -1;
    path_.clear();
}

bool SoundFontSynth::isLoaded() const {
    return soundFontId_ >= 0;
}

const std::string& SoundFontSynth::path() const {
    return path_;
}

void SoundFontSynth::selectPreset(int channel, int bank, int program) {
    if ((synth_ == nullptr) || (soundFontId_ < 0)) {
        return;
    }
    channel = std::clamp(channel, 0, 15);
    bank = std::max(bank, 0);
    program = std::clamp(program, 0, 127);
    fluid_synth_program_select(synth_, channel, soundFontId_, bank, program);
}

void SoundFontSynth::noteOn(int channel, int note, int velocity) {
    if ((synth_ == nullptr) || (soundFontId_ < 0)) {
        return;
    }
    fluid_synth_noteon(synth_, std::clamp(channel, 0, 15), std::clamp(note, 0, 127), std::clamp(velocity, 1, 127));
}

void SoundFontSynth::noteOff(int channel, int note) {
    if ((synth_ == nullptr) || (soundFontId_ < 0)) {
        return;
    }
    fluid_synth_noteoff(synth_, std::clamp(channel, 0, 15), std::clamp(note, 0, 127));
}

void SoundFontSynth::allNotesOff() {
    if (synth_ == nullptr) {
        return;
    }
    for (int channel = 0; channel < 16; ++channel) {
        fluid_synth_all_notes_off(synth_, channel);
        fluid_synth_all_sounds_off(synth_, channel);
    }
}

void SoundFontSynth::renderFrame(float& left, float& right) {
    left = 0.0f;
    right = 0.0f;
    if ((synth_ == nullptr) || (soundFontId_ < 0)) {
        return;
    }
    fluid_synth_write_float(synth_, 1, &left, 0, 1, &right, 0, 1);
}

void SoundFontSynth::recreate() {
    const std::string reloadPath = path_;
    path_.clear();
    soundFontId_ = -1;

    if (synth_ != nullptr) {
        delete_fluid_synth(synth_);
        synth_ = nullptr;
    }
    if (settings_ != nullptr) {
        delete_fluid_settings(settings_);
        settings_ = nullptr;
    }

    settings_ = new_fluid_settings();
    fluid_settings_setnum(settings_, "synth.sample-rate", sampleRate_);
    fluid_settings_setint(settings_, "synth.midi-channels", 16);
    synth_ = new_fluid_synth(settings_);

    if (reloadPath.empty() == false) {
        load(reloadPath);
    }
}

}  // namespace groove
