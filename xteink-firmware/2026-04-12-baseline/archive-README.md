# Diptych Firmware Archive

Created: 2026-04-10
Firmware checkout: /Users/james/Downloads/crosspoint-reader
Base commit: 1df543d on master
Device upload port used: /dev/cu.usbmodem1101

## Contents
- build/firmware.bin: final flashed firmware image.
- build/firmware.elf: linked firmware ELF for debugging/symbols.
- build/firmware.map: linker map for this build.
- diptych-selected-source.tar.gz: selected source snapshot for Diptych, game picker, Games icon, and relevant UI files.
- tracked-ui-diff.patch: diff for tracked UI/theme files touched by the Games icon/menu integration.
- SHA256SUMS: checksums for archive integrity.

## Verification Performed
- pio run -e default succeeded.
- pio run -e default -t upload --upload-port /dev/cu.usbmodem1101 succeeded and verified hashes.
- git diff --check produced no whitespace errors.
- Serial monitor confirmed Diptych normal input uses fast refresh; initial entry/room-style redraw can use full refresh.
- Serial monitor sample after the final upload showed no panic; prior GFX out-of-range spam was addressed by removing an overflowing bottom hint row.

## Notes
This archive intentionally does not copy the whole dirty firmware tree. It contains the final binary artifacts plus the selected files needed to recover the Diptych work. The firmware checkout also had unrelated pre-existing local changes and untracked game/cover-grid files before this archive was made.
