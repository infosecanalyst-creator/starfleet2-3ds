# Star Fleet II v2.0.22 - screen flows captured from the DOSBox harness

Captured 2026-11-21 by driving SF2.EXE with keystrokes (officer "ryan"). Dumps are in
`sf2-3ds/tests/screens/` (159 new files: `tut1_*`, `prac_*`, `menu_*`, `view_*`, `refuse_*`).
In the dumps, box-drawing characters show as `.` in the text rows (true CP437 codes are in `#raw`).

Where text is found on the bridge screen (0-based rows of the 25-row screen):
- Row 24, cols 0-23: prompt ("Select option", "Establish orbit?", "Shall we dock?", "Answer?", "PRESS KEY", "Continue?" ...). Cols 24+: the current menu row.
- Rows 15-22, cols 1-39: COMMS window (scrolling reports).
- Row 13, cols 18-26: Mode word (` DOCKED `, `NORM SPC`, `HYPRSPCE`, `IN ORBIT`, `ALIGNMNT`).
- Rows 15-16, cols 42-50: `STATUS:` / alert word (`DOCKED`, `GREEN`, `ORANGE`+`ALERT`, `RED`+`ALERT`).
- Row 11: `MsnElpTme` (MET) and Dist Trg; row 9: `1:<target>`; rows 3/5: Reg. Loc / Position; rows 15-16 cols 53-58: `s:`/`c:` set and current velocity.
- Row 1: panel title `TACTICAL` or `ORBITAL` (orbit display).

## (a) Tutorial 1 - Basic Navigation, step by step

Completed end to end: undock, system travel, manual steering, orbit, probe, hyperspace to SS-33, orbit an unknown planet, hyperspace back, dock, quit to sign-on.

| # | Keys | Dump | Proof text on screen |
|---|------|------|----------------------|
| 0 | `T` at sign-on | tut1_00_tutorials_menu | `TUTORIALS` box, `1  Basic navigation` ... `Press number of desired tutorial` |
| 1 | `1` | tut1_01_motto_press_key | `To Live is to Conquer...`, `PRESS KEY` |
| 2 | `Enter` | tut1_02_bridge_docked_nav_menu | Bridge; Mode ` DOCKED `, STATUS `DOCKED`, Position `8x 77y`, Reg. Loc `29x 1y`; COMMS `NAVIGATION CONTROL`; menu row is already the Nav submenu `System map / Nav map / War map / Helm` (tutorial starts inside Nav). `Esc` gives the top menu (tut1_03_bridge_docked_main_menu) |
| 3 | `S` (or `N`,`S` from top menu) | tut1_04_sysmap_start | `MAP OF STAR SYSTEM SS-3  (Class F)`, planet list row `A. K . - . - . -. x. Krellan`, menu `Targeter taRgets Database Lrs box Strg cmd` |
| 4 | move TDB onto planet. Keys actually sent: `Right`,`Right`,`Down` (TDB appeared at 10,70), `Shift+Right`, `Shift+Down`, `Right`, `Down`, `Shift+Right` x3, `Down`. Arrow = 2 x / 5 y, Shift+arrow = 10 x / 20 y; TDB sits on a 2x5 grid and the first press seems only to show it | tut1_05_sysmap_tdb_on_planet | TARGET box `Object  .Planet`, `Position. 52x 40y`, `Distance. 57.4`, `Bearing . 131` |
| 5 | `T` | tut1_06_targeter | `TARGETER` box: `Target # 1`, `Velocity: 0.9`, `[CR]: enter target   [Esc]: exit` |
| 6 | `F3` x4 (default is 0.9, not 0 as the manual implies; manual says F4 x5) | tut1_07_targeter_speed05 | `Velocity: 0.5`, `Time:    0.45  dy` |
| 7 | `Enter` | tut1_08_target_entered | `Target # 1   ENTERED` (stays ~1-2 s, then the box closes) |
| 8 | `F9` | tut1_10_f9_engines_start, tut1_13_sysmap_undocked | Helm window on map: `Report from Helm:` / `Nav. computer locked on target.` / `Maneuvering engines engaged.` then `Engines engaged.` then `We have undocked.`; TARGET `Object .SS-3A`; `Velocity.s: 0.5`; status word changes `DOCKED` -> `NORM SPC`; then `MESSAGE FROM STARFORT SF-1:` / `Good hunting, LASUM!` |
| 9 | `Esc`,`Esc` (ignored until the helm report finishes, ~8 s) | tut1_15_bridge_underway | Mode `NORM SPC`, STATUS `GREEN`, `set:133`, `1:SS-3A`, Dist Trg counting down, `s: 0.5` / `c: 0.5` |
| 10 | wait (~20 s) | tut1_16_target_reached | COMMS `Report from Helm:` / `Target reached, Excellency.`; Dist Trg `0`; `c: 0.0` |
| 11 | (prompt) | tut1_17_establish_orbit_prompt | row 24 `Establish orbit?` |
| 12 | `N` | (prompt returns to `Select option`) | target line `1:` now empty |
| 13 | manual nav: `F4` x3 then `F9` | tut1_20_manual_f9 | `s: 0.3`; COMMS `Report from Helm:` / `Ship underway.` / `Engines engaged.` |
| 14 | `Alt+S` | tut1_21_sysmap_manual_wrong_way | map status `NORM SPC`, `Headng: 135`, `Velocity.s: 0.3 c: 0.3` |
| 15 | steer `Shift+F6` x12, `F5` x5 | tut1_22_sysmap_manual_steered | `Headng: 310`, `Course  .s: 310.c:310` |
| 16 | `Alt+M` | tut1_23_bridge_manual_course | bridge `set:310`; (COMMS also showed `Planet in path.` / `Nav computer altering course.`) |
| 17 | `F10` (I stopped here instead of flying 60 points back) | tut1_24_f10_emergency_stop | `Report from Helm:` / `Emergency stop procedure completed.`; `s: 0.0 c: 0.0` |
| 18 | re-target planet (`Alt+S`, TDB, `T`, `Enter`, `F9`), wait, `Y` at `Establish orbit?` | tut1_25_orbit_entered | title `ORBITAL`, Mode `IN ORBIT`, `Orbt-Pos`, `Orb Alt` `28900`; COMMS `Entered orbit of planet SS-3A.` / `We're in a standard high orbit.` / `Altitude is 28900 km.` |
| 19 | (Krellan base planet asks) | tut1_26_orbit_resupply_prompt | `MESSAGE FROM PLANETARY ORBCON:` / `Do you wish to be resupplied?`, row 24 `Answer?`. `Y` -> `Just send us a message when you're ready for the transfer.` (tut1_28_bridge_in_orbit) |
| 20 | `Alt+N` | tut1_29_navmap | left legend `NAV STAR MAP`, TARGET `ID/class. SS-3 /F`, `Location. 29x  1y`, menu `Targeter taRgets Database Probes Strg cmd` |
| 21 | `Up` (TDB to 29,2), `P`, `L` | tut1_31_probe_display | `PROBES CONTROL` table, 5 rows `Loaded`, `[Esc]:exit .[CR]:launch`; TARGET `ID/class. SS-45/O` |
| 22 | `Enter` | (table closes; bridge DATA `Snsr Probes.L:4S:5`) | later popup tut1_33_probe_report_popup: `Probe #1 arrived at RegLoc 29,2.` / `Mapping of system SS-45 complete.` / `1 planet in system.` / `PRESS KEY` |
| 23 | `Esc`, `Left`, `Up` (TDB to 28,3) | tut1_34_navmap_tdb_ss33 | `ID/class. SS-33/G`, `Location. 28x  3y`, `Distance.  2.2` |
| 24 | `T` | tut1_35_hyper_targeter | `TARGETER`, `Target # 1`, `Velocity: 8.0`, `Max. Vel :  8` |
| 25 | `Enter` | tut1_37_target_system_map | title bar `TARGETER`, `MAP OF STAR SYSTEM SS-33 (Class G)`, `Object:  ID#:  Loc: 40,40`, row 24 `Select target` |
| 26 | `Shift+Down` x2 (to 40,0), `Enter` | back on nav map | `ID/class.SS-33 /G`, `Velocity.s: 8.0` |
| 27 | `F9` | tut1_41_navmap_helm_jump_report | `Nav. computer locked on target.` / `Course set to system edge for jump.` / `Maneuvering engines engaged.` / `Engines engaged.` (orbit is left automatically) |
| 28 | `Esc`,`Esc` | tut1_42_bridge_to_system_edge | Mode `NORM SPC`, `1:SS-33`, `s: 8.0` / `c: 0.9` |
| 29 | wait | tut1_43_bridge_hyperspace | Mode `HYPRSPCE`, Position blank (`  x   y`), Reg. Loc changes 29,1 -> 29,2 -> 28,3 |
| 30 | wait | tut1_44_emerged_target2_loaded | Mode `NORM SPC`, `EDC.. SS-33`, `1: 40,0`, COMMS `Target reached, Excellency.` then `Report from Helm:` / `Target bearing 121., distance  5.8.`; finally `Target reached, Excellency.` again at the 2nd target (no orbit prompt: target was empty space) |
| 31 | `Alt+S`, TDB on planet 52,20, `T`, `Enter`, `F9` | tut1_46_sysmap_ss33 | planet row `A. K . ? . ? . -. -. Unknown` |
| 32 | wait, `Y` at `Establish orbit?` | tut1_48_orbit_new_planet | `ORBITAL`, name box `SS-33A` / `UNKNOWN`, `Entered orbit of planet SS-33A.`, `Altitude is 21200 km.` |
| 33 | (automatic) | tut1_49_scanning_planet, tut1_50_short_range_scan_blocking | `Scanning planet`, then `SHORT RANGE SCAN` / `OBJECT: Planet  POSN: 52,20` / `ANALYSIS: Class K/Krella-like` / `Fair minerals; supplies available.` with row 24 prompt BLANK |
| 34 | key(s) to continue; `N` for planetary map | tut1_51_landing_parties_prompt | `Report from Executive Officer:` / `Excellency, should we send down` / `landing parties to this planet?`, row 24 `Answer?` |
| 35 | `N` | (bridge) | `As you wish, sir.` / `No landing parties assigned.` |
| 36 | `N`,`W` (war map), TDB `Right`,`Down`,`Down` onto starfort | tut1_53_warmap | legend `WAR STAR MAP`; with TDB on it: `Object  .Starfort`, `Location.  SF-1` |
| 37 | `T`, `Enter`, in target map move TDB near 8,77 (snaps), `Enter`, `F9` | tut1_56_target_sysmap_starfort | `Object: Starfort  ID#: SF-1    Loc:  8,77` |
| 38 | wait | tut1_58_shall_we_dock_prompt | COMMS `Target reached, Excellency.`; row 24 `Shall we dock?` |
| 39 | `Y` | (bridge) | Mode ` DOCKED `, STATUS `DOCKED`, COMMS `Report from Helm:` / `Docking procedure complete.`; Supplies climbs back to 100% |
| 40 | SIS: `Ctrl+I` | view_sis_prompt, view_sis_index | `SHIP INFORMATION SYSTEM` / `Enter topic:`; Enter -> `INFORMATION INDEX` ... `PRESS KEY` |
| 41 | `C`,`S`,`Q` | tut1_61_quit_prompt | row 24 `Do you want to stop?` |
| 42 | `Y` | tut1_62_after_quit_yes | blank screen with `Do you wish to give up on this mission?` |
| 43 | `N` | tut1_63_back_at_signon | `SIGN-ON OPTIONS`; `T` now shows `1* Basic navigation` (tut1_64_tutorials_menu_done_asterisk) |

Good checklist triggers: Mode word, STATUS word, row-24 prompt, `1:<target>` field, and the COMMS lines `We have undocked.`, `Target reached, Excellency.`, `Entered orbit of planet`, `Docking procedure complete.`, `Emergency stop procedure completed.`

## (b) Practice mission start

| # | Keys | Dump | Proof text |
|---|------|------|-----------|
| 1 | `P` at sign-on | prac_01_switch_to_practice_prompt | box `Do you want to switch to Practice mode?` |
| 2 | `Y` | prac_02_practice_mode_selected | box `Practice mode selected` (auto-closes) |
| 3 | (auto) | prac_03_missions_record | `MISSIONS RECORD FOR Altern RYAN`, grid A-E x 1-7, `[Enter] Select mission for practice`, `[V] View orders of selected mission` |
| 4 | `V` | prac_04_view_orders | `COMMUNICATION FROM KRELLAN FLEET HEADQUARTERS`, `Orders for Reconnaissance Mission into Region #1-A` |
| 5 | `Enter` (close orders), `Enter` (select) | back at sign-on | `Practice Mode        On` in CURRENT DEFAULT SETTINGS. P only toggles the mode; pressing P again asks `Do you want to switch to Career mode?` (prac_06) |
| 6 | `N` (start new mission) | prac_07_orders | orders with `Mission #: 021            PRACTICE MISSION`, `PRESS ANY KEY TO CONTINUE` |
| 7 | `Enter` | prac_08/09_simulation_checklist | `Please stand by for glory!`, `SIMULATION CHECKLIST`, ten items turning `ok` (takes ~5 s, no key needed) |
| 8 | (auto) | prac_10_motto_press_key | `TO THE STRONGEST GO THE STARS`, `PRESS KEY` |
| 9 | `Enter` | prac_11_bridge_xo_welcome | bridge, COMMS `Report from Executive Officer:` / `Welcome aboard the IKS FURY,` ..., row 24 `PRESS KEY` with EMPTY menu row |
| 10 | `Enter` x2 | (two more XO pages) | `Ready to leave starfort.` ..., `it is a true honor to serve as your Executive Officer.` |
| 11 | `Enter` | prac_14_starfort_message, prac_15_comply_prompt | `MESSAGE FROM STARFORT SF-1:` / `...test of the priority message alert.` / `Please give extra rations to Skippy.`; row 24 first blank, then `Comply with the request?` |
| 12 | `Y` | prac_16_log_window_standby | `Message acknowledged.` / `Com test was successful.`; the Commander's Log opens by itself: `VIEW MODE..COMMANDER'S LOG`, `**** CHRONICLE OF GLORY ****`, row 24 `PLEASE STANDBY` + `[M]: menu  [Esc]: exit` (`M` shows prac_17_log_menu) |
| 13 | `Esc` | prac_18_bridge_command_menu | `Select option` + `Nav Eng cmB Cmp Sec coM stG scI ctL`; Mode ` DOCKED `, `Time left. 10.00` |
| 14 | `F4` x5, `F9` | prac_19_undock_manual | `Ship underway.` / `Engines engaged.` / `We have undocked.` then `Good hunting, FURY!` |
| 15 | `Alt+J` | view_alt_j_starfort_prompt | popup `Do you wish to go to nearest starfort?` |
| 16 | `Y` | prac_21_altj_setting_targets, prac_22_local_system_jump_hyperspace | `Aye, sir.` / `Stopping engines.` / `Setting new targets.` / `Course set to system edge for jump.`; Mode `HYPRSPCE` briefly (local system jump) |
| 17 | wait | prac_23_shall_we_dock_prompt | `Shall we dock?` (`N` declines; later `N`,`H`,`D` docks: prac_25_manual_dock: `Ship data file archived.` / `Resupply & repairs complete.` / `Ready to undock.`) |
| 18 | `C`,`S`,`Q`,`Y`,`N` | prac_26_quit_prompt, prac_27_give_up_prompt | same two prompts as the tutorial; back to sign-on |

## (c) Command menu tree (hotkey = capital letter)

Main: **N**av, **E**ng, cm**B**, **C**mp, **S**ec, co**M**, st**G**, sc**I**, ct**L**. Esc backs out one level.

- **N Nav** (COMMS `NAVIGATION CONTROL`): **S**ystem map, **N**av map, **W**ar map, **H**elm
  - System map menu: **T**argeter, ta**R**gets (TARGETS table), **D**atabase (planet scan + `Display planetary map?`), **L**rs box, **S**trg cmd (-> Space forces / Legions)
  - Nav map / War map menu: **T**argeter, ta**R**gets, **D**atabase, **P**robes (-> **L**aunch Control, **O**perations), **S**trg cmd
  - Helm: **E**vasive (toggle), **O**rbit, **D**ock, em**H**ypmv (`Continue?`), **P**ursuit (`Select pursuit target`), **T**argets, **C**ircle (-> **S**earch, **P**atrol)
- **E Eng** (`ENGINEERING CONTROL`): **D**am con (DAMAGE CONTROL table, `PRESS KEY`), **T**eleprt, **P**robes, trac **B**m (`Select target`), pw**R** sys, **E**ngdata
  - Teleprt: **L**anding prty (in orbit: Mission table eXplore/Establish base/Hold/Obtain supplies/Combat/Negotiate, `Assign mission/personnel` + `Activate`), **B**oardng prty, **P**ersonl xfer, **M**aterial xfr (`Select target for xfer`)
  - pwR sys: **S**tatus, sc**H**ematics (full-screen `SUBLIGHT PROPULSION/POWER` diagram)
  - Engdata: **L**ogistics, shp**D**ata (`Access SPD records?` -> SHIP PERFORMANCE DATA screen, options 1-4)
- **B cmB** (`COMBAT CONTROL`): **C**ombat CC (full-screen CCC; its own menu `[T] TDB on/off [H] Headings [W] WeaponsCtl [N] Navigation [C] Commands`), **W**eapons (-> **P**hasers, **T**orpedoes, **O**bliterator pods; Phasers/Torps -> **D**estroy, disa**B**le, **M**ode, **S**tandby, **C**onditn)
- **C Cmp** (`COMPUTER CONTROL`): **C**mbt stats (-> **E**nemy, **K**rellan tables), **I**nformatn, self-**D**est (`SELF-DESTRUCT SEQUENCER` / `Continue?`), **S**pcl srvcs, cmdr **L**og (-> **V**iew log, make **E**ntry)
  - Informatn: **L**nd pty, **P**lanets (`Select planet`), **S**tr sys (`Select star system`), **B**ases, **I**ntellg (-> **U**GA ships, **S**tarbases, **M**isc., **R**eport), **R**ef Lib
  - Spcl srvcs: **M**sn tally (-> **P**ersonal, **K**rellan, **V**alues, **O**rders), **S**ave, **C**areer, se**T**tings, **Q**uit (`Do you want to stop?` -> `Do you wish to give up on this mission?`)
- **S Sec** (`SECURITY CONTROL`): **S**ecurity status, internal **D**efense (full-screen deck plan, text mode `#vgamode=1`), **I**nterrogation
- **M coM** (`COMMUNICATIONS CONTROL`): **C**mb log, **R**eports (INTERNAL REPORTS window, `[M] for menu [Esc] to exit`), **M**essages, s**T**atus, **S**end msg (-> **F**lt HQ, **E**nemy, **P**lanet, **K** ship, **G**neral, **S**tfort, **L**ndpty; each -> **S**end, **C**ancel)
- **G stG** (`STRATEGIC COMMAND`): **F**leet (FLEET COMMAND DISPLAY, `[M]` menu), **R**egion (-> **L**ocal, **T**otal; then **S**pace forces, **L**egions, l**O**sses), **S**ystem (-> **S**pace forces, **L**egions), **P**lanet (`PLANETARY COMMAND`: bo**M**bardment, **O**rbit control op, **B**ig brother -> **S**tatus, **I**nitiate, **C**ancel), **E**scort (-> **F**ormation, **T**argeting, **C**ommand), **I**nvmgt (INVASIONS table)
- **I scI** (`SCIENCES`): **S**canners (`Select target for scan`, TDB + Enter), **M**edical, dept.**Q** (-> **A**rtifacts, **X**-equip)
- **L ctL**: **H**ot keys (same as Ctrl+H), cmnd **T**rees (same as Ctrl+T, COMMAND STRUCTURE)

Shortcut views dumped: Alt+S (view_system_map), Alt+N (view_nav_map), Alt+W (view_war_map), Alt+X (view_combat_control), Alt+F (view_fleet, view_fleet_menu), Ctrl+O (view_orders), Ctrl+L (view_log_entry_mode, view_log_exit_warning), Ctrl+H, Ctrl+K, Ctrl+T (view_help_*), Ctrl+I (view_sis_prompt, view_sis_index), Ctrl+N (view_notepad), Alt+P (menu_eng_teleporters), Alt+J, Alt+O (Mode `ALIGNMNT`), Alt+R (`Change rest state?`), Alt+E, Shift+F8 (shields 100%), Shift+F8 again (red alert), orange alert, sign-on screens (view_signon_*).

## (d) Refusal messages seen

| Message (COMMS unless noted) | Trigger | Dump |
|---|---|---|
| `Report from Helm:` / `Velocity must be set.` | F9 with set speed 0 | refuse_f9_velocity_zero |
| `Engines already on` | F9 while already under way | refuse_f9_engines_already_on |
| `Report from Helm:` / `Cannot use hyp.eng for manual nav.` | F2 (hyperspace speed) with no nav target | refuse_hyperspeed_no_target |
| `Report from Helm:` / `Too far away to dock.` | N,H,D away from a starfort | refuse_dock_not_near_starfort |
| `Unable to establish orbit.` | N,H,O not at a planet | refuse_orbit_not_at_planet |
| `Report from Helm:` / `Unable to use while docked.` | N,H,H (emergency hyperspace) while docked | refuse_emerg_hyper_while_docked |
| `Report from Helm:` / `Cannot do - no nav target loaded.` | Alt+H (helm override) with no target | refuse_helm_override_no_target |
| `Report from Weapons Console:` / `Phasers not targeted.` | `[` (fire phasers), any alert state, no target | refuse_phasers_not_targeted |
| `Report from Weapons Console:` / `Torpedoes not targeted.` | `]` (fire torpedoes), no target | (same screen family; visible in refuse_landing_party_not_in_orbit) |
| `Report from Weapons Console:` / `We must be in orbit!` | B,W,O obliterator pods not in orbit | refuse_obliterator_not_in_orbit |
| `We must be in orbit.` (under `TELEPORTERS`) | Alt+P,L landing party not in orbit | refuse_landing_party_not_in_orbit |
| `We must be in orbit.` (under `PLANETARY COMMAND`) | G,P,M or G,P,O not in orbit | (seen; dump pruned) |
| `Report from Combat Control:` / `No valid target within range.` | E,T,B boarding party with nothing nearby | refuse_boarding_no_valid_target |
| `Report from Science Console:` / `That's our ship, Excellency!` | I,S then Enter with TDB on own ship | refuse_scan_own_ship |
| `SHORT RANGE SCAN` / `OBJECT: Nothing` / `Nothing there but the normal distribution of elementary molecules.` | I,S on empty space | refuse_scan_nothing_there |
| `Sorry, Excellency. There is no artifact we have discovered.` / `...no experimental equipment onboard.` | I,Q,A / I,Q,X | refuse_no_artifact |
| `Report from Security:` / `We have 0 captives on board.` | S,I (also S,S) | refuse_interrogation_no_captives |
| `We are not in a rest state.` + prompt `Change rest state?` | Alt+R | view_rest_state_prompt |
| popup `Only available on the flagship` | nav map P,O (probe Operations) | refuse_probe_operations_not_flagship |
| popup `NOT AVAILABLE IN TUTORIALS` (~1.5 s) | in a tutorial: Ctrl+O, Ctrl+L, C,S,C (Career), C,S,T (seTtings) | refuse_orders_not_available_in_tutorials |
| popup `NO INFORMATION ON RECORD-ACCESS INDEX FOR TOPICS` | SIS topic not found | refuse_sis_no_information |
| `Report from Science Console:` / `COMPUTER MALFUNCTION! Planet scan was lost. Attempting rescan.`, row 24 `Please stand by` | system map Database, `Y` to `Display planetary map?` (map still opens afterwards) | refuse_planetary_map_computer_malfunction |
| `UNABLE TO OPEN FILE: sf..inf.scn` (top-left of sign-on screen) | `I` at sign-on (info file missing from this install) | refuse_signon_info_unable_to_open_file |
| silent, nothing happens | shields F8/Shift+F8 while docked; C,S,S Save in tutorial; G,E,* escort commands with no escorts; M,C / M,M with no log entries | - |

## (e) Surprises, blocking prompts, timing

- Blocking prompts that swallow every other key until answered: `Establish orbit?`, `Shall we dock?`, `Answer?` (resupply, landing parties), `Comply with the request?`, `Continue?` (self-destruct, emergency hyperspace), `Access SPD records?`, `Change rest state?`, `Do you want to stop?`, `Do you wish to give up on this mission?`, `PRESS KEY` popups (probe report freezes the game clock too). Esc does not cancel them; they need Y/N (or any key for PRESS KEY).
- The first-orbit `SHORT RANGE SCAN` report blocks with an EMPTY prompt field (row 24 cols 0-23 blank). I pressed Enter, Space, Esc, N in turn before the landing-party question appeared, so I cannot say which key dismissed it or whether `Display planetary map?` was shown and answered by one of them - needs one careful re-run.
- While the Helm report window is printing (after F9, ~5-8 s) Esc presses are dropped; the map only closes once `We have undocked.` has appeared. Same after Enter in the targeter (`ENTERED` shows ~1-2 s).
- After `Y`/`N` to `Establish orbit?` and `Shall we dock?` the menu row shows the Nav submenu, not the top menu.
- The SPD records screen (E,E,D,Y) does not leave on Esc alone: Esc raises `Are you sure you want to return to the bridge?` and needs `Y`. The Commander's Log (Ctrl+L) Esc raises `WARNING! THIS WILL EXIT W/O CHANGES! Do you still want to exit?`.
- Overlays (Ctrl+H, Ctrl+K, Ctrl+I, Ctrl+N, popups) leave the bottom menu row visible, so "menu row shows Nav/Eng/..." does not prove the bridge is accepting menu keys. Ctrl+I needs Esc once per index page plus once for the prompt.
- Esc out of the Ctrl+T command tree lands in the Nav submenu.
- The planetary map (Database -> `Display planetary map?` -> Y) is an EGA graphics screen (INT10 mode 0Eh): the text dump is 25 blank rows (view_planetary_map_ega_graphics_blank_text). Only Esc leaves it (Enter does not). Dumps right after a mode switch take a few seconds.
- Internal Defense (S,D) reports `#vgamode=1` and draws the deck plan with custom characters.
- Practice mission: `P` does not start anything, it toggles the stored "Practice Mode" default; the mission starts with `N`. It persists across runs (I switched it back to Off). Running a tutorial left "Performance Mode" showing `Norm` on the sign-on screen (was `Turbo`); I set it back to Turbo in `C`.
- Mission clock (MET) does not advance while docked, and also stood still while stopped in open space; it ran while moving and in orbit. Ctrl+F freeze gives no visible text change at all (only the clock stops).
- Red alert is `Shift+F8` twice (first press only raises shields to 100%); `Shift+F7` twice cancels (`Cancelling red alert.` / `Shields lowered.` / `Changing alert status to ORANGE`, then GREEN on entering hyperspace).
- TDB on the tactical display moves relative to the ship's heading, not screen-up = north (Left+Down from the ship scanned 59,17 with the ship at 58,17 heading 225).
- In the target-system map of the hyperspace targeter the TDB snaps onto a nearby object (starfort at 8,77 from 8,75).
- Harness: a fast `xdotool key alt+j` did nothing and left the game ignoring all keys until a bare Alt tap; holding Alt ~0.2 s around the letter works for every Alt combo. Writing `/tmp/sfdump.req` non-atomically sometimes makes DOSBox read it empty and write `dump.txt` instead; write to a temp file and `mv` it.
- Side effects left in the game folder by this run: mission sequence number went 020 -> 021, tutorial 1 is now marked `*`, and `AUTOSAVE.021` appears in the Resume list.
