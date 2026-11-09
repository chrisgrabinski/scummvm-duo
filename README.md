# ScummVM Duo

An **unofficial** fork of [ScummVM](https://www.scummvm.org/) for Android, aimed at **dual-screen
Android devices such as the Retroid Pocket Duo or the AYN Thor**. It shows classic SCUMM
adventures the way a dual-screen handheld is meant to be used:

- **Top screen:** only the scene (and the game's text line), scaled to the available space.
- **Bottom screen:** the interface, laid out for touch: the sentence line, the verbs and the
  inventory stacked vertically, with larger text and bigger tap targets.

**Current state:** the two areas are drawn stacked in a single app window. That works on a
normal phone or tablet and on a device where one window spans both panels. It has **not** been
tried on a Retroid Pocket Duo or an AYN Thor, and **sending each area to its own physical
display is not implemented yet**. So on a handheld with two separate displays, the app currently
shows both areas on one of them. Driving the second display is the main piece of work still to do.

This fork is not affiliated with or endorsed by the ScummVM team. For the regular ScummVM, see
the [official site](https://www.scummvm.org/). The original project README is kept in
[README.scummvm.md](README.scummvm.md).

## Status

Checked on a Pixel 9 Pro by looking at the result (Zak, Fate of Atlantis and Monkey Island VGA), not by automated tests:

| Game | Layout |
| --- | --- |
| Zak McKracken (v2); Maniac Mansion (v1/v2) uses the same code but was not looked at | Sentence line, verbs in at most 4 columns (extra verbs on a last row), inventory 2 per row with scroll arrows |
| Indiana Jones and the Fate of Atlantis | Whole verb box, stretched to full width with equal columns, above the whole inventory box |
| The Secret of Monkey Island (VGA) | Same as Fate of Atlantis |
| Other SCUMM v3-v5 games | Scene on top, the original bottom strip stretched below it |
| SCUMM v6+ and HE games, Mac / FM Towns / NES versions | Unchanged (normal full-screen layout) |

Everything else in ScummVM behaves as in upstream. Only the SCUMM engine is wired up.
Cutscenes without a verb bar use the normal full-screen layout.

Known limitations:

- Layouts for Fate of Atlantis and Monkey Island rely on fixed screen positions measured from
  the VGA versions; other releases (EGA, Amiga, translations) may look off.
- Zak and Maniac Mansion: the sentence line is scaled like the original, so it is smaller than
  the verbs.
- Only tested on one device (a Pixel 9 Pro, single screen) and a handful of rooms.
- On a very tall portrait screen there is empty space between the scene and the interface.

## Using it

1. Build the APK (see [docs/duo/BUILDING.md](docs/duo/BUILDING.md)).
2. Install it, open ScummVM, add your own copy of the game and start it.

Tapping works best in mouse touch mode (tap to move the pointer and click). Choose it with the
touch-mode button at the top right, or for all 2D games with `touch_mode_2d_games=mouse` in
`scummvm.ini`. The layout does not change the touch mode itself.

To turn the layout off, add this to `scummvm.ini` (in the app's data folder):

```
dual_screen=false
```

Debug builds install as `org.scummvm.scummvm.debug`, so they sit next to the official app.
Release builds use the official application id; change `applicationId` in
`dists/android/build.gradle` if you want a distinct app.

## How it works

See [docs/duo/DESIGN.md](docs/duo/DESIGN.md). To keep the fork up to date with ScummVM, see
[docs/duo/MAINTAINING.md](docs/duo/MAINTAINING.md). In short, the SCUMM engine tells the backend where
its interface elements are on the 320x200 game screen, and the Android OpenGL backend redraws
them in the bottom area and maps taps back to the original positions.

## Game data

No game data is included or provided. You need your own copies of the games.

## Credits and license

- [ScummVM](https://www.scummvm.org/) and its contributors (see [AUTHORS](AUTHORS) and
  [COPYRIGHT](COPYRIGHT)), which this is based on. Upstream commit this fork starts from:
  `e9c3c1b4f54a29db1cf41d9707342d8124d06a59`.
- [Oboe](https://github.com/google/oboe) (Apache-2.0) is needed to build the Android audio
  backend; it is not included in this repository.
- The dual-screen changes were written with AI assistance (Claude, by Anthropic) and directed and
  reviewed by the repository owner. ScummVM's
  [AI-GUIDELINES.md](AI-GUIDELINES.md) apply to contributions to the ScummVM project; these
  changes are not meant for upstream as they stand.

ScummVM is licensed under the GPL, version 3 or later (see [COPYING](COPYING) and
[LICENSES](LICENSES)). The changes in this fork are released under the same license.
