#pragma once

#include <string>

struct _fluid_settings_t;
struct _fluid_synth_t;

namespace groove {

class SoundFontSynth {
public:
    SoundFontSynth();
    ~SoundFontSynth();

    void configure(double sampleRate);
    bool load(const std::string& path);
    void clear();
    bool isLoaded() const;
    const std::string& path() const;

    void selectPreset(int channel, int bank, int program);
    void noteOn(int channel, int note, int velocity);
    void noteOff(int channel, int note);
    void allNotesOff();
    void renderFrame(float& left, float& right);

private:
    void recreate();

    _fluid_settings_t* settings_ = nullptr;
    _fluid_synth_t* synth_ = nullptr;
    double sampleRate_ = 48000.0;
    int soundFontId_ = -1;
    std::string path_;
};

}  // namespace groove
