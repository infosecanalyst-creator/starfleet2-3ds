# Screen regression test

The port understands Star Fleet II by reading its 80x25 text screen. This test runs every saved
screen in `screens/` through the frontend (sf2ui) on a PC and compares what the port makes of it
with `expected/`: the kind of screen (bridge, menu, map, reader, plain, graphics), the hint line,
every touch button (and whether it is greyed), and any text that does not fit. Text that does not
fit always fails.

    make check     # run the test (CI runs this on every push)
    make update    # after an intended change: rewrite expected/, then review the git diff
    make png       # also render both 3DS screens of every dump into out/ (PPM)

## Adding screens
- From the 3DS: put an empty `debug.txt` in `/3ds/starfleet2/`. Every new screen is saved to
  `/3ds/starfleet2/dumps/NNN.txt` (at most one per 3 s, 300 per launch). Copy the useful ones here
  with a descriptive name, run `make update`, and check the new expected file by eye.
- From the Linux DOSBox harness: `echo name > /tmp/sfdump.req` writes `/tmp/sfdump/name.txt`.

## After a GOG update of the game
Run `make check` against dumps from the new version. A diff means a screen changed: decide
whether the port needs a fix or the expected file just needs `make update`.
