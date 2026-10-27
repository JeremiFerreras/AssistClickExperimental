# Live level scan: implementation and validation

Date: 2026-10-27

## What changed

- A live, read-only inventory pass now reads `PlayLayer::m_objects` while the level is running.
- The pass processes at most 512 lightweight object records per rendered frame (96 while the route scan is also running), reports start/progress/completion to the console, and counts hazards, solids, slopes, orbs, pads, state-changing objects, disabled objects, and decorations.
- It does not call collision-rectangle functions, simulate players, activate orbs/pads/portals, or retain `GameObject*` pointers between frames.
- If the object-array size changes while a pass is in progress, the pass discards its partial counts and starts again from index zero. After repeated changes, it aborts rather than continue with an invalid cursor.
- The route/physics search remains a separate scan that starts on pause. This is intentional: its simulation can touch shared game-object state, so letting it advance over a moving level would risk changing real gameplay state.

## Automated checks run

- `tests/core_tests.cpp`: **666 passed, 0 failed**.
- `tests/live_scan_tests.cpp`: **PASS**, including AddressSanitizer and UndefinedBehaviorSanitizer.
- `tests/verify_live_scan.py`: **PASS**.
- `tests/verify_video_profile.py`: **6/6 passed**.
- `mod.json`: valid JSON.
- `tests/live_scan_simulation.cpp`: **PASS** against 25,000 synthetic object profiles in 49 frame slices, capped at 512 reads per slice. See `live_scan_simulation_output.txt` for the console transcript.

## What has not been verified

A Geometry Dash/Geode build and a real gameplay run were **not** possible in this environment. CMake stops because the `GEODE_SDK` environment variable is not configured. The 25,000-object simulation is a deterministic unit-style demonstration of the batching/counting logic; it is not an execution inside Geometry Dash and does not prove collision prediction accuracy. Complete `IN_GAME_CHECKLIST.md` after building with the matching SDK.

The reference implementation at [StarryDawn72/open-source-gd-project](https://github.com/StarryDawn72/open-source-gd-project) identifies its verified source version as Geometry Dash 2.2081 and warns that the repository is not yet fully updated for 2.209. The live pass therefore avoids collision functions whose behavior and side effects are unrelated to a metadata inventory.
