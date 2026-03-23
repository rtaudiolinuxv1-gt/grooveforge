#include <cstdlib>
#include <iostream>

#include "generation/PatternGenerator.h"

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main() {
    groove::PatternGenerator generator;
    groove::GrooveScene seedScene;
    seedScene.patternBars = 4;
    seedScene.densities = {0.9f, 0.7f, 0.8f, 0.6f};

    const groove::GrooveScene scene = generator.createScene(seedScene);
    const auto& snare = scene.tracks[static_cast<int>(groove::TrackId::Snare)];
    const auto& bass = scene.tracks[static_cast<int>(groove::TrackId::Bass)];

    if (!require(snare.steps[4].active, "snare should anchor on bar 1 beat 2")) {
        return EXIT_FAILURE;
    }
    if (!require(snare.steps[12].active, "snare should anchor on bar 1 beat 4")) {
        return EXIT_FAILURE;
    }
    if (!require(snare.steps[20].active, "snare should anchor on bar 2 beat 2")) {
        return EXIT_FAILURE;
    }
    if (!require(bass.steps[0].active, "bass should anchor on first downbeat")) {
        return EXIT_FAILURE;
    }
    if (!require(bass.steps[16].active, "bass should anchor on second bar downbeat")) {
        return EXIT_FAILURE;
    }

    const groove::GrooveScene mutated = generator.mutateScene(scene);
    if (!require(mutated.tracks[static_cast<int>(groove::TrackId::Kick)].steps[0].active,
            "kick downbeat should survive mutation")) {
        return EXIT_FAILURE;
    }
    if (!require(mutated.tracks[static_cast<int>(groove::TrackId::Snare)].steps[4].active,
            "snare backbeat should survive mutation")) {
        return EXIT_FAILURE;
    }
    if (!require(mutated.tracks[static_cast<int>(groove::TrackId::Snare)].steps[20].active,
            "snare anchors should survive on later bars too")) {
        return EXIT_FAILURE;
    }

    std::cout << "pattern_generator_tests passed\n";
    return EXIT_SUCCESS;
}
