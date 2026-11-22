# Star Fleet II (3DS): interface review before release

Read-only review, Nov 21 2026. Basis: about 70 of the 221 screen renders (`tests/out`, every kind in `tests/expected` covered at least once), emulator sheets n2/sheet01-03 and sf2m/sheet03, `sf2ui.cpp` layout code, `sf2-plan.md`. Nothing was run on the emulator, PC or 3DS; no code changed.

Overall: the bridge, the three maps, the sign-on menus, the manual and the START menu are in good shape. The weak part is everything the game opens as a window over the bridge (combat control, damage control, log, notepad, help, landing-party table): those still fall back to a squashed 80x25 screen on top and a generic arrow pad below. Three prompts have missing or wrong answer buttons, and the alert action bar never appears.

## 1. The 12 highest-value changes, ranked

1. **Prompts with missing or wrong answer buttons** (blocks play).
   - `tut1_50_short_range_scan_blocking`: the game asks "Display planetary map?" inside the display frame (row 13). The touch screen shows neither the question nor YES/NO; the row offers SYSTEM MAP / NAV MAP / WAR MAP / HELM. This is the "scan report waits with an empty prompt" item in the plan.
   - `view_system_map_database_planet_map_prompt`: same question on the system map. The touch screen zooms into a fragment of the dialog ("etary map?"), with no YES/NO, and the top-screen card covers half the map.
   - `menu_eng_teleport_landing_party_in_orbit`: YES/NO are offered for a stale COMMS line ("Do you wish to be resupplied?") while the game is actually showing the landing-party mission table (eXplore/survey, Establish base ... Activate). The real choices have no buttons.
   - Change: detect a question in any framed box (not only row 24 and red/grey dialogs), show it in the prompt strip and give YES/NO. Ignore question marks inside the COMMS panel when a window is open. Give the mission table its own list buttons plus ACTIVATE.

2. **Red and orange alert are not recognised** (`view_red_alert`, `view_orange_alert`, `view_frozen`). The game replaces "STATUS:" with "RED / ALERT", so `bridge_situation()` returns unknown and the column falls back to AUTO / FREEZE. The alert bar (ECM ON/OFF) never shows in the one situation it was built for. Change: read rows 15-16 for "ALERT"; in alert show SHIELDS MAX, ECM ON/OFF, EVASIVE, FREEZE; show UNFREEZE while frozen.

3. **Game windows still get the squashed screen plus arrow pad.** `view_combat_control`, `view_combat_control_weapons_menu`, `menu_eng_damage_control`, `view_log_entry_mode`, `log_entry`, `view_notepad`, `view_help_hot_keys`, `view_sis_index`, `menu_eng_power_schematics`, `tut1_33_probe_report_popup`, `prac_03_missions_record`, `signon_commander_list`. The whole 80x25 game sits on the top screen in the 5x9 font and the touch screen is a 3x3 pad of PG UP / arrows / SPACE. Change, in order of value: combat control (grid on the touch screen with tap-to-target, its MAIN MENU and WEAPONS keys as buttons, target data on top); damage control and log (window text in the reader at 6x12); commander list and missions record (rows and cells as tappable buttons); hot keys and SIS index (scrolling list of tappable rows).

4. **Buttons that cannot be used are not greyed on the bridge.** Docked (`c1`, `tut1_02`): PHASERS, TORPS, SPD+, SPD- are live. Cruise with no target: PHASERS, TORPS live. `menu_nav_helm`: ORBIT away from a planet, DOCK away from a starfort. `menu_eng_teleporters`: LANDING PRTY when not in orbit. The `refuse_*` screens already prove what the game refuses. Change: grey from the same facts the refusal cards use (docked, velocity 0, no target, not in orbit, not near starfort).

5. **Department row is cryptic and paged** (every bridge screen): NAV ENG CMB CMP SEC, then MORE for COM STG SCI CTL, in 28-px buttons with a red hotkey letter. Change: one DEPARTMENTS button (or tapping the row) opens a full-touch-screen 3x3 grid with whole words: NAVIGATION, ENGINEERING, COMBAT, COMPUTER, SECURITY, COMMS, STRATEGY, SCIENCE, CONTROLS. No MORE. Same for sub-menus that page (DAM CON / TELEPRT / TRAC BM, CMBT STATS / INFORMATN, LANDING PRTY / BOARDNG PRTY, RNDVZING): they fit as full words in a grid.

6. **Stylus does too little outside the maps.** Tactical display: a tap only steers (plan item 2 not started). Orbital display: tapping the planet does nothing (should offer SCAN / LANDING PARTY / RESUPPLY). `menu_cmp_info_planets`, `menu_stg_system_space_forces`, `view_fleet`: rows are text, not tappable; selection needs the arrow buttons. `menu_sci_scanners_select_target`: the tactical crop is shown at half size, centred, with wide empty margins. Change: tap an object or a list row to select it, tap again (or an action strip) to act; let select-target views fill the touch screen.

7. **Tables are unreadable in the reader.** `menu_cmp_info_planets`, `menu_stg_system_space_forces`, `view_signon_default_settings`, `menu_stg_invmgt`: 80-column tables are wrapped at 50 columns, so each row breaks into two lines and the header no longer lines up; the default-settings page interleaves its two columns into nonsense. `menu_cmp_info_star_systems` turns table cells into buttons labelled "5 B ■ ?" / "2 O ■ 1". Change: a table mode: column header fixed on the top screen (or frozen at the top), one row per 24-px line, sideways drag for extra columns, row tap selects. Star systems: buttons that say "SS-24  (24,1)  class A  Krellan".

8. **Top screen is wasted on reader and map screens.** Reader pages (`b01`-`b08`, `view_orders`, `view_fleet`, all text pages): the top screen shows only "Reading on the touch screen / Lines 1-11 of 36" and a bar. Maps (`tut1_04`, `tut1_29`, `view_war_map`): over half the top screen is an empty tile saying "Map on the touch screen", while the useful TARGET / NAVIGATION panels beside it are in the 5x9 font. Change: reader: show the mission card (goal, days left) and "terms on this page"; for orders, a summary. Maps: use the empty tile for the target's details and a symbol legend at 6x12, and enlarge TARGET / NAVIGATION.

9. **Cards cover needed information and cut the hint strip.** n2 `c2whatnow`, `c4explain`, `d1sysmap`, `d5whatnow`; sf2m `r3_back`, `u_targetmap`: WHAT NOW, COACH and the tutorial checklist sit over ENERGY, LRS and DATA (fuel, shields, supplies) and over the Targeter's key line; the hint strip shows as a sliced row of half letters under the card. Change: on maps put the card in the empty map tile; on the bridge let it replace the hint strip cleanly (card bottom at y 240) and cap it at 3 lines; never draw a partial strip.

10. **Same word, different jobs; same job, different places.**
    - Docked bridge has two ORDERS buttons (action column and view tab, both Ctrl+O).
    - `menu_stg`: FLEET and SYSTEM in the menu row directly above FLEET and SYSTEM tabs that do something else. `menu_nav`: SYSTEM MAP / WAR MAP above SYSTEM / WAR.
    - ENGINES is in row 5 when cruising but row 6 in orbit; STOP replaces it in place.
    - Leave is BACK, EXIT or NO depending on screen; go on is ENTER, CONTINUE, SELECT or ACCEPT.
    - Change: docked column = LAUNCH + RESUPPLY (or REPAIR), not ORDERS. Rename tabs or menu items so no label appears twice (e.g. tabs STAR MAP / SYS MAP / WAR MAP). Fix ENGINES/STOP to one row. BACK always bottom-row right; one word per meaning.

11. **Targeter and Probes windows are duplicated, tiny and clipped on the touch screen.** `tut1_31_probe_display` (Probes Control drawn over the map at about 4-5 px text, 4 of 5 probes visible, right column cut), `tut1_35_hyper_targeter` (Targeter drawn tiny over the star map while the top screen shows it large). Breaks "never duplicated" and "no shrunk text". Change: draw the window only on the top screen; on the touch screen show the map with the target box plus a readout line ("Speed 8.0  Time 0.11 days") above SPD- / SPD+ / ENTER / EXIT; give each probe a button.

12. **Legibility and colour-only signals.**
    - Every non-bridge top screen uses the 5x9 font for 80 columns (SF-04, still unverified on hardware); combat control and help are the worst.
    - NAV panel Position value is drawn dark on grey and cannot be read (n2 `e1bridge`, `e3whatnow`, `e7end`).
    - Status lamps, ECM/SPD/RST/CMF/AAS switches, DAMAGE panel and the Pa/Ta/Ta row signal only by background colour; the lamp test looks like a real warning (`c1` shows MINES lit while docked).
    - Red hotkey letters on dark-blue buttons (NAV, SYSTEM MAP, YES/NO in `log_entry`) are low contrast.
    - SHLD+ / SHLD- / SPD+ / SPD- are 34 px wide in the 6x12 font with no value shown.
    - Change: lit lamps get a bright border and bold text, plus the existing plain-word alert; fix the Position attribute mapping; drop the red letter on touch buttons; make shields and speed two labelled steppers ("SHIELDS 0% - +", "SPEED 0.5 - +").

## 2. Verdict by screen kind

| Screen kind | Examples | Verdict | Why |
|---|---|---|---|
| Title / credits | `title_credits` | acceptable | Readable; top screen nearly empty; arrow row not needed |
| Name and password entry | `t05`, `signon_password_after_list` | good | Full keyboard, 24-px keys, SIGN ON AS one tap |
| Commander list | `signon_commander_list` | needs work | Squashed list on top, arrow pad below; rows not tappable |
| Sign-on options / tutorials menu | `view_signon_options_menu`, `tut1_00` | good | Big full-word buttons, scroll arrows, greyed items |
| Default settings | `view_signon_default_settings` | needs work | Two-column page linearised into unreadable text |
| Service record / skills / legions | `view_signon_service_record` | acceptable | Readable; cryptic lone "TO" button on skills page |
| Missions record | `prac_03_missions_record` | needs work | Arrow pad; grid cells not tappable; colour-only legend |
| Orders / intel / reader pages | `b01`-`b08`, `view_orders` | acceptable | Text readable at 6x12; only 11 lines; top screen wasted |
| Bridge docked | `c1`, `tut1_02` | acceptable | Clear layout; weapons and speed not greyed; two ORDERS |
| Bridge cruise | `bridge_cruising_green` | good | STOP / STARFORT sensible; tap steers |
| Bridge alert | `view_red_alert`, `view_orange_alert` | needs work | Alert not recognised; no ECM / shields / evasive |
| Bridge orbit | `tut1_28`, `tut1_25` | acceptable | Recognised; planet not tappable; TELEPORT live always |
| Bridge hyperspace | `tut1_43` | acceptable | Works; Position blank; STARFORT offered mid-jump |
| Bridge department menus | `menu_nav`, `menu_eng`, `menu_stg` | needs work | Abbreviations, MORE paging, labels clash with tabs |
| Bridge YES/NO prompts | `tut1_17`, `prac_15`, `tut1_58` | good | Question in the strip, two large buttons |
| Planetary-map prompt | `tut1_50`, `view_system_map_database_planet_map_prompt` | needs work | Question hidden, no YES/NO |
| System map | `tut1_04`, `tut1_05` | good | Fills the touch screen, tap to aim and select |
| Star (nav) map | `tut1_29`, `m0` | good | Same; small stars but the target box snaps |
| War map | `view_war_map`, `v_y` | good | Same layout; full-word buttons |
| Targeter | `tut1_06`, `tut1_35` | acceptable | Good buttons; window duplicated tiny on the hyper targeter |
| Targets list | `view_system_map_targets` | acceptable | DELETE / SWAP / RESET / ACCEPT clear; list only on top in 5x9 |
| Probes | `tut1_31`, `menu_navmap_probes` | needs work | Window tiny and clipped on the touch screen; no per-probe buttons |
| Probe report / message boxes | `tut1_33` | needs work | Arrow pad for a one-button message; text small on top |
| Target system map (jump) | `tut1_37` | acceptable | Works; top-screen card overlaps the star map |
| Combat control | `view_combat_control`, `..._weapons_menu` | needs work | Game on top in 5x9, arrow pad on touch |
| Combat status | `menu_cmp_cmbt_stats_enemy` | needs work | Table shrunk to tiny text, treated as a map |
| Fleet display and menu | `view_fleet`, `view_fleet_menu` | acceptable | Readable; fleet names vanish under the MENU popup; rows not tappable |
| Databases (planets, star systems, forces) | `menu_cmp_info_planets`, `..._star_systems` | needs work | Wrapped tables; "5 B ■ ?" buttons |
| Damage control / power schematic | `menu_eng_damage_control` | needs work | Squashed top, arrow pad |
| Log / log entry / notepad | `view_log_entry_mode`, `view_notepad` | needs work | Squashed top; pad or keyboard with no view of what is typed nearby |
| Landing-party table | `menu_eng_teleport_landing_party_in_orbit` | needs work | Wrong YES/NO; table has no buttons |
| Scanner select target | `menu_sci_scanners_select_target` | acceptable | Works; display half size with empty margins |
| Help: function keys / hot keys / SIS | `view_help_function_keys`, `view_help_hot_keys` | needs work | Treated as a map (random crop) or arrow pad; false "ALERT: =STATUS" |
| START menu | n2 `e2start` | good | Clear, greyed items explained |
| Exit confirm | code only | good | Plain wording, two large buttons |
| Manual / Library | `sm1`, n2 `e4library` | good | Shelves, list, checklist follow; controls match the standard |
| What now / coach / explain cards | n2 `c2`, `c4`, `d5` | acceptable | Useful text; covers gauges and cuts the hint strip |
| Graphics-mode screen | `view_planetary_map_ega_graphics_blank_text` | acceptable | Only CONTINUE / BACK / MANUAL; real map not judged |

## 3. Rule breaks found

- **Every key the game asks for is a button**: broken on the planetary-map question (2 screens) and the landing-party table; wrong buttons offered on the latter (change 1).
- **Touch screen is the main game screen**: broken on about 12 window screens where the game is on the top screen and the touch screen is an arrow pad (change 3).
- **Never duplicated**: Targeter and Probes windows appear on both screens (change 11).
- **No text cut off or shrunk below readable size**: Probes window clipped and tiny on touch; combat status table tiny; 5x9 for all 80-column top screens; hint strip sliced under cards; fleet names hidden under the MENU popup; `view_help_function_keys` cropped mid-word.
- **Unusable buttons greyed**: not done for the bridge action column and helm/teleporter items (change 4). The view tabs are greyed on fleet, database and force pages even though a mission is running (the test is "LOW PWR on row 0"), so they look unavailable when they would work.
- **One consistent place**: ENGINES moves rows; BACK/EXIT/NO and ENTER/CONTINUE/SELECT vary; duplicate ORDERS, FLEET, SYSTEM labels (change 10).
- **No side bars**: `menu_sci_scanners_select_target` and `menu_cmp_cmbt_stats_enemy` leave wide empty margins on the touch screen.
- **Touch targets**: all buttons checked are 24 px tall. The narrowest are the 28-px department buttons and the 34-px SHLD/SPD pairs; legal, but the tightest spots in the port.
- **Manual controls**: code closes on B or START and the hint matches. LEFT HAND mode changes them (ABXY sections, D-Down close): fine if that is the approved exception, otherwise a break of "fixed controls".

## 4. Not judged

- Anything on real hardware: 5x9 and 6x12 legibility on the actual panels, resistive-touch accuracy on 24- and 28-px targets, click volume, press animation, speed.
- Real combat: no dump has an enemy in range, so targeting, firing, damage and the alert bar in use are unseen.
- The EGA planetary map itself (only its blank text layer is in the corpus).
- Manual scrolling behaviour in motion (stick hold-to-scroll, D-Pad section moves, sticks not moving the game): read in code only.
- ZL swap layouts (`bridge_top_narrow`) and left-hand mode layouts.
- About 150 of the 221 renders were not opened one by one; they were grouped by kind and button set from `tests/expected`, and one or more of each group was viewed.
- Emulator runs other than n2 and sf2m (sf2o, d2, sf2k, sf2l, sf2f were not opened; d2 and n2 share a script).
