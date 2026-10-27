#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>

// Small, game-independent core for the in-game object inventory scan. The live
// pass only reads GameObject metadata; it deliberately does not ask GD to
// resolve collision boxes, update triggers, or simulate a player.
namespace ibscan {

struct ObjectProfile {
    bool disabled = false;
    bool decoration = false;
    bool hazard = false;
    bool solid = false;
    bool slope = false;
    bool orb = false;
    bool pad = false;
    bool stateChange = false;
};

struct Counts {
    std::size_t total = 0;
    std::size_t processed = 0;
    std::size_t disabled = 0;
    std::size_t decorations = 0;
    std::size_t hazards = 0;
    std::size_t solids = 0;
    std::size_t slopes = 0;
    std::size_t orbs = 0;
    std::size_t pads = 0;
    std::size_t stateChanges = 0;
};

enum class State { Idle, Running, Complete, Aborted };

struct StepResult {
    State state = State::Idle;
    Counts counts{};
    int progressPercent = 0;
    bool progressAdvanced = false;
    bool completed = false;
    bool objectCountChanged = false;
};

class IncrementalLevelScan {
public:
    void reset() {
        m_state = State::Idle;
        m_counts = Counts{};
        m_nextProgress = 10;
    }

    void begin(std::size_t objectCount) {
        m_counts = Counts{};
        m_counts.total = objectCount;
        m_nextProgress = 10;
        m_state = objectCount == 0 ? State::Complete : State::Running;
    }

    State state() const { return m_state; }
    Counts const& counts() const { return m_counts; }

    template <class Reader>
    StepResult step(std::size_t currentObjectCount, std::size_t maxObjects, Reader&& readObject) {
        StepResult out;
        if (m_state == State::Running && currentObjectCount != m_counts.total) {
            // Never continue indexing the previous array layout. The caller can
            // choose to restart from index 0, or abort after repeated changes.
            m_state = State::Aborted;
            out.objectCountChanged = true;
            out.state = m_state;
            out.counts = m_counts;
            return out;
        }

        if (m_state == State::Running) {
            std::size_t n = 0;
            while (m_counts.processed < m_counts.total && n < maxObjects) {
                ObjectProfile const profile = readObject(m_counts.processed);
                account(profile);
                ++m_counts.processed;
                ++n;
            }

            if (m_counts.processed >= m_counts.total) {
                m_counts.processed = m_counts.total;
                m_state = State::Complete;
            }

            if (m_state == State::Running && m_counts.total > 0) {
                int const pct = static_cast<int>((m_counts.processed * 100u) / m_counts.total);
                if (pct >= m_nextProgress) {
                    out.progressAdvanced = true;
                    out.progressPercent = (pct / 10) * 10;
                    m_nextProgress = out.progressPercent + 10;
                }
            }
        }

        out.state = m_state;
        out.counts = m_counts;
        out.completed = m_state == State::Complete;
        return out;
    }

private:
    void account(ObjectProfile const& p) {
        if (p.disabled) {
            ++m_counts.disabled;
            return; // GD skips disabled and group-disabled objects in its collision pass.
        }
        if (p.decoration) {
            ++m_counts.decorations;
            return; // Decorations do not participate in the collision scan.
        }
        if (p.hazard) ++m_counts.hazards;
        if (p.solid) ++m_counts.solids;
        if (p.slope) ++m_counts.slopes;
        if (p.orb) ++m_counts.orbs;
        if (p.pad) ++m_counts.pads;
        if (p.stateChange) ++m_counts.stateChanges;
    }

    State m_state = State::Idle;
    Counts m_counts{};
    int m_nextProgress = 10;
};

} // namespace ibscan
