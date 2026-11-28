// refusals.h - data for the "Why can't I...?" cards (Star Fleet II v2.0.22).
// Needs `struct Refusal { const char* phrase; const char* title; const char* text; const char* button; };`
// to be defined by the .cpp that includes this file.
//
// WHERE THE PHRASES COME FROM
//   The game keeps most officer messages in SF2.TXT, SF2D.TXT and SF2D2.TXT, one message line per
//   text line, every byte bit-inverted (b ^ 0xFF) except CR/LF. `strings` finds nothing there until
//   the files are decoded. Many of the same lines also sit as plain text in the overlays.
//   Tags used below:  [T]=SF2.TXT  [D]=SF2D.TXT  [D2]=SF2D2.TXT  [M]=SFIIM.EXE  [O]=OVERLAY.EXE
//   [O1]..[O6]=OVERLAY1..6.EXE  [B]=BOOM.EXE  [C]=CCC.EXE  [F]=FLEET.EXE
//   "Velocity must be set." is in SF2.TXT only (also seen in RPTLOG.2 / CMBLOG.2 from a real run),
//   so the .TXT wording is the one that reaches the screen when the two differ.
//
//   "Report from <Officer>:" headers found (SFIIM/SFIIS/SFTABLE/SDATA/OVERLAY3/5/6/FLEET.EXE):
//   Executive Officer, Helm, Engineering, Tactical Console, Weapons Console, Combat Control,
//   Damage Control, Science Console, Medical Console, Security, Communications, Probe Ops Control.
//   Other headers: "MESSAGE FROM LANDING PARTY:", "MESSAGE FROM BOARDING PARTY:", "MESSAGE FROM
//   PRIZE CREW:", "MESSAGE FROM FLEET HQ:", "MESSAGE FROM THE PLANET:", "MESSAGE FROM STARFORT SF-n:".
//
// MATCHING NOTES
//   * The first matching entry wins, so specific phrases stand before general ones (the two
//     catch-alls "IN HYPERSPACE" and "ust be in orbit" / "ust be orbiting" come after them).
//   * Each phrase is one message line (or part of one) and is at most 40 characters, so it fits
//     a COMMS row. For messages of two or three lines the phrase is taken from the LAST line or
//     the one before it, because the matcher looks at the last two lines only.
//   * The upper-case messages (CANNOT USE IN HYPERSPACE, NO ESCORTS SELECTED, ...) may be printed
//     on the prompt / command line and not in COMMS; if so they will simply never match there.
//     "VELOCITY MUST BE AT LEAST 1.0 FOR HYPERSPACE TRAVEL" is longer than 40 characters, so it
//     is certainly not a single COMMS row; the phrase used ("AT LEAST 1.0") is a short piece.
//
// UNCERTAIN (meaning inferred from the wording, the position in the text file and the manual; the
// game code was not traced and nothing was run). The card texts for these are kept cautious:
//   * "No target has been entered" [T], "no nav target loaded" [T]: which helm order prints each
//     is not known; both clearly mean the navigation computer has no destination.
//   * "Engines must be on" [T]: the order that needs it is unknown.
//   * "Sublight speed needs to be set" [D2]: probably a jump / launch order given with speed 0.
//   * "Target must be to our rear" [T]: read as a tractor beam refusal (Ops 1.2.4: the beam only
//     operates aft); it sits right after the tractor beam strings.
//   * "No target within range" [T][O]: read as a teleporter target message (it sits with the
//     material transfer strings), not a weapons one.
//   * "Target out of range" [T][O6]: probe launch (next line is "Maximum range is 5 light years").
//     A weapons message " out of range!" [D] also exists; it does not contain this phrase.
//   * "Cannot dock- no": SF2.TXT says "no BC berths empty", OVERLAY6.EXE says "no cruiser docks
//     empty"; the common part is used.
//   * "Insufficient energy available" [T][B][O4][O5] is printed by several systems (phasers,
//     emergency hyperspace maneuver, captive transfer); the text is general.
//   * "Request for escorts denied" [D][O]: the reason is not given by the game; the text guesses
//     "none to spare".
//   * "suitable for invasion" [T][O][O2]: the exact rules HQ uses are not in the manual.
//   * "Scanners inoperative due to ECM": the port's button reads "ECM OFF" while ECM is on; that
//     label is not in the list this file was allowed to use, so the button field is 0.
//   * Found but left out to keep the list at 60 (add them if they turn up in play): "Unable to
//     establish orbit", "Unable to enter orbit because / planetary shields are up", "Cannot use
//     pursuit mode in orbit", "Unable to fire torp - fixed soon.", "We're unarmed!", "Cannot
//     lock-on. <x> in way.", "Object in way.", "Target has engines on.", "Cannot be towed while
//     <x>.", "You must get within 10 points of us.", "Cannot send landing party down.", "EXCEEDS CAPACITY", "Too many captives - 500
//     max.", "Another probe already targeted there.", "Probe Ops Control cannot handle another
//     probe", "Cannot beam aboard, probes supply full.", "NO ESCORTS SELECTED", "MUST BE A
//     DESTROYER", "You are not authorized to order us.", "NOT ALLOWED TO COMMAND A DOCKED SHIP",
//     "Unable to establish communications", "Comm damaged - unable to transmit.", "Unable to
//     comply - you're docked.", "Request denied.", "CANNOT INVADE - CANCEL", "Invasion request
//     denied.", "Unable to support another invasion.", "ACCESS TO ORBIT CONTROL DENIED!", "Unable
//     to use on a planet with a unified government.", "No pods left!", "Can only use against
//     planets!", "CANNOT USE DISABLE MODE FOR BOMBARDMENT", "CANNOT CHANGE TARGETS WHILE IN
//     HYPERSPACE", "CAN ONLY USE FROM MAIN DISPLAY", "NOT ENOUGH CREW LEFT TO OPERATE SYSTEM",
//     "We have to complete repairs first.", "Ion storm - scanners inoperative."
//   * Button names: only the port's own labels are used. Things with no touch button (red alert
//     by SHLD+ at maximum twice, ZL/ZR hyperspace speed, game menus) are described in the text.

static const Refusal REFUSALS[] = {
  // ---------------------------------------------------------------- moving the ship
  // seen in the real game (harness, Nov 21 2026): tests/screens/refuse_*
  {"Engines already on", "THE ENGINES ARE ALREADY RUNNING", "HOW: nothing to do; change speed with SPD+ and SPD-, or tap STOP. WHY: the ship is already under way.", "SPD+"},
  {"Unable to establish orbit", "WHY CAN'T WE ORBIT?", "HOW: tap SYSTEM, pick the planet as your target and fly there; the helm offers to orbit when you arrive. WHY: the ship must be right next to the planet.", "SYSTEM"},
  {"Unable to use while docked", "WHY CAN'T I USE THIS NOW?", "HOW: tap LAUNCH to leave the starfort, then give the order again. WHY: this order does not work while the ship is docked.", "LAUNCH"},
  {"Torpedoes not targeted", "NOTHING TO SHOOT AT", "Nothing to shoot at: no enemy in range, so nothing to aim at. HOW: in a fight, get within 10 points of the enemy (phasers need 5). WHY: the torpedoes have no target.", "SHIP MENUS"},
  {"No valid target within range", "NOTHING TO BOARD", "HOW: fly right next to a crippled enemy ship, then repeat the order. WHY: a boarding party needs a target ship within teleporter range.", 0},
  {"our ship, Excellency", "YOU SCANNED YOUR OWN SHIP", "HOW: move the target box onto another ship or planet, then scan again. WHY: the target box was on your own ship.", 0},
  {"captives on board", "NO ONE TO INTERROGATE", "HOW: capture prisoners first (boarding parties and landing parties bring them back). WHY: there are no captives on the ship.", 0},
  {"not in a rest state", "REST STATE", "HOW: answer YES to put the crew at rest, NO to leave things as they are. WHY: the crew is on normal duty; rest speeds recovery but slows reactions.", 0},
  {"Velocity must be set", "WHY WON'T THE SHIP MOVE?", "HOW: tap ENGINES: it sets speed 0.5 and starts. WHY: the helm will not start the engines while the set speed is 0.", "ENGINES"},  // [T]
  {"Sublight speed needs to be set", "WHY WON'T THE SHIP MOVE?", "HOW: tap SPD+ to set a sublight speed (0.6 or more if you want to jump to another star), then try again. WHY: this order needs a normal-space speed and the set speed is 0.", "SPD+"},  // [D2]
  {"Engines must be on", "WHY IS THAT ORDER REFUSED?", "HOW: tap ENGINES (set a speed with SPD+ first if asked), then give the order again. WHY: this order only works while the engines are running.", "ENGINES"},  // [T]
  {"No target has been entered", "WHERE ARE WE GOING?", "HOW: tap SYSTEM (a place near this star) or STARS (another star), pick a destination, then set a speed. Or just steer by tapping the tactical display. WHY: the navigation computer has no destination to fly to.", "SYSTEM"},  // [T]
  {"no nav target loaded", "WHY CAN'T THE HELM DO THAT?", "HOW: tap SYSTEM or STARS and pick a destination first, then repeat the order. WHY: this helm order works on a navigation target and none is loaded.", "SYSTEM"},  // [T]
  {"Cannot rest while moving", "WHY CAN'T THE CREW REST?", "HOW: tap STOP, wait until the ship has halted, then order the rest again. WHY: the crew can only rest while the ship is standing still.", "STOP"},  // [T]
  {"WHILE MOVING OR IN REST", "WHY CAN'T I USE THIS NOW?", "HOW: tap STOP and wait for the ship to halt (and end the crew rest if it is on), then try again. WHY: this setting cannot be changed while the ship is moving or the crew is resting.", "STOP"},  // [D][O4] "CAN'T USE WHILE MOVING OR IN REST STATE"

  // ---------------------------------------------------------------- hyperspace
  {"AT LEAST 1.0", "WHY CAN'T WE JUMP TO THAT STAR?", "HOW: press ZR until the hyperspace speed is 1 or more, then start the engines. WHY: travel between stars is done in hyperspace, which needs a set speed of at least 1.0.", "ENGINES"},  // [D] "VELOCITY MUST BE AT LEAST 1.0 FOR HYPERSPACE TRAVEL"
  {"Can't use hyperdrive for sys targ", "WHY NO HYPERSPACE HERE?", "HOW: press ZL until hyperspace speed is 0, then set a normal speed with SPD+ and start the engines. WHY: your target is inside this star system, and hyperspace only works between stars.", "SPD+"},  // [T]
  {"Cannot use hyp.eng for manual nav", "WHY NO HYPERSPACE HERE?", "HOW: press ZL until hyperspace speed is 0 and use SPD+ to steer by hand. To use hyperspace tap STARS and pick another star first. WHY: hyperspace engines only fly to a star chosen on the map, not by hand.", "STARS"},  // [T]
  {"Hyperdrive out - unable to use", "WHY WON'T THE HYPERDRIVE WORK?", "HOW: tap SHIP MENUS, then ENGINEERING and open Damage Control to put repair teams on the hyperdrive; if it is destroyed only a starfort can replace it, so ask for a tow in SHIP MENUS, COMMS. WHY: the hyperspace engines are too damaged to run.", "SHIP MENUS"},  // [T][O5]
  {"Minimum capability in", "WHY WON'T THE ENGINES START?", "HOW: wait: the report says how long the repair takes (minimum capability first, full later). To hurry it, tap SHIP MENUS, ENGINEERING, Damage control and put more repair teams on SubLt Eng. WHY: the sublight engines were knocked out by a hit and the repair teams are working on them.", "SHIP MENUS"},  // seen, capture 3 (armed_4x): "SUB-LIGHT ENGINES inoperative." / "Minimum capability in 0.4 days."
  {"Power reserves are low.", "WHY IS LOW PWR LIT?", "HOW: dock at a starfort (STARFORT) or ask for a supply ship in SHIP MENUS, COMMS. In a fight, every hit on the shields drains energy: break off with STARFORT if ENERGY Quantity keeps falling. WHY: the ship's energy reserves are nearly used up.", "STARFORT"},  // seen, capture 3
  {"0.6c needed for the hyper jump", "WHY CAN'T WE REACH HYPERSPACE?", "HOW: tap SHIP MENUS, then ENGINEERING, open Damage Control and repair the sublight engines, then try the jump again. WHY: the ship must reach 0.6 of light speed to jump, and the damaged normal engines cannot go that fast.", "SHIP MENUS"},  // [O3] 3-line XO report, last line
  {"Must use emergency", "WHY WON'T THE HYPERDRIVE WORK?", "HOW: tap SHIP MENUS, then NAVIGATION, choose Helm, then the emergency hyperspace maneuver. It is risky but the only way out. WHY: a black hole in this system stops a normal jump and will pull the ship in.", "SHIP MENUS"},  // [D2] "of black hole. Must use emergency"
  {"Cannot change while in hyperspace", "WHY CAN'T I CHANGE COURSE?", "HOW: tap STOP and wait for the ship to drop back into normal space, change what you need, then set speed and start the engines again. WHY: in hyperspace the ship flies a straight line; only a full stop is allowed.", "STOP"},  // [T]
  {"IN HYPERSPACE", "WHY CAN'T I USE THIS NOW?", "HOW: wait until the ship arrives, or tap STOP to drop back into normal space, then try again. WHY: this control or display does not work while the ship is flying through hyperspace.", "STOP"},  // [T][D][D2][C][O3] CANNOT USE / UNAVAILABLE / NOT AVAILABLE / NO TARGETER / CANNOT CHANGE ... IN HYPERSPACE

  // ---------------------------------------------------------------- orbit and docking
  {"Cannot set orbit while docked", "WHY CAN'T WE ORBIT?", "HOW: tap LAUNCH to leave the starfort, fly next to the planet, then order the orbit. WHY: the ship is still attached to the starfort.", "LAUNCH"},  // [T]
  {"Cannot orbit while hyperdrive on", "WHY CAN'T WE ORBIT?", "HOW: press ZL until hyperspace speed is 0 (or tap STOP), move next to the planet at normal speed, then order the orbit. WHY: the ship cannot enter orbit with the hyperspace engines running.", "STOP"},  // [T]
  {"Cannot orbit with tractor beam on", "WHY CAN'T WE ORBIT?", "HOW: tap SHIP MENUS, then ENGINEERING, choose the tractor beam and switch it off, then order the orbit again. WHY: the ship cannot enter orbit while it is towing something.", "SHIP MENUS"},  // [T]
  {"Too far away to orbit", "WHY CAN'T WE ORBIT?", "HOW: tap SYSTEM, pick the planet as your target and fly there; the helm offers to orbit when you arrive. WHY: the ship must be within one point (right next to) the planet or star.", "SYSTEM"},  // [M]
  {"ust be orbiting", "WHY DO I NEED TO BE IN ORBIT?", "HOW: tap SYSTEM, pick the planet, fly there and answer yes when the helm asks to orbit. Then repeat the order. WHY: this order only works while your ship is circling that planet.", "SYSTEM"},  // [T][O] "We must be orbiting a planet." / "...the target planet" / [O2] "Must be orbiting OBB planet to cancel."
  {"ust be in orbit", "WHY DO I NEED TO BE IN ORBIT?", "HOW: tap SYSTEM, pick the planet, fly there and answer yes when the helm asks to orbit. Then repeat the order. WHY: this order only works while your ship is circling a planet.", "SYSTEM"},  // [D][B][C][O2] "We must be in orbit." / "Must be in orbit around planet."
  {"Cannot dock - velocity", "WHY CAN'T WE DOCK?", "HOW: tap STOP (or SPD- down to 0.1), wait for the ship to slow, then order the docking again. WHY: the ship must be almost standing still, at speed 0.1 or less, to dock.", "STOP"},  // [T] "Cannot dock - velocity > 0.1"
  {"Too far away to dock", "WHY CAN'T WE DOCK?", "HOW: tap STARFORT and let the autopilot fly you there; it offers to dock on arrival. WHY: the ship must be within one point (right next to) the starfort.", "STARFORT"},  // [T]
  {"Nothing to dock with", "WHY CAN'T WE DOCK?", "HOW: tap STARFORT to fly to the nearest Krellan starfort, then dock there. WHY: there is no starfort or captured starbase next to the ship.", "STARFORT"},  // [T]
  {"Cannot dock- no", "WHY IS THE STARFORT FULL?", "HOW: wait nearby (tap STOP) and try again shortly; a ship will leave soon. Or fly to another starfort. WHY: every dock big enough for your battlecruiser is taken (a starfort holds three).", "STOP"},  // [T] "...no BC berths empty." / [O6] "...no cruiser docks empty."

  // ---------------------------------------------------------------- weapons
  {"Phasers not energized", "WHY WON'T THE PHASERS FIRE?", "HOW: nothing to do unless an enemy is near. With AAS on, the ship goes to red alert by itself when an enemy appears. By hand: with shields at 100%, one more SHLD+ calls red alert. WHY: phasers only get power in Condition RED.", "SHLD+"},  // [T]
  {"Phasers not targeted", "NOTHING TO SHOOT AT", "Nothing to shoot at: no enemy in range, so nothing to aim at. HOW: in a fight, get within 5 points of the enemy (torpedoes reach 10). WHY: the phasers have no target locked.", "SHIP MENUS"},  // [T][O4]
  {"No phaser line-of-sight", "WHY WON'T THE PHASERS FIRE?", "HOW: wait until your orbit brings the target into view, or use torpedoes (Y). WHY: the planet is between you and the target, and phasers cannot shoot through a planet.", "TORPS"},  // [D]
  {"No torpedoes left", "WHY WON'T THE TORPEDOES FIRE?", "HOW: fight on with phasers (X), then tap STARFORT and dock to get new torpedoes. WHY: every torpedo on board has been fired.", "STARFORT"},  // [T]
  {"Not valid enemy target", "WHY CAN'T I TARGET THAT?", "HOW: pick an Alliance (enemy) ship, base or mine as the target. WHY: weapons will not lock on Krellan ships, empty space or other things that are not enemies.", "SHIP MENUS"},  // [T]
  {"Planetary bombardment not set", "WHY WON'T WE BOMBARD?", "HOW: tap SHIP MENUS, then STRATEGY, choose the planet menu and Bombardment, and pick the weapons and targets there first. You must be in orbit. WHY: the bombard order was given before a bombardment was set up.", "SHIP MENUS"},  // [T]
  {"Insufficient energy available", "WHY IS THERE NOT ENOUGH POWER?", "HOW: tap SHLD- to free power and slow down, or tap STARFORT and dock to refill. WHY: the ship's power reserves are too low for what you ordered.", "SHLD-"},  // [T][B][O4][O5]
  {"Shield control out", "WHY WON'T THE SHIELDS GO UP?", "HOW: tap SHIP MENUS, then ENGINEERING, open Damage Control and put repair teams on shield control, or tap STARFORT and dock. WHY: the shield system has been knocked out by damage.", "SHIP MENUS"},  // [T]
  {"Scanners inoperative due to ECM", "WHY WON'T THE SCANNERS WORK?", "HOW: switch your ECM (jammer) off with the ECM button, scan, then switch it back on if you still need it. WHY: your own jamming blinds your scanners too.", "ECM OFF"},  // [T]

  // ---------------------------------------------------------------- tractor beam
  {"Enemy ship not disabled", "WHY WON'T THE TRACTOR BEAM HOLD?", "HOW: cripple the ship first: tap SHIP MENUS, then COMBAT, Weapons and set them to Disable, then shoot until its engines are out. Or capture it. WHY: the beam cannot hold an enemy ship that still works.", "SHIP MENUS"},  // [T]
  {"Tractor beam unable to lock on", "WHY WON'T THE TRACTOR BEAM LOCK?", "HOW: move right next to the ship with a clear line to it and try again. WHY: the beam only locks on a nearby ship that is stopped, with nothing in between.", 0},  // [T]
  {"Target must be to our rear", "WHY WON'T THE TRACTOR BEAM LOCK?", "HOW: turn the ship so the target is behind you (tap the tactical display on the far side), or answer yes when asked to turn the ship. WHY: the tractor beam only points backwards.", 0},  // [T]
  {"first turn off your engines", "WHY CAN'T WE GET A TOW?", "HOW: tap STOP, wait for the ship to halt, then ask for the tow again in SHIP MENUS, COMMS. WHY: the other ship cannot take you in tow while your engines are on.", "STOP"},  // [D][O] "You must first turn off your engines."

  // ---------------------------------------------------------------- teleporters, landing and boarding parties
  {"Teleporters down", "WHY WON'T THE TELEPORTERS WORK?", "HOW: tap SHIP MENUS, then ENGINEERING, open Damage Control and put repair teams on the teleporters, or dock at a starfort. WHY: the teleporter system is damaged and cannot beam anything.", "SHIP MENUS"},  // [D][O] "Unable to do. Teleporters down."
  {"Cannot board - target shields up", "WHY CAN'T WE BOARD THAT SHIP?", "HOW: shoot until its shields are down (set weapons to Disable under SHIP MENUS, COMBAT so you do not destroy it), then send the boarding party. WHY: nobody can be beamed through a raised shield.", "SHIP MENUS"},  // [T]
  {"Insufficient troops for boarding", "WHY CAN'T WE BOARD THAT SHIP?", "HOW: dock at a starfort for more shock troops, or recall troops you left elsewhere. WHY: a boarding team is 10 shock troops and you have fewer than that on board.", "STARFORT"},  // [T]
  {"Cannot send landing party through", "WHY CAN'T THE LANDING PARTY GO?", "HOW: wait until the planet's shields are down (bombardment knocks them out), then send the party. WHY: nobody can be beamed through a raised planetary shield.", 0},  // [D] next line: "planetary shields."
  {"a gaseous giant planet", "WHY CAN'T THE LANDING PARTY GO?", "HOW: choose a planet with a solid surface for your landing party. WHY: a gas giant has no ground to stand on.", 0},  // [D] "Cannot put landing party on" / "a gaseous giant planet."
  {"Noone assigned for landing", "WHY DID NOBODY BEAM DOWN?", "HOW: in orbit tap the planet on the touch screen, give at least one team a mission in the list, then confirm. WHY: the landing party was sent with no teams picked.", 0},  // [T]
  {"party on the planet", "WHY CAN'T I SEND IT DOWN?", "HOW: send a landing party to this planet first (in orbit, tap the planet on the touch screen), then transfer to it. WHY: you can only beam things down to your own people on the planet.", 0},  // [T] "Cannot transfer. There is no landing" / "party on the planet."
  {"Target must be in Krellan hands", "WHY CAN'T I TRANSFER TO THAT?", "HOW: pick a Krellan ship or base, or capture the enemy ship with a boarding party first. WHY: crew and supplies can only be beamed to something our side controls.", 0},  // [T][O]
  {"No target within range", "WHY IS THERE NOTHING TO BEAM TO?", "HOW: fly right next to the ship or base (or orbit the planet) and try again. WHY: teleporters only reach a ship beside yours, or one sharing your orbit.", 0},  // [T][O]

  // ---------------------------------------------------------------- probes
  {"Target out of range", "WHY CAN'T THE PROBE GO THERE?", "HOW: pick a target within 5 light years of the ship, or fly closer first and launch from there. WHY: a probe can travel 5 light years at most.", 0},  // [T][O6] next line: "Maximum range is 5 light years."
  {"No probe in launch tube", "WHY WON'T THE PROBE LAUNCH?", "HOW: dock at a starfort (tap STARFORT) to get new probes, or beam some up from a Krellan base. WHY: all your probes are used; none is loaded.", "STARFORT"},  // [D2][O6]

  // ---------------------------------------------------------------- escorts and other ships
  {"Escort must first rendezvous", "WHY WON'T THE ESCORT OBEY?", "HOW: tap SHIP MENUS, then STRATEGY, Escorts and order that escort to rendezvous; give the other order once it has arrived. WHY: the escort is too far from your ship for this order.", "SHIP MENUS"},  // [O3]
  {"Escorts too far away for resupply", "WHY CAN'T I RESUPPLY ESCORTS?", "HOW: tap SHIP MENUS, then STRATEGY, Escorts and order a rendezvous; resupply them when they are beside you. WHY: supplies are beamed across and the escorts are out of reach.", "SHIP MENUS"},  // [O] next line: "Suggest you order rendezvous."
  {"Request for escorts denied", "WHY CAN'T I HAVE ESCORTS?", "HOW: try again later, or go on with what you have. WHY: Fleet HQ has no destroyers to spare for you right now.", 0},  // [D][O]
  {"No legions on board", "WHY CAN'T THAT SHIP INVADE?", "HOW: give the invade order to a troop ship that carries legions (see FLEET). WHY: that ship has no ground troops on board to land.", "FLEET"},  // [T][F] "No legions on board. Cannot comply."

  // ---------------------------------------------------------------- invasion and planets
  {"suitable for invasion", "WHY CAN'T THIS PLANET BE INVADED?", "HOW: pick a planet with people living on it that is not already Krellan. WHY: Fleet HQ only sends legions against planets worth conquering.", 0},  // [T][O2] "Planet not suitable for invasion." / [O] "...unsuitable for invasion."
  {"More information on planet", "WHY IS THE INVASION REFUSED?", "HOW: orbit the planet, tap it on the touch screen and send a landing party on a survey mission; ask again when it reports. WHY: Fleet HQ will not invade a planet it knows too little about.", 0},  // [O][O2] next line: "Suggest completing survey mission."
  {"Cannot use against planetary shields", "WHY WON'T THE POD LAUNCH?", "HOW: bombard the planet until its shields are down, then release the pod. WHY: a raised planetary shield stops the pod.", "SHIP MENUS"},  // [T][B]
  {"Use not allowed - Krellans on planet", "WHY WON'T THE POD LAUNCH?", "HOW: beam your landing party up first (tap the planet on the touch screen) and make sure no Krellan troops or base are there. WHY: the pod kills everything on the planet, including our own people.", 0},  // [T][B]

  // ---------------------------------------------------------------- general
  {"Already surveyed!", "WHY CAN'T I SEND AN EXPLORE TEAM?", "HOW: move to another mission row with the arrows, or tap CANCEL. WHY: this planet has been explored already, so an explore/survey team has nothing to do here.", 0},   // emulator Nov 26
  {"Maximum range is 5 light years", "WHY WON'T THE PROBE LAUNCH?", "HOW: tap CONTINUE, then pick a star closer to your ship (within 5 squares of the pink box on the STARS map). WHY: a probe reaches 5 light years at most.", 0},   // harness Nov 27
  {"No detailed planet scan yet done", "WHY IS THERE NO DATA ON THIS PLANET?", "HOW: fly there and enter orbit; the scan runs by itself. WHY: the database only knows planets you have scanned from orbit.", 0},   // harness Nov 27
  {"ILLEGAL TARGET", "WHY CAN'T I TARGET THIS?", "HOW: put the target box on an enemy ship (tap the ship; if the name does not show, move the box one step right), then ENTER. WHY: there is nothing under the box to aim at.", 0},   // harness Nov 27
  {"Already docked", "ALREADY DOCKED", "You are docked at the starfort. LAUNCH leaves it.", "LAUNCH"},   // harness Nov 27
  {": not loaded", "WHY WON'T THE TORPEDOES FIRE?", "HOW: wait a few seconds and tap TORPS again. WHY: both tubes fire together and then reload. Tapping faster only queues shots and wastes torpedoes.", 0},   // harness Nov 26: "Torp #1: not loaded"
  {"Target already disabled", "WHY DID THE SHOT STOP?", "HOW: stop firing at this ship; board it (SHIP MENUS, then ENGINEERING, TELEPORTERS, Boarding party) or pick another target. WHY: with weapons set to disable, the crew will not fire on a ship that is already helpless.", 0},   // harness Nov 26
  {"WITH GAME FROZEN", "WHY CAN'T I CHANGE THIS NOW?", "HOW: tap FREEZE (or press L) to let time run again, then make the change. WHY: this setting cannot be changed while the game is paused.", "FREEZE"},  // [T] "CANNOT CHANGE WITH GAME FROZEN"
  {"AVAILABLE AT THIS RANK", "WHY CAN'T I USE THIS YET?", "HOW: carry on with your missions; this unlocks when you are promoted. WHY: your rank is too low for this command.", 0},  // [D][F][C] "FUNCTION NOT AVAILABLE AT THIS RANK" / "NOT AVAILABLE AT THIS RANK"
  {"No repair teams available", "WHY IS NOTHING BEING REPAIRED?", "HOW: tap SHIP MENUS, then ENGINEERING, open Damage Control and move a team from a less important system, or dock at a starfort. WHY: all your repair teams are already busy.", "SHIP MENUS"},  // [T][O]
};
