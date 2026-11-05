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
| D-Pad | Arrow keys |
| Circle Pad | Course -1 / +1 degree (left/right), sublight speed (up/down) |
| ZL | Swap the screens (dashboard on the touch screen, game controls on top) |
| C-Stick up / down | Hyperspace speed +1 / -1 |
| C-Stick | Review earlier messages (bridge) |
| START | Manual |
| SELECT | Quick save on the bridge; keyboard elsewhere |

The built-in manual (START) holds the port guide plus the game's Quick Reference, Operations
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
`sd/3ds/starfleet2/` is the one place the app lives: the build moves `starfleet2.3dsx` there, next
to `starfleet2.conf`; the installer adds `manual.txt` and `game/` (the contents of the GOG install's
`base` folder; the game itself is not part of this repository) and uploads the folder over Wi-Fi
to ftpd (port 5000), checking every file's size on the 3DS.

Logs on the 3DS: `/3ds/starfleet2/debug.log` (speed, audio underruns, text-fit problems every
10 s) and `/3ds/starfleet2/error.log` (fatal errors).

## Licences
- This port is licensed under the GNU General Public License, version 2 or (at your option) any
  later version (GPL-2.0-or-later); the full text is in `LICENSE`. DOSBox and DOSBox-3DS are
  GPL-2.0-or-later (see `dosbox/COPYING`). DOSBox files changed for this port carry a
  "Modified for starfleet2-3ds" notice at the top.
- Terminus Font: SIL Open Font License 1.1 (`assets/OFL.txt`). Tamzen: `assets/Tamzen-LICENSE`.
  X11 Misc Fixed 5x8: public domain.
- *Star Fleet II* and its manuals are (c) Trevor C. Sorensen; buy it on GOG or Steam.
