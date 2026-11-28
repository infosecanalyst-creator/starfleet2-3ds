# Star Fleet II 3DS: cold-read play test (new player, touch + buttons only)

Harness: /tmp/sfw2, screenshots in /tmp/h2/shots/ (c* = Tutorial 1, d* = Practice 1-A, e* = manual, sb*/sc* = STEP BACK captures).
Keyboard injection was used once, only to diagnose the blocker in finding 1 (Escape did nothing either); recovery was ./tosignon.sh.
Tester note: my first few taps on the SYSTEM tab used picture x instead of touch x; those results were discarded and are not findings.

## Findings, most severe first

### 1. BLOCKER: Tutorial 1 cannot be left by touch once it is finished
- Where: bridge, docked, right after Tutorial 1C step 4/4.
- Screen said (WELL DONE card): "TUTORIAL 1 DONE. You can fly, orbit, probe, jump and dock. Next: Practice mission 1-A (START > MANUAL > Your first mission). To leave: SHIP MENUS, COMPUTER, Special services, Quit, YES, then NO."
- What happened: there is no SHIP MENUS button on that bridge (row shows SYSTEM MAP, NAV MAP, WAR MAP, HELM). After docking, STARS, HELM, ORDERS and Y give "THE GAME SAYS / NOT AVAILABLE IN TUTORIALS. Tutorials switch this off; it works in a mission or in Practice."; SYSTEM, WAR, COMBAT, FLEET, LOG, SYSTEM MAP, NAV MAP, WAR MAP, LAUNCH, B and X do nothing at all, with no message. START menu: EXIT GAME and GAME SETTINGS are greyed. SELECT gives the generic "1) Pick where to go first: tap SYSTEM, tap a planet, TARGETER, ENTER, then B. 2) Then LAUNCH..." which no longer works either. The card with the leave recipe disappears on the first tap and nothing repeats it. A keyboard Escape did nothing as well, so I could not tell whether the game itself is locked at this point or only the port's route is missing.
- Shots: c094, c095, c096, c097-c112, c103 (START menu), c113.
- Fix: when a tutorial checklist chain ends, show a touch button that leaves (e.g. a ringed "LEAVE TUTORIAL" in place of LAUNCH, and enable START > EXIT GAME inside tutorials) that performs the quit sequence itself. Card text: "TUTORIAL 1 DONE. Tap LEAVE TUTORIAL (or START > EXIT GAME). Then at sign-on: Practice (demo) mission." Keep that text as the SELECT answer until the player has left.

### 2. WRONG: the checklist jumps 16-17 steps ahead whenever a probe is launched or a star target is accepted
- Where: Tutorial 1B and the "Your first mission (1-A)" checklist. Reproduced 4 times.
- Screen said / what happened:
  - 1B step 5/27 "Tap LAUNCH (or A): the probe goes [LAUNCH]": one tap -> 22/27 "Press B for the bridge [B] / Tap ENGINES: the ship flies there" (c058 -> c059).
  - 1B step 11/27 "Tap ENTER to accept speed 8 [ENTER]": tap -> 22/27 while the game shows "MAP OF STAR SYSTEM SS-33" / "Select target" (c067 -> c068). The real next step ("Tap an empty spot on the bottom edge") is never shown.
  - 1-A step 7/35 "Tap LAUNCH: a probe flies to that star [LAUNCH]" -> 24/35 (d028 -> d029).
  - 1-A step 13/35 "Tap ENTER to accept the speed [ENTER]" -> 24/35 (d045 -> d046).
- Why a newcomer is lost: following the card literally in 1B (B, B, ENGINES) the ship jumps to SS-33 with no planet targeted and stops at the system edge; the card then says "Establish orbit? Answer YES [YES]" but no question is asked and no YES button exists (c069-c075). SELECT only repeats it. The only way back is START > STEP BACK, one step per START+tap (I needed 15-16 of them each time).
- Cause (read in the source afterwards, /home/claude/sf2-3ds/sf2ui/sf2ui.cpp lines 1433-1442 and library_data.h): the "off the track but further on" rule moves the list to any later step whose proof text newly appears and is unique in the list. In 1B and 1-A the step "Tap ENTER | ENTERED" (1B step 21, 1-A step 23) is the only one with proof "ENTERED", and that word evidently also appears on screen after a probe launch and after accepting a star target. In 1A the same proof occurs twice, so 1A is immune.
- Fix: give those steps a proof that only the planet target produces, or drop the far-jump rule for proofs as short as "ENTERED"; at minimum never far-jump while a step with a ringed button is pending and was just pressed. Add a "back to step N" or "restart this list" entry so recovery is not 16 menu round trips.

### 3. WRONG: 1-A checklist tells you to STOP, then to wait for a probe that can never arrive while stopped
- Where: "Your first mission (1-A)" steps 2 and 9.
- Screen said: "Tap STOP: stay put while you plan [STOP]" ... "Wait for a probe report in COMMS".
- What happened: after STOP the ship is in EMG STOP and the mission clock does not move: MsnElpTme 0.08 / Time left 9.92 for over 3 minutes, on the star map and the bridge (L freeze was off; I checked). Probes control showed probe 1 "Transit TG 18 ... REG.L 6, 0", i.e. not moving (d038). SELECT just repeats "Wait for a probe report in COMMS". The report came only after I tapped SPD+ and ENGINES on my own (d040-d042). The port's own manual page 6/112 says "The clock runs while the ship moves or orbits. It stood still while docked and while stopped in open space" (e007).
- Shots: d031-d042, e007.
- Fix: remove the STOP step, or reword step 9: "Time only passes while the ship moves: tap SPD+ once, then ENGINES, and wait for the probe report in COMMS." Add a wait hint when a "Wait:" step is current and the clock has not moved for 10 s: "Clock is stopped: the ship must be moving or in orbit."

### 4. WRONG: on the mission orders page, PG DN throws the orders away
- Where: Practice > Start a new mission, "COMMUNICATION FROM KRELLAN FLEET".
- Screen said: "Lines 1-11 of 26", "Read the screen, then tap CONTINUE (or A)", buttons PG UP / arrows / PG DN / SPACE, game line "PRESS ANY KEY TO CONTINUE".
- What happened: tapping PG DN to read lines 12-26 counted as "any key"; the orders were gone and loading began (d004 -> d005). The later "Reconnaissance Mission into Region #1-A" card (d008) rescues the goal, but the orders themselves are never read.
- Fix: on "press any key" pages make PG UP/PG DN/arrows scroll the port's own reader only and send no key; only CONTINUE/A sends the key.

### 5. WRONG: red alert gets no guidance, and SELECT answers with a stale checklist step
- Where: 1-A, in orbit, TELEPORTERS menu, when "***ENEMY DETECTED: RED ALERT***" came.
- Screen said: hint bar "X/Y fire  L freeze  R+tap: explain  SELECT: what now?". SELECT: "Your first mission (1-A), step 29 of 35: Landing parties? Answer NO. You are on the bridge, stopped. Off track? START has STEP BACK and SKIP STEP." (d076). No coach card appeared. The ship was in orbit, not stopped.
- How the fight went by touch: no shots were exchanged. PHASERS gave a good card: "NOTHING TO SHOOT AT ... HOW: in a fight, get within 5 points of the enemy (torpedoes reach 10). WHY: the phasers have no target locked." (d081), but nothing says how to close in. COMMS said "UGA ship detected. Bearing: 358 Distance: 20.0"; the enemy was not drawn on the SYSTEM map, TARGETS was empty (d082-d084); X on the COMBAT screen gave no feedback (d079); the COMBAT tab shows TDB ON/OFF, HEADINGS, WEAPONSCTL, MORE with only "Tap one of the keys below" (d078). GO is unexplained; tapping it engaged the engines and the alert fell to ORANGE (d086). The alert flipped red/orange twice more on the way out; the exec's "Should we set the weapons to disable rather than destroy?" has no port advice (d092); then the ship jumped home.
- Fix: on RED ALERT show a coach card and make it the SELECT answer while the alert lasts: "RED ALERT: an enemy is near. Fight: tap COMBAT, tap the red ship, then PHASERS (within 5) or TORPS (within 10). Run: tap STARFORT. L freezes the clock while you think." Add an explain card for GO.

### 6. WRONG: Tutorial 1B step 9 names the wrong direction
- Screen said: "Tap the star one left, one up: SS-33".
- What happened: the star just probed (SS-37) is at 29x 4y; SS-33 is at 28x 3y, i.e. one left, one DOWN from it (and one left, two up from the ship marker). There is no star one left and one up of SS-37 (c064, c065). I found it by trying the nearest star.
- Fix: "Tap the star just below-left of the one you probed. The TARGET box on the top screen must read SS-33."

### 7. WRONG: stale ring and stale card point at the wrong thing
- The checklist ring for the old step "Landing parties? Answer NO [NO]" ringed NO on a different question, "Beam landing party up?" (d069).
- "NOTHING TO SHOOT AT" stayed up through the hyperspace jump and covered the checklist while the game asked "Shall we dock?" (d095, d096); it also re-appeared on returning to the bridge (d085).
- Fix: ring a YES/NO button only when the question on screen is the step's own question; drop explain cards when the screen or alert state changes.

### 8. CONFUSING: several steps never tick by themselves
- 1A step 11 "Tap the tactical display to turn the ship": 4 taps, ship turned each time (status "Course 66° (-124°)"), no tick (c036-c041). 1A step 14 "Tap planet A again", 1B step 19 "Tap the planet (the letter A)", 1-A step 4 "Tap a star close to the pink box", 1-A step 10 "Read it: how many planets in the system?": none tick (c044, c045, c079, d025, d026, d042). They tick when the next step is done, but the player sees no confirmation and, at step 11, no visible next button until told. The source shows these steps have no proof text and no button.
- Also 1A step 9 "Now fly by hand: tap SPD+ three times" ticks on the first tap (c032).
- Fix: proof texts: step 11 "Course " status line; planet steps "Planet" in the TARGET box; star step "SS-" in ID/class; SPD+ step: tick at s: 0.3.

### 9. CONFUSING: "THE GAME SAYS" card hides the checklist exactly when a YES/NO is needed, and the list skips those answers
- 1C: "Do you wish to go to nearest starfort?" covers step 2 "Go to nearest starfort? Answer YES" (c090).
- 1-A in orbit: after "YOU HAVE COMPLETED THE MINIMUM REQUIREMENTS OF YOUR MISSION!" the list went 27 -> 32 ("Objective done? Tap STARFORT", ring on STARFORT) while the game was still asking "Display planetary map?"; steps 28 "Planet map? Answer NO" and 29 "Landing parties? Answer NO" were skipped; SELECT said only "The game is waiting for YES or NO." (d060, d061).
- 1A end: the "WELL DONE / PART 1A DONE" card and the ring on STARS appeared while "Do you wish to be resupplied?" YES/NO was still unanswered (c053).
- Fix: when a question is on screen and the current (or a skipped) step answers it, put the answer in the card: "THE GAME SAYS: Display planetary map?  Checklist: answer NO." Do not finish or far-jump a list while a YES/NO is pending.

### 10. CONFUSING: how a newcomer finds the 1-A checklist, and conflicting first orders
- The only pointers are the Tutorial-1-done card (lost on first tap, see 1) and browsing the manual. The mission-start cards ("Reconnaissance Mission into Region #1-A ...", "COACH: LEAVE THE STARFORT") do not mention it (d008, d015).
- The coach card says "Pick where to go first: SYSTEM, tap a planet, TARGETER, ENTER, B. Then LAUNCH"; the checklist says step 1 "Tap LAUNCH to leave the starfort", step 2 "Tap STOP" (d015 vs d020).
- Fix: in Practice 1-A add to the coach card "First time? START > MANUAL > Your first mission > FOLLOW THIS CHECKLIST", or start that checklist automatically the first time; make both give the same order.

### 11. CONFUSING: buttons change under the finger
- Star map: tapping a star the box is already on opens the TARGETER at once; the tap meant for the TARGETER button then lands on SPD- (same place) and drops the speed 8.0 -> 7.0 (d044, d045).
- Bridge: STARFORT is replaced by GO/FREEZE or STOP/FREEZE the moment a red alert starts (d074, d087).
- 1-A step 2 says "[STOP]" about 2 s before the LAUNCH button turns into STOP (d021, d022).
- Fix: keep TARGETER/ENTER and SPD-/SPD+ in different slots; ignore taps for ~0.4 s after a layout swap.

### 12. CONFUSING: hint bar contradicts the step or the screen
- "Tap a planet or starfort, then tap TARGETER" stays up when the step is "Press B for the bridge" (c024) and on the jump-arrival map whose buttons are ENTER/BACK/KEYBOARD, no TARGETER (d046).
- "Tap a star: the box jumps to it. Then TARGETER" while the step is "Tap PROBES" and while only LAUNCH CONTROL / OPERATIONS are shown (c056, c057, c065).
- 1B step 10 "Tap TARGETER [TARGETER]" was shown with only LAUNCH CONTROL / OPERATIONS on screen (after a STEP BACK; B brings the map buttons back) (c065).
- Fix: while a checklist is running, the hint bar shows the current step; on the arrival map: "Tap where you want to arrive, then ENTER".

### 13. CONFUSING: unexplained game windows and questions
- Commander's Log ("VIEW MODE / COMMANDER'S LOG / CHRONICLE OF GLORY", "PLEASE STANDBY", MENU / EXIT) opens by itself at mission start; top screen is a squeezed jumble (d013). Only the manual page explains it.
- "Make changes?" after BEAM DOWN (NO means go) (d065). "Do you wish to conclude the mission?" after docking; the checklist ends one step earlier (d097). "Do you want to delete them?" and "Would you like your log files ... saved into a mission subdirectory?" at mission end (d099, d101). "Do you want to switch to Practice mode?" with only "Tap YES or NO" (d001).
- For about a second YES/NO buttons stood under "MSG: Please give our regards to Blinky Bill." before "Comply with the request?" appeared (d011).
- Fix: one-line advice in the hint bar for each: "The ship's log opens once: tap EXIT", "NO = send them now", "YES ends the mission and shows your score", "YES is fine", "NO is fine", "YES: Practice does not count on your record".

### 14. CONFUSING: no closing word after the first mission, and Practice stays on
- After the tally the sign-on banner again says "Practice is on: tap Start a new mission" (d103). No "mission done" card, and nothing says how to turn Practice off for a real mission.
- The goal bar keeps "Goal: Discover at least one mineral-rich planet in UGA or neutral space" after the game announced it was met (d059).
- Fix: card at sign-on: "PRACTICE 1-A DONE. For a real mission: Change default settings > Practice Mode Off, then Start a new mission." Goal bar: "Goal met: fly home (STARFORT) and dock".

### 15. CONFUSING: dead tabs in tutorials
- In Tutorial 1 ORDERS says "NOT AVAILABLE IN TUTORIALS..." (good) but LOG and FLEET do nothing and say nothing (c097-c099).
- Fix: same card for every refused tab.

### 16. POLISH
- 1B done card: "Next: the way home. Next: Tutorial 1C: come home" (double "Next:") (c088).
- Top-screen title bar cuts sentences: "Some data files from this mission are no longer", "Would you like your log files (reports, messages," (d099, d101).
- "All 7 lines shown" while 4 lines are visible (c004).
- The same hint line is printed twice on the tutorial list and reader screens (c002, c003).
- START menu header "Build unknown" (c061).
- Manual page 6/112 ends with a developer note: "Whether it runs while the crew rests has not been checked yet." (e007). Library text for 1B also has "in our test run it was not clear which key answers it".
- "A mission is highlighted: tap SELECT below" is easy to confuse with the SELECT button, which the same screens advertise as "what now?" (d002). Suggest "tap CHOOSE below".
- Tutorial list shows "1 Basic navigation (done)" before this player had done it (c002).
- Checklist card covers the ENERGY and LRS panels for the whole tutorial (c025 onward); with no hold-R here I could not check a way to move it.
- Star-map box moves slowly to the tapped planet (about 3 s) with no "moving" cue (c021, c022).
- Manual scroll by single Circle Pad presses is slow (14 presses for about 8 lines); hold could not be tested here.

## What worked well
- Sign-on banner "New? Tap Tutorials, then 1. Help: START > MANUAL" and the "COACH: START HERE" card with a ring on Tutorials (c000, d000).
- Practice start is led step by step: "A mission is highlighted: tap SELECT below", then "Practice is on: tap Start a new mission" with a ring (d002, d003).
- Mission summary card and the Goal bar (d008, d009). SELECT explaining a game question: "A routine request from the starfort: YES is the usual answer" (d012).
- SELECT "what now?" always answered and names where you are; STEP BACK and SKIP STEP both work with clear status lines (c062, c076).
- Rings on the right button for most steps; enlarged Targeter and Probes Control on the top screen (c058, c067).
- All three success cards appeared (c053, c088, c094).
- Landing party screens: "+ TEAM adds 10 people to the lit row", "- TEAM brings a team back; ALL BACK: everyone" (d063, d070). Send and recall both worked by touch.
- "NOTHING TO SHOOT AT" HOW/WHY card; "FROZEN: the clock is stopped. Press L to carry on" (d081, d036).
- Manual: opens on "What this game is" (1/112), HOW/WHY layout, controls shown on both screens ("Sticks scroll  D-Pad sections  L/R next  B close"), Circle Pad scroll, R next section and D-Pad section moves all worked, FOLLOW THIS CHECKLIST button works (d016-d020, e002-e004). No cut-off text inside the manual.

## How far I got
1. Tutorial 1: all three lists finished (1A 21/21, 1B 27/27 with 2 forced recoveries by STEP BACK/SKIP STEP, 1C 4/4). SELECT tried at 7 points; STEP BACK and SKIP STEP both used. Then blocked at leaving (finding 1).
2. Practice 1-A: started the way the port says, found and followed the checklist from the manual, launched, probed SS-18, jumped, orbited SS-18A, scan met the mission goal, sent one landing team down and recalled it (off-checklist), flew home with STARFORT, docked, concluded, saw the tally, back at sign-on. Two forced recoveries (finding 2) and one self-rescue (finding 3).
3. Red alert: happened three times around SS-18; no shots fired by either side; guidance as in finding 5.
4. Manual: opened in a mission and at sign-on; first page and controls are right for a newcomer.

---
## Appendix: raw running notes (taken during play)
- Sign-on: banner "New? Tap Tutorials, then 1. Help: START > MANUAL" - clear. Tutorials list shows "1 Basic navigation (done)" on a fresh look; duplicate hint line top+bottom of touch screen. (c000,c002)
- (tester note: my first taps on "SYSTEM" used picture x instead of touch x; those results are discarded.)
- 1A step 9 "Now fly by hand: tap SPD+ three times" ticks after the FIRST tap (c032: 10/21, s: 0.1). POLISH: tick only at 3 or say "tap SPD+ (up to three times)".
- 1A step 7 wait: ~25 s flight; Dist Trg counts down in NAV panel, fine. Checklist card covers ENERGY+LRS panels on top screen whole time (c025-c034).
- 1A step 11 "Tap the tactical display to turn the ship": tapped the tactical display 4 times; ship turned each time (status line "Course 66° (-124°)", Course set changed 224>190>66>109) but the step never ticked (c036-c041). SELECT still says step 11. WRONG/stuck-step.
- 1A step 14 "Tap planet A again": box on planet (TARGET "Planet 52x 40y") but step did not tick after 7 s (c044,c045), whereas the identical step 2 ticked. No ring on TARGETER.
- 1A end: "WELL DONE / PART 1A DONE..." card appears while the game is still asking "MSG: Do you wish to be resupplied?" YES/NO; step 21 ("It offers supplies: answer YES or NO") was ticked before I answered and the ring already moved to STARS (c053). Card covers COMMS msgs.
- 1B: after step 5 "Tap LAUNCH (or A): the probe goes [LAUNCH]" one tap on LAUNCH jumped the checklist from 5/27 to 22/27 ("√ Tap ENTER / ► Press B for the bridge [B] / Tap ENGINES: the ship flies there") (c058 -> c059). 16 steps skipped. WRONG, severe.
- 1B step 9 "Tap the star one left, one up: SS-33": SS-33 is at 28x 3y; the star just probed (SS-37) is 29x 4y, so SS-33 is one left, one DOWN from it (one left/two up from the ship marker). No star exists one-left-one-up of SS-37. (c064,c065) WRONG wording.
- 1B step 10 "Tap TARGETER: speed 8 is offered [TARGETER]" while the buttons shown are only LAUNCH CONTROL / OPERATIONS (probe submenu still open) - no TARGETER button to tap (c065). (Happened after the 5->22 jump + STEP BACK; step 6 earlier said "Press B: the map buttons come back".)
- 1B step 11 "Tap ENTER to accept speed 8 [ENTER]" (next line: "Tap an empty spot on the bottom edge"): tapping ENTER jumped the checklist 11 -> 22/27 "Press B for the bridge [B]" while the game is on "MAP OF STAR SYSTEM SS-33 (Class G)" with "Select target" (c068). Second reproduction of the jump-to-22 bug: any ENTER/LAUNCH tap matches the later "Tap ENTER" step 21.
- 1B after jump-to-22: following the card literally (B, B, ENGINES) the ship jumped to SS-33 and stopped at the system edge ("MSG: Target reached, Excellency."), checklist 24/27 "Establish orbit? Answer YES [YES]" but no question is asked and no YES button exists (c073-c075). SELECT only repeats it. Newcomer stuck unless they use STEP BACK.
- 1B steps 19 ("Tap the planet (the letter A)") did not tick with box on planet (c079), same as 1A step 14. STEP BACK / SKIP STEP both work (status "Checklist: back one step" / "Checklist: step skipped") but each needs START + tap; no way to go back many steps.
- 1B success card OK (c088) but reads "Next: the way home. Next: Tutorial 1C: come home" (double Next).
- 1C: steps fine (4 steps). "THE GAME SAYS / Do you wish to go to nearest starfort?" card covers the checklist so step 2 ("Go to nearest starfort? Answer YES") is hidden exactly when needed (c090).
- BLOCKER at end of Tutorial 1 (c094-c112): card says "TUTORIAL 1 DONE. You can fly, orbit, probe, jump and dock. Next: Practice mission 1-A (START > MANUAL > Your first mission). To leave: SHIP MENUS, COMPUTER, Special services, Quit, YES, then NO." There is no SHIP MENUS button on the bridge (buttons: SYSTEM MAP, NAV MAP, WAR MAP, HELM, PHASERS, TORPS, SHLD+/-, SPD+/-, LAUNCH, WHAT NOW?, 7 tabs). After docking: STARS, HELM, ORDERS, Y -> "THE GAME SAYS / NOT AVAILABLE IN TUTORIALS. Tutorials switch this off; it works in a mission or in Practice."; SYSTEM, WAR, COMBAT, FLEET, LOG, SYSTEM MAP, NAV MAP, WAR MAP, LAUNCH, B, X -> nothing at all. START menu: EXIT GAME and GAME SETTINGS greyed. SELECT gives the generic "1) Pick where to go first: tap SYSTEM..." which itself no longer works. No touch route out.
- Practice start: sign-on > Practice (demo) mission > "Do you want to switch to Practice mode?" (hint only "Tap YES or NO") > mission grid "A mission is highlighted: tap SELECT below" > back at sign-on "Practice is on: tap Start a new mission" (ring) - works. Orders screen "COMMUNICATION FROM KRELLAN FLEET" shows "Lines 1-11 of 26" with PG UP/PG DN buttons and "Read the screen, then tap CONTINUE (or A)"; tapping PG DN to read lines 12-26 did not scroll: the game took it as "PRESS ANY KEY TO CONTINUE" and the orders were gone (d004->d005). Newcomer never sees the mission orders. WRONG.
- Mission start: after "Comply with the request?" YES, a "COMMANDER'S LOG / VIEW MODE / CHRONICLE OF GLORY" window opens by itself with "PLEASE STANDBY" and buttons MENU / EXIT, no explanation (d013). Top screen there is a squeezed jumble. EXIT returns. Then COACH: LEAVE THE STARFORT card (good, d015). For ~1 s the YES/NO buttons appeared under "MSG: Please give our regards to Blinky Bill." before the question text "Comply with the request?" showed (d011).
- Finding the 1-A checklist: only via the Tutorial-1-done card ("START > MANUAL > Your first mission") or by browsing the manual; the mission-start cards (Recon mission card, COACH: LEAVE THE STARFORT) do not mention it. Manual page 3/112 has "FOLLOW THIS CHECKLIST" button - works (d017,d020). Coach card says "Pick where to go first ... Then LAUNCH" but checklist step 1 is "Tap LAUNCH to leave the starfort" then "Tap STOP: stay put while you plan" - contradictory orders.
- 1-A checklist: step 4 "Tap a star close to the pink box" did not tick with box on SS-18 (d025,d026). Step 7 "Tap LAUNCH: a probe flies to that star [LAUNCH]" -> checklist jumps 7/35 -> 24/35 "Press B for the bridge [B] / Tap ENGINES: the ship flies there" (d028->d029). Same jump bug as tutorial 1B, third reproduction.
- 1-A step 9 "Wait for a probe report in COMMS": after step 2 "Tap STOP: stay put while you plan" the ship is in EMG STOP and the mission clock does not advance at all (MsnElpTme 0.08 / Time left 9.92 unchanged for 3+ minutes, on star map and on bridge; L freeze was off - checked). Probe 1 shows "Transit TG 18 ... REG.L 6, 0" (d038) and never arrives. Infinite wait with no feedback; SELECT just repeats "Wait for a probe report in COMMS". (Same in Tutorial 1A: MET stayed 0.98 while stopped.)
- 1-A step 13 "Tap ENTER to accept the speed [ENTER]" -> jumps 13/35 -> 24/35 (d046), 4th reproduction. On the in-jump system map ("MAP OF STAR SYSTEM SS-18", "Select target", buttons ENTER/BACK/KEYBOARD) the hint bar says "Tap a planet or starfort, then tap TARGETER" - there is no TARGETER button there.
- Star map: tapping a star the box is already on opens the TARGETER at once; a following tap aimed at the TARGETER button lands on SPD- (same position) and lowers speed 8.0 -> 7.0 (d044,d045).
- 1-A step 10 "Read it: how many planets in the system?" has no action; it ticked only when I went on (probe report d042 fine: "1 planet in system. Type: I").
- 1-A in orbit: after "YOU HAVE COMPLETED THE MINIMUM REQUIREMENTS OF YOUR MISSION!" the checklist jumped 27 -> 32 ("Objective done? Tap STARFORT", ring on STARFORT) while the game was still asking "Display planetary map?" YES/NO; steps 28 "Planet map? Answer NO" and 29 "Landing parties? Answer NO" were skipped, and THE GAME SAYS card hides the checklist anyway; SELECT says only "The game is waiting for YES or NO." (d060,d061). Goal bar keeps saying "Goal: Discover at least one mineral-rich planet..." after it is met.
- Landing party (off-checklist): YES at "Landing parties?" opens a clear team screen ("+ TEAM adds 10 people to the lit row", BEAM DOWN) - good; then "Make changes?" YES/NO after BEAM DOWN (NO = go) is cryptic. Recall: TELEPORTERS > LANDING PARTY > "Beam landing party up?" - the checklist ring for the stale step "Landing parties? Answer NO [NO]" rings NO on this different question (d069). No coach text on how to recall a party (found by guessing LANDING PARTY).
- RED ALERT (d074-d076): COMMS "***ENEMY DETECTED: RED ALERT***", top-right "RED ALERT", hint bar changes to "X/Y fire  L freeze  R+tap: explain  SELECT: what now?". No coach card appeared. SELECT says "Your first mission (1-A), step 29 of 35: Landing parties? Answer NO. You are on the bridge, stopped." - nothing about the alert (and the ship is in orbit, in the TELEPORTERS menu). The STARFORT button I had just tapped vanished (replaced by GO / FREEZE) with no word on whether the go-home order took.
- Red alert by touch: PHASERS -> good card "NOTHING TO SHOOT AT ... HOW: in a fight, get within 5 points of the enemy (torpedoes reach 10). WHY: the phasers have no target locked." (d081) but nothing says HOW to close on the enemy; enemy (COMMS: "UGA ship detected. Bearing: 358 Distance: 20.0") is not drawn on the SYSTEM map and TARGETS list is empty (d082-d084); hardware X on the COMBAT screen gave no feedback (d079). COMBAT tab screen (TDB ON/OFF, HEADINGS, WEAPONSCTL, MORE) has only "Tap one of the keys below". The card re-popped when returning to the bridge (d085). GO button unexplained; tapping it engaged engines and the alert dropped to ORANGE (d086). No shots exchanged.
- Home: stale "NOTHING TO SHOOT AT" card (from a PHASERS tap just as the ship jumped) stayed up through hyperspace and covered the checklist while the game asked "Shall we dock?" (d095,d096). Docking fine. Then "Do you wish to conclude the mission?" - checklist (35 steps) ends at "Shall we dock? Answer YES"; no word on this question (d097). Exec question "Should we set the weapons to disable rather than destroy?" also has no port help (d092).
- Mission end: tally screen (top screen 80-col table, small), B leaves; then DOS housekeeping questions "Do you want to delete them?" and "Would you like your log files ... saved into a mission subdirectory?" with no port advice; top-screen title bar cuts both sentences off ("Some data files from this mission are no longer", "Would you like your log files (reports, messages,") (d099,d101). Back at sign-on banner still "Practice is on: tap Start a new mission"; no "mission done" card, no hint how to leave Practice for a real mission (d103).
