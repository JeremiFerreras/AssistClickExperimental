#include "../src/LiveScanCore.hpp"

#include <cstdio>
#include <vector>

using namespace ibscan;

static int failures = 0;
#define CHECK(c, msg) do { if (!(c)) { ++failures; std::printf("FAIL: %s\n", msg); } } while (0)

static void testChunkedScan() {
    std::vector<ObjectProfile> objects(10000);
    Counts expected{};
    expected.total = objects.size();
    for (std::size_t i = 0; i < objects.size(); ++i) {
        auto& o = objects[i];
        o.disabled = (i % 97 == 0);
        o.decoration = (i % 11 == 0);
        o.hazard = (i % 7 == 0);
        o.solid = (i % 5 == 0);
        o.slope = (i % 101 == 0);
        o.orb = (i % 29 == 0);
        o.pad = (i % 53 == 0);
        o.stateChange = (i % 17 == 0);
        if (o.disabled) ++expected.disabled;
        else if (o.decoration) ++expected.decorations;
        else {
            if (o.hazard) ++expected.hazards;
            if (o.solid) ++expected.solids;
            if (o.slope) ++expected.slopes;
            if (o.orb) ++expected.orbs;
            if (o.pad) ++expected.pads;
            if (o.stateChange) ++expected.stateChanges;
        }
    }

    IncrementalLevelScan scan;
    scan.begin(objects.size());
    int progressEvents = 0;
    int frames = 0;
    while (scan.state() == State::Running && frames < 1000) {
        // Same guard used in the mod: no more than ~10% of a small level or
        // 512 lightweight metadata reads per frame on huge levels.
        std::size_t const perFrame = std::min<std::size_t>(512, std::max<std::size_t>(1, objects.size() / 10));
        auto result = scan.step(objects.size(), perFrame, [&](std::size_t i) { return objects[i]; });
        if (result.progressAdvanced) {
            ++progressEvents;
            CHECK(result.progressPercent >= 10 && result.progressPercent <= 90, "progress mark out of range");
        }
        ++frames;
    }
    auto const c = scan.counts();
    CHECK(scan.state() == State::Complete, "scan did not finish");
    CHECK(c.processed == objects.size(), "not every object was visited");
    CHECK(c.hazards == expected.hazards, "hazard counts mismatch");
    CHECK(c.solids == expected.solids, "solid counts mismatch");
    CHECK(c.slopes == expected.slopes, "slope counts mismatch");
    CHECK(c.orbs == expected.orbs, "orb counts mismatch");
    CHECK(c.pads == expected.pads, "pad counts mismatch");
    CHECK(c.stateChanges == expected.stateChanges, "state-change counts mismatch");
    CHECK(c.disabled == expected.disabled, "disabled counts mismatch");
    CHECK(c.decorations == expected.decorations, "decoration counts mismatch");
    CHECK(frames <= 100, "scan exceeded safe chunk count for the test level");
    CHECK(progressEvents >= 8 && progressEvents <= 10, "expected decile progress reporting");
}

static void testArrayMutationAbortsBeforeReading() {
    IncrementalLevelScan scan;
    scan.begin(100);
    std::size_t reads = 0;
    auto first = scan.step(100, 10, [&](std::size_t) { ++reads; return ObjectProfile{.hazard = true}; });
    CHECK(first.state == State::Running, "initial chunk should continue");
    auto changed = scan.step(101, 10, [&](std::size_t) { ++reads; return ObjectProfile{}; });
    CHECK(changed.objectCountChanged, "array size change was not detected");
    CHECK(changed.state == State::Aborted, "changed array should abort current index pass");
    CHECK(reads == 10, "scanner read objects after array size changed");
}

static void testEmptyLevelAndDisabledFiltering() {
    IncrementalLevelScan empty;
    empty.begin(0);
    CHECK(empty.state() == State::Complete, "empty object list should complete safely");

    IncrementalLevelScan scan;
    scan.begin(2);
    auto result = scan.step(2, 2, [](std::size_t i) {
        if (i == 0) return ObjectProfile{.disabled = true, .hazard = true};
        return ObjectProfile{.decoration = true, .hazard = true};
    });
    CHECK(result.completed, "small scan should finish in one chunk");
    CHECK(result.counts.hazards == 0, "disabled/decorative hazards should not count as active collision hazards");
    CHECK(result.counts.disabled == 1, "disabled count incorrect");
    CHECK(result.counts.decorations == 1, "decoration count incorrect");
}

int main() {
    testChunkedScan();
    testArrayMutationAbortsBeforeReading();
    testEmptyLevelAndDisabledFiltering();
    if (failures) {
        std::printf("live scan tests: FAILED (%d)\n", failures);
        return 1;
    }
    std::printf("live scan tests: PASS\n");
    return 0;
}
