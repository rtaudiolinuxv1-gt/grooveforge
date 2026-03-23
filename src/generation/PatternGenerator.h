#pragma once

#include <random>

#include "core/GrooveTypes.h"

namespace groove {

class PatternGenerator {
public:
    PatternGenerator();

    GrooveScene createScene(const GrooveScene& templateScene);
    GrooveScene mutateScene(const GrooveScene& currentScene);

private:
    void generateInstrument(InstrumentDefinition& instrument, int stepsPerBar, int patternBars);
    void generateKick(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps);
    void generateSnare(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps);
    void generateHat(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps, bool open);
    void generateClap(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps);
    void generatePerc(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps);
    void generateBass(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps);
    void generateLead(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps);
    bool chance(float probability);
    float randomVelocity(float minVelocity, float maxVelocity);
    int chooseBassNote(int stepIndex, int stepsPerBar, int rootNote);
    int chooseLeadNote(int stepIndex, int stepsPerBar, int rootNote);
    bool isAnchor(const InstrumentDefinition& instrument, int stepIndex, int stepsPerBar) const;

    std::mt19937 engine_;
};

}  // namespace groove
