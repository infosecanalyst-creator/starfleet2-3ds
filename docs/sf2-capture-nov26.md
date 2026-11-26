# Star Fleet II capture report (DOSBox harness, 2026-11-21 session)

Capture stopped early on the coordinator's instruction. 64 new dumps were added to `/home/claude/sf2-3ds/tests/screens/` (nothing else in the repo was touched, nothing committed).
Rows/columns below are 0-based on the 80x25 text screen. "V" = verified on screen, "I" = inferred.

Sessions used: Tutorial 2 (system SS-20, planets B and C) for A and the first fight; Practice mission 029 (region 1-A) for B, the disable-mode fight and D.

## Not done / gaps (read first)

- **B was not completed.** No mineral-rich planet was found, so the "completed" state was never seen.
- **No armed enemy was met.** Every enemy was a freighter or starliner; none fired. No "enemy firing", shield damage, hull damage, "you destroyed" or escape was recorded. (The ops manual says Level 1 has no enemy warships.) No "miss" message was ever seen either.
- **The port's own rendering of the EGA planet map was not captured.** The `shot` taken then shows the port's bottom screen on its own title menu (CONTINUE / BACK / MANUAL), because the port UI overlay was never dismissed in my runs; the top screen showed the stale bridge dashboard. Only the DOSBox window picture shows the map.
- Table hotkeys `X` (eXplore) and `C` (Combat) were not pressed; boarding a disabled ship was not reached; Tutorial 4 (Weapons) was not run.
- Harness state left behind: DOSBox still running, practice mission 029 in progress (ship moving at 0.3 in SS-17, system map open, 0 torpedoes). Practice Mode is probably left **On** (toprac.sh switches it on). Mission sequence number is now 029 (was 028), Tutorial 2 is now marked `*`, and `AUTOSAVE.029` exists.

## A. Orbit sequence

### A1. Entering orbit and the scan report (V)

1. Row 24 cols 0-23: `Establish orbit?` Accepts `y` / `n`.
2. After `y`, COMMS (rows 15-22, cols 1-39): `Report from Helm:` / `Entered orbit of planet SS-20B.` / `We're in a standard high orbit.` / `Altitude is 21100 km.` Panel title becomes `ORBITAL`, Mode `IN ORBIT`. Row 24 shows `Select option` for about a second.
3. Row 24 prompt goes blank; COMMS is cleared and shows only `             Scanning planet` (row 18). Lasts about 3 s.
4. COMMS fills with the report, and a small box opens in the middle display:
   - Row 15: `          SHORT RANGE SCAN`
   - `OBJECT: Planet            POSN: 47,37`
   - `ID: SS-20B          NAME: Unknown`
   - `ANALYSIS: Class K/Krella-like`
   - `Mammalian/Tech Lvl=4:medium-industry`
   - `Rich minerals; supplies available.`
   - `Planet has not been surveyed.`
   - `No shields up-no active plt defenses`
   - Box: row 11 top border, **row 12 cols 28-49 `Display planetary map?`**, row 13 bottom border (cols 27-52). It covers the Mode-row area to the right of `IN ORBIT`.
   - Row 24 cols 0-23 stay **blank**. This is the "empty prompt": the question is the box in the middle display, not row 24.
5. Keys at this question (V): `Esc`, `Space`, `Enter`, `x` do nothing (screen identical after each). Only `y` and `n` are accepted. Mission clock stands still while it waits.
6. Other report variants seen (V): gas giant: `ANALYSIS: Class G/gas giant` / `Sensors show no intelligent lifeforms.` / `Gas giant is of no value at all except` / `to dispose of captives.` / `High level of radioactivity.`; desert: `ANALYSIS: Class D/desert` / `Humanoid/Tech Lvl=5:medium-nuclear` / `Fair minerals; supplies available.`; `No minerals; supplies available.`; with ships present the report is followed by `UGA freighter in orbit!` / `UGA starliner in orbit!` (one line per ship), which scrolls the first lines out.

### A2. Answering `y`: the planetary map (V)

- Screen switches to EGA graphics; the text dump is 25 blank rows (`#vgamode=2`). DOSBox window picture: title `Map of Planet SS-20B`, colour surface map on the left, globe top right, data column on the right (`NAME: unknown`, `CLASS: K Krella-like`, `DIAMETER`, `GRAVITY`, `ATMOS: Nominal`, `SUPPLIES: avail`, `MINERALS: rich`, `RADIOACT: no`, `LIFEFORMS: Mammalian`, `TECH LVL: 4/medium-industry`, `POPULATION: 83 million`, `CITIES: 15/ 15  DAMAGE: 0 %`, `PLT DEF: No  SHIELDS: 0`, `MILITIA: ?`, `UGA TROOPS: ?`, `TREATY: No`, `OWNERSHIP: UNKNOWN`, `BASE: No  LP TEAMS: 0`, `LEGIONS: 0  INV.ZONES: 0`), bar `IN ORBIT:  1 BC 0 DS 0 SS 0 TS  0 LGN`, bottom `RED ALERT` / `TIME:`. Pictures kept at `/tmp/sfw/keep_planet_map_ega.png` (DOSBox window) and `/tmp/sfw/pm1v.png` (port shot, see gaps).
- Leaving: `Enter`, `Space`, `x`, `n` do nothing. **Only `Esc` leaves.**
- After `Esc`: text screen with row 11 `Returning control to main bridge` + `Reporting to bridge` (about 1-2 s), then the bridge with `Select option`.
- After the scan the orbital display carries extra labels: row 2 `Shld:` and `LP:N`, row 3 `PD:N` and `Bs:N`, row 4 `T:N`, row 11 `Sp:Y`, row 12 `Cls:K` and `TchL:4`.
- **In this one run the landing-parties question did not appear after the map.** The bridge went straight to `Select option` (COMMS showed `Cancelling red alert.` / `Shields lowered.` / `Changing alert status to ORANGE`). Seen once; whether `y` always skips the question is not established.

### A3. Answering `n`: the landing-parties question (V, three planets)

- COMMS: `Report from Executive Officer:` / `Excellency, should we send down` / `landing parties to this planet?`; row 24 `Answer?`. Accepts `y` / `n`.
- `n`: COMMS adds `As you wish, sir.` / `No landing parties assigned.`; row 24 `Select option` with the top menu.
- `y`: COMMS adds `By your command, sir.` / `ENGINEERING CONTROL` / `TELEPORTERS` and the table opens (same table as `E`,`T`,`L`).
- Gas giant: after `n` to the map question there is **no** landing-parties question (V).

### A4. Landing-party table (V unless marked)

Position: rows 15-23, cols 40-79; COMMS stays visible on the left. Row 24: `Assign mission/personnel` + one menu item `Activate`.

```
      Mission       |Tms|RegCrw|ShkTrp
eXplore/survey      | 0 |   0  |   0
Establish base      | 0 |   0  |   0
Hold/reinforce      | 0 |   0  |   0
Obtain supplies     | 0 |   0  |   0
Combat              | 0 |   0  |   0
Negotiate           | 0 |   0  |   0
Supply:  0/  0dTotal| 0 |   0  |   0
```

- The selected row is shown by attribute 0x70 (black on grey) on the mission name; the hotkey letter is red. On the Supply row the highlight is only the 3-character number after `Supply:`.
- **Row selection:** `Up` / `Down` move the bar (Down from Supply wraps to eXplore). Letter keys jump to a row: `e` Establish, `h` Hold, `o` Obtain, `n` Negotiate, `s` Supply (all V); `x` eXplore and `c` Combat are I (red letters, not pressed).
- **Numbers:** there is no number entry. `+` adds one team to the selected row, `-` removes one (no effect at 0). Digits (`3`), `Left`, `Right` are ignored. Each team is 10 people with a fixed split that the game fills in: eXplore 5 crew/5 troops, Establish 6/4, Hold 4/6, Obtain 8/2, Combat 0/10, Negotiate 9/1. The Total row sums Tms/RegCrw/ShkTrp.
- **Supply row:** `+` / `-` change units by 1. Shown as `Supply:  2/  2d` = units / days they last (2 units = 2 days for 6 teams, 3 days for 5 teams).
- **Activate:** `Enter` or `a`. COMMS: `Report from Engineering:` / `Teleporters activated.` then `MESSAGE FROM LANDING PARTY:` / `Arrived safely. Mission started.` / `Landing party out.`; row 24 returns to `Select option` with the Teleporters submenu (`Landing prty / Boardng prty / Personl xfer / Material xfr`); `LP:N` becomes `LP:Y`; Crew and Shk Troops in DATA drop (275/150 to 270/145 for one eXplore team).
- **Leaving without sending anyone:** `Esc` closes the table to the Teleporters submenu and discards the assignments (reopening shows all zeros). `Enter` with every row at 0 also just closes it, with no teleport message.
- **Shift+A in assign mode:** no visible effect (sent once as xdotool `shift+a`; in recall mode I sent it as `A` and it worked, so this may be a key-injection difference; treat as unconfirmed).

### A5. Recall flow (V)

1. `E`,`T`,`L` (or `L` in the Teleporters submenu) with a party on the planet: COMMS `Report from Engineering:` / `5 crew, 5 troopers & 0 captives` / `are still alive on the planet.`; row 24 `Beam landing party up?` with an empty menu row. Accepts `y` / `n` (`n` not tried; the manual says it lets you add teams).
2. `y`: the table opens listing the teams on the planet; row 24 `Select # teams to stay` + `Activate` + ` [Shift][A]:select all`.
3. `-` lowers the number staying; `Shift+A` sets every row to 0 (all return).
4. `Enter`: `Report from Engineering:` / `Teleporters activated.` then `Report from Engineering:` / `10 members of landing party on board.`; Crew/Shk Troops return to 275/150.

## B. Mission 1-A completion (not achieved)

Verified:
- Orders page (`Ctrl+O`, full screen, `PRESS ANY KEY TO EXIT`), rows 10-12: ` 2. Your primary objective is to discover at least one mineral-rich planet` / `    in UGA or neutral space.` / `    INCOMPLETE- no suitable planets discovered`.
- It stayed exactly that after: orbiting and scanning a gas giant (SS-33A), and after orbiting and scanning SS-17A (`Fair minerals; supplies available.`) with the map and landing parties declined. So a "Fair minerals" planet, scanned from orbit, does not count.
- Systems checked in practice mission 029 (start SS-0 at 4,1): SS-8 (6,2) 0 planets; SS-24 (7,3) 0; SS-44 (7,4) 0; SS-33 (7,6) 1 gas giant; SS-35 (5,7) none on the map; SS-17 (3,8) 1 class D; probes at SS-49 (10,5) and SS-18 (10,6) reported 0 planets. All 5 probes were used.
- In practice the probe report arrives in COMMS with no popup: `Report from Probe Ops Control:` / `Probe #3 arrived at RegLoc 7,6.` / `Mapping of system SS-33 complete.` / `1 planet in system.` / `Type: G` / `No enemy activity detected.` (or `0 planets in system.`).
- Nav map `Database` menu: `System / Bases / Lndprty / sTars / Mapped`. `sTars` lists every system with Rx, Ry, class and planet count (`?` until probed or visited); useful for picking systems.

Inferred (from strings in the game's programs and the manuals, not seen on screen):
- The status line has the forms `COMPLETED` / `INCOMPLETE` followed by `- no suitable planets discovered` or `- located: %d  surveyed: %d`. So the line probably becomes `COMPLETED- located: 1  surveyed: 0` style text, and a landing-party eXplore/survey is counted separately.
- The trigger is most likely a planet whose scan line reads `Rich minerals` in UGA or neutral space (Tutorial 2's SS-20B reads `Rich minerals`, but `Ctrl+O` is blocked in tutorials, so it could not be checked there).
- A message box exists with `CONGRATULATIONS, <name>!` / `You have successfully completed the major objectives of your mission!` and a prompt `Do you wish to conclude the mission?` (`Answer?`). The training manual says such a box appears when objectives are met.

## C. Combat

### C1. How red alert starts (V)

Automatic, no key. COMMS in order:
`***ENEMY DETECTED: RED ALERT***` / `Report from Executive Officer:` / `Changing alert status to RED` / `Report from Weapons Console:` / `Shields raised!` / `Report from Combat Control:` / `UGA ship detected.` / `Bearing: 21.  Distance: 21.5` / `Course: 30.  Velocity: 0.7`.
- Status box (rows 15-16, cols 42-50) reads `RED` / `ALERT`; Shields 100%; ECM usage jumps to 84%. The DATA panel (rows 9-15, cols 59-79) switches to `TARGET DATA. 24` (24 = torpedoes left).
- When the enemy is at the planet you are entering orbit of: `UGA ship detected.` / `It's in orbit.` / `Report from Weapons Console:` / `Torpedoes locked on target.` (or `Phasers locked on target.`). Row 24 is blank while these print (about 6 s).
- Other alerts seen: `Report from Combat Control:` / `Enemy activity detected.` (on arriving in a system); `Report from Communications:` / `Enemy ship sending distress call.`; `Report from Combat Control:` / `Enemy ship arrived in our system.`; `Report from Tactical Console:` / `UGAS changed orbit position.`

### C2. The XO's disable/destroy question (V)

Appears during orbit entry, before `Entered orbit`. COMMS: `Report from Executive Officer:` / `Excellency, UGA freighters and` / `starliners can provide valuable` / `information, power or supplies,` / `and are worth a lot upon delivery.` / `Should we set the weapons to` / `disable rather than destroy?`; row 24 `Answer?`. Accepts `y` / `n`.
- `n`: no reply line; orbit entry continues (weapons stay Auto/Destroy).
- `y`: `Report from Weapons Console:` / `Aye, sir. Weapons set to disable.`
- Order with an enemy in orbit: red alert messages, this question, `Entered orbit`, `Scanning planet`, scan report + `Display planetary map?`, landing-parties question.

### C3. Target data and displays (V)

- Weapon indicators, row 8 cols 61-72: `Pa. Ta. Ta.` = phaser, torpedo 1, torpedo 2; small letter `a` auto, `m` manual (`Tm`). With no torpedoes left: `Pa. T . T .`.
- TARGET DATA rows 10-15: header `O.Pos.Brng.Vel.Wpn` in orbit (`R.Pos...` in normal space); one row per enemy, e.g. ` L,23.  0...0.. T ` = orbit level (L or H) and orbit position, bearing, velocity, and up to three letters `P` `T` `T` showing which weapon is locked on that ship.
- Orbital display: enemy ships are single glyphs (raw 0x9A, red) around the planet. A second glyph (raw 0x05) is probably own ship (I).
- No enemy was ever seen on the TACTICAL (normal space) display or in LRS: every fight was in orbit.

### C4. cmB menu and weapons menus (V)

- `B`: COMMS `COMBAT CONTROL`; menu `Combat CC` (`C`), `Weapons` (`W`).
- `W`: menu `Phasers` (`P`), `Torpedoes` (`T`), `Obliterator pods` (`O`).
- `T`: COMMS `Report from Weapons Console:` / `Torp #1 target: freighter` / `Rel. position:   0,  0` / `Mode: aut Setting: Destroy` / `Torp #2 target: freighter` / `Rel. position:   0,  0` / `Mode: aut Setting: Destroy`. Row 24 `Select torp #1 setting`, menu `Destroy` (`D`), `disaBle` (`B`), `Mode` (`M`), `Standby` (`S`), `Conditn` (`C`).
  - Picking a setting prints `Torp #1 new setting: Conditional` and moves on to `Select torp #2 setting` (same menu). So the Torpedoes item walks tube 1 then tube 2.
  - `M`: row 24 `Select option`, menu `Auto` (`A`), `Manual` (`M`).
  - `M` (Manual): row 24 `Select target`; a target box appears on the orbital display and a `TARGET ID` box opens across rows 14-16 (cols 15-62) naming what is under it: `Planet SS-20C UNKNOWN`, or `UGA freighter SABERHAGEN O:L P: 4`. Arrow keys move the box; `Enter` confirms: `Mode changed to MANUAL` / `Freighter targeted.`; indicator becomes `Tm`; menu returns to Phasers/Torpedoes/Obliterator pods.
- `P`: one step only. COMMS `Phasers targeted on freighter` / `at relative position  0, 0.` / `Mode: AUT Setting: Destroy`; same five-item menu, row 24 `Select option`.
- When a manually targeted ship dies: `Torp #2 manual lock-on lost.` and the tube goes back to auto.

### C5. Firing keys and messages (V)

Keyboard set B: **`Delete` fires torpedoes (both tubes), `Insert` fires phasers.**

| Event | Exact COMMS text |
|---|---|
| Lock | `Report from Weapons Console:` / `Torpedoes locked on target.` ; `Phasers locked on target.` |
| Torpedoes fired | `Report from Weapons Console:` / `Torpedo #1 away!` / `Torpedo #2 away!` (count drops by 2 about 2 s later) |
| Torpedo hit | `Our torp hit freighter` / `  Size:  262 units    Power:  66 %` (also `Our torp hit starliner`) |
| Phaser hit | `Our phaser hit freighter` / `  Size:  698 units    Power:  -1 %` |
| Destroyed | `Enemy ship destroyed - Hail Zagar!` / `Report from Weapons Console:` / `Freighter destroyed!` |
| Shields dropped (disable mode) | `They've lowered their shields!` |
| Disabled | `Enemy ship disabled!` (repeats on every later hit; ship stays in TARGET DATA with Power 0-5 %) |
| Refused: no lock | `Report from Weapons Console:` / `Phasers not targeted.` |
| Refused: recharging | `Report from Weapons Console:` / `Phasers not energized.` |
| Refused: reloading | `Report from Weapons Console:` / `Torp #1: not loaded` / `Torp #2: not loaded` |
| Refused: empty | `Report from Weapons Console:` / `No torpedoes left.` |
| After running dry | `Report from Executive Officer:` / `Excellency, I would like to point` / `out that we have supply depots on` / `our planetary bases.` |
| Miss | never seen |

"Power" is the target's remaining power after the hit. First fight: two torpedo hits (66 %, 56 %) then one phaser shot destroyed a freighter; the second freighter died to one phaser hit.

### C6. End of the fight (V)

When the last enemy in range is destroyed, with no prompt and no key: `Report from Executive Officer:` / `Cancelling red alert.` / `Report from Combat Control:` / `Shields lowered.` / `Report from Executive Officer:` / `Changing alert status to ORANGE`. Status box `ORANGE` / `ALERT`, Shields 0 %, ECM 25 %. No mission-complete box appeared in Tutorial 2 or the practice mission.
Disabled ships do not end the alert; it stayed RED.

### C7. Alt+X combat control with an enemy present (V)

Full-screen: grid in the middle with own ship `o` at centre, left column `Sys Loc`, `Velocity`, `Set Vel`, `Target:`, `Power`, right column DAMAGE, `TARGET DATA` (`Rel. Posn.Vel.Wpn`, e.g. `  2,  9 ..0..PTT`), indicators ` P . T . T `, and a `WEAPONS. 22` panel (`Phaser Trp1 Trp2`, `Mode` Auto/Manual, `Bmb: N`, `Setting` Destroy/disaBle/Standby/coNditional). Bottom row: `Brng:    SL:`.
The mission clock did not advance while this screen was open in orbit (2.25 on entry and on exit).

- MAIN MENU (rows 16-23, cols 0-17): `[T] TDB on/off`, `[H] Headings`, `[W] WeaponsCtl`, `[N] Navigation`, `[C] Commands`, `[Enter] Select`, `[Esc] Exit`.
- `T`: a target box appears at the ship; bottom row shows `Brng:  0 SL:68,67.Planet SS-20C UNKNOWN`. Arrows move it (`Brng: 45 SL:69,68.Nothing`). `T` again hides it.
- `H`: toggles the heading tick labels along the grid edge.
- `W`: WEAPONS MENU: `[G] Show Trgt`, `[C] Cont. Fire`, `[M] Manual Tgt`, `[Left][Right] Weapon`, `[Enter] Select`, `[Esc] MainMenu`. Asterisks in the WEAPONS panel mark the selected weapon's mode and setting (`*Auto`, `*Destroy`).
  - `G`: draws a box on the current target.
  - `Right` / `Left`: select Phaser / Trp1 / Trp2; the panel then shows that weapon's values (for Trp1: `*coNditional`, with the hints `Destroy warships` / `disaBle noncmbat`).
  - `M`: popup rows 11-13 `Select manual target?` (y/n). I answered `n`; the panel still changed to `*Manual` and back on the bridge tube 2 showed `Tm`. So `M` switches the selected weapon to manual even when the target question is declined (observation V, reading I).
  - `C` (Cont. Fire) not pressed.
- `N`: NAV MENU: `[P] Pursue`, `[N] Nav Target`, `[V] Evasive`, `[TAB] R SdeSlp`, `[sTAB] L SS`, `[Enter] Select`, `[Esc] MainMenu`. Items not exercised.
- `C`: popup `NOT AVAILABLE AT THIS RANK` (about 2 s).
- `Esc` from the main menu returns to the bridge.

### C8. Harness warning for scripted fights (V)

The game buffers keys deeply. A loop sending `Delete` `Insert` every second queued far more shots than were processed; it kept firing for over 30 s after I stopped and emptied the torpedo racks, and later menu keys landed in the wrong place. Send one fire key, then wait for the result.

## D. Freeze, and targets while moving

### D1. Freeze (V)

- `BackSpace` toggles freeze. (The training manual also names Alt+F.)
- Text does not change at all. Two attribute changes only:
  - The MET value (row 11, cols 11-16) changes from attribute 0x02 (green) to **0x8F (blinking bright white)**.
  - The warning-light cycle on row 0 stops where it is (the highlighted light no longer moves).
- MET, `Orbt-Pos` and the orbit glyphs stop; `BackSpace` again resumes them. Dumps 4 s apart while frozen are identical in text, attributes and raw.
- So a "frozen" indicator can be detected only from the attribute of the MET digits.

### D2. Target entered on the system map while already moving (V)

- Ship under way at 0.3 to target 1 (44,25). On the system map I moved the box to 44,35, `T`, `Enter` (no F9). Result: it was stored as **target #2**; the ship kept flying to target 1.
- `R` (taRgets) shows the `TARGETS` table: `R/S  Object   x   y  Bearing`, rows `1:  S   44  25   266` and `2:  S   44  35     0`, `Target #: 1`, keys `[D]elete [S]wap [R]eset [CR]:accept`.
- `S` (swap) then `Enter`: the ship turned at once to the new target 1 (course 266 to 301) **without pressing F9**.
- Not tested: entering a target while moving when the target list is empty.

## New dump files in tests/screens (in capture order)

Orbit and landing parties (Tutorial 2, SS-20B):
1. `bridge_red_alert_establish_orbit_prompt.txt` - bridge at RED alert in normal space, COMMS `UGA ship detected` + bearing/course, row 24 `Establish orbit?`
2. `orbit_entered_before_scan.txt` - just after `y`: `Entered orbit of planet SS-20B.`, `Select option`
3. `orbit_scanning_planet.txt` - `Scanning planet`, blank prompt
4. `orbit_scan_report_planet_map_question.txt` - SHORT RANGE SCAN report + `Display planetary map?` box, blank row 24
5. `orbit_planet_map_ega.txt` - EGA planet map (text rows blank, `#vgamode=2`)
6. `orbit_planet_map_returning_to_bridge.txt` - `Returning control to main bridge` text screen after Esc
7. `landing_party_table.txt` - empty table from E,T,L, eXplore row selected
8. `landing_party_table_one_explore_team.txt` - after one `+`: eXplore 1 / 5 / 5
9. `landing_party_table_supply_row_selected.txt` - Supply number highlighted (after `s`), one team on each mission
10. `landing_party_table_one_team_each_supply2.txt` - one team on every mission, `Supply:  2/  2d`, total 6 / 32 / 28
11. `landing_party_table_after_shift_a.txt` - assign mode after `-` on eXplore and Shift+A (no change from Shift+A), `Supply:  2/  3d`
12. `landing_party_teleporters_activated.txt` - `Teleporters activated.`
13. `landing_party_arrived_message.txt` - `MESSAGE FROM LANDING PARTY:` / `Arrived safely. Mission started.`, Teleporters submenu
14. `landing_party_beam_up_question.txt` - `5 crew, 5 troopers & 0 captives are still alive`, row 24 `Beam landing party up?`
15. `landing_party_table_recall_teams_to_stay.txt` - recall table, `Select # teams to stay`, `[Shift][A]:select all`
16. `landing_party_table_recall_after_shift_a.txt` - recall table after Shift+A, all zeros
17. `landing_party_recalled.txt` - `10 members of landing party on board.`

First fight (Tutorial 2, SS-20C, weapons on Destroy):
18. `combat_01_orbit_red_alert_ships_in_orbit_torps_locked.txt` - entering orbit, RED, `It's in orbit.`, `Torpedoes locked on target.`, two targets listed
19. `combat_02_xo_disable_rather_than_destroy_question.txt` - XO question, `Answer?`
20. `orbit_scan_report_planet_map_question_enemy_in_orbit.txt` - scan report + map question with `UGA freighter in orbit!` lines
21. `orbit_landing_parties_question.txt` - XO landing-parties question, `Answer?`
22. `landing_party_table_from_xo_question.txt` - table opened by `y`, `By your command, sir.`
23. `combat_03_bridge_orbit_two_freighters_target_data.txt` - orbital display with two freighters, TARGET DATA with T locks
24. `combat_04_cmb_menu_enemy_present.txt` - cmB menu
25. `combat_05_cmb_weapons_menu.txt` - Weapons menu
26. `combat_06_torpedoes_select_torp1_setting.txt` - torpedo status report, `Select torp #1 setting`
27. `combat_07_torp1_conditional_select_torp2_setting.txt` - `Torp #1 new setting: Conditional`, `Select torp #2 setting`
28. `combat_08_torp2_mode_auto_manual_menu.txt` - Auto / Manual menu
29. `combat_09_manual_target_select_tdb_target_id.txt` - `Select target`, target box on the planet, TARGET ID box
30. `combat_10_manual_tdb_on_freighter_target_id.txt` - target box on a freighter: `UGA freighter SABERHAGEN O:L P: 4`
31. `combat_11_torp2_manual_freighter_targeted.txt` - `Mode changed to MANUAL` / `Freighter targeted.`, indicator `Tm`
32. `combat_12_phasers_settings_menu.txt` - phaser status report and settings menu
33. `combat_13_bridge_before_firing.txt` - bridge just before firing
34. `combat_14_torpedo1_away.txt` - `Torpedo #1 away!`
35. `combat_15_torpedo2_away.txt` - `Torpedo #2 away!`
36. `combat_16_torp_hits_size_power.txt` - two `Our torp hit freighter` reports, torpedoes 22
37. `combat_17_phaser_fired_freighter_destroyed.txt` - `Enemy ship destroyed - Hail Zagar!` / `Freighter destroyed!` / `Torp #2 manual lock-on lost.`
38. `combat_18_one_freighter_left_torp2_back_to_auto.txt` - one target left, indicators back to `Ta`
39. `combat_19_altx_combat_control_in_orbit.txt` - Alt+X screen, main menu
40. `combat_20_altx_tdb_on_moved.txt` - Alt+X with target box moved (`Brng: 45 SL:69,68.Nothing`)
41. `combat_21_altx_weapons_menu.txt` - Alt+X weapons menu
42. `combat_22_altx_weapons_select_manual_target_question.txt` - popup `Select manual target?`
43. `combat_23_altx_nav_menu.txt` - Alt+X nav menu
44. `combat_24_altx_commands_not_available_at_this_rank.txt` - popup `NOT AVAILABLE AT THIS RANK`
45. `combat_25_phasers_not_targeted_refusal.txt` - `Phasers not targeted.`
46. `combat_26_phaser_hit_freighter_destroyed.txt` - `Our phaser hit freighter` / Size / Power, then destroyed
47. `combat_27_red_alert_cancelled_after_kills.txt` - `Cancelling red alert.` / `Shields lowered.` / `Changing alert status to ORANGE`

Practice mission 029:
48. `mission1a_orders_before_incomplete.txt` - Ctrl+O orders page, `INCOMPLETE- no suitable planets discovered`
49. `orbit_scan_report_gas_giant_planet_map_question.txt` - gas giant scan report + map question
50. `navmap_database_menu.txt` - nav map Database menu (`System / Bases / Lndprty / sTars / Mapped`)
51. `navmap_database_star_systems_list.txt` - STAR SYSTEMS list with planet counts
52. `combat_00_enemy_detected_red_alert_message.txt` - `***ENEMY DETECTED: RED ALERT***`
53. `combat_00b_shields_raised.txt` - `Shields raised!`
54. `combat_02b_weapons_set_to_disable.txt` - `Aye, sir. Weapons set to disable.`
55. `orbit_landing_parties_declined.txt` - `As you wish, sir.` / `No landing parties assigned.`
56. `combat_28_disable_mode_torp_hits_starliner_freighter.txt` - disable mode, hits on a starliner and a freighter, three targets
57. `combat_29_phaser_hit_they_lowered_shields.txt` - `They've lowered their shields!`
58. `combat_30_refused_phasers_not_energized_torps_not_loaded.txt` - `Phasers not energized.` / `Torp #1: not loaded` / `Torp #2: not loaded`
59. `combat_31_enemy_ship_disabled.txt` - `Enemy ship disabled!`
60. `combat_33_xo_supply_depots_hint.txt` - XO supply-depots hint after torpedoes ran out
61. `combat_32_no_torpedoes_left_tractor_beam_select.txt` - `No torpedoes left.`, indicators `T . T .`, tractor beam `Select target` with target box (reached by stray buffered keys)
62. `orbit_running_before_freeze.txt` - orbit running, for comparison
63. `orbit_frozen_backspace.txt` - same screen frozen (MET attribute 0x8F)
64. `sysmap_targets_table_two_targets_while_moving.txt` - system map TARGETS table with two targets while under way

Numbering note: `combat_00`, `00b`, `02b` and `28`-`33` come from the second (disable-mode) fight; `01`-`27` are the first fight in true order. `32` and `33` are one step out of time order (33 happened first).
