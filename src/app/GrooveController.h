#pragma once

#include <string>

#include "audio/JackEngine.h"
#include "export/OfflineRenderer.h"
#include "generation/PatternGenerator.h"

namespace groove {

class GrooveController {
public:
    GrooveController();

    bool initialize();

    GrooveScene scene() const;
    void setScene(const GrooveScene& scene);
    void regenerateScene();
    void mutateScene();
    void toggleStep(int instrumentIndex, int stepIndex);
    void addInstrument(const std::string& name, InstrumentRole role);
    void removeInstrument(int instrumentIndex);
    void moveInstrumentUp(int instrumentIndex);
    void moveInstrumentDown(int instrumentIndex);
    void setInstrumentName(int instrumentIndex, const std::string& name);
    void setInstrumentRole(int instrumentIndex, InstrumentRole role);
    void setBpm(int bpm);
    void setPatternBars(int bars);
    void setStepsPerBar(int stepsPerBar);
    void setRepeatsBeforeMutation(int repeats);
    void setSwing(float swing);
    void setMutationEnabled(bool enabled);
    void setMutationAmount(float mutationAmount);
    void setInstrumentDensity(int instrumentIndex, float density);
    void setInstrumentSampleEnabled(int instrumentIndex, bool enabled);
    void setInstrumentSoundfontEnabled(int instrumentIndex, bool enabled);
    void setInstrumentMidiEnabled(int instrumentIndex, bool enabled);
    void setInstrumentMidiChannel(int instrumentIndex, int channel);
    void setInstrumentSoundfontChannel(int instrumentIndex, int channel);
    void setInstrumentSoundfontBank(int instrumentIndex, int bank);
    void setInstrumentSoundfontProgram(int instrumentIndex, int program);
    bool loadSample(int instrumentIndex, const std::string& path);
    void clearSample(int instrumentIndex);
    bool loadSoundfont(const std::string& path);
    void clearSoundfont();
    std::vector<SoundFontPreset> soundfontPresets() const;

    bool exportWavBars(const std::string& path, int bars) const;
    bool exportWavSeconds(const std::string& path, double seconds) const;

    bool startRecording(const std::string& path, AudioFileFormat format);
    void stopRecording();
    bool isRecording() const;
    std::string recordingPath() const;

    void setPlaying(bool playing);
    bool isPlaying() const;
    int currentStep() const;
    bool audioReady() const;
    void tickAutomation();

private:
    bool validInstrumentIndex(int instrumentIndex) const;
    void pushScene();
    void normalizeScene();

    PatternGenerator generator_;
    JackEngine engine_;
    OfflineRenderer offlineRenderer_;
    GrooveScene scene_;
    int completedPatternRepeats_ = 0;
    bool audioReady_ = false;
};

}  // namespace groove
