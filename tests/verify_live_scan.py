from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
mod = json.loads((ROOT / "mod.json").read_text(encoding="utf-8"))

assert '#include "LiveScanCore.hpp"' in main
profile_start = main.index("static ibscan::ObjectProfile liveObjectProfile(GameObject* object)")
live_start = main.index("void liveLevelScanSlice(PlayLayer* pl)", profile_start)
live_end = main.index("std::string scanStatus() const", live_start)
live = main[profile_start:live_end]
assert "objectAtIndex" in live
assert "getObjectRect(" not in live
assert "simulate(" not in live
assert "m_isDisabled" in live and "m_isGroupDisabled" in live
assert "objectCountChanged" in live and "restarted" in live
assert "live level scan started" in live
assert "live level scan progress" in live
assert "live level scan complete" in live
scan_slice = main[main.index("void scanSlice() {"):main.index("// Save the route so far", main.index("void scanSlice() {"))]
assert scan_slice.index("liveLevelScanSlice(pl)") < scan_slice.index("if (!scanning()) return;")
assert "if (!pl->m_isPaused)" in scan_slice
assert "Auto scan progress" in scan_slice
assert "Auto scan finished" in main
assert "read-only" in mod["settings"]["auto-scan"]["description"]
print("live scan static checks: PASS")
