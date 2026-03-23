#include "generation/PatternGenerator.h"

#include <algorithm>
#include <array>

namespace groove {

namespace {

constexpr std::array<int, 8> kMinorScale = {0, 3, 5, 7, 10, 12, 15, 17};
constexpr std::array<int, 8> kMajorScale = {0, 2, 4, 7, 9, 12, 14, 16};

}  // namespace

PatternGenerator::PatternGenerator() : engine_(std::random_device {}()) {}

GrooveScene PatternGenerator::createScene(const GrooveScene& templateScene) {
    GrooveScene scene = normalizedScene(templateScene);
    scene.seed = engine_();

    for (auto& instrument : scene.instruments) {
        resizeInstrumentSteps(instrument, totalStepCount(scene));
        for (auto& step : instrument.steps) {
            step = Step {false, 0.0f, instrument.rootNote};
        }
        generateInstrument(instrument, scene.stepsPerBar, totalStepCount(scene));
    }

    return scene;
}

GrooveScene PatternGenerator::mutateScene(const GrooveScene& currentScene) {
    GrooveScene mutated = normalizedScene(currentScene);
    mutated.seed = engine_();
    if ((mutated.mutationEnabled == false) || (mutated.mutationAmount <= 0.0f)) {
        return mutated;
    }

    const int totalSteps = totalStepCount(mutated);
    for (auto& instrument : mutated.instruments) {
        const float density = std::clamp(instrument.density, 0.0f, 1.0f);
        const float mutation = std::clamp(mutated.mutationAmount, 0.05f, 1.0f);

        for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
            auto& step = instrument.steps[static_cast<std::size_t>(stepIndex)];
            if (isAnchor(instrument, stepIndex, mutated.stepsPerBar)) {
                continue;
            }

            if (chance(mutation * 0.35f)) {
                step.active = (step.active == false);
            }

            if (step.active) {
                step.velocity = randomVelocity(0.45f, 1.0f);
                if (instrument.role == InstrumentRole::Bass) {
                    step.note = chooseBassNote(stepIndex, mutated.stepsPerBar, instrument.rootNote);
                } else if (instrument.role == InstrumentRole::Lead || instrument.role == InstrumentRole::Custom) {
                    step.note = chooseLeadNote(stepIndex, mutated.stepsPerBar, instrument.rootNote);
                } else {
                    step.note = instrument.rootNote;
                }
            } else if (chance(density * mutation * 0.25f)) {
                step.active = true;
                step.velocity = randomVelocity(0.35f, 0.82f);
                if (instrument.role == InstrumentRole::Bass) {
                    step.note = chooseBassNote(stepIndex, mutated.stepsPerBar, instrument.rootNote);
                } else if (instrument.role == InstrumentRole::Lead || instrument.role == InstrumentRole::Custom) {
                    step.note = chooseLeadNote(stepIndex, mutated.stepsPerBar, instrument.rootNote);
                } else {
                    step.note = instrument.rootNote;
                }
            }
        }
    }

    return mutated;
}

void PatternGenerator::generateInstrument(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps) {
    switch (instrument.role) {
    case InstrumentRole::Kick:
        generateKick(instrument, stepsPerBar, totalSteps);
        break;
    case InstrumentRole::Snare:
        generateSnare(instrument, stepsPerBar, totalSteps);
        break;
    case InstrumentRole::ClosedHat:
        generateHat(instrument, stepsPerBar, totalSteps, false);
        break;
    case InstrumentRole::OpenHat:
        generateHat(instrument, stepsPerBar, totalSteps, true);
        break;
    case InstrumentRole::Clap:
        generateClap(instrument, stepsPerBar, totalSteps);
        break;
    case InstrumentRole::Perc:
        generatePerc(instrument, stepsPerBar, totalSteps);
        break;
    case InstrumentRole::Bass:
        generateBass(instrument, stepsPerBar, totalSteps);
        break;
    case InstrumentRole::Lead:
    case InstrumentRole::Custom:
        generateLead(instrument, stepsPerBar, totalSteps);
        break;
    }
}

void PatternGenerator::generateKick(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps) {
    const int beat = stepsPerBar / 4;
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        if (stepInBar == 0) {
            instrument.steps[stepIndex] = makeStep(1.0f, instrument.rootNote);
            continue;
        }
        if ((stepInBar == (beat * 2)) && chance(instrument.density * 0.75f)) {
            instrument.steps[stepIndex] = makeStep(0.9f, instrument.rootNote);
            continue;
        }
        if (((stepInBar == (beat - 1)) || (stepInBar == (beat + 1)) || (stepInBar == (beat * 3))) && chance(instrument.density * 0.42f)) {
            instrument.steps[stepIndex] = makeStep(randomVelocity(0.45f, 0.82f), instrument.rootNote);
        }
    }
}

void PatternGenerator::generateSnare(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps) {
    const int beat = stepsPerBar / 4;
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        if ((stepInBar == beat) || (stepInBar == (beat * 3))) {
            instrument.steps[stepIndex] = makeStep(stepInBar == beat ? 0.92f : 1.0f, instrument.rootNote);
            continue;
        }
        if (((stepInBar == (beat * 2 + beat / 2)) || (stepInBar == (stepsPerBar - 1))) && chance(instrument.density * 0.28f)) {
            instrument.steps[stepIndex] = makeStep(randomVelocity(0.35f, 0.65f), instrument.rootNote);
        }
    }
}

void PatternGenerator::generateHat(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps, bool open) {
    const int subdivision = open ? std::max(1, stepsPerBar / 4) : std::max(1, stepsPerBar / 8);
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        const bool slot = (stepInBar % subdivision) == 0;
        if (slot == false) {
            continue;
        }
        const float chanceScale = open ? 0.35f : 0.86f;
        if (chance(instrument.density * chanceScale)) {
            const float minVelocity = open ? 0.35f : 0.30f;
            const float maxVelocity = open ? 0.62f : 0.78f;
            instrument.steps[stepIndex] = makeStep(randomVelocity(minVelocity, maxVelocity), instrument.rootNote);
        }
    }
}

void PatternGenerator::generateClap(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps) {
    const int beat = stepsPerBar / 4;
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        if ((stepInBar == beat) && chance(instrument.density * 0.55f)) {
            instrument.steps[stepIndex] = makeStep(0.75f, instrument.rootNote);
            continue;
        }
        if ((stepInBar == (beat * 3)) && chance(instrument.density * 0.80f)) {
            instrument.steps[stepIndex] = makeStep(0.88f, instrument.rootNote);
            continue;
        }
        if ((stepInBar == (beat * 2 + beat / 2)) && chance(instrument.density * 0.22f)) {
            instrument.steps[stepIndex] = makeStep(randomVelocity(0.35f, 0.58f), instrument.rootNote);
        }
    }
}

void PatternGenerator::generatePerc(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps) {
    const int accent = std::max(1, stepsPerBar / 8);
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        const bool favored = ((stepInBar + accent) % (accent * 2)) == 0;
        const float probability = favored ? instrument.density * 0.65f : instrument.density * 0.25f;
        if (chance(probability)) {
            instrument.steps[stepIndex] = makeStep(randomVelocity(0.30f, 0.72f), instrument.rootNote + ((stepIndex / accent) % 3) * 2);
        }
    }
}

void PatternGenerator::generateBass(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps) {
    const int beat = stepsPerBar / 4;
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        const bool favored = (stepInBar == 0) || (stepInBar == beat) || (stepInBar == (beat * 2)) || (stepInBar == (beat * 3));
        const float probability = favored ? instrument.density * 0.90f : instrument.density * 0.30f;
        if (chance(probability)) {
            instrument.steps[stepIndex] = makeStep(randomVelocity(0.45f, 0.88f), chooseBassNote(stepIndex, stepsPerBar, instrument.rootNote));
        }
    }

    for (int barIndex = 0; barIndex < (totalSteps / stepsPerBar); ++barIndex) {
        const int downbeat = barIndex * stepsPerBar;
        instrument.steps[downbeat] = makeStep(0.88f, instrument.rootNote);
    }
}

void PatternGenerator::generateLead(InstrumentDefinition& instrument, int stepsPerBar, int totalSteps) {
    const int accent = std::max(1, stepsPerBar / 8);
    for (int stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const int stepInBar = stepIndex % stepsPerBar;
        const bool favored = (stepInBar % (accent * 2)) == 0;
        const float probability = favored ? instrument.density * 0.55f : instrument.density * 0.18f;
        if (chance(probability)) {
            instrument.steps[stepIndex] = makeStep(randomVelocity(0.30f, 0.72f), chooseLeadNote(stepIndex, stepsPerBar, instrument.rootNote));
        }
    }
}

bool PatternGenerator::chance(float probability) {
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    return distribution(engine_) < std::clamp(probability, 0.0f, 1.0f);
}

float PatternGenerator::randomVelocity(float minVelocity, float maxVelocity) {
    std::uniform_real_distribution<float> distribution(minVelocity, maxVelocity);
    return distribution(engine_);
}

int PatternGenerator::chooseBassNote(int stepIndex, int stepsPerBar, int rootNote) {
    std::uniform_int_distribution<int> variation(0, static_cast<int>(kMinorScale.size()) - 1);
    const int stepInBar = stepIndex % stepsPerBar;
    const int beat = stepsPerBar / 4;
    if ((stepInBar == 0) || (stepInBar == (beat * 2))) {
        return rootNote;
    }
    if ((stepInBar == beat) || (stepInBar == (beat * 3))) {
        return rootNote + 7;
    }
    return rootNote + kMinorScale[static_cast<std::size_t>(variation(engine_))];
}

int PatternGenerator::chooseLeadNote(int stepIndex, int stepsPerBar, int rootNote) {
    std::uniform_int_distribution<int> variation(0, static_cast<int>(kMajorScale.size()) - 1);
    const int stepInBar = stepIndex % stepsPerBar;
    if ((stepInBar % std::max(1, stepsPerBar / 4)) == 0) {
        return rootNote + kMajorScale[static_cast<std::size_t>(variation(engine_))];
    }
    return rootNote + kMajorScale[static_cast<std::size_t>(variation(engine_))] + ((stepInBar / std::max(1, stepsPerBar / 8)) % 2);
}

bool PatternGenerator::isAnchor(const InstrumentDefinition& instrument, int stepIndex, int stepsPerBar) const {
    const int stepInBar = stepIndex % stepsPerBar;
    const int beat = stepsPerBar / 4;

    switch (instrument.role) {
    case InstrumentRole::Kick:
    case InstrumentRole::Bass:
        return stepInBar == 0;
    case InstrumentRole::Snare:
    case InstrumentRole::Clap:
        return (stepInBar == beat) || (stepInBar == (beat * 3));
    case InstrumentRole::ClosedHat:
    case InstrumentRole::OpenHat:
    case InstrumentRole::Perc:
    case InstrumentRole::Lead:
    case InstrumentRole::Custom:
        return false;
    }
    return false;
}

}  // namespace groove
