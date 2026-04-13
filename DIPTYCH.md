# Diptych — Build Summary & XTeink Porting Guide

> *DIPTYCH*
> */ˈdiptik/  noun*
> *An artwork made of two related panels that are meant to be viewed together as one piece.*

A dual-world, 1-bit pixel-art puzzle game about two halves of a figure trying to become one again. Three chapters, 15 shards, one hinge. Currently deployed as a browser game at **https://jlud7.github.io/Diptych/**. This doc captures what we built and how to port it to the XTeink X4 e-ink device.

---

## Part 1 — What we built

### The game at a glance

- **Bounded world**: 9×9 grid of rooms (81 total), max-axis radius 4. Trying to walk past the edge hits a visible wall.
- **Two worlds**: every room exists simultaneously as a Light world (black figures on cream) and Shadow world (cream figures on black). The player walks in both at once.
- **15×15 tile grid per room**, 24px tiles, 480×800 canvas.
- **Three chapters**, each introducing a new mechanic that layers on top of the previous ones:
  - **Chapter I — The Surface Rift** (5 shards): split-mode push puzzles
  - **Chapter II — The Wound Beneath** (5 void shards): pressure plates + doors that open only while weighted
  - **Chapter III — The Hinge** (5 echo shards): mirror tiles that invert movement for a burst
- **15 puzzles total**, each hand-authored with specific wall layouts that force multi-step, multi-session push paths (Zelda-style difficulty).
- **5 hearts of HP**. Ghouls patrol some rooms and chase you one tile per step. Contact costs a heart and respawns you at the Watcher's room. Losing all 5 resets the current chapter's progress.
- **8 easter egg rooms** at the corners and edges of the world, with lore, signatures, and atmospheric flavor.
- **Souls/Bloodborne-voiced dialogue** throughout. Nothing in the game explains itself.

### Game content breakdown

| Category | Count | Locations |
|---|---|---|
| Tutorial rooms | 5 | Watcher (0,0), Split Light sign (1,0), Split Shadow sign (2,0), Wall (3,0), Sage (4,0) |
| Ghoul encounters | 3 | The Ambush (1,-1), patrols at (-1,-1) and (1,2) |
| Chapter 1 shard puzzles | 5 | (0,-1), (-1,0), (1,1), (2,-1), (0,2) |
| Chapter 2 void puzzles | 5 | (0,-3), (-3,0), (3,3), (-2,-2), (2,3) |
| Chapter 3 echo puzzles | 5 | (0,-2), (-2,0), (2,2), (-2,2), (2,-2) |
| Easter egg rooms | 8 | See below |
| **Hand-crafted total** | **31** | |
| Procedural "hallway" rooms | 50 | Everything else in the 9×9 |
| **Grand total** | **81** | |

### Easter egg rooms

- **(4, -4) The Graveyard** — headstones bearing epitaphs of lost travelers. One is the author's signature: *Here begins the one who carved the hinge. — James — @JLUDDY7*
- **(-4, 4) The First Diptych** — two figures standing side by side in both worlds, with a plaque: *Before the panels parted. Before the hinge. There was only a figure.*
- **(-4, 0) The Empty Watcher** — a mirror of the starting room but she is gone. *The one who waits is not here. She has gone to look for what she lost. Or perhaps she was never here at all.*
- **(0, 4) The Bell** — silent bell connected to the Sage's dialogue. *The bell has already rung. You could not have heard it. You were not yet two.*
- **(0, -4) The Sunken Shrine** — water frames a silent marker. *Here the depth becomes deeper. The bottom is still above us. Do not look down.*
- **(3, 4) The Two Wells** — paired water bodies. *Two wells. One water. Drop something in. Another you will find it.*
- **(3, -3) The Still Pool** — a dormant mirror tile. *The eye is resting. Do not wake it. It dreams of you walking backward.*
- **(-3, -3) The Reflected Grave** — a headstone in light, a prone player sprite in shadow. *Here lies you. You have not yet died. The grave is patient.*

### Controls

| Key | Action |
|---|---|
| ↑ ↓ ← → | Walk in both worlds |
| 1 | Light walks alone (5-step split) |
| 2 | Shadow walks alone (5-step split) |
| Enter | Speak, confirm, collect |
| Escape | End a walk early |
| 0 (dev) | Skip to Chapter 2 (from title screen) |
| 9 (dev) | Skip to Chapter 3 (from title screen) |

### Architecture (current state)

- **Tech**: React 19 + Vite, all game logic rendered to a single `<canvas>` element
- **Source file**: `soul-searcher/src/SoulSearcher.jsx` — one ~2000-line file containing constants, sprites, room definitions, dialogue, state, rendering, input
- **Build**: `npm run build` in `soul-searcher/` → `dist/` with minified JS + CSS
- **Standalone HTML**: `soul-searcher-standalone.html` (root) — all JS + CSS inlined into a single ~240KB file, no external dependencies
- **GitHub Pages**: `docs/index.html` is a copy of the standalone HTML, served from the `main` branch, `/docs` path
- **Public URL**: https://jlud7.github.io/Diptych/
- **Repo**: https://github.com/jlud7/Diptych (remote still accepts the old `Inkborne` URL via redirect)

### Key mechanical concepts (for porting reference)

1. **Tile types**: `EMPTY`, `WALL`, `TREE`, `WATER`, `DOOR`, `SWITCH`, `MIRROR` — each has walkability rules in `isWalkable(tile, doorsOpen)`.
2. **Entities**: live on top of tiles. Types include `player`, `half_light`, `half_shadow`, `npc`, `sign`, `ghoul`. NPCs and blocking signs are impassable; other entities allow walking through.
3. **Combined mode** (default): arrow keys move both the light player and shadow player by the same dx/dy. Room transitions happen when either goes off-grid.
4. **Split mode**: pressing 1 or 2 activates split for 5 steps. One self moves, the other is frozen at the "anchor" position. When steps run out (or Escape), the moving self snaps back.
5. **Push mechanic**: walking into a half-shard pushes it one tile in the direction of movement. Pushes fail if the destination is a wall or another entity.
6. **Collection**: when a light half and shadow half are at the same (x,y) AND the player walks onto that tile, the shard is collected. Triggers a modal.
7. **Pressure plates** (Ch2): `SWITCH` tiles, when pressed (by player or weighted by a shard half), open doors in the *other* world. Non-linked rooms use live pressure only; if you step off or the weight is removed, doors close. Linked rooms (Ch3 only) latch once both plates are pressed simultaneously.
8. **Mirror mode** (Ch3): stepping on a `MIRROR` tile activates inverted movement for 5 steps. Shadow position moves opposite to light. When the 5 steps run out, both snap back to the mirror tile (the "mirror anchor").
9. **Ghouls**: entities with simple chase AI. Each player move, each ghoul moves one tile toward the player using Manhattan-preferring pathfinding. Collision triggers a catch modal, damage, and respawn.
10. **Boundary walls**: `sealBoundaryEdges()` is called on every room produced by the generator. If the room is at rx=-4, rx=4, ry=-4, or ry=4, the outward-facing edge is filled with walls.

---

## Part 2 — Porting to the XTeink X4

### The problem

The current codebase is a React web app. It runs in a browser by drawing to a `<canvas>` element with the Canvas 2D API. It cannot run directly on the XTeink X4, which is an ESP32-C3-based e-ink device with roughly 400KB of RAM and no JavaScript runtime.

To play Diptych on the XTeink X4, the game must be **rewritten in native C or C++** that compiles against the device's SDK (ESP-IDF or Arduino-ESP32). The game *logic* ports cleanly — it's deterministic, grid-based, and uses no frameworks. The *rendering* layer must be rewritten to use the device's e-ink display driver. The *input* layer must be rewritten to read whatever buttons/switches the device exposes.

### Path forward

#### Step 1 — Gather device info (from you)

Before we can port, I need from you:

1. **The SDK**. Which toolchain does your XTeink X4 use?
   - ESP-IDF (raw Espressif SDK, C)
   - Arduino-ESP32 (C++ with Arduino abstractions)
   - PlatformIO (wraps either)
   - Something custom/vendor-specific

2. **The display driver**. Which library talks to the e-ink panel?
   - GxEPD2 (common Arduino e-ink lib)
   - LilyGo / XTeink-specific driver
   - Direct SPI code
   
   And what's the panel's resolution? (e.g., 2.9" 296×128, 4.2" 400×300, 5.83" 648×480, etc.)

3. **The input hardware**. How do you press keys on the device?
   - Physical buttons wired to GPIO pins?
   - Touch?
   - External keyboard over BLE/USB?

4. **A hello-world sketch**. Ideally, the smallest possible program you can give me that:
   - Initializes the display
   - Draws a black rectangle and some text
   - Reads a button press
   - Compiles and runs on your device
   
   This is the most important thing. Without it I'm guessing at pin assignments, library names, and board configs. With it, I have a working skeleton to build on.

5. **Anything salvageable from the old Inkborne build**. I noticed the repo was renamed from `Inkborne` to `Diptych`, and some earlier commits (`c47944c`, `bda8478`) mention ESP32-C3 and e-ink optimization. If there's an existing C++ game loop, display driver, input handler, or framebuffer code from that version, that's a huge head start — we reuse the plumbing and just drop in the new game logic.

#### Step 2 — Translate the game to C

Once I have the SDK and a working skeleton, here's what the port looks like. Everything in the JSX file has a direct C equivalent:

| JSX concept | C equivalent |
|---|---|
| `const W = 480, H = 800` | `#define W 480` etc. — probably shrink to match your panel resolution |
| 24×24 sprite strings `"000111..."` | `const uint8_t player_sprite[24*24]` packed bitmaps |
| `parseRoom(string)` | `static const uint8_t room_XY[15][15]` pre-baked arrays |
| `useState()` values | fields in a single `struct game_state` |
| `setLightWorld(...)` | `state.lightWorld = ...` |
| Movement logic functions | 1:1 translation, same branches |
| `ctx.fillRect()` | `epd.fillRect()` or direct framebuffer writes |
| `ctx.fillText()` | `epd.setFont(); epd.drawString()` or a bitmap font |
| Keyboard event handler | Button interrupt handler or polling loop |

The game state struct would look roughly like:

```c
typedef struct {
    uint8_t chapter;            // 1, 2, or 3
    int8_t room_x, room_y;      // current room coords (-4..4)
    uint8_t light_x, light_y;   // player position in light world
    uint8_t shadow_x, shadow_y; // player position in shadow world
    uint8_t split_mode;         // 0=none, 1=light, 2=shadow, 3=mirror
    uint8_t split_steps;
    uint8_t split_anchor_x, split_anchor_y;
    uint8_t hp;                 // 0..5 hearts
    uint32_t shards;            // bitmask of 5 ch1 shards
    uint32_t void_shards;       // bitmask of 5 ch2 shards
    uint32_t echo_shards;       // bitmask of 5 ch3 shards
    uint8_t ambushed;
    uint8_t doors_latched_l;
    uint8_t doors_latched_s;
    // ... etc
    // current room contents
    uint8_t light_tiles[15][15];
    uint8_t shadow_tiles[15][15];
    entity_t light_entities[16];
    entity_t shadow_entities[16];
    uint8_t light_entity_count, shadow_entity_count;
} game_state_t;
```

Memory budget check on ESP32-C3 (~400KB RAM):
- Game state: a few KB
- Current room tiles: 2 × 15 × 15 = 450 bytes
- All 31 hand-crafted rooms compiled in as `const` (stored in flash, not RAM): negligible RAM cost
- Sprite bitmaps: ~20 sprites × 24 × 24 / 8 = 1.5KB
- Framebuffer for the display: depends on panel size. 400×300 1bit = 15KB
- **Total active RAM**: well under 50KB

Plenty of headroom. The ESP32-C3 can do this easily.

#### Step 3 — Adapt for e-ink specifics

- **Partial refresh vs full refresh**: e-ink full refresh is slow (~1-2 seconds) but gives crisp output. Partial refresh is fast (~300ms) but ghosts. Use partial refresh for player movement, full refresh every N moves or on scene changes (room transitions, modals, dialogue) to clear ghosting.
- **Redraw only on state changes**: the game is turn-based, so there's no animation loop. Each input triggers one redraw.
- **Resolution adaptation**: the current 480×800 canvas may not match your panel. Likely candidates:
  - 296×128 → too small for the full dual-world view. Would need to show one world at a time with a toggle.
  - 400×300 → close; scale tiles from 24px to ~18-20px and fit both worlds vertically.
  - 648×480 → native fit, minimal changes.
  - 800×480 → rotate 90°, fits the 480×800 layout natively.
- **Font**: the game uses Courier New. On ESP32 you'll use a bitmap font from your e-ink library. Pick one that feels ink-like (serif or typewriter).

#### Step 4 — Build, flash, play

With a working SDK skeleton and the ported C code, the final loop is:

1. Compile to an ELF/BIN via the SDK toolchain
2. Flash over USB (ESP-IDF's `idf.py flash` or Arduino's upload)
3. Reset the device
4. The title screen should render on the panel
5. Press a button to begin
6. Iterate on bugs, pacing, partial-refresh artifacts

### What I'll need from you next session

To start the port effectively, open a new session and bring:

1. **The SDK / framework name** (ESP-IDF? Arduino-ESP32? PlatformIO?)
2. **The panel model and resolution** (look on the back of the device, or any docs you have)
3. **A hello-world sketch** that compiles and runs on your device, drawing at least one rectangle and reading at least one button
4. **Any old Inkborne code** you want to reuse (display init, input handler, main loop, etc.) — this is the biggest accelerator if it exists

With those, I can do the port in a focused session without guessing at hardware details.

### What I can do without that info

If the hardware details aren't handy, I can still do preparatory work:

- Extract all the game data (rooms, sprites, dialogue) from the JSX file into a portable format (C header files, JSON, or raw bitmaps)
- Write the core game-logic module in plain C, with a stub rendering interface so it can be unit-tested on a desktop
- Produce a "reference implementation" doc showing exactly how each JSX function maps to C

Say the word and I'll start with that prep work, or wait until you have the SDK info ready.

---

## Dev workflow (current repo)

### Run locally
```bash
cd soul-searcher
npm install
npm run dev            # dev server
```

### Build for web
```bash
cd soul-searcher
npm run build          # produces dist/
```

### Rebuild the standalone HTML (what GitHub Pages serves)
```bash
cd /Users/james/Documents/Diptych
# Regenerate soul-searcher-standalone.html from dist/
# Then:
cp soul-searcher-standalone.html docs/index.html
git add -u
git commit -m "Deploy: <description>"
git push origin main
```

### Git remote
```
origin: https://github.com/jlud7/Inkborne.git  (auto-redirects to Diptych)
```

### Dev shortcuts (on title screen)
- `0` → skip to Chapter 2 with all Ch1 shards pre-collected
- `9` → skip to Chapter 3 with all Ch1 + Ch2 shards pre-collected

Remove these before final release.
