# Split screen design

## Overview

SCUMM games draw everything into one 320x200 frame: the scene and text line at the top, and a
verb and inventory bar in the bottom rows (from the verb virtual screen's `topline`, 144 for most
games). The Android backend can show that frame as two stacked areas:

- **Top area:** rows above the split, with their aspect ratio kept, centered.
- **Bottom area:** the interface, drawn by the backend.

Everything is drawn from the game's own texture, so the game logic, fonts and colors are
untouched.

## Parts

### OSystem API (`common/system.h`)

- `kFeatureSplitScreen`: turns the layout on or off. The split row is passed in the transient
  config key `split_screen_row`.
- `setSplitScreenElements()`: the engine reports `SplitScreenElement`s (a rectangle in game
  coordinates and a group id) so the backend can lay them out.

Group ids: 0 sentence line, 1 verbs, 2 inventory items, 3 inventory arrows, 4 and 5 boxes shown
whole (verb box, inventory box), 6 verb texts inside the verb box.

### Backend

- `backends/graphics/windowed.h`: split state, the area computation and the two coordinate
  conversions (`convertWindowToVirtual`, `convertVirtualToWindow`), which map taps in the bottom
  area to the game position of what was tapped.
- `backends/graphics/split-screen-layout.h`: the layout of the bottom area. It has three modes:
  - plain: the bottom rows of the frame stretched to the area;
  - rows: sentence line, then verbs and inventory as rows of separately scaled texts, at most 4
    verb columns (the verbs of extra columns go on a last row);
  - boxes: whole boxes of the frame, stacked. The verb box is redrawn in column slices so that its
    columns are equal and its texts keep their proportions.
- `backends/graphics/opengl/opengl-graphics.cpp`: draws the cells from the game texture.
- `backends/graphics/android/android-graphics.*`: enables the feature and forwards the elements.

### SCUMM engine

- `engines/scumm/gfx.cpp`: `updateSplitScreen()` enables the layout when the verb screen exists
  (SCUMM v0-v5, not Mac, FM Towns or NES) and disables it otherwise, for example in cutscenes.
- `engines/scumm/verbs.cpp`: `updateSplitScreenElements()` reports the interface elements once
  per frame when they change. Zak and Maniac Mansion report text verbs and inventory entries;
  Fate of Atlantis and Monkey Island (VGA) report the boxes to show whole.

## Adding a game

Report its interface in `ScummEngine::updateSplitScreenElements()`. For games whose interface is
bitmaps, report group 4 and 5 boxes (and group 6 verb texts if the verb box has a grid of
buttons). Games that report nothing use the plain stretched strip.
