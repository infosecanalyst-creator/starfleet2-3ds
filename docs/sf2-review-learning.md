# Star Fleet II (3DS): learning-experience review

Nov 21 2026. Read-only review of repo /home/claude/sf2-3ds (library-port.txt, refusals.h, sf2ui.cpp, docs/screen-flows.md, tests/expected, tests/screens) and emulator sheets sf2o and sf2m. Nothing was run, built or changed.

Evidence tags: [code] read in sf2ui.cpp, [dump] tests/screens + tests/expected, [shot] emulator sheet, [flow] docs/screen-flows.md.

## 1. The 10 highest-value changes, ranked

### 1. ORANGE and RED alert are not recognised: the bridge loses its buttons and its help exactly when the player needs them
- Where: `bridge_situation()` in sf2ui/sf2ui.cpp (line ~1495); then `bridge_bottom()` action rows, `coach_step()`, `advise()`.
- Evidence: in alert the status box reads `ORANGE` / `ALERT` or `RED` / `ALERT`, with no `STATUS:` word [dump view_orange_alert, view_red_alert]. The function returns SIT_UNKNOWN when `STATUS` is missing. tests/expected/view_red_alert.txt and view_orange_alert.txt confirm the result: buttons `[AUTO] [FREEZE]`, what now = "Follow the prompt line above the buttons...".
- Effect on a new player: (a) first combat gets no "COACH: ENEMY!" card, no ECM button, and SELECT gives the generic fallback; (b) worse, mission 1-A sends the player into "UGA or neutral space", which is ORANGE: ENGINES, STOP and STARFORT vanish from the bridge, and every card says "tap ENGINES" / "tap STARFORT". There is no other touch route to F9.
- Change: read the alert word from rows 15-16 (`RED`, `ORANGE`), not only row 16 after `STATUS:`. Treat ORANGE as cruise (it means "enemy territory, nothing in range"; shields are down on purpose), with an "ORANGE: Alliance space" note. In RED keep ENGINES/STOP reachable (e.g. alert rows = STOP-or-ENGINES and FREEZE; ECM is already in CTL > Hot keys) or add a 7th row. Add expected-file assertions so view_red_alert must show the alert bar and advice.
- Also fix in `advise()`: "Enemy near and shields are down: tap SHLD+" must only fire in RED; in ORANGE shields at 0% are normal.

### 2. First orbit scan report blocks the game and the port points at the wrong button
- Where: `advise()` orbit branch; `bottom_rows()` (no CONTINUE on an empty prompt); library cards "Orbit a planet" step 3-4 and "Tutorial 1, step by step" step 28.
- Evidence: the SHORT RANGE SCAN report waits with an EMPTY prompt [flow (e)]; tests/expected/tut1_50_short_range_scan_blocking.txt shows hint "Tap display: steer / X/Y: fire", no CONTINUE button, and what now = "You are in orbit. TELEPORT sends a landing party..." ringing TELEPORT. Which key continues, and whether "Display planetary map?" is asked, is still unconfirmed [plan, Open].
- Why it matters: this is the pay-off moment of mission 1-A (the scan is the discovery). The player sees a report, nothing responds, and help says TELEPORT.
- Change: detect `SHORT RANGE SCAN` in COMMS with a blank prompt: show CONTINUE (ringed), hint "Scan report: read it, then CONTINUE", and a card that reads the result out ("Class K, fair minerals: this is what your orders ask for / not what they ask for"). Do the one careful harness re-run first; then make the Orbit card and Tutorial step 28 say exactly what was seen.

### 3. Every signpost sends a newcomer to Practice, but only Tutorial 1 is guided
- Where: COACH_TEXT[CS_MENU] and COACH_BUTTON[CS_MENU]; `advise()` SIGN-ON branch; hint "New? Choose Practice first"; library card "Your first 10 minutes".
- Evidence: Practice has 7 dismissals before the player can act (orders, checklist, motto, three XO pages, Comply?, Log) [flow (b)], then no checklist: after LAUNCH the advisor only says "Choose where to go". The practice system SS-0 holds one gas giant [shot sf2o d1sysmap], so "fly to a planet" teaches little and cannot meet the goal. Tutorial 1 starts on the bridge and has the self-ticking list.
- Change (pick one, the first is cheaper):
  a) Sign-on coach rings Tutorials and says "Start with Tutorial 1: a checklist walks you through it (about N minutes)". Practice becomes step two.
  b) Or keep Practice first, but turn "Your first 10 minutes" into STEP lines and auto-start it when a Practice mission's first bridge appears (same mechanism as `tut_num == 1`).
- Either way add the missing checklist "Your first mission (1-A)": STARS > Probes > Auto_tg; wait for probe reports; pick a system with planets; jump; orbit each planet; read the scan; ORDERS shows COMPLETED; STARFORT; dock. Today "How a mission flows" is prose only and nothing says how to *find* a mineral-rich planet.

### 4. The Practice coach ring becomes a trap after one use
- Where: COACH_BUTTON[CS_MENU], `advise()` SIGN-ON branch, hint line 732.
- Evidence: "Practice (demo) mission" only toggles a mode [flow (b) 5]; back at sign-on with `Practice Mode On` the ring is still on Practice [shot sf2o a4menu]. Tapping it again asks "switch to Career mode?" and undoes the choice.
- Change: if the settings block shows `Practice Mode ... On`, ring "Start a new mission" and say "Practice is on. Tap Start a new mission." Also fix the two pages in between: tests/expected/prac_03_missions_record.txt gives what now "Read the page, then tap CONTINUE" but the buttons are SELECT / BACK / VIEW; say "Mission 1-A is highlighted: tap the SELECT button below."

### 5. The "leave the starfort" coach card fires during the XO's welcome and replaces the mission card
- Where: mission card block (line ~2708) and coach block (line ~2780) in `render`.
- Evidence [code]: `mis_show_pending` is cleared at 2709, then the coach test `!mis_show_pending` at 2783 passes in the same frame, so `explain(COACH: LEAVE THE STARFORT)` overwrites the mission card before it is drawn. The first bridge screen is the XO page with `PRESS KEY` [flow (b) 9], so the card says "Tap LAUNCH" while only CONTINUE works, and its 9 s are spent before LAUNCH is usable.
- Change: hold CS_DOCKED until the prompt is `Select option` (no PRESS KEY, no `?`, Log closed). Show the mission card first and let it stay until dismissed; then the coach card. On the XO pages show "Your officers are reporting in: CONTINUE (3 pages)".

### 6. Help cards cover the proof the step asks the player to look at
- Where: `chk_card()` and `explain_card()` (both anchor to the bottom of the top screen).
- Evidence [shot sf2m n_navmap, o_star, g_sysmap; sf2o d1sysmap]: on map views the card sits on the TARGET box (Object / ID/class / Distance / Bearing) while the large panel above shows only "Map on the touch screen". Stars are not labelled on the touch map, so "Tap star SS-33" can only be checked in the covered box. On the bridge the card covers Time left and s:/c:, which "Ship won't move" and the Time card tell the player to read.
- Change: on map views draw cards inside the empty "Map on the touch screen" panel. On the bridge keep the VEL s:/c: and Time left cells visible (shorter card, or put the 1-line form in the hint strip).

### 7. The Tutorial 1 checklist cannot tell it has gone off track, and has no way back
- Where: `chk_step_done()` / `chk_update()`; "Tutorial 1, step by step" in assets/library-port.txt; `advise()` checklist branch.
- Evidence [shot sf2m sheet03-04]: at step 21 the run was on the SYSTEM map, not STARS. Step 21's proof `Max. Vel` also appears in the sublight Targeter (`Max. Vel : 0.9`), so it ticked; step 22 then says "Press A to accept hyperspace speed 8" over a box showing Velocity 0.9; the list stays at 22/32 with a sublight target `38,5` loaded. SELECT only repeats the step text. Steps 6, 20, 22 tick on any B or A press.
- Change:
  - Proofs that cannot be met on the wrong screen: step 21 `Velocity: 8` (or `Max. Vel :  8`), step 18 `NAV STAR MAP`, step 29 `WAR STAR MAP`.
  - One action per step. Split steps 14, 19, 26, 27, 30 (each hides 3-4 actions behind one ring; step 14 rings SYSTEM while the player is already on it).
  - When a checklist is active, SELECT = the step plus the live situation line ("You are on the SYSTEM map; this step needs STARS") and two touch buttons: BACK ONE STEP, SKIP STEP (the "manual tick" the design promised).
  - Split into three lists with a success card each: A "Fly to a planet and orbit" (1-17), B "Probe and jump to another star" (18-28), C "Come home" (29-32). 32 steps at 17-50% game speed is too long for one sitting.

### 8. Refusal and advice texts that assume an enemy or a state that is not there
- Where: sf2ui/refusals.h; hint strip (line ~1027); GLOSSARY.
- Evidence: "Phasers not targeted" / "Torpedoes not targeted" print in any alert state with nothing around [flow (d)]. The bridge hint always advertises "X/Y fire", so a curious newcomer presses X at Altern rank (no enemy warships) and is told "move closer until the enemy is on the tactical display".
- Change: lead both cards with "Nothing to shoot at: no enemy in range." Drop "X/Y fire" from the hint unless the status is RED. "Phasers not energized" says "tap SHLD+ until shields are at maximum, then tap it again": SHLD+ is 5% a tap (20 taps) [code K_F8]; say "With AAS on, the ship goes to red alert by itself when an enemy appears" and give a one-tap RED ALERT (Shift+F8 twice [flow (e)]).

### 9. No feedback on success
- Where: `chk_update()` ("Checklist done", 3 s toast); `advise()` goal line; end of Tutorial 1.
- Gaps: objective flips to COMPLETED silently (the advisor just drops "(not done yet)"); Tutorial 1 ends with a toast and no next step; first orbit, first jump and first dock pass without comment.
- Change: a card that stays until dismissed: "TUTORIAL 1 DONE. You can fly, orbit, probe, jump and dock. Next: Practice mission 1-A (START > MANUAL > Your first mission). To leave: CMP > Special services > Quit > YES, then NO to keep it." And "OBJECTIVE COMPLETE: head home, tap STARFORT" when ORDERS/COMMS shows it. One line on first orbit/jump/dock ("In orbit. This is where scans and landing parties happen.").

### 10. Timed cards and a hidden mode switch
- Where: `explain()` timeouts (7-15 s); R handling (line ~3124-3140); A_MODE.
- Evidence: coach 9 s, refusal 10 s, mission 12 s, what now 15 s, for 30-45 words in a 6x12 font while the game runs. A short R press (under 0.6 s, with no tap) silently cycles the tap mode AUTO > STEER > TARGET [code]; a newcomer told "hold R" who lets go early ends up in TARGET mode, and "Tap the tactical display to turn the ship" stops working. The mode button is only drawn in the unknown-situation layout.
- Change: cards stay until B / a tap on the card / the situation changes; SELECT twice re-shows the last card. Take the mode cycle off the R tap (put it in START > settings) or at least keep the toast up 4 s and name how to undo it.

## 2. Quick wording fixes (old -> new)

assets/library-port.txt
- "Your first 10 minutes" step 1: "tap SELECT on the mission chart" -> "tap the SELECT button under the mission chart (mission 1-A is already highlighted)". (SELECT is also the hardware "What now?" button, named two sentences later.)
- "Your first 10 minutes" steps 3-6 order: LAUNCH, then SYSTEM, target, "tap ENGINES" -> "SYSTEM, tap the planet, TARGETER, A, B, then LAUNCH". After LAUNCH the ship is moving and the button reads STOP, not ENGINES [shot sf2o e1bridge]; Tutorial 1 itself targets first, then launches. Same order in COACH_TEXT[CS_DOCKED] and the docked branch of `advise()`.
- "Saving your game": "answer NO when asked to delete the mission files" -> "answer YES to 'Do you want to stop?', then NO to 'Do you wish to give up on this mission?' (YES there throws the mission away)" [flow 41-43]. Add: "Save does nothing inside a tutorial" [flow (d)].
- "Time: days left": "The clock runs while the ship moves, orbits, resupplies in dock or rests" -> "The clock runs while the ship moves or orbits. It stood still while docked and while stopped in open space" [flow (e)]; mark "rests" unconfirmed.
- "Freeze": "The mission time flashes white: the game is frozen" -> unconfirmed; the harness saw "no visible text change at all (only the clock stops)" [flow (e)]. Better: the port shows its own FROZEN label (the FREEZE button reads RESUME) and the card says that.
- "Tutorial 1" step 19: "Tap the star above yours, PROBES, then LAUNCH" -> three steps: "Tap the star just above the pink box" / "Tap PROBES, then LAUNCH CONTROL" / "Tap LAUNCH". (Buttons read LAUNCH CONTROL and LAUNCH [shot sf2m p_probes, q_launchctl]; LAUNCH is also the bridge button.)
- Step 20: "Press A to launch the probe" -> "Tap LAUNCH (or A). Then B to get the map buttons back".
- Step 21: "Tap star SS-33 (up and left), then TARGETER" -> "Tap the star one left, one up; the TARGET box must read SS-33" / "Tap TARGETER".
- Step 23: "Its map opens: tap near an edge, press A" -> "SS-33's map opens. Tap any empty spot on its bottom edge: that is where you arrive. Press A".
- Step 25: "The ship runs to the edge, jumps and arrives" -> "Nothing to do for a few minutes: the ship flies to the edge, jumps, and drops out at SS-33".
- Step 28: "Read the scan, press A; landing parties: NO" -> fix after the harness re-run; add the planetary-map question if it is asked.
- Steps 29-31 (WAR, tap starfort, TARGETER, A, tap starfort, A, B, ENGINES) -> "Tap STARFORT, answer YES" / "Wait: the ship flies home". Mention the WAR map in WHY.
- Step 32: "Answer YES to dock. Tutorial 1 is done" -> "Answer YES to dock" (the success card says the rest).
- "Tutorial 1" IF IT DOESN'T WORK: "To start again, leave with START and pick Tutorials, 1" -> "To start again: CMP, Special services, Quit, YES, then NO; at sign-on tap Tutorials, 1".
- "Travel to a planet" step 3: "Tap Targeter in the menu row" -> "Tap TARGETER"; step 4: "Circle Pad up or down changes it" -> "SPD- / SPD+ change it" (those are the buttons shown [shot sf2o d3targeter]).
- "Jump to another star system" step 9 proof `HYPRSPCE` is fine; step 4 proof -> `Select target` stays, step 3 proof -> `Max. Vel :  8`.
- "Red alert: fight a ship" step 2: "If AAS is on manual, raise the shields yourself" -> "Shields not at 100%? Tap SHLD+" (AAS is unexplained at this point).
- "Velocity (VEL)": "sublight speed, 0.0 to 1.0" -> "0.0 to 0.9" (the same card later says top speed is 0.9; the Targeter shows Max. Vel 0.9).
- "STATUS" IF IT DOESN'T WORK (condition YELLOW footnote) -> cut.

sf2ui/sf2ui.cpp, GLOSSARY
- SPD+: "(up to 1.0)" -> "(up to 0.9)".
- SHLD+: "Maximum twice in a row = red alert." -> "At 100%, one more tap calls red alert."
- SHLD-: "Lowest twice in a row cancels red alert." -> "At 0%, one more tap cancels red alert."
- PHASERS: "(raise shields to the top twice for red alert). Ready when the P light in DAMAGE is green." -> "They fire only at red alert, at an enemy within 5 points. Ready when the P under DAMAGE is green."
- ECM ON: "jam enemy targeting so they miss more often" -> "jams weapon locks, theirs AND yours: switch it off before you fire". Same fix in COACH_TEXT[CS_ALERT] ("ECM jams their aim").
- WAR: "Who owns which star systems..." -> add "This is the map that shows starforts."
- SCI: "Scans, sensor probes and Department Q." -> "Scanners, Medical (crew rest and morale), Department Q." (Probes are launched from the star maps [flow (c)].)
- SEC: "Internal security, prisoners, boarding parties." -> "Security status, defence inside your ship, interrogating captives." (Boarding parties are under ENG > Teleporters.)
- CMP: add "Save and Quit are here: Special services."
- CTL: "The ship's switches: ..." -> "Hot keys (the ship's switches: AAS, CMF, rest, ECM) and the command tree."
- STOP: "The ship coasts to a halt." -> "The ship stops at once."

COACH_TEXT, hints, advisor
- CS_SYSMAP "Tap one to aim at it, tap it again to pick it." and the map hint "Tap a star or ship to aim, tap it again to select" -> "Tap a planet or starfort: the box jumps to it. Then tap TARGETER." (Cards, advisor and checklist all say TARGETER; "star or ship" is wrong on the system map.)
- CS_MENU: 45 words, four ideas -> "New here? Tap Tutorials, then 1: a checklist walks you through your first flight. Stuck anywhere: press SELECT."
- CS_CRUISE: drop "STARFORT flies you home" (too early); keep steer / speed / SYSTEM.
- `advise()` default: "Read the page, then tap CONTINUE (or A)" on screens with no CONTINUE (missions record, Tutorials menu, "give up on this mission?") -> screen-specific lines; for the give-up page: "NO keeps this mission for later. YES throws it away."
- `advise()` "Answer?" + landing parties: add "NO is fine on a first visit; YES opens the landing-party table."
- `advise()` orbit: "TELEPORT sends a landing party down to explore." -> add "Not on a gas giant (Cls G)."
- Quick reference (non-flight): "ZR: move a ring over the buttons with the D-Pad" while ZR is hyperspace speed on the bridge and the START footer says "ZL Swap" -> say "ZR (menus only)".
- Checklist advice: "(START > MANUAL has the whole list.)" fine; the menu says MANUAL while the cards say "Library": pick one word.

sf2ui/refusals.h
- "Phasers not targeted" / "Torpedoes not targeted": "HOW: move closer until the enemy is on the tactical display..." -> "No enemy in range, so nothing to aim at. In a fight: get within 5 points (torpedoes 10)."
- "Phasers not energized": see change 8.
- "Velocity must be set": "(or LAUNCH / ENGINES, which set a speed for you)" -> "Tap ENGINES: it sets speed 0.5 and starts."
- "No valid target within range" button SYSTEM -> none (SYSTEM does not help board a ship).

## 3. What to cut
- Tutorial 1 steps 29-31 (manual WAR-map route home): replace with STARFORT.
- Tutorial 1 step 4 as a required step (0.5 instead of 0.9 teaches nothing; keep as an optional line "SPD- / SPD+ change the speed").
- "X/Y fire" from the bridge hint outside RED.
- The stale "Goal:" line in tutorials: `mis_goal` is saved in settings and shown in Tutorial 1's what-now and hint strip ("Discover at least one mineral-rich planet" [expected tut1_02]); clear it at sign-on or when `tut_num > 0`.
- From the START shelf for a first-timer: "Ranks and your career" and "Reading the top screen" can move to REFERENCE / BRIDGE; START should be: What this game is, Your first flight (Tutorial 1), Your first mission, Freeze, Saving, Time.
- BRIDGE-shelf cards for things an Altern never meets in the first hours (supply ship / sideslip / tractor lights, EDC, TARGET DATA): keep, but sort below the eight panels; the Library opened on "Supply ship, sideslip and tractor lights" in the sf2o run.
- PROCEDURES at Altern rank: Escorts, Bombard, Invade, Capture show a rank note or sort last ("from Centurion").
- Coach text "START > COACH switches me off" from the first card (not needed in the first minute).
- The YELLOW footnote in "STATUS".

## 4. What I could not judge from the material
- Anything in combat: no recorded fight exists (the RED dump is a manual red alert with no enemy). Target lock, TARGET DATA, weapon lights, the "Red alert" checklist and the 52 unconfirmed refusal cards are unverified.
- Landing parties, Tutorials 2-8, escorts, capture, bombard, invasion: card text only, no screens.
- What mission 1-A accepts as "mineral-rich" and whether a scan alone completes it or a survey party is needed; how COMPLETED appears.
- Whether a target entered while already under way is flown without pressing ENGINES again (decides the wording of "Travel to a planet" when moving).
- Whether "tap it again to select" does anything on the maps.
- What Freeze looks like on the 3DS (colour/blink is not in the text dumps).
- Real pacing: flight time per leg on hardware (emulator 17-95% of real time), so whether Tutorial 1 is 15 or 45 minutes.
- Sound, click feedback, font legibility of the 5x9 fallback on the real screen, and whether card text is readable at arm's length.
- Tutorial 1 steps 21-32 by touch: the only touch run (sf2m) left the track at step 21, so nothing after that has been seen on the 3DS build.
- Help fading (learned-state file) and "Terms on this page" are not built, so not reviewed.
