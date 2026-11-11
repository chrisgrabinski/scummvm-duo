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
  (SCUMM v0-v6 without Humongous Entertainment games, not Mac, FM Towns or NES) and disables it otherwise, for example in cutscenes.
- `engines/scumm/verbs.cpp`: `updateSplitScreenElements()` reports the interface elements once
  per frame when they change. Zak and Maniac Mansion report text verbs and inventory entries;
  Fate of Atlantis and Monkey Island (VGA) report the boxes to show whole.

## Wide rooms

The scene is scaled to the full height of the top area. A 320 pixel wide room is then centered, and
where the area is wider than that, a room that scrolls is shown wider than the original screen:

- `OSystem::getSceneColumns()` tells the engine how many game columns fit across the top area when
  the scene rows fill its height (`WindowedGraphicsManager`, from the same numbers as the layout).
- `ScummEngine::updateWideScreen()` asks this when a room starts, after a load and once per frame, and
  `setScreenWidth()` changes `_screenWidth` to 320 plus the same number of extra columns at each side
  (`_wideMargin`, whole pairs of strips, at most the room width and 640). The virtual screens, the
  text surface and the backend screen are created again; the verb screen keeps its content.
- The scripts keep seeing the 320 pixel screen in the middle. The camera limits they set are moved
  in by the margin (`getCameraLimits()`), so the same part of the room is shown, and the camera
  position they read is the one they asked for once the camera is as close as it gets
  (`scriptCameraX()`). Screen positions from scripts are shifted by the margin
  (`screenXOffset()`): text that is not above an actor, verbs drawn on the scene, boxes, the mouse
  position, blast objects and grabbed cursors. Nothing is shifted on the verb screen, so the
  interface stays in the first 320 columns of the frame, and the backend takes it from there
  (`interfaceWidth()`).
- A widened frame keeps the pixel shape of the 320x200 one when aspect ratio correction is on.

It is used by the games with a split layout and by Sam & Max, not with Hercules, CGA or EGA
rendering, and can be turned off with `wide_rooms=false`. The Curse of Monkey Island is not widened:
its 640x480 screen is centered at the full height.

What scripts place relative to the edges of the 320 pixel screen stays where it was, for example an
actor waiting just outside it is now in view. The original pause and save screens are drawn at
their usual place, at the left of a widened screen.

## Sam & Max

The inventory box of Sam & Max is a set of room objects drawn over the scene, so the scene and
the box can never be shown apart by moving rectangles. `OSystem::setScreenSnapshotMode()` lets the
engine ask the backend to keep a copy of the latest frame (`kSnapshotRolling`) and, while the box
is open (room object 995), to show the game screen twice, stacked: the copy from just before the
box opened on top (the scene, frozen) and below it the live screen with everything that is
unchanged from that copy painted black, so that only the box remains. Taps on the upper screen are
ignored; taps on the lower one go to the game. The box is still opened and closed with the game's
own button.

The lower screen (`kSnapshotLowerScreen`) shows the box all the time. The engine draws the box
and its icons (the objects script 59 would draw) offscreen with the regular object code, by
temporarily pointing the main virtual screen at a spare buffer, and gives the result to the
backend with `setLowerScreenImage()`. Taps there arrive as `EVENT_LOWER_SCREEN_TOUCH` and make the
engine run script 60, the box's click script, with the position of the tap; while it runs, scripts
59 (draw the box) and 62 (close it) are not run, only the verb that closing would start. The game
uses the cursors 875-878 for look, pick up, talk and use inside the box and 883-886 in the scene,
so the engine converts between them around the call. The button that opens the box is actor 1, which
the game keeps in the bottom left corner of the scene (clicking it opens the box); it is kept out
of the room.

## The Curse of Monkey Island

The inventory chest of The Curse of Monkey Island is made of blast objects (the chest, the scroll
arrows and one per item), which the game's scripts queue every frame while it is open; the game finds what
was clicked by testing the mouse position against them. While the chest is open, the engine draws these
objects into a separate image (`drawBlastObjectsToLowerScreen()`) shown on the lower screen with
`kSnapshotLowerScreen`, and does not draw them on the main screen. A tap on the lower screen is pushed
back as mouse events at the same position, so the game's own code handles it. The game still freezes
the camera while the chest is open.

The chest is always on the lower screen. When the game has not opened it, `setupComiChest()` runs the
drawing script (47) itself, keeps the resulting blast objects and queues them again each frame, so the
game can hit-test them. Details that took some care:

- Presses on the lower screen count only on items and arrows (or on the coin), so an empty part of the
  chest never clicks through to the scene. The game is put in its inventory mode (variable 312 = 3) for a
  tap and, while the pointer hovers there, for as long as it stays, so the cursor reacts to items.
- The verb coin is drawn on the main screen, not as blast objects. While it is open after a press on the
  lower screen, the top screen is frozen (`kSnapshotFrozenTop`) and a circle around the coin is copied from
  the main screen over the chest at the end of the frame (`finishLowerScreen()`). The circle's size and
  center were measured by eye from screenshots.
- A right click on the top screen would open the game's own inventory, so it is dropped while the
  chest is on the lower screen.
- The chest is shown only while the hero is in the room, not in menus or cutscenes.

## Monkey Island, Monkey Island 2 and Day of the Tentacle

These games keep the verbs in the left 144 columns of the verb screen and the inventory at their
right. `updateSplitScreenElements()` reports the verb texts (group 6), a verb box (group 4) and an
inventory box (group 5), and the layout shows the two boxes whole, one above the other.

- The verb box is stretched to the full width, with equal columns and the texts centered in them
  (`addStretchedBox()`).
- Monkey Island 2 also reports its inventory slots (group 7): two scroll arrow cells and a 4 by 2 grid
  of 40 by 24 pixel slots, from measurements of what the game draws. They go through the same code: the
  slot columns are of equal width and stretched, filled with a plain background column, and each icon
  is kept at its size and centered. The first column (the arrows) keeps its own width at the left,
  with no extra background. The arrow cells are always reported, as the arrows come and go.
- A conversation replaces the verbs by lines of text, which are wider than the verb box. It is
  detected by a text verb wider than 150 pixels (Monkey Island 2), or by a text verb at the left edge
  (Monkey Island 1: its lines are small and narrow, and no normal verb starts there). Then the lines are reported as rows (group 1), the
  first line included, which would else look like the sentence line, and there are no boxes.

After a change to `split-screen-layout.h`, touch the `.cpp` files that include it (the build does
not notice a changed header).

## Adding a game

Report its interface in `ScummEngine::updateSplitScreenElements()`. For games whose interface is
bitmaps, report group 4 and 5 boxes (and group 6 verb texts if the verb box has a grid of
buttons). Games that report nothing use the plain stretched strip.
