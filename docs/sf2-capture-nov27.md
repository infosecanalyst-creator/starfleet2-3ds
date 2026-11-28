# Star Fleet II capture report 2 (DOSBox harness, 2026-11-27)

Rows/columns are 0-based on the 80x25 text screen. V = verified on screen, I = inferred from manuals/strings.
New dumps are in `/home/claude/sf2-3ds/tests/screens/` (nothing else in the repo touched, nothing committed).

## A. Practice mission 1-A completed and ended (practice mission #030) - all V unless marked

### A1. What completes the objective
- Trigger: the automatic orbit scan of a planet whose report line reads `Rich minerals; ...` (here SS-11A, class A/aquatic, Tech 5, three UGA freighters in orbit). No landing party, no survey needed. A `Fair minerals` planet (SS-17A) did not count, and neither did a completed landing-party survey of that Fair planet (orders still `INCOMPLETE- no suitable planets discovered`).
- Exact order on entering orbit of the rich planet (answered `y` to `Establish orbit?`):
  1. Red alert messages (enemy in orbit), `Entered orbit of planet SS-11A.` ...; no XO disable question this time.
  2. `Scanning planet`, then the SHORT RANGE SCAN report in COMMS (rows 15-22): `ANALYSIS: Class A/aquatic` / `Mammalian/Tech Lvl=5:medium-nuclear` / `Rich minerals; supplies available.` / `Planet has not been surveyed.` / `No shields up-no active plt defenses` / `UGA freighter in orbit!` x3. Row 24 shows `Select option` during this.
  3. About 3 s later a box in the middle of the screen, rows 11-13, cols 6-71: row 12 ` YOU HAVE COMPLETED THE MINIMUM REQUIREMENTS OF YOUR MISSION!`, bottom border carries `PRESS KEY`. Row 24 still reads `Select option` (menu row visible but not active). Any key (Enter used).
  4. Second box, rows 11-13, cols 14-65: ` TO END THE MISSION YOU MUST RETURN TO STARFORT  ` + `PRESS KEY`. Any key.
  5. Third box, rows 11-13, cols 18-60: ` OR YOU CAN CONTINUE UNTIL TIME EXPIRES  ` + `PRESS KEY`. Any key.
  6. Only then the usual `Display planetary map?` box (rows 11-13, row 24 blank; y/n), then the XO landing-parties question (`Answer?`, y/n).
  The mission clock stands still during the three boxes. Nothing is added to COMMS by the completion.
- `Ctrl+O` afterwards: row 12 reads `    COMPLETED` (nothing after the word; before it was `    INCOMPLETE- no suitable planets discovered`). `PRESS ANY KEY TO EXIT`.

### A2. Going home and concluding
- `Alt+J` in orbit works (see D1): popup `Do you wish to go to nearest starfort?` -> `y` -> ship leaves orbit by itself, jumps, arrives, row 24 `Shall we dock?` (y/n).
- `y`: COMMS `Report from Helm:` / `Docking procedure complete.`, Mode ` DOCKED `, then a box rows 9-11, cols 18-58: ` Do you wish to conclude the mission?` with row 24 `Answer?`. Accepts `y` / `n`.
- `n`: box closes, normal docking continues (`Report from Executive Officer:` / `Ship data file archived.` then `Resupply & repairs complete.` / `Ready to undock.`; torpedoes, crew, probes, supplies refill; the clock runs during resupply). The mission goes on. `N`,`H`,`D` while docked: `Report from Helm:` / `Already docked.` The question comes back on every later docking (undocked with F4 F4 F9, F10, then `N`,`H`,`D`: same box again).
- `y`: full-screen text, about 1.5 s, no key needed: row 10 `CONGRATULATIONS, ALTERN!` (centred; uses the RANK, not the name), row 11 `You have successfully completed the major objectives of your mission!`.
- Then the MISSION TALLY screen (same as Cmp > Spcl srvcs > Msn tally), page `PERSONAL`: rows of ships destroyed/captured/delivered, penalties, and exploration credits (`Star systems mapped 8 / 8`, `Planets scanned 2 / 10`, `Planets surveyed 1 / 15`, `Planet w/supplies 1 / 20`, `Planet w/rich minrls 1 / 20`, `Planet w/fair minrls 1 / 15`), bottom line `SLAVES       0    KRELS       88`. Row 24 `Select option` + menu `Personal` (P) `Krellan` (K) `Values` (V) `Orders` (O). `K` and `V` switch pages (title cell top-left changes: ` PERSONAL `, ` KRELLANS `, `  VALUES  `); `O` showed nothing in my run. `Esc` leaves.
- After `Esc`, plain text screen (rows 0-5): `Some data files from this mission are no longer needed and are now` / `file debris cluttering up your drive.` / `Do you want to delete them?` / `Note that your Service Record will NOT be deleted.` / (blank) / `Answer (Y/N)?`. I answered `n` -> `PRESS KEY` appears on row 21 (col 35). (`y` not tried.)
- Any key -> rows 0-2: `Would you like your log files (reports, messages, ship data)` / `saved into a mission subdirectory?` / `Answer (Y/N)?`. `n` -> `PRESS KEY` on row 23. (`y` not tried.)
- Any key -> SIGN-ON OPTIONS menu. That is the whole ending for a practice mission: no debriefing, no evaluation page, no promotion, no medal.
- Service Record (`S` at sign-on) afterwards: `MISSIONS RECORD FOR Altern RYAN` grid still shows `0` in cell 1-A (a practice mission is not recorded); `[PgDown] Next page`, `[Esc] Return`, `[V] View orders`. PgDown pages: `SKILLS PROGRESSION` table (`PRESS [Enter] TO CONTINUE` / `PRESS [Esc] TO EXIT SERVICE RECORD`), `SERVICE RECORD` / `PROMOTIONS` (`No promotions on record.`), `MISSION AWARDS` (`No mission awards on record.`), each `PRESS KEY FOR NEXT or [ESC]`.
- I (strings in SFIIS.EXE, not seen): in career mode a `COMMUNIQUE FROM IMPERIAL FLEET HQ` / `Congratulations, <name>!` / `Due to the successful completion of all missions at your current rank, you are hereby promoted to:` page exists. Mission failure/time-out was not met.

### A3. Useful facts found on the way
- Probes: max range 5 light years: `Report from Engineering:` / `Target out of range.` / `Maximum range is 5 light years.` + `PRESS KEY` (COMMS-sized window on the nav map).
- Launching a probe while docked: `Report from Engineering:` / `Starfort provided replacement probe.` But the clock does not run while docked (or while stopped in open space), so a probe stays `Transit` until the ship moves or orbits.
- Bridge DATA `Snsr Probes.L:5S:5` = 5 loaded + 5 in store. Stored probes are loaded only after used ones are cleared: probe table `[D]:detonate`, `Shift+D` = all -> box `Detonate all probes on station?` (y/n) -> `Report from Executive Officer:` / `Probe detonations complete`; table shows 5 `Loaded` again, DATA `L:5S:0`.
- Probe table: cursor starts on row 1; a row that is not `Loaded` shows `[D]:detonate [L]:locate [T]:targets`, a `Loaded` row shows `[CR]:launch`. Move with Up/Down.
- Probe statuses seen: `Loaded`, `Transit`, `Passive`; DETECT column shows `enemy`. Reports in COMMS: `Report from Probe Ops Control:` / `Probe #2 arrived at RegLoc 5,10.` / `Mapping of system SS-18 complete.` / `0 planets in system.` / `No enemy activity detected.`; with planets: `2 planets in system` style line then `Types: A A` (`Type: P` for one), `Enemy activity detected.`, `Possible UGAS starbase detected.`; later from a passive probe: `Just received report from probe #3` / `in Region Location 9,8.` / `Enemy ship just left system.` / `Course was 334..`. A probe sent to empty space: `Mapping of RL 10,12 complete.`
- Nav map Database > sTars: Left/Right/Up/Down move a highlighted entry (starts on SS-0 each time, wraps), Enter -> box `SS-45 at RL 4x 4y. Press for [N]av or [W]ar map`. `N` puts the nav-map target box on that system. Game bug seen: after using this a second time in one visit the box location read garbage (`Location.-44132552`) and a probe launch was refused as out of range; closing and reopening the nav map fixed it. Arrow keys on the nav map (1 unit per press) are the reliable way.
- System map Database on a planet not yet scanned from orbit: `Report from Science Console:` / `No detailed planet scan yet done.`
- Time: in Turbo performance mode the clock in orbit runs about 1 day per 20-30 real seconds; 10 days go fast. Supplies fell to 44 % in 4.9 days.
- The star list planet count turns from `?` to a number after a probe or visit.

### A dumps
- `navmap_star_list_selected_nav_or_war_prompt` - sTars list with the `Press for [N]av or [W]ar map` box
- `navmap_probe_launched_docked_starfort_replacement` - `Starfort provided replacement probe.`
- `navmap_probe_target_out_of_range_5ly` - out-of-range refusal + PRESS KEY
- `probes_detonate_all_question` - `Detonate all probes on station?`
- `probe_report_comms_zero_planets` - probe report in COMMS
- `sysmap_database_no_detailed_planet_scan` - Database refusal on unscanned planet
- `mission1a_scan_rich_minerals_enemy_in_orbit` - the scan report with `Rich minerals`
- `mission1a_completed_minimum_requirements_box` - box 1
- `mission1a_to_end_mission_return_to_starfort_box` - box 2
- `mission1a_or_continue_until_time_expires_box` - box 3
- `mission1a_orders_after_completed` - Ctrl+O with `COMPLETED`
- `mission1a_home_shall_we_dock` - `Shall we dock?` at the starfort
- `mission1a_docked_conclude_the_mission_question` - conclude box, `Answer?`
- `mission1a_conclude_no_normal_resupply` - after `n`
- `mission_end_01_congratulations_screen` .. `mission_end_08_after_no_press_key` - the ending screens in order (congratulations, tally personal/krellan/values, delete-debris question, PRESS KEY, save-logs question, PRESS KEY)
- `service_record_missions_record_after_practice`, `service_record_skills_progression`, `service_record_promotions`, `service_record_mission_awards`

## D (done during A)
### D1. Alt+J in orbit (V)
Works. Box rows 11-13 cols 19-59 `Do you wish to go to nearest starfort?` (y/n). `y`: COMMS `Report from Helm:` / `Aye, sir.` / `Setting new targets.` / `Report from Helm:` / `Nav. computer locked on target.` / `Course set to system edge for jump.` / `Maneuvering engines engaged.` / `Engines engaged.` / `Leaving orbit.`; then (enemy had been in orbit) `Cancelling red alert.` / `Shields lowered.` / `Changing alert status to ORANGE`, later GREEN in hyperspace; on arrival `Second target loaded into nav.` / `MESSAGE FROM STARFORT SF-1:` / `Welcome to our system, DEATH HAWK.` / `Long live Emperor Zae IV!` / `Target reached, Excellency.` and `Shall we dock?`. Dumps: `altj_in_orbit_go_to_nearest_starfort_question`, `altj_in_orbit_yes_setting_new_targets`.

### D4. Landing-party explore on an unsurveyed planet, and `+` on a surveyed one (V)
- SS-17A (class P/poisonous, `Fair minerals; no supplies available.`, `Planet has not been surveyed.`). `E`,`T`,`L`, `+` on eXplore (1 team 5/5), `s` `+` `+` (Supply 2 = 10 d for one team), Enter at MET 1.20: `Teleporters activated.` / `MESSAGE FROM LANDING PARTY:` / `Arrived safely. Mission started.` / `Landing party out.`; `LP:N` -> `LP:Y`.
- Exactly 1.00 day later (MET 2.20): `MESSAGE FROM LP ON SS-17A:` / `Survey complete. This planet` / `is unoccupied by the Alliance.` / `Name of this planet is UYARFYW.` / `There is not a unified government.` The orbital display name box changes from `UNKNOWN` to the name.
- Later: `MESSAGE FROM LP ON SS-17A:` / `Under attack by natives.` (MET 2.40) and `MESSAGE FROM LP ON SS-17A:` / `They have broken off the attack.` (2.75).
- `E`,`T`,`L` again: `Report from Engineering:` / `0 crew, 5 troopers & 0 captives` / `are still alive on the planet.`, row 24 `Beam landing party up?`. **`n`** opens the normal assign table (all zeros) so teams can be added. `+` on eXplore there: COMMS line `Already surveyed!` and the count stays 0.
- Recall (`l`, `y`, Shift+A, Enter): `Teleporters activated.` / `Report from Engineering:` / `5 members of landing party on board.` / `They have 5 crew & 0 troopers dead.`
- Dumps: `landing_party_survey_complete_report`, `landing_party_under_attack_by_natives`, `landing_party_attack_broken_off`, `landing_party_beam_up_question_after_losses`, `landing_party_table_after_beam_up_no`, `landing_party_already_surveyed_refusal`, `landing_party_recalled_with_dead`.

### D3 (first try, practice mission). Boarding a disabled ship that is not adjacent (V)
- Weapons to disable by menu: `B`,`W`,`P`,`B` -> `Setting changed to Disable`; `B`,`W`,`T`,`B`,`B` -> `Torp #1 new setting: Disable` / `Torp #2 new setting: Disable`.
- After `Enemy ship disabled!`: `E`,`T`,`B` -> row 24 `Select target to board`, a target box on the orbital display (clock stops). Arrows move it one cell; Enter. With the freighter in another part of the orbit: `Report from Combat Control:` / `No valid target within range.` Manual: the ship must be adjacent and its shields down.
- Dumps: `boarding_select_target_to_board`, `boarding_no_valid_target_within_range_disabled_ship_not_adjacent`.

### D2. F9 in orbit with no nav target (V, Tutorial 4)
- Set speed 0: `Report from Helm:` / `Velocity must be set.`; ship stays `IN ORBIT`.
- With a speed set (F4 x3 = 0.3) then F9: `Report from Helm:` / `Ship underway.` / `Engines engaged.` / `Leaving orbit.`; Mode `NORM SPC`, title back to `TACTICAL`. So yes, F9 alone leaves orbit once a speed is set. After F10 the Mode field reads `EMG STOP` for a few seconds.
- Dumps: `orbit_f9_velocity_must_be_set`, `orbit_f9_with_speed_leaving_orbit`.

### D1b. Alt+J inside a tutorial (V, Tutorial 4, normal space)
Same popup `Do you wish to go to nearest starfort?` (y/n); `n` closes it with no message. (`y` not followed in a tutorial.) Dump `tut4_altj_in_tutorial_question`.

### Quitting once the objectives are complete (V, Tutorial 4 after a kill) - new
- `C`,`S`,`Q` no longer asks `Do you want to stop?`. Row 24 asks `Ready to claim victory?` (y/n).
  - `y`: box ` TO END THE MISSION YOU MUST RETURN TO STARFORT  ` + `PRESS KEY`; back to the Spcl srvcs menu.
  - `n`: box ` TO EXIT, USE EXIT COMMAND [Ctrl][X] ` + `PRESS KEY`; back to the menu.
- `Ctrl+X`: row 24 `Exit to DOS?` (y/n) -> `y` -> box `ARE YOU SURE YOU WANT TO EXIT TO DOS?` (row 24 still `Exit to DOS?`, y/n) -> `y` -> the game ends at the DOS prompt `C:\>` with no further question. Leaving Tutorial 4 this way did NOT mark it `*`.
- Dumps: `quit_ready_to_claim_victory_prompt`, `quit_claim_victory_yes_box`, `quit_claim_victory_no_use_exit_command_ctrl_x_box`, `ctrl_x_exit_to_dos_prompt`, `ctrl_x_are_you_sure_exit_to_dos_box`.

## B / C. Tutorials

### Tutorials menu (V) - dump `tutorials_menu_all_eight`
Box rows 4-14, cols 16-61, title `TUTORIALS`:
`1* Basic navigation` / `2  Teleporters 1 - landing parties` / `3  Teleporters 2 - transfers` / `4  Weapons` / `5  Escorts` / `6  Capturing ships (tractor beam/boarding)` / `7  Invasions` / `8  Strategic command` / `"*" - means you have run the tutorial` / bottom border `Press number of desired tutorial`.
Only tutorial 1 carried `*` at the start of this session (the earlier report said 2 was marked; it is not now).

### Tutorial 4 - Weapons (V)
Start: after the motto screen (`To Live is to Conquer, to Conquer is to Live!`, any key) the bridge is IN ORBIT of SS-18A (Reg. Loc 20,13; class A, Tech 5), title `ORBITAL`, STATUS `RED` / `ALERT`, Shields 100 %, MET 1.78, Time left 18.22, Torps 22, three UGA freighters in TARGET DATA with P/T/T locks, menu row = Nav submenu, COMMS only `NAVIGATION CONTROL`. No armed enemy: nothing fires back.
What the manual asks, and what was confirmed:
1. Freeze (`BackSpace`), read the `Pa Ta Ta` indicators and TARGET DATA. V: indicator cells are black-on-green (attr 0x20) when ready, red background (0x40) when not ready. Wpn-column letters: bright red (0x0C) = Destroy/auto, bright blue (0x09) = Standby, red on white (0x74) = Destroy/manual.
2. Unfreeze, `Delete` = torpedoes: `Torpedo #1 away!` / `Torpedo #2 away!`. The XO disable question (`Should we set the weapons to` / `disable rather than destroy?`, `Answer?`) appears right after this first salvo; `n` -> `As you wish, sir.` A fire key pressed while the question is up is lost. Then `Our torp hit freighter` / `  Size:  368 units    Power:  55 %`.
3. `Insert` = phasers: `Our phaser hit freighter` / `  Size:  623 units    Power:  29 %`.
4. Freeze, `B`,`W`,`T`: `S` -> `Torp #1 new setting: Standby`, prompt moves to `Select torp #2 setting`; `M`,`M` -> row 24 `Select target` + TARGET ID box. Enter on empty space: `*** ILLEGAL TARGET ***`. On the ship the TARGET ID reads `UGA freighter LUCKY SUN O:M P:13`; Enter -> `Mode changed to MANUAL` / `Freighter targeted.`; indicator `Tm`. Note: the box had to sit one column to the right of where the ship glyph is drawn before the ID showed the ship.
5. `Alt+M` to the main menu, unfreeze, `Delete`: `Torp #1: set to standby` and only tube 2 fires; `Our torp hit freighter` / `Power:  -1 %` / `Enemy ship destroyed - Hail Zagar!` / `Freighter destroyed!` / `Torp #2 manual lock-on lost.`
6. Differs from what one would expect: right after the first kill the three boxes of a completed mission appear (`YOU HAVE COMPLETED THE MINIMUM REQUIREMENTS OF YOUR MISSION!`, `TO END THE MISSION YOU MUST RETURN TO STARFORT`, `OR YOU CAN CONTINUE UNTIL TIME EXPIRES`, each `PRESS KEY`). The manual says to ignore them.
7. Obliterator pod: `B`,`W`,`O` -> `OBLITERATOR PODS CONTROL` / `Report from Weapons Console:` / `Obliterator pod armed.`, row 24 `Confirm firing?` (y/n). `y` -> `Pod released!` / `WARNING! WARNING!` / `Obliterator pod will explode in 10 sec` / `Time to detonation:  10` counting down, row 24 `Press [Alt]+[S] to stop ` ; then `Obliterator pod exploded on planet.` / `  Planet damage level:  57 %` / `  Cities destroyed: 4` / `  Est. killed: 168,000,000` / `Report from Combat Control:` / `Enemy raised planetary shields!`. Pods 2 -> 1.
Not done: phaser manual mode, barrage mode (`G`,`P`,`M`), firing in normal space.
Dumps: `tut4_01_start_orbit_three_freighters`, `tut4_02_xo_disable_question_after_first_torpedoes`, `tut4_03_manual_target_illegal_target`, `tut4_04_torp1_standby_torp2_manual_indicators`, `tut4_05_completed_minimum_requirements_box_after_kill`, `tut4_06_obliterator_pod_armed_confirm_firing`, `tut4_07_obliterator_pod_countdown_alt_s_to_stop`, `tut4_08_obliterator_pod_exploded_planet_damage`.

### What sets the `*` on a tutorial (V)
- Leaving a tutorial through `C`,`S`,`Q` -> `Do you want to stop?` `y` -> `Do you wish to give up on this mission?` marks it, whichever answer is given (`n`: tutorial 8 marked after about a minute of play with nothing achieved; `y`: tutorial 3 marked). Both answers go straight to the sign-on menu.
- Not marked: leaving by `Ctrl+X` exit to DOS (tutorial 4), or by killing the program (tutorials 2, 5, 7 were only opened). So `*` means "was quit through the Quit command", not "was completed". Menu after this session: 1*, 3*, 6*, 8*. Dump `tutorials_menu_marks_1_3_8`.

### Practice mission selection (V)
`P` at sign-on (Practice off) -> `Do you want to switch to Practice mode?` `y` -> MISSIONS RECORD grid. Arrow keys move a white frame from cell to cell. Only cell 1-A (shown with a magenta background, attr 0x50) is usable for an Altern: on 2-B and 1-E both `Enter` and `V` do nothing at all (no message). On 1-A `Enter` returns to the sign-on menu. So a higher practice mission cannot be chosen; `Change default settings` was not explored. Dump `practice_missions_grid_cursor_on_2b`.

### Tutorial 2 - Teleporters 1, landing parties (start only V; flow V in the Nov 21 report)
Start: NORM SPC in SS-20 (Reg. Loc 9,7), TACTICAL, STATUS `ORANGE` / `ALERT`, MET 0.51, Time left 24.49, full stores (Torps 24, Crew 275, Shk Troops 150, Supplies 98 %, Probes L:5 S:5), Nav submenu, COMMS `NAVIGATION CONTROL`. Dump `tut2_01_first_bridge_screen`.
Manual steps (own words): 1) `Alt+S`, read the PLANETS list (B and C class K, A class I). 2) Target planet B, F9, `y` at `Establish orbit?`, `n` at the map. 3) `y` at the landing-parties question (or `E`,`T`,`L`). 4) `+` x3 on eXplore, x3 on Establish base, x3 on Negotiate; no supplies. 5) `A` or Enter: `Teleporters activated.` / `Arrived safely. Mission started.` 6) Go to planet A (class I, no supplies): 3 explore + 2 base teams and 20 supply units. 7) Back at B: `E`,`T`,`L`, recall the explore and negotiate teams with `-`, leave the base teams. 8) War map, starfort, dock, quit.
Step texts confirmed on screen in other sessions of this report: `Entered orbit of planet`, `Teleporters activated.`, `Arrived safely. Mission started.`, `Survey complete. This planet`, `Under attack by natives.`, `N members of landing party on board.`, `Already surveyed!`. Not replayed here.

### Tutorial 3 - Teleporters 2, transfers (V) - BROKEN START in this install
- After the motto screen a text screen shows row 0 `Thanks, now back to work!` and a box rows 1-3: ` PLANET INTEGRITY FAILURE: code=1999 ` / `*** PRESS KEY ***`. Key -> box ` YOU MAY WANT TO RESTART THIS MISSION  ` / `*** PRESS KEY ***`. Key -> box ` OR IF YOU WANT TO CONTINUE, PRESS [N] ` / `*** PRESS KEY ***`. `n` -> row 24 `Exit to DOS?` on an otherwise empty screen; `n` -> the bridge appears and the tutorial runs. (Happened on the only start tried.)
- Start: IN ORBIT of SS-16B (Reg. Loc 2,10; class D), STATUS `ORANGE` / `ALERT`, MET 6.71, Torps 4, Crew 105, Shk Troops 0, Supplies 81 %, Probes L:5 S:0, Pods 0; COMMS `Report from Communications:` / `Enemy ship sending distress call.` Top menu (not the Nav submenu).
- Manual steps: 1) `E`,`T`,`P`, box on supply ship KS-9, Enter. 2) In the table move to the supply ship's shock troops, `+` to 20, down, 20 crew, Enter. 3) `M` Material xfer the same way: 1000 energy, 20 torpedoes, 185 supplies, 5 probes, 2 pods, `A`/Enter. 4) Or `M`,`S`,`K` to ask the supply ship. 5) Starfort, quit.
- What happened: `E`,`T`,`P` -> row 24 `Select target for xfer` with a TARGET ID box (rows 14-16). The supply ship sits at the same orbit position as our ship (space forces list: KS-9 `REL.POS 0, 0`, `ORBIT`), so only one glyph shows and the box reads `Our ship` there. Enter on the planet: `Report from Engineering:` / `Cannot transfer. There is no landing` / `party on the planet.` Enter on `Our ship`: the PERSONNEL table opens (rows 16-23, cols 40-79: header `  Krellan     . OUR SHIP  .  TARGET   `, rows `shock troops`, `crew`, `captives`, then `  TARGET` rows `marines`, `crew`, `passengers` with `XXX`), row 24 `Assign personnel` + `Activate`; the target columns were all 0, so I did not transfer. Esc closes.
- `M`,`S`,`K`: `Report from Executive Officer:` / `Should we request resupply from KS-9?`, row 24 `Answer?`. `y`: `Comm, make it so!` / `Report from Communications:` / `Aye, requesting resupply.` / `Message sent.` then `Resupply complete. The following` / `were teleported to us:` / `  Material    Received CurrentLvl (%)` / `Power reserves: 1389      100` / `Supplies      : 189.5     100` / `Torpedoes     :   20      100` / `Sensor probes :    0        0` / `Oblit. Pods   :    2      100`. The DATA panel still showed Torps 4 a few seconds later (not followed further).
- Dumps: `tut3_01_planet_integrity_failure_code_1999`, `tut3_02_you_may_want_to_restart_box`, `tut3_03_or_continue_press_n_box`, `tut3_04_exit_to_dos_prompt_after_n`, `tut3_05_first_bridge_screen_orbit`, `tut3_06_personnel_transfer_target_planet`, `tut3_07_personnel_transfer_table`, `tut3_08_request_resupply_from_ks9_question`, `tut3_09_resupplied_by_supply_ship`.

### Tutorial 5 - Escorts (start only V)
Start: DOCKED at SS-1 (Reg. Loc 7,0), STATUS `DOCKED`, MET 0.00, Time left 20.00, full stores, Nav submenu. Dump `tut5_01_first_bridge_screen`. (The DATA panel was not checked for the escort count.)
Manual steps (not played): 1) `G`,`E`,`F`: formation display, `+`/`-` or Up/Down to scroll, Enter to assign #1 Box; answer `n` to the escort captain's message. 2) Nav map, target SS-41 at 6,9, F9. 3) Orbit planet 41A; `n` to `Escorts to orbit?`, `n` map, `n` landing parties. 4) Formation #20 System Search; watch on the system map. 5) `G`,`E`,`T` targeting: Left/Right choose escort, `+` select, `-` deselect, Up/Down option, Enter (Disable, Slave to Us, Exclusive Fire, Hold Fire, Bombardment). 6) `G`,`E`,`C` commands: Data, Release, Swap, Conscript (box on TACTICAL), Rendezvous, Go to Station. 7) Quit.

### Tutorial 6 - Capturing ships (V, played start to finish = task D3)
Start: IN ORBIT of SS-46B (Reg. Loc 26,11; class K, Tech 5), ORBITAL, STATUS `RED` / `ALERT`, Shields 100 %, MET 2.50, Time left 22.50, Torps 18, one freighter in TARGET DATA (`H, 8` with `PTT`), Nav submenu. Dump `tut6_01_first_bridge_screen`.
| Step | Keys | Text on screen when it works (dump) |
|---|---|---|
| Demand surrender | `M`,`S`,`E`; row 24 `Select target`, move the box onto the freighter (no ID box is shown here), Enter | window `MESSAGE TRANSMISSION CONTROL` / `Destination: Enemy scum` / list `Demand surrender`, `Offer to surrender`; menu `Send` `Cancel` (`tut6_02_...`). Enter sends |
| Surrender | (auto) | `MESSAGE FROM UGA freighter:` / `We surrender. Do not shoot.` / `Standing by for boarding party.` then `Report from Executive Officer:` / `Excellency, should we prepare` / `a boarding party?`, row 24 `Answer?` (`tut6_03_...`) |
| Boarding mission | `y` | `By your command, sir.` / `ENGINEERING CONTROL` / `TELEPORTERS` / `Report from Combat Control:` / `150 shock troops standing by.`; menu `Capture` (C) `Kill` (K) `Hit&Run` (H) (`tut6_04_...`) |
| XO advice | `c` | `Report from Executive Officer:` / `Excellency, I recommend that we` / `send over two teams of shock troops` / `to secure the enemy ship.`; row 24 `PRESS KEY` (`tut6_05_...`) |
| Ship profile screen | any key | separate screen (custom-character text mode, `#vgamode=1`; the text dump is not readable). Picture: red deck profile of the ship, box `TYPE:FREIGHTER` / `CLASS:STAR TRADER` / `CREW: 115 MRN: 20`, table `SHOCK TROOPS` / `ST RESERVES 15/150` / `UGA MARINES ???` with ALIVE / KILLED, `SHIP SECT` / `CRITICAL SECT` counts for UGA and KRELLAN, bottom `SELECT OPTION | [B]OARD [W]ITHDRAW [A]CTIVATE`. Cursor starts top-left (`tut6_06_...`; picture /home/claude/stage/sf2-boarding-screen-start.png) |
| Place teams, go | arrows to a deck, `Space` per team (twice on the same spot is allowed), `A` | ship turns green, box `SHIP HAS BEEN CAPTURED!` / `105 CAPTIVES TAKEN.` (`tut6_07_...`; picture /home/claude/stage/sf2-boarding-screen-captured.png) |
| Back to bridge | `Esc` | `Report from Executive Officer:` / `UGAS CLARKE has 128 units power.` / `Cancelling red alert.` / `Shields lowered.` / `Changing alert status to ORANGE` / `Report from Combat Control:` / `Freighter has been captured.` / `We have 20 troops left, 0 dead.` / `Enemy have lost 15 dead, 105 captives`; row 24 `Leave team of guards?` (y/n) (`tut6_08`, `tut6_09`) |
| Recall | `n` | `Report from Engineering:` / `Teleporters activated.` / `Troops returned with 105 captives.` (`tut6_10`). No separate "recall?" question appeared, unlike the manual |
| Tractor beam, too far | `E`,`B`, box on ship, Enter | `TRACTOR BEAM CONTROL` / `Report from Engineering:` / `Target out of range.` (`tut6_11`) |
| Change orbit | `PgDn` | `Report from Helm:` / `Orbit lowered.` / `Orbit type: med, Altitude:12300 km.`; Orbt-Pos `M` |
| Tractor beam | `E`,`B`, box on ship, Enter (ship one cell away) | `Report from Engineering:` / `Lock-on achieved.` / `Tractor beam on.` (`tut6_12`) |
| AAS question | (auto, once the clock runs) | `Report from Executive Officer:` / `Excellency, tractor beam will be` / `cut off if shields are raised.` / `Should we turn off the AAS?`, `Answer?`; `n` -> `As you wish, sir.` / `AAS left on` (`tut6_13`) |
| Tow home | `Alt+J`, `y` | `Cancelling rest state.` ... `Leaving orbit.`; in hyperspace `Report from Security:` / `A captive just revealed this:` / `There is a UGA planet located at` / `71,18 in star system SS-29.` (`tut6_14`); `Shall we dock?` |
| Dock | `y` | COMMS `Congratulations, Altern` / `for your present of a captured enemy` / `ship. You will be well rewarded.` / `MESSAGE FROM STARFORT SF-1:` / `The cargo of the UGAS CLARKE` / `has been valued at 400 krels.` / `Your share of booty is 40 krels.` (`tut6_15`). No box, no conclude question; `C`,`S`,`Q` asks the normal `Do you want to stop?` |
Skipped manual steps: the science scan of the freighter, prize crew (Personl xfer). Differences from the manual: a boarding try on a disabled ship that is not adjacent is refused (`No valid target within range.`, practice mission), but here the surrendered freighter was boarded from across the orbit without moving next to it; the order of the guard/recall questions.
Target-box note: on the orbital display the box sometimes had to be one column right of the drawn ship glyph and sometimes exactly on it; where a TARGET ID box is shown, read it before pressing Enter.

### Tutorial 7 - Invasions (start V; the invasion itself is a graphics screen)
Start: IN ORBIT of SS-20A (Reg. Loc 0,5), STATUS `ORANGE` / `ALERT`, MET 2.40, Time left 17.60, TARGET DATA panel empty, COMMS `MESSAGE FROM FLEET HQ:` / `SS-20A invasion fleet ready.` / `Report from Executive Officer:` / `Excellency, monitoring satellites` / `deployed in polar orbits with` / `C&C sats in synchronous orbits.` / `We are ready to unleash hell.` Dump `tut7_01_first_bridge_screen`.
- `G`,`I`: full-screen `INVASIONS` table (rows `ID#  SS-20A`, `Name  JIEOJ`, `Tech Lvl 4`, `Cities org: 5/d: 0 c: 0`, `Shields 0`, `Inv Stat  Scheduled`, `Bttlcrsr orb: 1/ass: 0`, `Dstryr orb: 4/ass: 1`, `Spplyshp` ...). Dump `tut7_02_invasions_table`. Esc returns to the stG menu.
- `G`,`P`: menu `boMbardment` `Orbit control` `op Big brother`. `O` switches to the EGA graphics planet/invasion program (`#vgamode=2`, text dump blank): everything after this in the tutorial (city view `V`, bombard `B`, select `S`, invade `I`, zones F1-F8, assault `A`, orders `O`, drop ships `D`) is drawn in graphics and cannot be checked from text dumps. Not played. Dump `tut7_03_orbit_control_ega_graphics_blank_text`.
Manual steps: 1) Freeze, `G`,`I` read the invasion table. 2) Esc, `P`,`O` (a `COMPUTER MALFUNCTION` message is expected). 3) Box on a city, `V` view, Esc, `B` bombard (`y` to the warning, `B` again) until four cities remain. 4) `S` on a city, `y` select all, `I` start. 5) `A` assault zone 1, corner `2`, `3` units; same for zones 2-4 (F1-F4 views, F9/F10 back). 6) In each zone `O`,`C` build a landing pad; `G`,`C`,`R` reduce. 7) `D`,`B`,`W`,`6`, pick zone, `L` launch drop ships. 8) On surrender `W` withdraw, Esc for the honors report, `D`,`S` drop two legions. 9) Quit.

### Tutorial 8 - Strategic command (V for the start and the lists)
Start: IN ORBIT of SS-5A (Reg. Loc 25,15; class P), STATUS `RED` / `ALERT`, Shields 90 %, energy 48 %, Torps 16, MET 3.11, Time left 56.89, TARGET DATA empty, COMMS `NAVIGATION CONTROL` / `Report from Combat Control:` / `Enemy ship just left system.` / `Course was 266..` on all three starts: the enemy heavy cruiser the manual describes leaves at once, so there is no fight here. Soon after: `MESSAGE FROM E-1 AHQBLEBIUY:` / `We're running low on power and are` / `in need of a fill-up.` Dump `tut8_01_start_orbit_enemy_just_left_system`.
- `G`,`R`,`T`,`S`: full-screen ` KRELLAN ` `SPACE FORCES` list (`NAME TP ID # R.LOC SYS LOC VEL POWER SPPLY ... STATUS`), escorts shown as `E-1`, `E-4` with R.LOC `ours` (`tut8_02`). `O` switches the right half to `ORDERS` / `OBJECTIVE` (`Escort ship`, `Invade` / `Planet SS-48A:GDEZOSQ`, `Free operation` / `Starfort 2`) (`tut8_03`). Footer: `next page   [O]:orders   [Esc]:exit`.
- Down to a ship, Enter: command window (rows 0-19, cols 0-27) `STRATEGIC COMMAND` / `ID# : KS-0` / `NAME: SLAYER` / `TYPE: battlecruiser` / `Current Orders` / `Invade Planet SS-48A` / `Commands`: `Data`, `Move to`, `Rendezvous`, `Withdraw`, `Hold position`, `Patrol system`, `Invade`, `Escort ship`, `Attack`, `Free operation`; `[CR]: assign  [Esc]: exit`; row 24 `Make selection` (`tut8_04`). I moved down to Rendezvous and pressed Enter; no acknowledgement text was caught (`tut8_05` shows the window unchanged), so the order step is NOT confirmed.
- `G`,`F`: `RL:25,15 Press [M] for menu  FLEET COMMAND DISPLAY 1`, `BATTLE FLEETS` boxes (`tut8_06`).
- `C`,`I`,`I`,`U`: `ENEMY SHIPS` table (`SHIP TYPE / KNOWN / DESTROYD / CAPTURED`; Heavy cruisers known 1, Freighters destroyed 1) (`tut8_07`). No position is given for the cruiser.
Manual steps: 1) Freeze; system map (red dot = heavy cruiser). 2) `G`,`R`,`T`,`S`, `O`; pick supply ship KS-4, Enter, Rendezvous, Enter. 3) `G`,`F`, `M`; select KS-0, `C`,`R`. 4) PgDn pages: independent destroyers (order two to rendezvous), invasions, starforts. 5) `G`,`R`,`L`,`S` local list. 6) Unfreeze; when ships arrive resupply with `M`,`S`,`K` and conscript the destroyers. 7) Go to SS-10A at 24,4 with escorts. 8) Order an escort for a supply ship; order KS-0 and troop ships to invade SS-48A (`C`,`I`). 9) Patrol / Free operation orders; quit.

## A (addition). A real CAREER mission 1-A completed and concluded (mission #031) - V
Side effect to know about: when I re-entered the practice grid and pressed Enter on 1-A, the sign-on screen showed `Practice Mode       Off`, so the next `N` started a career mission. I completed and concluded it. Officer ryan's career now has 1-A done and the game hands out mission 1-B (#032 was started for task B). Tutorials 3, 6, 8 are also newly marked `*`.
- Start differs from practice only in the orders header: `Mission #: 031` with no `PRACTICE MISSION` text.
- Completion: first planet scanned (SS-39A, `ANALYSIS: Class I/ice`, `Rich minerals; no supplies available.`, no enemy) gave the same three `PRESS KEY` boxes, then the map question. So a rich planet of any class counts, UGA ships need not be there.
- Docking: `Shall we dock?` `y` -> box ` Do you wish to conclude the mission?` / `Answer?` -> `y`.
- Screens after `y`, in order:
  1. `CONGRATULATIONS, ALTERN!` / `You have successfully completed the major objectives of your mission!` (rows 10-11, about 2 s, no key).
  2. MISSION TALLY, page PERSONAL (`SLAVES 0  KRELS 30`), menu Personal/Krellan/Values/Orders, `Esc` leaves. (`career_end_01_mission_tally`)
  3. Career only: `CAREER UPDATE SUMMARY FOR Altern RYAN` (row 0) / `You have successfully completed 1 mission. in this campaign in 2 attempts.` / `Your current accumulated personal wealth totals for this campaign:` / `WEALTH :      30      Krels` / `SLAVES :       0` / `Your averages per mission:` / `WEALTH : 15 Krels` / `SLAVES : 0` / `Your current balance:       30   Krels` / `Progress towards battlecruiser upgrade:` / `0  %` with a 0-100 scale bar / `Amount still needed :   29970  Krels` / `NOTE: Only Prefects and above can upgrade their battlecruiser` / row 24 `PRESS ANY KEY TO CONTINUE`. (`career_end_02_career_update_summary`)
  4. Career only: `CAREER STATISTICS` table (ships DSTRYD/CAPTRD/DELVRD/TOTAL; events `Star systems mapped 5`, `Planets scanned 1`, `Planets w/rich minerals 1`, ...; `LOSSES & PENALTIES`; `KRELS 30  SLAVES 0`). No prompt text; any key continues. (`career_end_03_career_statistics`)
  5. `Some data files from this mission are no longer needed ...` / `Answer (Y/N)?`. This time `y`: `Deleting files...` / `...deleting planet surface files` / `del paf*.031Illegal Path.` / `...deleting careers.2.*` / `Unable to delete: save_nm.031.` / `...deleting save_nm.031` / `...deleting *.031` / `PRESS KEY` (row 21). (`career_end_04_deleting_files_yes`)
  6. `Would you like your log files (reports, messages, ship data)` / `saved into a mission subdirectory?` / `Answer (Y/N)?` -> `n` -> `PRESS KEY` (row 24 this time).
  7. SIGN-ON OPTIONS.
- No promotion, medal or debriefing text after one mission (promotion needs all missions of the rank: I, from the program strings).
- Service Record afterwards: MISSIONS RECORD cell 1-A now shows `2` on a green background (attr 0x20); every other cell `0` on red (0x40). (The summary said "in 2 attempts", so the number may be the attempt count: I.) SKILLS PROGRESSION, PROMOTIONS (`No promotions on record.`), MISSION AWARDS (`No mission awards on record.`), SPECIAL AWARDS (`No special awards on record.`) pages, each `PRESS KEY FOR NEXT or [ESC]`. (`career_end_05_missions_record_1a_done`, `career_end_06_skills_progression`)
- Next `N`: text screen `As an ALTERN you have been assigned the standard battlecruiser` / `IKS KRELLA. She's a good ship, so treat her well.` / `PRESS KEY` (not shown in practice), then orders `Subject  : Orders for Reconnaissance Mission into Region #1-B`, `Mission #: 032`: `locate and survey with landing parties at least 5 new Krellan-inhabitable planets (A,D,K class with supplies & no radiation)`, `You have 25 days`. (`career_new_mission_ship_assigned_press_key`, `career_mission_1b_orders`)

## B. Armed enemy: NOT ACHIEVED - what was established instead
No enemy that shoots back was met, so there is still no capture of: an enemy on the TACTICAL display or LRS in normal space, closing range, a miss, enemy fire, shield or hull damage, DAMAGE panel colours, damage control repairs, own ship disabled/destroyed, escaping. Do not treat any of those as known.
Where I looked (V):
- Tutorial 4 (Weapons): three unarmed freighters in orbit only (see above).
- Tutorial 6: one unarmed freighter that surrenders.
- Tutorial 8 is the only tutorial the manual gives an armed enemy (a heavy cruiser). On all three starts the first COMMS lines were `Enemy ship just left system.` / `Course was 266..`: it is gone before the player can act. Its intel table lists `Heavy cruisers  known 1` without a position. Systems roughly on that course from 25,15: SS-30 (13,14), SS-49 (5,14), SS-27 (8,13); not chased.
- Practice mode cannot be set to a higher mission (grid refuses silently, see above), so a harder practice mission is not available to an Altern.
- Practice mission #030, system SS-11: a probe reported `Enemy activity detected.` / `Possible UGAS starbase detected.`; the ships in orbit of SS-11A were three freighters; the starbase was not visited. Career missions 1-A (#031) and 1-B (#032, two systems): no enemy at all.
- Leaving orbit in Tutorial 4 (F9) put the ship at Position 5,46, far from the planet at once; the freighters were not on TACTICAL or LRS there, so there is still no dump of an enemy in normal space.
Leads for a next try (I): fly to the `Possible UGAS starbase` system when a probe reports one (starbases are armed per the manual); or in Tutorial 8 chase to SS-30/SS-49; career mission 1-E/2-B orders mention warships but need promotion first (2-B: `eliminate as many UGA warships as possible`).

### ECM and our own locks (V, Tutorial 4, in orbit with three freighters locked)
- `Alt+E` on: COMMS `Report from Weapons Console:` / `ECM switched on.` The `ECM` label (row 18, cols 41-43, left of `Usage`) changes from attr 0x08 (dark grey) to 0x50 (black on magenta). The Wpn letters (P/T/T) vanish from every TARGET DATA row within about 2 s (targets stay listed), and the LRS box (rows 2-9, cols 1-16) fills with a pattern (sensors off). ECM Usage % did not change (75 %).
- Firing with ECM on: `Delete` -> `Report from Weapons Console:` / `Torpedoes not targeted.`; `Insert` -> `Report from Weapons Console:` / `Phasers not targeted.` So yes: ECM on blocks our own locks and shots (matches the manual, which describes it for normal space; here it was the same in orbit).
- `Alt+E` off: `Report from Weapons Console:` / `ECM switched off.` then `Report from Weapons Console:` / `Torpedoes locked on target.`; the P/T/T letters return.
- Dumps: `ecm_off_before_locks_present`, `ecm_switched_on_locks_lost_lrs_blank`, `ecm_on_fire_refused_not_targeted`, `ecm_switched_off_locks_return`.

### Other combat facts picked up (V)
- Scan mineral words seen so far: `Rich minerals`, `Fair minerals`, `Poor minerals`, `No minerals`.
- In the practice mission the XO disable question did not appear when entering an orbit with three freighters (it had appeared in earlier runs); in Tutorial 4 it appears after the first shot or fire attempt.
- Disable sequence again (practice #030): `Our phaser hit freighter` / `  Size:  259 units    Power:   6 %` / `They've lowered their shields!` / ... / `Enemy ship disabled!`.

## D. Summary of the small checks
1. Alt+J: works in orbit (leaves orbit by itself, full message list in D1 above) and works inside a tutorial (question in Tutorial 4; the whole trip to the starfort and docking in Tutorial 6). V.
2. F9 in orbit with no target: with speed 0 `Velocity must be set.`; with a speed set the ship leaves orbit (`Ship underway.` / `Engines engaged.` / `Leaving orbit.`). V.
3. Boarding: full flow recorded in Tutorial 6 (surrendered ship). On a ship disabled by our weapons in the practice mission the boarding command reached `Select target to board` but was refused with `No valid target within range.` because the ship was elsewhere in the orbit; boarding a weapon-disabled ship after closing in was not done.
4. `+` on eXplore: refused with `Already surveyed!` on a surveyed planet; on an unsurveyed planet the team reports `Survey complete...` exactly 1.00 day after landing. V.

## State left behind
- Harness 1: DOSBox restarted, at the SIGN-ON OPTIONS menu, officer ryan.
- Officer ryan's career: mission 1-A completed (career), mission 1-B started as #032 and abandoned by restart (an AUTOSAVE.032 probably exists; `R` Resume would list it). Sequence number is now 032. Tutorials marked: 1, 3, 6, 8. Practice Mode default reads Off.
- Mission #031 files were deleted by the game's own "file debris" step (I answered `y`).
- Nothing under /home/claude/sf2-3ds was changed except new files in tests/screens (not committed). Two pictures of the boarding screen are in /home/claude/stage (sf2-boarding-screen-start.png, sf2-boarding-screen-captured.png).
- Harness 2 (/tmp/sfw2, DISPLAY :10) was not touched.

## Dump count
105 new `.txt` dumps were added to tests/screens in this session; every one is named in the section it belongs to above (prefixes: `mission1a_`, `mission_end_`, `career_end_`, `career_`, `service_record_`, `navmap_`, `probes_`, `probe_`, `sysmap_database_`, `landing_party_`, `boarding_`, `altj_`, `orbit_f9_`, `quit_`, `ctrl_x_`, `tutorials_menu_`, `practice_missions_grid_`, `tut2_` .. `tut8_`, `ecm_`).
