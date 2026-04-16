# Diptych tools

## `diptych-level-builder.html`

A standalone browser room editor you can use to design custom Diptych puzzle rooms **without modifying the game runtime**.

### What it does

- Edits both 15×15 worlds (Light + Shadow) side-by-side
- Paints tiles (`empty`, `wall`, `tree`, `water`, `door`, `switch`, `mirror`)
- Paints entities (`npc`, `sign`, `half-light`, `half-shadow`, `ghoul`)
- Exports/imports a JSON room file for iteration
- Generates a C++ snippet compatible with `DiptychActivity` helper methods

### How to use

1. Open `tools/diptych-level-builder.html` in any browser.
2. Build the room in Light and Shadow panels.
3. Click **Export JSON** to save your room draft.
4. Click **Generate C++ snippet** when you want to manually port the room into a branch.

This keeps your design workflow separate from the game code and lets you "build" levels Mario-Maker style before integrating them.
