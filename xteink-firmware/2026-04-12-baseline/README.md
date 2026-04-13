# 2026-04-12 baseline snapshot

The first XTeink X4 firmware backup, captured on 2026-04-12.

## What this represents

- **Firmware binary**: built on 2026-04-10, flashed to the device the same day, and still running at the time of this snapshot. SHA256 `f3d8d32ad0c064ae8817c110b5327b5350ee68acb77097949d53978df0cfb9d5`.
- **Source**: the full `src/activities/game/` and `src/game/` trees from `/Users/james/Downloads/crosspoint-reader`, unchanged since the Apr 10 build.
- **Upstream base**: commit `1df543d` on `crosspoint-reader/master` (`perf: Eliminate per-pixel overheads in image rendering (#1293)`).
- **Diptych state**: Chapter 1 only. 5 shards, basic light/shadow split, no ghouls/plates/mirrors/hearts/easter eggs/bounded world. ~1061 lines of C++ across `DiptychActivity.cpp` (938) and `DiptychActivity.h` (123). This is the "pre-port" baseline — the next snapshot should capture the full-parity port that lines up with the web game on `jlud7/Diptych`.

## Contents

- `firmware.bin` — ESP32-C3 image (5.8 MB)
- `SHA256SUMS` — checksums of everything in this folder
- `archive-README.md` — the README from the original 2026-04-10 local archive at `~/Documents/XTEINK-JLL/custom/diptych-firmware-archive-2026-04-10/`, included for provenance
- `SHA256SUMS.archive` — the original archive's checksums (included for cross-check)
- `tracked-ui-diff.patch` — diff for tracked UI/theme files touched by the Games icon/menu integration, from the original archive
- `activities-game/` — every `.cpp`/`.h` in `src/activities/game/` at time of snapshot (~28 files, 13 games + picker)
- `game/` — every `.cpp`/`.h` in `src/game/` at time of snapshot (shared dungeon/renderer/save/state/types — used by the other games, not Diptych)

## Verification performed in the original build

From the archive README:
- `pio run -e default` succeeded
- `pio run -e default -t upload --upload-port /dev/cu.usbmodem1101` succeeded and verified hashes
- `git diff --check` produced no whitespace errors
- Serial monitor confirmed Diptych normal input uses fast refresh; initial entry/room-style redraw uses full refresh
- Serial monitor sample after upload showed no panic

## What's not included

- `firmware.elf` (~43 MB) and `firmware.map` (~15 MB). Locally available at the original archive path; regenerable by rebuilding the source in this snapshot.
- Non-game firmware source. The Games suite was integrated via edits to `HomeActivity`, `ActivityManager`, multiple theme files, and a few others; `tracked-ui-diff.patch` captures the tracked-file portion of those edits.
