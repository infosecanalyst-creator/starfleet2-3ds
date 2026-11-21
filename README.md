# Star Fleet II: Krellan Commander for the New 3DS XL

> **Bring your own game files.** No game data is included: you need your own copy of *Star Fleet II: Krellan Commander* (GOG or Steam).

> Not affiliated with or endorsed by the game's rights holders, the DOSBox team or Nintendo. Star Fleet is a trademark of its owner, used only to say which game files it runs.

A split-cockpit frontend for the GOG release of *Star Fleet II: Krellan Commander* (v2.0,
Trevor C. Sorensen & Mark L. Baldwin). The unmodified DOS game runs inside a DOSBox-3DS fork;
the frontend reads the game's 80x25 text screen and redraws it across both 3DS screens:

- **Top screen: everything you watch.** Alert lights, communications, energy, long-range scan,
  damage, data, velocity, mission time and status, in Terminus 6x12, tiled edge to edge.
- **Touch screen: everything you touch.** The tactical display (Terminus 8x14), navigation, action
  buttons, the game's own command menu (read live from the screen) and one-tap view tabs.

Other displays are split by content so nothing appears twice: menus become big touch buttons,
maps (star, system and war maps, Combat Control) go on the touch screen to be tapped, and the
rest of the screen stays on top.

## What the stylus adds
The PC game is keyboard-only. On the 3DS:
- **Tap to steer:** tap the tactical display and the port turns the ship toward that point
  (it presses the course keys for you, 15 degrees and 1 degree at a time).
- **Tap to target:** when the game's target box is up, a tap drives it onto the star, planet or ship
  you touched (closed loop, snapping to the nearest object); tap it again to select.
- **One-tap sign-on:** the officer you sign on with is remembered (`/3ds/starfleet2/officer.txt`).
- **Tap menus:** every command menu, `[M]: menu`-style key hint, `C  Change settings`-style menu
  line and yes/no question becomes a button.
- **Keyboard only when needed:** it opens by itself for your name, password and notepad.
- Every touch button clicks and visibly presses in. Long prompts scroll instead of being cut off.

## Buttons
| Button | Action |
|---|---|
| A / B | Enter / Esc |
| X / Y | Fire phasers / torpedoes |
| L | Freeze / resume the simulation |
| R | Tap mode: auto (default), steer, target |
| D-Pad | Arrow keys (menus, target box) |
| Circle Pad | Left / right: course -1 / +1 degree (hold to keep turning). Up / down: sublight speed +0.1 / -0.1 |
| ZL | Swap the screens on the bridge and in the manual (other screens stay unswapped so they stay readable). Swapped, the touch screen is a touchpad: slide moves the pointer, a quick still tap presses |
| ZR | Button ring: the D-Pad moves a gold ring over the touch buttons, A presses the ringed one, B or ZR goes back |
| C-Stick up / down | Hyperspace speed +1 / -1 |
| C-Stick left / right | Bridge: review earlier messages |
| Sticks (off the bridge) | Scroll orders, reports and other long pages: up / down a line, left / right a page |
| START | Menu: RESUME, MANUAL, GAME SETTINGS, EXIT GAME (asks first); the menu header shows the build version |
| SELECT | Bridge: quick save under the name of your last save (the game's Ctrl+Q). Elsewhere: show or hide the keyboard |
| LEFT HAND (manual) | Swaps the D-Pad with Y / A / B / X (remembered in `/3ds/starfleet2/lefty.txt`) |

The built-in manual (START, then MANUAL) holds the port guide plus the game's Quick Reference, Operations
Manual and Training Manual, built from **your** GOG PDFs by `tools/mkmanual.py`. Manual controls:
sticks scroll, D-Pad moves between sections, L/R previous/next, tap a section, B or START closes.

## Layout
```
sf2ui/    the frontend (portable C++: layout, fonts, input); fonts.h is generated
glue/     DOSBox glue (screen reader, key injection) and the 3DS platform layer
dosbox/   DOSBox-3DS fork (MrHuu/dosbox-3ds, DOSBox SVN r4483); hooks are under SF2_3DS
tools/    mkfont.py (Terminus -> fonts.h), mkmanual.py (manual from your GOG PDFs)
sd/       files for the SD card (DOSBox settings)
```

## Building and installing
Needs devkitPro with `3ds-dev` and `3ds-sdl` (SDL 1.2), plus Python 3 with `pdfplumber`.
```
./build-3ds.sh
./install-3ds.sh <3DS-IP> "/path/to/STAR FLEET II - Krellan Commander Version 2.0"
```
`sd/3ds/` mirrors the SD card. The build moves the app to `sd/3ds/starfleet2.3dsx` (on the card it
lives directly in `/3ds`, as `sdmc:/3ds/starfleet2.3dsx`); `sd/3ds/starfleet2/` holds
`starfleet2.conf`, and the installer adds `manual.txt` and `game/` there (the contents of the GOG
install's `base` folder; the game itself is not part of this repository). It uploads both over
Wi-Fi to ftpd (port 5000), checking every file's size on the 3DS.

CI builds every push to `main` and puts `starfleet2.3dsx` (with the build log and
`starfleet2.elf.gz` / `starfleet2.sym.gz` symbols for crash addresses) on the `builds` branch.

Logs on the 3DS: `/3ds/starfleet2/debug.log` (start, exit, write failures, text-fit problems) and
`/3ds/starfleet2/error.log` (fatal errors). An empty `/3ds/starfleet2/debug.txt` turns on the full
debug log (speed and audio underruns every 10 s, key presses), buffered and capped at 2000 lines.

## Licences
- This port is licensed under the GNU General Public License, version 2 or (at your option) any
  later version (GPL-2.0-or-later); the full text is in `LICENSE`. DOSBox and DOSBox-3DS are
  GPL-2.0-or-later (see `dosbox/COPYING`). DOSBox files changed for this port carry a
  "Modified for starfleet2-3ds" notice at the top.
- Terminus Font: SIL Open Font License 1.1 (`assets/OFL.txt`). Tamzen: `assets/Tamzen-LICENSE`.
  X11 Misc Fixed 5x8: public domain.
- *Star Fleet II* and its manuals are (c) Trevor C. Sorensen; buy it on GOG or Steam.
