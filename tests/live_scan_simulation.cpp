// Source-level simulation of the new live inventory scanner.
// This deliberately uses a synthetic list: without a running GD process, this
// cannot claim to be an in-game execution or to validate exact collision physics.
#include "../src/LiveScanCore.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

using namespace ibscan;

static ObjectProfile makeLevelObject(std::size_t i) {
    ObjectProfile p;
    p.disabled = (i % 37u == 0u);
    p.decoration = (i % 13u == 0u);
    p.hazard = (i % 11u == 0u);
    p.solid = (i % 4u == 0u);
    p.slope = (i % 101u == 0u);
    p.orb = (i % 31u == 0u);
    p.pad = (i % 89u == 0u);
    p.stateChange = (i % 23u == 0u);
    return p;
}

int main() {
    constexpr std::size_t kObjects = 25000;
    std::vector<ObjectProfile> loadedLevel;
    loadedLevel.reserve(kObjects);
    for (std::size_t i = 0; i < kObjects; ++i) loadedLevel.push_back(makeLevelObject(i));

    IncrementalLevelScan scan;
    scan.begin(loadedLevel.size());
    std::printf("[InputBuffer] live level scan started: %zu objects; read-only indexing while gameplay continues.\n", kObjects);

    int frames = 0;
    while (scan.state() == State::Running) {
        // This cap is the guard used by main.cpp: 512 metadata records per
        // frame during gameplay, never the full array in one frame.
        auto const result = scan.step(loadedLevel.size(), 512, [&](std::size_t i) { return loadedLevel[i]; });
        ++frames;
        if (result.progressAdvanced) {
            auto const& c = result.counts;
            std::printf("[InputBuffer] live level scan progress: %d%% (%zu/%zu objects; hazards=%zu, solids=%zu, orbs=%zu, pads=%zu).\n",
                        result.progressPercent, c.processed, c.total, c.hazards, c.solids, c.orbs, c.pads);
        }
        if (result.completed) {
            auto const& c = result.counts;
            std::printf("[InputBuffer] live level scan complete: %zu/%zu objects; hazards=%zu, solids=%zu, slopes=%zu, orbs=%zu, pads=%zu, state-change=%zu, disabled=%zu, decorations=%zu.\n",
                        c.processed, c.total, c.hazards, c.solids, c.slopes, c.orbs, c.pads,
                        c.stateChanges, c.disabled, c.decorations);
        }
    }
    std::printf("[Simulation] completed in %d frame slices; max object reads per frame = 512.\n", frames);
    std::printf("[Simulation] physics route scan is intentionally deferred until the real level is paused.\n");
    return scan.state() == State::Complete && scan.counts().processed == kObjects ? 0 : 1;
}
