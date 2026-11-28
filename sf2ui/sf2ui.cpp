// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 the starfleet2-3ds authors
// sf2ui: Star Fleet II split-cockpit frontend. See sf2ui.h.
#include "sf2ui.h"
#include "draw.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#ifndef PORT_VERSION
#define PORT_VERSION "unknown"
#endif

namespace sf2ui {

// ------------------------------------------------------------------ colours
// house palette (claude/3ds-look-and-feel.md): navy, blue button faces, gold accents
static const uint16_t BG = rgb(8, 12, 40);
static const uint16_t FRAME = rgb(120, 150, 230);
static const uint16_t TITLE = rgb(255, 204, 64);
static const uint16_t BTN = rgb(40, 56, 120), BTN_EDGE = rgb(120, 150, 230), BTN_TXT = rgb(235, 240, 250);
static const uint16_t BTN_DOWN = rgb(220, 180, 60), BTN_SEL = rgb(70, 92, 170), HOT = rgb(255, 150, 120);
static const uint16_t PROMPT_BG = rgb(20, 30, 70), HINT = rgb(170, 185, 225), LOUPE = rgb(255, 204, 64);
static const uint16_t FOCUS = rgb(255, 204, 64), SHADOW = rgb(20, 30, 70);

// ------------------------------------------------------------------ state
static const Host* H = 0;

struct Ev { Key k; bool down; };
static std::vector<Ev> queue;          // pending key events
static uint32_t next_key_ms = 0;
static const uint32_t KEY_GAP_MS = 35; // spacing between injected key events

enum ActKind { A_NONE, A_GRID, A_STEPBACK, A_STEPSKIP, A_KEY, A_MODE, A_MANUAL, A_KB, A_SECTION, A_CLOSE, A_TYPE, A_SIGNON, A_SAVE, A_VIEW, A_PAUSE, A_QUIT, A_QUIT_YES, A_SETTINGS, A_LEFTY, A_SECLIST, A_ROWPAGE, A_RDLINE, A_MENUSCROLL, A_LAUNCH, A_ENGINES, A_MISSION, A_COACH, A_FOLLOW, A_LISTPICK, A_LEAVE, A_RDPAGE, A_STOPLIST };
struct Act { ActKind kind; Key key; uint8_t mods; int arg; };
enum { M_SHIFT = 1, M_CTRL = 2, M_ALT = 4 };

struct Btn {
  int x0, y0, x1, y1;
  std::string label; int hot; bool sel;
  const Font* font; Act act; bool on_top;
  bool off;                            // can't be used on this screen: greyed out, taps do nothing
  bool focus;                          // D-Pad focus (gold ring)
  bool coach;                          // the coach points at this button (blinking cyan ring)
};
static std::vector<Btn> btns;          // buttons drawn on the touch screen this frame
struct BtnId { int x0, y0, x1, y1; std::string label; bool valid; };
static BtnId pressed_id = {0, 0, 0, 0, "", false};   // button under the stylus
static BtnId flash_id = {0, 0, 0, 0, "", false};     // button just released (brief press look)
static uint32_t flash_until = 0;
static bool flash_on = false;       // a release flash is showing or just ended (needs redraws)
static BtnId id_of(const Btn& b) { BtnId i = {b.x0, b.y0, b.x1, b.y1, b.label, true}; return i; }
static bool same(const BtnId& i, const Btn& b) { return i.valid && i.x0 == b.x0 && i.y0 == b.y0 && i.x1 == b.x1 && i.y1 == b.y1 && i.label == b.label; }

enum TapMode { TM_AUTO, TM_STEER, TM_TARGET };
static TapMode tap_mode = TM_AUTO;     // tactical taps: AUTO = target when the game shows its target box, else steer
static const char* TAP_LABEL[3] = {"AUTO", "STEER", "TARGET"};
static bool manual_open = false, kb_open = false, kb_forced = false, kb_user_hidden = false;
static std::string toast; static uint32_t toast_until = 0;
static bool marquee = false;      // prompt strip is scrolling (needs regular redraws)

static bool dragging = false; static int drag_x, drag_y, drag_ly;
static int tap_x = -1, tap_y = -1;

// target box driver (closed loop: press an arrow, look where the box went, repeat)
// START menu (pause): resume, manual, quick save, game settings, exit.
static bool pause_open = false, quit_confirm = false;
static int pause_focus = 0;            // D-Pad focus in the START menu (confirm: 0 YES, 1 NO)
// Explain mode (plan item 6): hold R and tap anything on the touch screen; a card on the top screen
// says in plain words what it is. R pressed and released with no tap still changes the tap mode.
static bool r_held = false, r_used = false;
static uint32_t r_since = 0; static bool quickref_up = false;   // R held 0.6 s with no tap: quick reference
static std::string explain_title, explain_text; static uint32_t explain_until = 0;
// Busy feedback: a button was pressed and the game hasn't answered yet (see frame()).
static uint32_t busy_since = 0; static bool busy_now = false;
// Jumping straight to another view: close what's open (Esc) until the game is back at a view, then switch.
static int view_goal = -1, view_esc = 0; static uint32_t view_next = 0;
static bool tdb_drive = false; static int tdb_tc, tdb_tr, tdb_steps; static uint32_t tdb_next = 0; static uint32_t tdb_show_until = 0;
static bool tdb_enter_after = false; static int tdb_best = 9999; static int tdb_lc = -1, tdb_lr = -1; static uint32_t tdb_press_t = 0;

// ZR focus mode (rule 6): on the game screens the D-Pad and A belong to the game (arrow keys, Enter),
// so ZR hands them to the touch buttons for a while: the D-Pad moves a gold ring, A presses the ringed
// button, B or ZR hands them back. (fx, fy): centre of the ringed button.
static bool focus_mode = false; static int fx = 160, fy = 200;
static bool lefty_on = false, lefty_loaded = false;   // left-handed play (rule 10)
static bool reader_on = false;   // the touch screen shows the page's text (P2-1 reader)
static bool menu_scrolls = false; // a menu too long for the touch screen scrolls there
// Rule 14 alert strip: a light coming on (row 0) or a new COMMS line shows for 3 s where the eyes are
// (over the touch screen's prompt line), boxed in red (danger) or gold (news), always in words.
static std::string alert_text; static bool alert_red = false; static uint32_t alert_until = 0;
static void lefty_load();

static TextScreen last;                // last screen drawn (for change detection)
static uint32_t ui_version = 1, drawn_version = 0;
static bool last_text_mode = false;

// manual
#include "library_data.h"
// shelf: 0 START, 1 BRIDGE, 2 PROCEDURES, 3 PROBLEMS, 4 REFERENCE (the port's own Library cards,
// built into the app from assets/library-port.txt), 5 BOOKS (manual.txt: the port's guide and the
// game's manuals from the player's own GOG files).
struct Step { std::string btn, what, done; };   // a checklist step (PROCEDURES cards)
struct Section { std::string title; std::vector<std::string> paras, lines; int wrap = 0; int shelf = 5; std::vector<Step> steps; std::string next, done; };
// The checklist being followed (learning design step 3): shown on the top screen while playing;
// steps tick themselves from what the game shows or which button was pressed.
static bool tut1_done_loaded = false;
static int tut_num = 0; static bool tut_started = false;   // a tutorial chosen on the game's Tutorials menu (0 = none)
static int chk_sec = -1, chk_step = 0; static bool chk_had = false; static uint32_t chk_since = 0;
static std::string chk_pressed;         // label of the last touch button / hardware button pressed
static std::vector<Section> manual;
static int man_sel = 0, man_scroll = 0, man_list_top = 0;
static int man_focus = 0;   // D-Pad focus in the manual: 0 the section list, -1 LEFT HAND, -2 CLOSE MANUAL (rule 6)

static void dirty() { ui_version++; }

// ---- rule 16, remember the setup: the tap mode and the manual section the player last read, in
// one small file (settings.txt, "key=value" lines). Written only when a value actually changed
// (a tap-mode press, closing the manual), through the same background writer as lefty.txt.
static std::string set_manual;          // remembered manual section title
static std::string set_written;         // the file's contents as last read or written
// Mission card (plan item 7): read from the orders screen, remembered in settings.txt
static std::string mis_title, mis_goal; static int mis_days = 0;
static bool mis_show_pending = false;    // show the card on the next bridge screen
// Coach (plan item 8): step-by-step help through a first mission. On until switched off in the
// START menu (remembered as coach=0 in settings.txt).
static bool coach_on = true;
static int coach_card = -1;            // the coach step whose card is up (-1: none)
static int coach_seen = 0;               // bit per coach step already shown
static void load_settings() {
  FILE* f = H && H->settings_path ? fopen(H->settings_path, "rb") : 0;
  if (!f) return;
  char line[160];
  while (fgets(line, sizeof line, f)) {
    size_t n = strlen(line); while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
    set_written += std::string(line) + "\n";
    if (!strncmp(line, "tapmode=", 8)) {
      {}   // (the tap mode is no longer kept: a stray R press left newcomers in TARGET with steering dead)
    } else if (!strncmp(line, "manual=", 7)) set_manual = line + 7;
    else if (!strncmp(line, "mission=", 8)) mis_title = line + 8;
    else if (!strncmp(line, "goal=", 5)) mis_goal = line + 5;
    else if (!strncmp(line, "days=", 5)) mis_days = atoi(line + 5);
    else if (!strncmp(line, "coach=", 6)) coach_on = atoi(line + 6) != 0;
    else if (!strncmp(line, "tut1=", 5)) tut1_done_loaded = atoi(line + 5) != 0;
  }
  fclose(f);
}
static void manual_closed() {          // the section on screen when the manual closes is remembered
  if (man_sel >= 0 && man_sel < (int)manual.size()) set_manual = manual[man_sel].title;
}
#define tut1_done tut1_done_loaded       // Tutorial 1 was finished once: sign-on points at Practice next
static void save_settings() {
  if (!H || !H->settings_path) return;
  std::string text = std::string("tapmode=") + TAP_LABEL[tap_mode] + "\n";
  if (!set_manual.empty()) text += "manual=" + set_manual + "\n";
  if (!mis_goal.empty()) text += "mission=" + mis_title + "\ngoal=" + mis_goal + "\ndays=" + std::to_string(mis_days) + "\n";
  if (!coach_on) text += "coach=0\n";
  if (tut1_done) text += "tut1=1\n";
  if (text == set_written) return;     // nothing changed: no SD write
  set_written = text;
  if (H->save) H->save(H->settings_path, text.data(), (unsigned)text.size());   // (3DS: in the background)
  else if (FILE* f = fopen(H->settings_path, "wb")) { fwrite(text.data(), 1, text.size(), f); fclose(f); }
}
static bool swap_wanted = false;   // ZL swap is on (it may not apply to this screen)
static int top_w = TOP_W;   // width the top-screen panel is laid out for (320 when swapped)

// ---- fit checking: text that doesn't fit its space is recorded so it can be fixed
static std::vector<std::string> fit_log;
static void fit_problem(const std::string& what) {
  for (auto& f : fit_log) if (f == what) return;
  if (fit_log.size() < 64) fit_log.push_back(what);
}
static void fit_text(Surf& s, int x, int y, int x_max, const char* t, const Font& f, uint16_t c, const char* where) {
  if (x + text_width(t, f) > x_max) fit_problem(std::string(where) + ": \"" + t + "\"");
  draw_text(s, x, y, t, f, c);
}
static uint32_t now() { return H && H->ms ? H->ms() : 0; }

// ------------------------------------------------------------------ keys
static void push(Key k, bool d) { queue.push_back(Ev{k, d}); }
static void press(Key k, uint8_t mods = 0) {
  if (mods & M_SHIFT) push(K_LSHIFT, true);
  if (mods & M_CTRL) push(K_LCTRL, true);
  if (mods & M_ALT) push(K_LALT, true);
  push(k, true); push(k, false);
  if (mods & M_ALT) push(K_LALT, false);
  if (mods & M_CTRL) push(K_LCTRL, false);
  if (mods & M_SHIFT) push(K_LSHIFT, false);
}
static Key key_for_char(char c) {
  if (c >= 'a' && c <= 'z') return (Key)(K_A + (c - 'a'));
  if (c >= 'A' && c <= 'Z') return (Key)(K_A + (c - 'A'));
  if (c >= '0' && c <= '9') return (Key)(K_0 + (c - '0'));
  switch (c) {
    case ' ': return K_SPACE; case '-': return K_MINUS; case '=': return K_EQUALS;
    case '[': return K_LBRACKET; case ']': return K_RBRACKET; case ',': return K_COMMA;
    case '.': return K_PERIOD; case '/': return K_SLASH; case '`': case '~': return K_GRAVE;
    case ';': return K_SEMICOLON; case '\'': return K_QUOTE;
  }
  return K_NONE;
}
static void pump_keys() {
  uint32_t t = now();
  while (!queue.empty() && (int32_t)(t - next_key_ms) >= 0) {
    Ev e = queue.front(); queue.erase(queue.begin());
    if (H && H->key) H->key(e.k, e.down);
    next_key_ms = t + KEY_GAP_MS;
  }
}

static void say(const char* msg, uint32_t ms = 2200) { toast = msg; toast_until = now() + ms; dirty(); }
void notify(const char* msg) { say(msg, 3000); }

// ---- remembered officer (one-tap sign-on). Saved after a successful sign-on.
static std::string typed;                       // characters typed on the on-screen keyboard
static int typed_y = -1;                        // screen row they were typed on (a new prompt starts afresh)
static uint32_t pass_seen = 0;                  // when the password prompt appeared (auto sign-on waits)
static std::string off_name, off_pass;          // remembered officer
static std::string pend_name, pend_pass;        // typed at this sign-on, saved once it succeeds
static int autologin = 0;                       // 1 = name sent, waiting for the password prompt; 2 = password sent
static uint32_t pass_sent = 0; static int pass_tries = 0; static uint32_t pass_sig = 0;
// SF-09: the officer's password is not written to officer.txt as readable text. It is XORed with a
// key made from the name and hex-encoded ("enc1:..."). This is obfuscation only, so a glance at the
// file on the SD card does not show it; it is NOT encryption. It is the game's own DOS sign-on
// password (it protects nothing but your officer file) and the key is in this source.
static std::string pass_key(const std::string& name) { return "SF2-3DS|" + name; }
static std::string pass_encode(const std::string& name, const std::string& pw) {
  const std::string k = pass_key(name); std::string o = "enc1:"; char h[3];
  for (size_t i = 0; i < pw.size(); i++) { snprintf(h, sizeof h, "%02x", (unsigned)((unsigned char)pw[i] ^ (unsigned char)k[i % k.size()])); o += h; }
  return o;
}
static std::string pass_decode(const std::string& name, const std::string& line) {
  if (line.compare(0, 5, "enc1:") != 0) return line;           // an older file with a plain password: still read
  const std::string k = pass_key(name); std::string o;
  for (size_t i = 5, n = 0; i + 1 < line.size(); i += 2, n++) {
    unsigned v = 0; if (sscanf(line.c_str() + i, "%2x", &v) != 1) return "";
    o += (char)(v ^ (unsigned char)k[n % k.size()]);
  }
  return o;
}
static void load_officer() {
  off_name.clear(); off_pass.clear();
  FILE* f = H && H->profile_path ? fopen(H->profile_path, "rb") : 0;
  if (!f) return;
  char a[64] = "", b[160] = "";
  if (fgets(a, sizeof a, f) && !fgets(b, sizeof b, f)) b[0] = 0;
  fclose(f);
  for (char* p : {a, b}) { size_t n = strlen(p); while (n && (p[n - 1] == '\n' || p[n - 1] == '\r')) p[--n] = 0; }
  off_name = a; off_pass = pass_decode(a, b);
}
static void save_officer(const std::string& n, const std::string& p) {
  if (!H || !H->profile_path) return;
  std::string text = n + "\n" + pass_encode(n, p) + "\n";
  if (H->save) H->save(H->profile_path, text.data(), (unsigned)text.size());   // (3DS: in the background)
  else {
    FILE* f = fopen(H->profile_path, "wb");
    if (!f) return;
    fwrite(text.data(), 1, text.size(), f);
    fclose(f);
  }
  off_name = n; off_pass = p;
}
static void type_text(const std::string& t) {
  for (char c : t) { Key k = key_for_char(c); if (k != K_NONE) press(k); }
}

// ------------------------------------------------------------------ screen reading helpers
static std::string row_str(const TextScreen& ts, int r, int c0, int c1) {  // [c0, c1)
  std::string s;
  for (int c = c0; c < c1 && c < 80; c++) s += (char)ts.ch[r][c];
  return s;
}
static std::string trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \x00\xdb\xb3\xba", 0, 5), b = s.find_last_not_of(" \x00\xdb\xb3\xba", std::string::npos, 5);
  if (a == std::string::npos) return "";
  return s.substr(a, b - a + 1);
}
static bool screen_has(const TextScreen& ts, const char* needle, int r0 = 0, int r1 = 24) {
  for (int r = r0; r <= r1; r++) if (row_str(ts, r, 0, 80).find(needle) != std::string::npos) return true;
  return false;
}
static int parse_int_after(const std::string& s, size_t pos) {
  while (pos < s.size() && !(s[pos] == '-' || (s[pos] >= '0' && s[pos] <= '9'))) pos++;
  return pos < s.size() ? atoi(s.c_str() + pos) : 0;
}
static bool is_bridge(const TextScreen& ts) {
  // the custom layout lifts panels from fixed places, so every one of them must be present;
  // variants (Damage Control swaps ENERGY/VEL, overlay windows) use the full-screen view instead
  std::string r0 = row_str(ts, 0, 0, 80), r1 = row_str(ts, 1, 0, 80), r2 = row_str(ts, 2, 0, 20);
  std::string r9 = row_str(ts, 9, 59, 79), r14 = row_str(ts, 14, 40, 52), r17 = row_str(ts, 17, 40, 60);
  auto has = [](const std::string& h, const char* n) { return h.find(n) != std::string::npos; };
  return has(r0, "LOW PWR") && has(r1, "NAV") && (has(r1, "TACTICAL") || has(r1, "ORBITAL")) && has(r1, "VEL") &&
         has(r1, "DAMAGE") && has(r2, "LRS") && has(r9, "DATA") && (has(r14, "ss") || has(row_str(ts, 14, 30, 50), "TARGET ID")) && has(r17, "ENERGY");
}
// Any flight screen (the bridge and its variants: Damage Control, overlay windows) keeps the
// LOW PWR status line on row 0; reading screens never have it.
static bool in_flight(const TextScreen& ts) { return row_str(ts, 0, 0, 80).find("LOW PWR") != std::string::npos; }
static bool is_box(uint8_t c) { return c >= 0xB3 && c <= 0xDA; }
// The target designator box: a 3x3 frame of magenta box-drawing cells. Returns its centre.
static bool find_tdb(const TextScreen& ts, int& cc, int& cr, int c0 = 1, int r0 = 1, int c1 = 78, int r1 = 23) {
  for (int r = r0; r <= r1; r++)
    for (int c = c0; c <= c1; c++) {
      bool ok = true;
      for (int dr = -1; dr <= 1 && ok; dr++)
        for (int dc = -1; dc <= 1 && ok; dc++) {
          if (!dr && !dc) continue;
          int rr = r + dr, cx = c + dc;
          if (rr < 0 || rr > 24 || cx < 0 || cx > 79) { ok = false; break; }
          uint8_t a = ts.at[rr][cx] & 15;
          if (!(a == 5 || a == 13) || !is_box(ts.ch[rr][cx])) ok = false;
        }
      if (ok) { cc = c; cr = r; return true; }
    }
  return false;
}

// ------------------------------------------------------------------ sign-on
static bool at_name_prompt(const TextScreen& ts) {
  return screen_has(ts, "SIGN-ON SCREEN") && screen_has(ts, "Enter your name") && !screen_has(ts, "password");
}
static bool at_pass_prompt(const TextScreen& ts) { return screen_has(ts, "password:"); }
static std::string lower_str(std::string v) { for (auto& c : v) c = (char)tolower((unsigned char)c); return v; }
// The commander list (shown first when the game knows more than one commander): is the remembered
// officer on it, and how many rows below (+) or above (-) the highlighted row?
static bool signon_list(const TextScreen& ts, int* moves) {
  if (off_name.empty() || !ts.text_mode || !screen_has(ts, "Select Commander", 24, 24)) return false;
  int mine = -1, sel = -1, firstrow = -1;
  for (int r = 1; r <= 22; r++) {
    std::string row; int numcol = -1;
    for (int c = 0; c < 80; c++) {
      uint8_t ch = ts.ch[r][c]; row += (ch > 32 && ch < 127) ? (char)ch : ' ';
      if (numcol < 0 && ch >= '0' && ch <= '9') numcol = c;
    }
    int num = 0; char nm[40] = "";
    if (sscanf(row.c_str(), " %d %39s", &num, nm) != 2 || num < 1) continue;
    if (firstrow < 0) firstrow = r;
    if (lower_str(nm) == lower_str(off_name)) mine = r;
    if (numcol >= 0 && (ts.at[r][numcol] >> 4) != 0) sel = r;      // the highlighted number
  }
  if (mine < 0) return false;
  if (sel < 0) sel = firstrow;
  if (moves) *moves = mine - sel;
  return true;
}
// The password prompt is up, still empty, and the name above it is the remembered officer's.
static bool pass_waiting(const TextScreen& ts);
static bool signon_pass(const TextScreen& ts) {
  if (off_name.empty() || !ts.text_mode || !at_pass_prompt(ts) || !pass_waiting(ts)) return false;
  for (int r = 0; r < 25; r++) {
    std::string row = lower_str(row_str(ts, r, 0, 80));
    size_t q = row.find("enter your name:");
    if (q != std::string::npos) return trim(row.substr(q + 16)) == lower_str(off_name);
  }
  return false;
}
// The password prompt is still waiting: its field holds no '*' yet and no answer has come up.
// (Nov 18 2026: after a good sign-on the prompt line stays on screen under "Welcome aboard", and
// re-sending the password there typed "poop" into the next screens - "p" chose Practice.)
static bool pass_waiting(const TextScreen& ts) {
  for (int r = 0; r < 25; r++) {
    std::string row = row_str(ts, r, 0, 80);
    size_t k = row.find("password:");
    if (k != std::string::npos) {
      if (row.find('*', k) != std::string::npos) return false;
      break;
    }
  }
  return !screen_has(ts, "Welcome") && !screen_has(ts, "not on file") && !screen_has(ts, "PRESS ANY KEY");
}

// ------------------------------------------------------------------ widgets
// A message box the game draws over the screen (double-line frame, plain top and bottom edges).
// Found on every frame; its cells are left blank in the panels (they would show slices of it)
// and the message goes on a card on the top screen instead.
static int pop_r0 = -1, pop_c0, pop_r1, pop_c1;
static std::string pop_text;
static void find_popup(const TextScreen& ts) {
  pop_r0 = -1; pop_text.clear();
  if (!ts.text_mode) return;
  for (int r = 1; r <= 21; r++)
    for (int c = 0; c < 66; c++) {
      if (ts.ch[r][c] != 0xC9) continue;
      int c2 = c + 1; while (c2 < 80 && ts.ch[r][c2] == 0xCD) c2++;
      if (c2 >= 80 || ts.ch[r][c2] != 0xBB || c2 - c < 12) continue;
      for (int r2 = r + 2; r2 <= r + 8 && r2 <= 23; r2++) {
        if (ts.ch[r2][c] != 0xC8 || ts.ch[r2][c2] != 0xBC) continue;
        bool ok = true;
        // (the bottom edge may carry words: "====PRESS KEY====")
        int edge = 0; std::string edge_text;
        for (int k = c + 1; k < c2; k++) { if (ts.ch[r2][k] == 0xCD) edge++; else if (ts.ch[r2][k] >= 32 && ts.ch[r2][k] < 127) edge_text += (char)ts.ch[r2][k]; else ok = false; }
        if (edge * 10 < (c2 - c - 1) * 5) ok = false;
        for (int k = r + 1; k < r2; k++) if (ts.ch[k][c] != 0xBA || ts.ch[k][c2] != 0xBA) ok = false;
        if (!ok) break;
        pop_r0 = r; pop_c0 = c; pop_r1 = r2; pop_c1 = c2;
        for (int k = r + 1; k < r2; k++) {
          std::string l;
          for (int x = c + 1; x < c2; x++) l += (ts.ch[k][x] >= 32 && ts.ch[k][x] < 127) ? (char)ts.ch[k][x] : ' ';
          size_t a = l.find_first_not_of(' '), b = l.find_last_not_of(' ');
          if (a != std::string::npos) { if (!pop_text.empty()) pop_text += ' '; pop_text += l.substr(a, b - a + 1); }
        }
        if (!trim(edge_text).empty()) pop_text += " " + trim(edge_text);
        return;
      }
    }
}
static bool in_popup(int r, int c) { return pop_r0 >= 0 && r >= pop_r0 && r <= pop_r1 && c >= pop_c0 && c <= pop_c1; }
static void cells(Surf& s, const TextScreen& ts, int c0, int r0, int c1, int r1, const Font& f,
                  int x, int y, int cw, int lh) {
  for (int r = r0; r <= r1; r++)
    for (int c = c0; c <= c1; c++) {
      uint8_t a = ts.at[r][c];
      uint16_t bg = VGA[(a >> 4) & 15];
      uint16_t fg = ((a >> 4) & 15) == (a & 15) ? VGA[a & 15] : READ_FG[a & 15];
      // a row the game's message box lies over is left blank in this panel (the card has the message)
      bool hid = pop_r0 >= 0 && r >= pop_r0 && r <= pop_r1 && c1 >= pop_c0 && c0 <= pop_c1 && is_bridge(ts);
      draw_cell(s, x + (c - c0) * cw, y + (r - r0) * lh, cw, lh, hid ? ' ' : ts.ch[r][c], fg, hid ? VGA[0] : bg, f);
    }
}
// Cells centred in an inner box with the most line spacing that fits (font height .. +3).
static void cells_in(Surf& s, const TextScreen& ts, int c0, int r0, int c1, int r1, const Font& f,
                     int ix0, int iy0, int ix1, int iy1, int lead = 0) {
  int cols = c1 - c0 + 1, rows = r1 - r0 + 1;
  int lh = lead ? lead : (iy1 - iy0) / rows;
  if (!lead) { if (lh > f.h + 3) lh = f.h + 3; if (lh < f.h) lh = f.h; }
  int w = cols * f.w, hgt = rows * lh;
  char where[48];
  snprintf(where, sizeof where, "panel cols %d-%d rows %d-%d", c0, c1, r0, r1);
  if (w > ix1 - ix0 || hgt > iy1 - iy0) fit_problem(std::string(where) + " is bigger than its tile");
  for (int r = r0; r <= r1; r++) {      // a letter/digit on both sides of an edge = a sliced word
    if (in_popup(r, c0) || in_popup(r, c1)) continue;   // a message box lies over this edge
    if (c0 > 0 && isalnum(ts.ch[r][c0]) && isalnum(ts.ch[r][c0 - 1])) fit_problem(std::string(where) + " cuts a word at its left edge");
    if (c1 < 79 && isalnum(ts.ch[r][c1]) && isalnum(ts.ch[r][c1 + 1])) fit_problem(std::string(where) + " cuts a word at its right edge");
  }
  cells(s, ts, c0, r0, c1, r1, f, ix0 + (ix1 - ix0 - w) / 2, iy0 + (iy1 - iy0 - hgt) / 2, f.w, lh);
}
static void title_text(Surf& s, int x, int y, const char* t, uint16_t c) {
  draw_text(s, x, y - F6x12.ink_top, t, F6x12, c);
}
// A framed tile with an optional title in its top edge. Returns the inner box.
struct Box { int x0, y0, x1, y1; };
static Box tile(Surf& s, int x0, int y0, int x1, int y1, const char* title) {
  s.fill(x0, y0, x1, y1, BG);
  int top = y0 + (title ? 4 : 0);
  s.rect(x0, top, x1, y1, FRAME);
  if (title && *title) {
    int tw = text_width(title, F6x12), tx = x0 + (x1 - x0 - tw) / 2;
    if (tw > x1 - x0 - 6) fit_problem(std::string("tile title \"") + title + "\"");
    s.fill(tx - 2, y0, tx + tw + 2, y0 + 10, BG);
    title_text(s, tx, y0, title, TITLE);
    return Box{x0 + 2, y0 + 10, x1 - 2, y1 - 2};
  }
  return Box{x0 + 2, y0 + 2, x1 - 2, y1 - 2};
}

static void add_btn(int x0, int y0, int x1, int y1, const std::string& label, const Font* f, Act a,
                    int hot = -1, bool sel = false) {
  Btn b; b.x0 = x0; b.y0 = y0; b.x1 = x1; b.y1 = y1; b.label = label; b.hot = hot; b.sel = sel;
  b.font = f; b.act = a; b.on_top = false; b.off = false; b.focus = false; b.coach = false;
  btns.push_back(b);
}
static Act key_act(Key k, uint8_t mods = 0) { Act a; a.kind = A_KEY; a.key = k; a.mods = mods; a.arg = 0; return a; }
static Act kind_act(ActKind k, int arg = 0) { Act a; a.kind = k; a.key = K_NONE; a.mods = 0; a.arg = arg; return a; }

static const uint16_t OFF_BG = rgb(14, 16, 22), OFF_EDGE = rgb(36, 40, 50), OFF_TXT = rgb(72, 78, 92);
static void draw_btn(Surf& s, int i) {
  const Btn& b = btns[i];
  bool down = !b.off && (same(pressed_id, b) || (flash_on && same(flash_id, b) && (int32_t)(flash_until - now()) > 0));
  s.fill(b.x0, b.y0, b.x1, b.y1, BG);
  if (b.off) s.round_rect(b.x0 + 1, b.y0 + 1, b.x1 - 1, b.y1 - 1, OFF_BG, OFF_EDGE);
  else s.round_rect(b.x0 + 1, b.y0 + 1, b.x1 - 1, b.y1 - 1, down ? BTN_DOWN : (b.sel ? BTN_SEL : BTN), down ? BTN_TXT : BTN_EDGE);
  if (!b.off && !down) s.hline(b.x0 + 3, b.x1 - 2, b.y1 - 1, SHADOW);   // house style: raised edge (gone while pressed)
  // label: pick the biggest font that fits, never cut off
  const Font* f = b.font;
  if (text_width(b.label.c_str(), *f) > b.x1 - b.x0 - 4 && f != &F6x12) f = &F6x12;
  if (text_width(b.label.c_str(), *f) > b.x1 - b.x0 - 4) f = &F5x9;   // last resort (long manual titles)
  std::string lab = b.label;
  if (text_width(lab.c_str(), *f) > b.x1 - b.x0 - 4) {
    fit_problem("button \"" + lab + "\"");
    while (!lab.empty() && text_width(lab.c_str(), *f) > b.x1 - b.x0 - 4) lab.erase(lab.size() - 1);
  }
  if (b.focus && !b.off) {
    s.rect(b.x0, b.y0, b.x1, b.y1, FOCUS);
    s.rect(b.x0 + 1, b.y0 + 1, b.x1 - 1, b.y1 - 1, FOCUS);
  } else if (b.coach && !b.off && (now() / 500) % 2) {
    s.rect(b.x0, b.y0, b.x1, b.y1, rgb(80, 230, 255));
    s.rect(b.x0 + 1, b.y0 + 1, b.x1 - 1, b.y1 - 1, rgb(80, 230, 255));
  }
  int tx = b.x0 + (b.x1 - b.x0 - text_width(lab.c_str(), *f)) / 2 + (down ? 1 : 0);
  int ty = b.y0 + (b.y1 - b.y0 - f->h) / 2 + (down ? 1 : 0);
  for (size_t k = 0; k < lab.size(); k++) {
    char one[2] = {lab[k], 0};
    draw_text(s, tx + (int)k * f->w, ty, one, *f, b.off ? OFF_TXT : (int)k == b.hot ? HOT : BTN_TXT);
  }
}

static void split(int total, int n, int i, int& a, int& b) {
  int base = total / n, extra = total % n;
  a = i * base + (i < extra ? i : extra);
  b = a + base + (i < extra ? 1 : 0);
}
// Rule 11 (buttons >= 24 px, 6 px from the screen edges, 8 px apart; Ryan, Nov 20: 4 px only between
// the segmented view tabs): the touch screen is laid out as
//   upper area 0..UP_H (game text, maps, keyboard, the bridge's tactical display)
//   prompt strip UP_H..UP_H+12, menu row ROW1 (24 px), views row ROW2 (24 px, ends 6 px above the edge)
static const int UP_H = 166, ROW1_Y0 = 178, ROW1_Y1 = 202, ROW2_Y0 = 210, ROW2_Y1 = 234;
static const int GAP = 8, TAB_GAP = 4;
// The bridge's action column runs down beside the menu row, which then stops 8 px short of it.
static const int BRIDGE_ROW_X1 = 230;
// n buttons side by side between x0 and x1 with gap-px gaps: button i spans a..b
static void split_gap(int x0, int x1, int n, int i, int& a, int& b, int gap = GAP) {
  int w = x1 - x0 - gap * (n - 1);
  int base = w / n, extra = w % n;
  a = x0 + i * (base + gap) + (i < extra ? i : extra);
  b = a + base + (i < extra ? 1 : 0);
}

// ------------------------------------------------------------------ shared bottom rows
struct MenuItem { std::string label; int hot; bool sel; char key; uint8_t mods; };
// cells that separate menu items: box-drawing, blocks and shades, or the dark-grey filler
static bool menu_sep(const TextScreen& ts, int c) {
  uint8_t ch = ts.ch[24][c];
  return (ch >= 0xB0 && ch <= 0xDF) || ts.at[24][c] == 0x08;
}
static bool has_letter(const std::string& w) { for (char c : w) if (isalpha((unsigned char)c)) return true; return false; }
// Key hints in brackets on one row: "[M]: menu  [Esc]: exit  [Ctrl][M]: menu  [D]elete [CR]:accept".
static const Key VIEW_KEYS[7] = {K_N, K_S, K_W, K_X, K_F, K_O, K_L};
static const uint8_t VIEW_MODS[7] = {M_ALT, M_ALT, M_ALT, M_ALT, M_ALT, M_CTRL, M_CTRL};
static void parse_hints(std::string row, std::vector<MenuItem>& items, std::string* prompt) {
  for (auto& ch2 : row) if ((uint8_t)ch2 >= 0xB0 && (uint8_t)ch2 <= 0xDF) ch2 = ' ';   // drop border/filler cells
  size_t p = 0;
  uint8_t mods = 0;
  while ((p = row.find('[', p)) != std::string::npos) {
    size_t q = row.find(']', p);
    if (q == std::string::npos || q - p > 6) break;
    std::string key = row.substr(p + 1, q - p - 1), lab;
    std::string ku = key; for (auto& ch2 : ku) ch2 = (char)toupper((unsigned char)ch2);
    if (ku == "CTRL" || ku == "ALT" || ku == "SHFT" || ku == "SHIFT") {     // modifier for the next key
      mods |= ku == "CTRL" ? M_CTRL : ku == "ALT" ? M_ALT : M_SHIFT;
      p = q + 1;
      continue;
    }
    size_t t = q + 1;
    bool joined = key.size() == 1 && t < row.size() && isalpha((unsigned char)row[t]);   // "[D]elete"
    if (joined) lab = key;
    while (t < row.size() && (row[t] == ':' || row[t] == ' ' || row[t] == '=' || row[t] == '-')) t++;
    while (t < row.size() && row[t] != '[' && (uint8_t)row[t] >= ' ' && (uint8_t)row[t] < 0x7F &&
           !(row[t] == ' ' && t + 1 < row.size() && row[t + 1] == ' ')) lab += row[t++];
    MenuItem m; m.hot = -1; m.sel = false; m.key = 0; m.mods = mods; mods = 0;
    if (key == "Esc" || key == "ESC") m.key = 27;
    else if (key == "Enter" || key == "CR") m.key = 13;
    else if (key.size() == 1) m.key = key[0];
    if (m.key) {
      m.label = trim(lab).empty() ? key : trim(lab);
      if (m.label.size() > 10 && m.label.find(' ') != std::string::npos) m.label = m.label.substr(0, m.label.find(' '));   // a button, not a sentence
      for (auto& ch2 : m.label) if (ch2 >= 'a' && ch2 <= 'z') ch2 -= 32;
      if (joined) m.hot = 0;
      items.push_back(m);
      if (prompt && prompt->find('[') != std::string::npos) *prompt = trim(prompt->substr(0, prompt->find('[')));
    }
    p = q + 1;
  }
}
// Key hints listed one per row in a column ("[Enter]  Select mission for practice" / "[Esc]  Return
// to..." / "[V]  View orders..."): the keys the game waits for on that page. Needs an Enter or Esc
// among them and no modifier keys, so a page that only describes keys is left alone.
static std::vector<MenuItem> column_hints(const TextScreen& ts) {
  std::vector<MenuItem> best, cur; int col = -1;
  auto close = [&]() {
    bool anchor = false;
    for (auto& m : cur) if (m.key == 13 || m.key == 27) anchor = true;
    if (cur.size() >= 2 && cur.size() <= 6 && anchor && cur.size() > best.size()) best = cur;
    cur.clear(); col = -1;
  };
  for (int r = 1; r <= 23; r++) {
    std::string row = row_str(ts, r, 0, 80);
    size_t b = row.find('['), e = row.rfind(']');
    // a hint row: "[key] words", the words starting right after the last bracket
    size_t w = e == std::string::npos ? e : row.find_first_not_of(' ', e + 1);
    bool hint = b != std::string::npos && e != std::string::npos && e > b && w != std::string::npos && w - e <= 12 && isalnum((unsigned char)row[w]);
    if (!hint || (col >= 0 && (int)b != col)) close();
    if (!hint) continue;
    // (labels may be several spaces away: "[Enter]     Select mission")
    std::string packed = row.substr(0, e + 1) + " " + trim(row.substr(e + 1));
    std::vector<MenuItem> h; parse_hints(packed, h, 0);
    col = (int)b;
    for (auto& m : h) {
      if (m.mods) { cur.clear(); col = -2; break; }      // a list that describes key chords, not a prompt
      if (!(m.key == 13 || m.key == 27 || isalnum((unsigned char)m.key)) || !has_letter(m.label)) continue;
      if (m.key == 27) m.label = "BACK";
      cur.push_back(m);
    }
  }
  close();
  return best;
}
static std::vector<MenuItem> parse_menu(const TextScreen& ts, std::string& prompt) {
  std::vector<MenuItem> items;
  int c = 0;
  // prompt: text before the first item separator
  prompt = trim(row_str(ts, 24, 0, 24));
  c = 24;
  while (c < 80) {
    uint8_t a = ts.at[24][c], ch = ts.ch[24][c];
    (void)a;
    if (ch != ' ' && ch != 0 && !menu_sep(ts, c)) {
      int s = c;
      while (c < 80 && !menu_sep(ts, c)) c++;
      std::string w = trim(row_str(ts, 24, s, c));
      if (w.size() >= 2 && has_letter(w) && w.find('[') == std::string::npos) {
        MenuItem m; m.label = w; m.hot = 0; m.sel = false; m.key = 0; m.mods = 0;
        int lead = 0; while (s + lead < c && ts.ch[24][s + lead] == ' ') lead++;
        for (int k = s; k < c; k++) if (ts.at[24][k] == 0x70 || ts.at[24][k] == 0x74) m.sel = true;
        for (size_t k = 0; k < w.size(); k++)
          if ((ts.at[24][s + lead + k] & 15) == 12 || ts.at[24][s + lead + k] == 0x74) { m.hot = (int)k; m.key = w[k]; break; }
        if (!m.key) for (size_t k = 0; k < w.size(); k++) if (w[k] >= 'A' && w[k] <= 'Z') { m.hot = (int)k; m.key = w[k]; break; }
        for (auto& ch2 : m.label) if (ch2 >= 'a' && ch2 <= 'z') ch2 -= 32;
        items.push_back(m);
      }
    }
    c++;
  }
  int dividers = 0;
  for (int k = 24; k < 80; k++) if (ts.at[24][k] == 0x08 || ts.ch[24][k] == 0xB3 || ts.ch[24][k] == 0xBA) dividers++;
  // long words are prose, not a menu - except under the game's own "Select option" ("Launch Control | Operations")
  if (prompt.compare(0, 13, "Select option") != 0) for (auto& m : items) if (m.label.size() > 12) dividers = 0;
  if (row_str(ts, 24, 0, 80).find("]:") != std::string::npos) dividers = 0;   // an open window's key hints win
  if (items.size() < 2 || dividers < 2) {
    items.clear();
    std::string row = row_str(ts, 24, 0, 80);
    prompt = trim(row);
    // key hints in brackets: "[M]: menu  [Esc]: exit  [Ctrl][M]: menu"
    { int e = 0; while (e < 80 && !menu_sep(ts, e)) e++;      // the prompt is the text before the first separator
      prompt = trim(row_str(ts, 24, 0, e)); }
    parse_hints(row, items, &prompt);
  }
  // A window open over the screen with its own key hints ("[D]elete [S]wap [R]eset [CR]:accept"):
  // those keys are what the game is waiting for, so they win over the menu row.
  for (int r = 23; r >= 1; r--) {
    std::vector<MenuItem> h;
    parse_hints(row_str(ts, r, 0, 80), h, 0);
    bool words = true; for (auto& m : h) if (!has_letter(m.label)) words = false;   // ("[\x18]/[\x19]" is not a set of buttons)
    if (h.size() >= 2 && words) { items = h; break; }
  }
  { std::vector<MenuItem> v = column_hints(ts); if (!v.empty()) items = v; }
  // The Targeter window: its speed is set with the function keys, which it does not mention.
  // (keys 1-4 here stand for F3, F4, F1, F2: sublight -/+ and hyperspace -/+)
  if (screen_has(ts, "TARGETER") && screen_has(ts, "Velocity:") && screen_has(ts, "enter target")) {
    bool hyper = false;
    for (int r = 1; r < 24; r++) { std::string row = row_str(ts, r, 0, 80); size_t q = row.find("Max. Vel"); if (q != std::string::npos && atof(row.c_str() + row.find(':', q) + 1) > 1.0) hyper = true; }
    MenuItem a; a.hot = -1; a.sel = false; a.mods = 0; a.label = "SPD-"; a.key = hyper ? 3 : 1;
    MenuItem b = a; b.label = "SPD+"; b.key = hyper ? 4 : 2;
    items.insert(items.begin(), b); items.insert(items.begin(), a);
  }
  // "Select target" on the bridge display (weapons, tractor beam, scanners, COMMS > Enemy,
  // pursuit): the menu row left behind it is dead; ENTER picks what the box is on, BACK gives up.
  // (capture 3: no ENTER, and the old menu buttons stayed)
  { std::string d = row_str(ts, 1, 26, 50);
    if ((prompt.compare(0, 13, "Select target") == 0 || prompt.compare(0, 21, "Select pursuit target") == 0) &&
        (d.find("TACTICAL") != std::string::npos || d.find("ORBITAL") != std::string::npos) &&
        [&] { int c, r; return find_tdb(ts, c, r, 28, 2, 49, 13); }()) {     // (a stale prompt after the lock has no box)
      items.clear();
      MenuItem e; e.hot = -1; e.sel = false; e.mods = 0; e.label = "ENTER"; e.key = 13;
      MenuItem x = e; x.label = "BACK"; x.key = 27;
      items.push_back(e); items.push_back(x);
    } }
  return items;
}
static bool window_hints(const TextScreen& ts) {     // a window with its own keys is open
  if (!column_hints(ts).empty()) return true;
  for (int r = 23; r >= 1; r--) {
    std::vector<MenuItem> h; parse_hints(row_str(ts, r, 0, 80), h, 0);
    bool words = true; for (auto& m : h) if (!has_letter(m.label)) words = false;
    if (h.size() >= 2 && words) return true;
  }
  return false;
}

static bool cursor_question(const TextScreen& ts);
// A yes/no question shown anywhere on rows 1-23 ("Do you want to switch to Practice mode?"), whatever
// its colours (hardware, Nov 20: the Practice question came up inside the menu screen with no YES/NO
// buttons). A short line that ends in '?' (or '?' then a [Y]/[N] hint) and asks Do/Are you / want to.
// A one-line question in its own small box (border above and below): "Select manual target?",
// "Detonate all probes on station?". (Cold read 2: such a box had no YES/NO and locked the game.)
static std::string boxed_question(const TextScreen& ts) {
  for (int r = 2; r < 23; r++)
    for (int c = 3; c < 78; c++) {
      if (ts.ch[r][c] != '?') continue;
      int cl = c; while (cl > 0 && !(ts.ch[r][cl] >= 0xB0 && ts.ch[r][cl] <= 0xDF)) cl--;
      int cr = c + 1; while (cr < 79 && ts.ch[r][cr] == ' ') cr++;
      if (cl <= 0 || c - cl < 8 || !(ts.ch[r][cr] >= 0xB0 && ts.ch[r][cr] <= 0xDF)) continue;
      int up = 0, dn = 0;
      for (int x = cl + 1; x < cr; x++) { uint8_t u = ts.ch[r - 1][x], d = ts.ch[r + 1][x]; if (u == 0xC4 || u == 0xCD) up++; if (d == 0xC4 || d == 0xCD) dn++; }
      if (up * 10 < (cr - cl - 1) * 7 || dn * 10 < (cr - cl - 1) * 5) continue;
      return trim(row_str(ts, r, cl + 1, cr));
    }
  return "";
}
static std::string dialog_question(const TextScreen& ts) {
  { std::string bq = boxed_question(ts); if (!bq.empty()) return bq; }
  // On the bridge an answered question stays in COMMS; the game is only asking while its prompt
  // field says so (harness, Nov 21: YES/NO stayed up after "Do you wish to be resupplied?" was answered).
  // (a question in a message box, like the starfort autopilot's, is still a question)
  if (is_bridge(ts) && trim(row_str(ts, 24, 0, 24)).compare(0, 13, "Select option") == 0) { find_popup(ts); if (pop_r0 < 0) return ""; }
  for (int r = 1; r < 24; r++) {
    // a question in a window over another screen (the bridge's "Do you wish to go to nearest
    // starfort?") shares its row with panel text: look at each stretch between frame lines
    std::string full = row_str(ts, r, 0, 80);
    std::vector<std::string> parts; std::string cur;
    for (char c : full) { uint8_t u = (uint8_t)c; if (u >= 0xB0 && u <= 0xDF) { parts.push_back(cur); cur.clear(); } else cur += c; }
    parts.push_back(cur);
    for (auto& part : parts) {
    std::string row = trim(part);
    size_t q = row.rfind('?');
    if (q == std::string::npos || row.size() > 70) continue;
    std::string tail = row.substr(q + 1);
    bool end_ok = trim(tail).empty() || tail.find("[Y") != std::string::npos || tail.find("[N") != std::string::npos ||
                  tail.find("(Y") != std::string::npos;
    if (!end_ok) continue;
    if (row.find("Do you") != std::string::npos || row.find("Are you") != std::string::npos ||
        row.find("want to") != std::string::npos || row.find("Overwrite") != std::string::npos ||
        row.find("overwrite") != std::string::npos || row.find("Replace") != std::string::npos ||
        row.find("Display planetary map") != std::string::npos)
      return row;
    }
  }
  return "";
}
static int glay_kind();   // the generic layout's kind (G_PLAIN / G_MENU / G_MAP, defined below)
// What the touch screen's button row offers right now (set by bottom_rows; the idle redraw of the
// prompt strip reuses it, which is safe because that path only runs while the screen is unchanged).
enum RowMode { R_DEFAULT, R_MENU, R_WINDOW, R_YESNO, R_ANYKEY, R_SIGNON };
static RowMode row_mode = R_DEFAULT;
static int row_page = 0;   // page of the game's menu row shown (MORE moves on; P2-6)
// Newcomer guidance: one short line saying what to do on this screen (at most 52 characters so it
// fits the touch screen's prompt strip without scrolling; the top screen's hint strip shows it too).
enum Situation { SIT_UNKNOWN, SIT_DOCKED, SIT_CRUISE, SIT_ALERT, SIT_ORBIT };
static Situation bridge_situation(const TextScreen& ts);
static bool in_orbit(const TextScreen& ts);
// 0 none, 1 ORANGE (in enemy or neutral space, nobody near: shields stay down), 2 RED (a fight)
static int alert_level(const TextScreen& ts) {
  std::string a0 = row_str(ts, 15, 40, 52), a1 = row_str(ts, 16, 40, 52);
  std::string w = a1.find("ALERT") != std::string::npos ? a0 : a0.find("STATUS") != std::string::npos ? a1 : std::string();
  return w.find("RED") != std::string::npos ? 2 : w.find("ORANGE") != std::string::npos ? 1 : 0;
}
// The clock is frozen: the game changes nothing but the colour of the mission time (blinking white).
static bool frozen(const TextScreen& ts) {
  for (int c = 11; c <= 16; c++) if (ts.at[11][c] == 0x8F) return true;
  return false;
}
// The landing-party table (teleporters): 0 not up, 1 assigning teams, 2 recalling teams.
static int lp_table(const TextScreen& ts) {
  if (row_str(ts, 15, 40, 80).find("Mission") == std::string::npos || row_str(ts, 15, 40, 80).find("Tms") == std::string::npos) return 0;
  std::string p = row_str(ts, 24, 0, 30);
  return p.find("teams to stay") != std::string::npos ? 2 : p.find("Assign mission") != std::string::npos ? 1 : 0;
}
// The damage control table (Engineering > Damage control): the arrows pick a system, + and - its
// repair teams, Enter assigns them (capture 3, armed_11..14).
static bool dc_table(const TextScreen& ts) {
  return row_str(ts, 24, 0, 24).find("Assign repair teams") != std::string::npos && screen_has(ts, "Avl.Repair Teams");
}
// Which weapons have a lock: the Wpn letters in TARGET DATA (bit 0 phasers, bit 1 torpedoes).
static int weapon_locks(const TextScreen& ts) {
  if (row_str(ts, 9, 59, 79).find("TARGET DATA") == std::string::npos) return 0;
  int m = 0;
  for (int r = 11; r <= 14; r++) { std::string w = row_str(ts, r, 74, 79); if (w.find('P') != std::string::npos) m |= 1; if (w.find('T') != std::string::npos) m |= 2; }
  return m;
}
static bool practice_on(const TextScreen& ts) {     // the sign-on menu's settings block: "Practice Mode  On"
  for (int r = 14; r < 24; r++) { std::string row = row_str(ts, r, 0, 80); size_t p = row.find("Practice Mode");
    if (p != std::string::npos) return trim(row.substr(p + 13, 12)).compare(0, 2, "On") == 0; }
  return false;
}
static bool has_btn(const char* label) { for (auto& b : btns) if (b.label == label) return true; return false; }
static const char* context_hint(const TextScreen& ts) {
  if (!lefty_loaded) lefty_load();
  if (focus_mode) return lefty_on ? "Ring: ABXY move, D-Rt press, D-Dn or ZR done" : "Ring: D-Pad move, A press, B or ZR done";
  if (!ts.text_mode) return lefty_on ? "Tap CONTINUE or press D-Right" : "Tap CONTINUE or press A";
  if (is_bridge(ts) && row_mode != R_YESNO) {
    int c, r;
    bool box = find_tdb(ts, c, r, 29, 2, 49, 13);
    bool target = tap_mode == TM_TARGET || (tap_mode == TM_AUTO && box);
    if (target) return box ? "Tap a ship: the box jumps there. ENTER picks it" : "No target box (Weapons: Manual)";
    switch (bridge_situation(ts)) {   // the next step for this situation (plan item 4)
      case SIT_DOCKED: return "Docked: LAUNCH leaves the starfort";
      case SIT_ALERT:
        if (alert_level(ts) == 2) return lefty_on ? "Red alert! D-Lt phasers, D-Up torps, L freeze" : "Red alert! X phasers, Y torpedoes, L freeze";
        return "Orange: Alliance space. Tap the display to steer";
      case SIT_ORBIT: return "In orbit. Tap the planet: landing parties";
      default: break;
    }
    if (row_str(ts, 24, 0, 24).find("PRESS") != std::string::npos) return "Read COMMS (top screen), then tap CONTINUE";
    if (frozen(ts)) return "FROZEN: the clock is stopped. L carries on";
    if (row_str(ts, 24, 0, 24).find("Select target") != std::string::npos) return "Tap a ship: the box goes there. ENTER picks it";
    return "Tap the display to steer the ship";
  }
  if (!kb_open && autologin == 0 && signon_list(ts, 0)) return "Tap SIGN ON AS, or tap a commander's name";
  if (kb_open) {
    if (at_name_prompt(ts)) return off_name.empty() ? "Type your officer name, then tap ENTER"
                                                    : "Tap SIGN ON AS, or type a new name and ENTER";
    if (signon_pass(ts)) return "Tap SIGN ON AS, or type the password and ENTER";
    if (at_pass_prompt(ts)) return "Type a password (new recruits choose one), ENTER";
    return "Type on the keyboard, then tap ENTER";
  }
  if (row_mode == R_YESNO) {
    static const struct { const char* q; const char* tip; } TIPS[] = {
      {"recruit", "First time? Tap YES to enlist as a new recruit"},
      {"disable rather than destroy", "YES: a disabled ship can be captured"},
      {"Make changes", "NO sends them as planned; YES reopens the table"},
      {"conclude the mission", "YES ends the mission and shows your score"},
      {"want to delete them", "YES is fine: old mission files are cleared"},
      {"mission subdirectory", "NO is fine"},
      {"switch to Practice mode", "YES: Practice does not count on your record"},
      {"planetary map", "NO is fine (YES shows a picture of the planet)"},
      {"landing parties to this planet", "NO is fine on a first visit"},
      {"Beam landing party up", "YES: bring teams back. NO: send more teams"},
      {"claim victory", "YES: the game tells you how to end the mission"},
      {"Confirm firing", "YES drops the pod: it wrecks the planet below"},
      {"Detonate all probes", "YES clears used probes so stored ones load"},
      {"Exit to DOS", "NO keeps playing. YES restarts at sign-on"},
      {"be resupplied", "YES tops up your supplies"},
      {"Establish orbit", "YES: orbit is where the planet is scanned"},
      {"Shall we dock", "YES: the starfort repairs and resupplies"},
      {"nearest starfort", "YES: the ship flies home by itself"},
      {"Comply with the request", "YES is the usual answer"},
      {"give up on this mission", "NO keeps it for later; YES throws it away"},
      {"want to stop", "YES leaves the mission (a choice follows)"},
      {"Select manual target", "YES: then tap the ship to aim at"},
    };
    for (auto& tp : TIPS) if (screen_has(ts, tp.q)) return tp.tip;
    return "Tap YES or NO";
  }
  if (in_flight(ts) == false && row_str(ts, 24, 24, 80).find("Personal") != std::string::npos && row_str(ts, 24, 24, 80).find("Krellan") != std::string::npos)
    return "Your mission score. BACK (or B) closes it";
  if (screen_has(ts, "COMMANDER'S LOG") && screen_has(ts, "PLEASE STANDBY")) return "The ship's log opens once at the start: tap EXIT";
  if (glay_kind() == 2 && in_flight(ts) && !has_btn("TARGETER") && has_btn("ENTER"))
    return "Tap where you want to arrive, then ENTER";
  if (screen_has(ts, "SIGN-ON OPTIONS")) return practice_on(ts) ? "Practice is on: tap Start a new mission" : tut1_done ? "Next: Practice (demo) mission. SELECT: what now?" : "New? Tap Tutorials, then 1. Help: START > MANUAL";
  if (screen_has(ts, "MISSIONS RECORD", 0, 3)) return has_btn("SELECT") ? "A mission is highlighted: tap the SELECT key below" : "Your career, one square per mission. B goes back";
  if (row_mode == R_WINDOW) return "Tap one of the keys below";
  if (glay_kind() == 1) return menu_scrolls ? "Tap an item; drag or tap the arrows for more" : "Tap an item to choose it";
  if (chk_sec >= 0 && chk_step < (int)manual[chk_sec].steps.size() && in_flight(ts)) {
    static std::string stepline; stepline = "\x10 " + manual[chk_sec].steps[chk_step].what;
    if (stepline.size() <= 52) return stepline.c_str();
  }
  if (glay_kind() == 2) return screen_has(ts, "MAP OF STAR SYSTEM") ? "Tap a planet or starfort, then tap TARGETER" : "Tap a star: the box jumps to it. Then TARGETER";
  if (row_str(ts, 1, 0, 80).find("COMBAT") != std::string::npos && row_str(ts, 1, 0, 80).find("INFORMATION") != std::string::npos)
    return "Combat screen: ships near you. B: back to the bridge";
  if (lp_table(ts) == 1) return "+ TEAM adds 10 people to the lit row";
  if (lp_table(ts) == 2) return "- TEAM brings a team back; ALL BACK: everyone";
  if (glay_kind() == 3 && dc_table(ts)) return "Arrows pick a system, + TEAM adds, ASSIGN";
  if (glay_kind() == 3) return "The game's window. Its keys are the buttons below";
  if (row_mode == R_ANYKEY) return lefty_on ? "Read the screen, then tap CONTINUE (or D-Rt)" : "Read the screen, then tap CONTINUE (or A)";
  { bool blank = true; for (int r = 0; r < 25 && blank; r++) for (int c = 0; c < 80; c++) if (ts.ch[r][c] > ' ' && ts.ch[r][c] != 0xFF) { blank = false; break; }
    if (blank) return "Loading..."; }
  if (screen_has(ts, "COMMANDER'S LOG", 1, 1)) return lefty_on ? "Your ship's diary: read it, then EXIT (or D-Dn)" : "Your ship's diary: read it, then EXIT (or B)";
  if (in_flight(ts)) return lefty_on ? "The buttons are this page's keys. D-Dn backs out" : "The buttons are this page's keys. B backs out";
  if (reader_on) return lefty_on ? "Drag or sticks to scroll \xb3 D-Rt: Enter \xb3 D-Dn: back" : "Drag or sticks to scroll \xb3 A: Enter \xb3 B: back";
  return lefty_on ? "Sticks scroll the page \xb3 D-Rt: Enter \xb3 D-Dn: back" : "Sticks scroll the page \xb3 A: Enter \xb3 B: back";
}
// Prompt strip (y UP_H..UP_H+12): the game's question, a toast, or the line being typed; scrolls when long.
// On the bridge it stops at x 236 (the action buttons run down to y 190 beside it).
static int strip_x1 = BOT_W;
static void prompt_strip(Surf& s, const TextScreen& ts, const std::string& prompt) {
  strip_x1 = is_bridge(ts) ? 236 : BOT_W;
  const int SY = UP_H;
  s.fill(0, SY, strip_x1, SY + 12, PROMPT_BG);
  s.fill(0, SY + 12, strip_x1, ROW1_Y0, BG);   // (on the bridge the action column runs down beside it)
  std::string p24 = trim(row_str(ts, 24, 0, 24));
  bool q_up = !dialog_question(ts).empty() || p24.find('?') != std::string::npos ||
              (p24.compare(0, 7, "Select ") == 0 && p24 != "Select option");   // ("Select torp #2 setting" was hidden by MSG:)
  if (!alert_text.empty() && (int32_t)(alert_until - now()) > 0 && !(q_up && !alert_red)) {     // (a question is not hidden by a passing message)
    uint16_t edge = alert_red ? rgb(255, 80, 64) : TITLE;
    s.fill(0, SY, strip_x1, ROW1_Y0, alert_red ? rgb(70, 12, 10) : rgb(60, 46, 8));
    s.rect(0, SY, strip_x1, ROW1_Y0, edge);
    const Font* f = text_width(alert_text.c_str(), F6x12) <= strip_x1 - 8 ? &F6x12 : &F5x9;
    fit_text(s, 4, SY + 1 + (12 - f->h) / 2, strip_x1 - 4, alert_text.c_str(), *f, alert_red ? rgb(255, 235, 230) : TITLE, "alert strip");
    marquee = false;
    return;
  }
  if (busy_now) {                                // moving bar: the game is still working on the press
    draw_text(s, 4, SY, "Working...", F6x12, LOUPE);
    int x0 = 74, x1 = strip_x1 - 6, w = 40, span = x1 - x0 - w;
    int pos = (int)((now() / 12) % (uint32_t)(2 * span)); if (pos > span) pos = 2 * span - pos;
    s.fill(x0, SY + 4, x1, SY + 8, BTN);
    s.fill(x0 + pos, SY + 3, x0 + pos + w, SY + 9, LOUPE);
    marquee = false;
    return;
  }
  bool showing_toast = !toast.empty() && (int32_t)(toast_until - now()) > 0;
  std::string line = showing_toast ? toast : prompt;
  if (!showing_toast && kb_open && ts.cursor_y >= 0 && ts.cursor_y < 25) line = trim(row_str(ts, ts.cursor_y, 0, 80));
  // (saving by name: the game's box lies under the bridge panels, so the cursor's row is a mix of them)
  if (!showing_toast && kb_open && screen_has(ts, "Enter file name")) line = "Save as: " + typed + "_";
  if (!showing_toast && !kb_open && row_str(ts, 14, 30, 50).find("TARGET ID") != std::string::npos) {
    // (capture 3: "Box is on: UGA freighter CORNHUSKER O:M P: 4" was cut at both ends; the orbit
    // and power fields are in TARGET DATA already)
    std::string id = trim(row_str(ts, 15, 16, 62)); size_t o = id.find(" O:");
    if (o != std::string::npos) id = trim(id.substr(0, o));
    if (id.compare(0, 4, "UGA ") == 0) id = id.substr(4);
    line = id.empty() ? "The box is on nothing yet" : "Box on: " + id + " (ENTER)";
  }
  if (!showing_toast && row_mode == R_YESNO) {    // the question itself, readable beside YES/NO
    std::string q = dialog_question(ts);           // (on the bridge its window is cut by the tiles)
    if (!q.empty()) line = q;
  }
  uint16_t pc = showing_toast ? LOUPE : READ_FG[ts.at[24][0] & 15 ? ts.at[24][0] & 15 : 14];
  if (line.empty()) { line = context_hint(ts); pc = HINT; }   // nothing asked: say what to do here
  int lw = text_width(line.c_str(), F6x12);
  if (lw <= strip_x1 - 8) {
    draw_text(s, 4, UP_H, line.c_str(), F6x12, pc);
    marquee = false;
  } else {
    // scroll: pause at the start, glide to the end, pause, repeat
    int travel = lw - (strip_x1 - 8), pause_ms = 1200, glide_ms = travel * 25;
    int cycle = (int)(now() % (uint32_t)(2 * pause_ms + glide_ms)), off;
    off = cycle < pause_ms ? 0 : cycle < pause_ms + glide_ms ? (cycle - pause_ms) / 25 : travel;
    Surf clip = s; (void)clip;
    for (size_t k = 0; k < line.size(); k++) {
      int cx = 4 - off + (int)k * 6;
      if (cx < 0 || cx + 6 > strip_x1 - 4) continue;   // whole characters only
      char one[2] = {line[k], 0};
      draw_text(s, cx, UP_H, one, F6x12, pc);
    }
    marquee = true;
  }
}
// The game's abbreviated menu words in full, for the bridge's menu grid (UI review 5: "NAV ENG CMB
// CMP SEC" and "DAM CON / TELEPRT" are cryptic, and paging with MORE hides half of them).
struct FullWord { const char* abbr; const char* full; };
static const FullWord FULLWORDS[] = {
  {"NAV", "NAVIGATION"}, {"ENG", "ENGINEERING"}, {"CMB", "COMBAT"}, {"CMP", "COMPUTER"}, {"SEC", "SECURITY"},
  {"COM", "COMMS"}, {"STG", "STRATEGY"}, {"SCI", "SCIENCE"}, {"CTL", "CONTROLS"},
  {"DAM CON", "DAMAGE CONTROL"}, {"TELEPRT", "TELEPORTERS"}, {"TRAC BM", "TRACTOR BEAM"}, {"CMBT STATS", "COMBAT STATUS"},
  {"INFORMATN", "INFORMATION"}, {"LANDING PRTY", "LANDING PARTY"}, {"BOARDNG PRTY", "BOARDING PARTY"},
  {"SPCL SRVCS", "SPECIAL SERVICES"}, {"FLT HQ", "FLEET HQ"}, {"CMB LOG", "COMBAT LOG"}, {"WEAPONSCTL", "WEAPONS CONTROL"},
  {"RNDVZING", "RENDEZVOUS"}, {"STRG CMD", "STRATEGIC CMD"}, {"PWR SCHM", "POWER SCHEMATIC"}, {"PWR SYS", "POWER SYSTEMS"}, {"ENGDATA", "ENGINE DATA"}, {"EMHYPMV", "EMERG. HYPERSPACE"}, {"INTRNL DEF", "INTERNAL DEFENCE"},
  // (the Weapons menu's "Phasers" sat beside the fire button PHASERS: capture 3)
  {"PHASERS", "PHASER SETUP"}, {"TORPEDOES", "TORPEDO SETUP"},
};
static std::string full_word(const std::string& l) { for (auto& w : FULLWORDS) if (l == w.abbr) return w.full; return l; }
static std::string abbr_word(const std::string& l) { for (auto& w : FULLWORDS) if (l == w.full) return w.abbr; return l; }
static bool mission_on = false;          // a mission is being flown (set by the lights row, cleared at sign-on)
static bool save_pending = false;
static uint32_t btn_sig = 0, btn_sig_t = 0;          // the buttons on screen, and when they last changed
static uint32_t leave_until = 0, leave_next = 0;
static bool leave_xdos = false;   // a finished tutorial: leave by the game's Exit to DOS; the start script runs the game again     // START > LEAVE: the port is walking the game's Quit menu
static bool tut_finished = false;                    // the tutorial's checklists are done: SELECT says how to leave
static bool goal_met = false;                        // the game said the mission's minimum is met
static void rd_page(int dir);
static TextScreen bridge_keep; static bool bridge_kept = false;   // the last full bridge screen (for the top screen under a window)
static bool grid_open = false, grid_shown = false;   // the bridge's menu grid: asked for (main menu) / on screen now
static void bottom_rows(Surf& s, const TextScreen& ts) {
  grid_shown = false;
  // prompt strip (UP_H..UP_H+12), menu row (ROW1), views row (ROW2)
  s.fill(0, UP_H + 12, BOT_W, BOT_H, BG);   // the gaps between the rows too (no stale pixels)
  std::string prompt;
  std::vector<MenuItem> items = parse_menu(ts, prompt);

  bool yesno = (prompt.find('?') != std::string::npos && items.empty()) || cursor_question(ts);
  // A question alone on an otherwise empty screen ("Do you wish to give up on this mission?" after
  // Quit): the game takes only Y or N (emulator, Nov 21: no buttons, the player was stuck).
  if (!yesno) {
    int rows_used = 0; std::string q;
    for (int r = 0; r < 25; r++) { std::string t = trim(row_str(ts, r, 0, 80)); if (!t.empty()) { rows_used++; if (t.back() == '?') q = t; } }
    if (rows_used <= 2 && !q.empty()) { yesno = true; items.clear(); prompt = q; }
  }
  // On the bridge the helm's questions ("Establish orbit?", "Shall we dock?") come up in the prompt
  // field while the menu row stays drawn beside them; the game then takes only Y or N.
  // (Emulator, Nov 21: no YES/NO buttons there, so the player was stuck.)
  if (!yesno && is_bridge(ts)) { std::string q = trim(row_str(ts, 24, 0, 24)); if (!q.empty() && q.back() == '?') { yesno = true; prompt = q; } }
  { std::string dq = dialog_question(ts); if (!dq.empty()) { yesno = true; prompt = dq; } }
  if (yesno) items.clear();
  for (int r = 1; r < 24 && !yesno; r++) {        // a dialog asking a question ("Do you still want to exit?")
    std::string row = row_str(ts, r, 0, 80);
    size_t q = row.find('?');
    int bg = q == std::string::npos ? 0 : (ts.at[r][q] >> 4) & 7;
    if (q != std::string::npos && (bg == 4 || bg == 7) &&      // inside a red or grey dialog box
        (row.find("Do you") != std::string::npos || row.find("Are you") != std::string::npos || row.find("want to") != std::string::npos)) {
      yesno = true; items.clear();
    }
  }
  if (!yesno && items.empty())
    for (int r = 1; r < 24 && !yesno; r++) {     // "Do you want to save these settings?" dialog
      std::string row = trim(row_str(ts, r, 0, 80));
      for (int c = 0; c < 80; c++)
        if (ts.ch[r][c] == '?' && ((ts.at[r][c] >> 4) & 15) == 7 && row.find("Do you") != std::string::npos) yesno = true;
      if (row.size() > 2 && row[row.size() - 1] == '?' && ((ts.at[r][40] >> 4) & 15) == 7) yesno = true;
    }
  // a message box that says PRESS KEY wins over the menu row drawn behind it
  if (!yesno && pop_r0 >= 0 && pop_text.find("PRESS KEY") != std::string::npos) items.clear();
  if (screen_has(ts, "Enter file name")) { items.clear(); yesno = false; prompt = "Save as:"; }
  if (!yesno && screen_has(ts, "PROBES CONTROL") && !items.empty()) {     // (cold read 2: only the unmentioned D-Pad reached a loaded probe)
    items.push_back(MenuItem{"NEXT PROBE", -1, false, 5, 0});
  }
  if (!yesno && screen_has(ts, "MESSAGE TRANSMISSION CONTROL"))   // Send is Enter, Cancel is Esc (capture 3: S and C did nothing)
    for (auto& it : items) {
      std::string u = it.label; for (auto& c : u) c = (char)toupper((unsigned char)c);
      if (trim(u) == "SEND") { it.key = 13; it.mods = 0; it.hot = -1; }
      else if (trim(u) == "CANCEL") { it.key = 27; it.mods = 0; it.hot = -1; }
    }
  if (dc_table(ts)) {
    items.clear(); yesno = false;
    items.push_back(MenuItem{"ASSIGN", -1, false, 13, 0});
    items.push_back(MenuItem{"BACK", -1, false, 27, 0});
    prompt = "Repair teams: pick a system, + / - TEAM";
  }
  if (int lp = lp_table(ts)) {         // the landing-party table: Enter activates, Esc drops the plan, Shift+A recalls all
    items.clear(); yesno = false;
    items.push_back(MenuItem{lp == 2 ? "BEAM UP" : "BEAM DOWN", -1, false, 13, 0});
    items.push_back(MenuItem{"CANCEL", -1, false, 27, 0});
    if (lp == 2) items.push_back(MenuItem{"ALL BACK", -1, false, 'A', M_SHIFT});
    prompt = lp == 2 ? "How many teams stay?" : "Teams to send down";
  }
  // (the bridge's menu row stays drawn under a PRESS KEY prompt, but only a key press is wanted)
  if (!yesno && is_bridge(ts) && row_str(ts, 24, 0, 24).find("PRESS KEY") != std::string::npos) items.clear();
  // Tutorial 3 opens with "PLANET INTEGRITY FAILURE" boxes; the last says "IF YOU WANT TO CONTINUE,
  // PRESS [N]" (capture, Nov 27): N carries on, then NO to "Exit to DOS?".
  if (!yesno && screen_has(ts, "WANT TO CONTINUE, PRESS [N]")) {
    items.clear();
    items.push_back(MenuItem{"CARRY ON (N)", -1, false, 'N', 0});
    prompt = "The game hit a data error. CARRY ON keeps playing";
  }
  bool anykey = items.empty() && !yesno &&
                (prompt.find("PRESS") != std::string::npos || screen_has(ts, "PRESS ANY KEY", 18, 24) || screen_has(ts, "PRESS KEY", 18, 24) ||
                 (pop_r0 >= 0 && pop_text.find("PRESS KEY") != std::string::npos));
  row_mode = !items.empty() ? (window_hints(ts) ? R_WINDOW : R_MENU) : yesno ? R_YESNO : anykey ? R_ANYKEY : R_DEFAULT;
  if (screen_has(ts, "SIGN-ON OPTIONS") && prompt.find("www.") != std::string::npos) prompt = "Tap an option";   // (the game's advert line does not fit and is no prompt)
  prompt_strip(s, ts, prompt);
  const int rx1 = is_bridge(ts) ? BRIDGE_ROW_X1 : BOT_W - 6;   // the row's right end
  if (!items.empty()) {
    // P2-6: the game's menu row in 308 px (224 on the bridge). Even widths when every label fits; otherwise widths that
    // follow the labels (6x12, 4 px padding each side); if even that is too wide, pages of items
    // with a MORE button that shows the next page.
    int n = (int)items.size();
    std::vector<int> need(n);
    bool even_ok = true;
    for (int i = 0; i < n; i++) {
      need[i] = std::max(24, text_width(items[i].label.c_str(), F6x12) + 8);
      int a, b; split_gap(6, rx1, n, i, a, b);
      if (text_width(items[i].label.c_str(), F6x12) > b - a - 4) even_ok = false;
    }
    std::string sig; for (auto& m : items) sig += m.label + "|";
    static std::string row_sig;
    if (sig != row_sig) { row_sig = sig; row_page = 0; grid_open = false; }
    int total = GAP * (n - 1); for (int w : need) total += w;
    // On the bridge a menu too long for its row becomes a grid of whole words over the tactical
    // display, instead of pages behind MORE. The main menu (always up) opens it with SHIP MENUS;
    // a submenu shows it at once.
    bool main_menu = false; for (auto& m : items) if (m.label == "NAV") for (auto& m2 : items) if (m2.label == "ENG") main_menu = true;
    if (is_bridge(ts) && !kb_open && ((!even_ok && total > rx1 - 6) || main_menu)) {
      if (main_menu && !grid_open) {
        add_btn(6, ROW1_Y0, rx1, ROW1_Y1, "SHIP MENUS", &F8x16, kind_act(A_GRID));
      } else {
        grid_shown = true;
        s.fill(0, 0, 236, UP_H, BG);
        int cols = n > 10 ? 3 : 2, rows = (n + cols - 1) / cols;
        for (int i = 0; i < n; i++) {
          int a, b, y0, y1; split_gap(6, 230, cols, i % cols, a, b); split_gap(6, UP_H - 4, rows < 5 ? 5 : rows, i / cols, y0, y1);
          Key k = items[i].key == 27 ? K_ESC : items[i].key == 13 ? K_ENTER : key_for_char(items[i].key);
          std::string lab = full_word(items[i].label);
          const Font* lf = text_width(lab.c_str(), F8x16) <= b - a - 6 ? &F8x16 : &F6x12;
          add_btn(a, y0, b, y1, lab, lf, key_act(k, items[i].mods), -1, items[i].sel);
          if (items[i].label == "LANDING PRTY" && !in_orbit(ts)) btns.back().off = true;   // (the game refuses it away from a planet)
        }
        if (main_menu) add_btn(6, ROW1_Y0, rx1, ROW1_Y1, "CLOSE MENUS", &F8x16, kind_act(A_GRID));
        else add_btn(6, ROW1_Y0, rx1, ROW1_Y1, "BACK", &F8x16, key_act(K_ESC));
      }
    } else {
    int from = 0, to = n;                     // items shown: [from, to)
    bool more = false;
    if (!even_ok && total > rx1 - 6) {        // pages: as many items as fit beside a 44-px MORE
      std::vector<int> starts(1, 0);
      int w = 0;
      for (int i = 0; i < n; i++) {
        int add = need[i] + (i > starts.back() ? GAP : 0);
        if (w + add + GAP + 44 > rx1 - 6 && i > starts.back()) { starts.push_back(i); w = need[i]; }
        else w += add;
      }
      int pages = (int)starts.size();
      row_page %= pages;
      from = starts[row_page]; to = row_page + 1 < pages ? starts[row_page + 1] : n;
      more = true;
    }
    int m = to - from, x = 6, avail = rx1 - 6 - (more ? 44 + GAP : 0);
    int used = GAP * (m - 1); for (int i = from; i < to; i++) used += need[i];
    int extra = std::max(0, avail - used);
    for (int i = from; i < to; i++) {
      int a, b;
      if (even_ok) split_gap(6, rx1, n, i, a, b);
      else { int k = i - from; a = x; b = a + need[i] + extra / m + (k < extra % m ? 1 : 0); x = b + GAP; }
      Key k = items[i].key == 27 ? K_ESC : items[i].key == 13 ? K_ENTER :
              items[i].key == 5 ? K_DOWN : items[i].key >= 1 && items[i].key <= 4 ? (Key)(K_F1 + (items[i].key == 1 ? 2 : items[i].key == 2 ? 3 : items[i].key == 3 ? 0 : 1)) : key_for_char(items[i].key);
      add_btn(a, ROW1_Y0, b, ROW1_Y1, items[i].label, &F8x16, key_act(k, items[i].mods), items[i].hot, items[i].sel);
    }
    if (more) add_btn(rx1 - 44, ROW1_Y0, rx1, ROW1_Y1, "MORE", &F8x16, kind_act(A_ROWPAGE));
    }
  } else if (yesno) {
    int a, b;
    split_gap(6, rx1, 2, 0, a, b); add_btn(a, ROW1_Y0, b, ROW1_Y1, "YES", &F8x16, key_act(K_Y), 0);
    split_gap(6, rx1, 2, 1, a, b); add_btn(a, ROW1_Y0, b, ROW1_Y1, "NO", &F8x16, key_act(K_N), 0);
  } else if (kb_open && (at_name_prompt(ts) || signon_pass(ts)) && !off_name.empty() && autologin == 0) {
    std::string up = off_name;
    for (auto& ch2 : up) if (ch2 >= 'a' && ch2 <= 'z') ch2 -= 32;
    std::string lab = "SIGN ON AS " + up;
    const int hx0 = rx1 - 104;                // HIDE KEYS: 104 px at the right end
    if (text_width(lab.c_str(), F8x16) > hx0 - GAP - 6 - 4) lab = "SIGN ON";
    add_btn(6, ROW1_Y0, hx0 - GAP, ROW1_Y1, lab, &F8x16, kind_act(A_SIGNON), -1, true);
    add_btn(hx0, ROW1_Y0, rx1, ROW1_Y1, "HIDE KEYS", &F8x16, kind_act(A_KB));
  } else if (!kb_open && autologin == 0 && signon_list(ts, 0)) {
    // the commander list: one tap signs the remembered officer on; ENTER picks the highlighted one
    std::string up = off_name; for (auto& ch2 : up) if (ch2 >= 'a' && ch2 <= 'z') ch2 -= 32;
    int a, b; split_gap(6, rx1, 3, 0, a, b);
    std::string lab = "SIGN ON AS " + up; const Font* lf = &F8x16;
    if (text_width(lab.c_str(), *lf) > b - a - 8) lf = &F6x12;
    if (text_width(lab.c_str(), *lf) > b - a - 8) { lab = "SIGN ON"; lf = &F8x16; }
    add_btn(a, ROW1_Y0, b, ROW1_Y1, lab, lf, kind_act(A_SIGNON), -1, true);
    split_gap(6, rx1, 3, 1, a, b); add_btn(a, ROW1_Y0, b, ROW1_Y1, "ENTER", &F8x16, key_act(K_ENTER));
    split_gap(6, rx1, 3, 2, a, b); add_btn(a, ROW1_Y0, b, ROW1_Y1, "BACK", &F8x16, key_act(K_ESC));
  } else {
    int a, b;
    split_gap(6, rx1, 3, 0, a, b); add_btn(a, ROW1_Y0, b, ROW1_Y1, anykey ? "CONTINUE" : "ENTER", &F8x16, key_act(K_ENTER));
    split_gap(6, rx1, 3, 1, a, b); add_btn(a, ROW1_Y0, b, ROW1_Y1, "BACK", &F8x16, key_act(K_ESC));
    split_gap(6, rx1, 3, 2, a, b); add_btn(a, ROW1_Y0, b, ROW1_Y1, kb_open ? "HIDE KEYS" : "KEYBOARD", &F8x16, kind_act(A_KB));
  }
  // Seven views (MANUAL moved to the START menu: eight 6-letter buttons can't be 4 px apart in 308 px).
  // A segmented row of tabs: the one row allowed 4-px gaps (Ryan, Nov 20).
  static const char* views[7] = {"STARS", "SYSTEM", "WAR", "COMBAT", "FLEET", "ORDERS", "LOG"};
  // The views only exist during a mission (the status lights are on the top row then).
  // (full-screen pages inside a mission, like the fleet display and the databases, have no lights
  // row: the tabs stay live there. UI review: they were greyed mid-mission.)
  if (row_str(ts, 0, 0, 80).find("LOW PWR") != std::string::npos) mission_on = true;
  else if (screen_has(ts, "SIGN-ON OPTIONS") || screen_has(ts, "Enter your name") || screen_has(ts, "MISSIONS RECORD", 0, 3) || screen_has(ts, "FLEET HEADQUARTERS")) mission_on = false;
  bool in_mission = mission_on;
  for (int i = 0; i < 7; i++) {
    int a, b; split_gap(6, BOT_W - 6, 7, i, a, b, TAB_GAP);
    add_btn(a, ROW2_Y0, b, ROW2_Y1, views[i], &F6x12, kind_act(A_VIEW, i));
    if (!in_mission) btns.back().off = true;
    // Tutorials refuse ORDERS and LOG ("NOT AVAILABLE IN TUTORIALS"), and in orbit that box stayed
    // up for good with all input dead (capture 3). Grey them there.
    if (tut_num > 0 && (i == 5 || i == 6)) btns.back().off = true;
  }
}

// ------------------------------------------------------------------ on-screen keyboard
// The line the cursor is on asks a yes/no question ("Are you a new recruit [Y]?").
static bool cursor_question(const TextScreen& ts) {
  if (ts.cursor_y < 0 || ts.cursor_y > 24) return false;
  std::string row = row_str(ts, ts.cursor_y, 0, 80);
  if (row.find('?') == std::string::npos) return false;
  return row.find("[Y]") != std::string::npos || row.find("[N]") != std::string::npos ||
         row.find("Y/N") != std::string::npos || row.find("y/n") != std::string::npos;
}
static bool wants_text(const TextScreen& ts) {
  if (cursor_question(ts)) return false;
  return screen_has(ts, "Enter your name") || screen_has(ts, "enter your password") ||
         screen_has(ts, "Enter new password") || screen_has(ts, "NOTEPAD") || screen_has(ts, "Notepad") ||
         screen_has(ts, "Enter file name");       // saving a mission by name (seen Nov 21: no keyboard, no prompt)
}
static void keyboard(Surf& s) {
  static const char* rows[4] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL-", "ZXCVBNM,./"};
  // 5 rows of 24-px keys, 8 px apart, y 6..158 (rule 11). Ten 24-px keys in a row leave 68 px for
  // nine gaps in 308: 8 px each where possible, 7 px for four of them (the only gaps under 8).
  s.fill(0, 0, BOT_W, UP_H, BG);
  for (int r = 0; r < 4; r++)
    for (int k = 0; k < 10; k++) {
      int a = 6 + k * 24 + k * 7 + std::min(k, 5);   // gaps: 8 8 8 8 8 7 7 7 7
      std::string lab(1, rows[r][k]);
      Act ka = key_act(key_for_char(rows[r][k])); ka.kind = A_TYPE; ka.arg = rows[r][k];
      add_btn(a, 6 + r * 32, a + 24, 6 + r * 32 + 24, lab, &F8x16, ka);
    }
  const int ky0 = 6 + 4 * 32, ky1 = ky0 + 24;
  add_btn(6, ky0, 62, ky1, "ESC", &F8x16, key_act(K_ESC));
  Act sp = key_act(K_SPACE); sp.kind = A_TYPE; sp.arg = ' ';
  Act del = key_act(K_BACKSPACE); del.kind = A_TYPE; del.arg = 8;
  Act ent = key_act(K_ENTER); ent.kind = A_TYPE; ent.arg = 13;
  add_btn(70, ky0, 186, ky1, "SPACE", &F8x16, sp);
  add_btn(194, ky0, 250, ky1, "DEL", &F8x16, del);
  add_btn(258, ky0, 314, ky1, "ENTER", &F8x16, ent);
}

// ------------------------------------------------------------------ bridge layout
static void bridge_top(Surf& s, const TextScreen& ts) {
  static const int starts[8] = {2, 12, 22, 32, 42, 52, 62, 72};
  for (int i = 0; i < 8; i++) {
    int a, b; split(TOP_W, 8, i, a, b);
    std::string lab = trim(row_str(ts, 0, starts[i], starts[i] + 7));
    uint8_t at = ts.at[0][starts[i] + 1];
    s.fill(a, 0, b, 12, BG);
    s.fill(a, 0, b - 1, 12, VGA[(at >> 4) & 15]);
    uint16_t fg = (at >> 4) & 15 ? VGA[at & 15] : READ_FG[at & 15];
    fit_text(s, a + (b - a - 1 - text_width(lab.c_str(), F6x12)) / 2, 0, b - 1, lab.c_str(), F6x12, fg, "alert light");
  }
  // P2-2: the house hint strip runs along the bottom (y 224) under both columns (COMMS keeps 12-px rows).
  Box in = tile(s, 0, 12, 238, 124, "COMMS");
  cells_in(s, ts, 1, 15, 39, 22, F6x12, in.x0, in.y0, in.x1, in.y1);

  // energy + mission time
  in = tile(s, 0, 124, 138, 224, "ENERGY");
  static const int trow[5] = {18, 19, 20, 21, 22}, erow[4] = {18, 19, 20, 22};
  for (int i = 0; i < 5; i++) {
    uint8_t at = ts.at[trow[i]][41];
    int y = in.y0 + i * 12;
    s.fill(in.x0, y, in.x0 + 20, y + 12, VGA[(at >> 4) & 15]);
    std::string t = row_str(ts, trow[i], 41, 44);
    fit_text(s, in.x0 + 1, y, in.x0 + 20, t.c_str(), F6x12, (at >> 4) & 15 ? VGA[at & 15] : READ_FG[at & 15], "energy toggle");
  }
  for (int i = 0; i < 4; i++) {
    int r = erow[i], y = in.y0 + i * 15;
    std::string lab = trim(row_str(ts, r, 45, 53)), val = trim(row_str(ts, r, 54, 58));
    while (val.size() < 4) val = " " + val;
    fit_text(s, in.x0 + 23, y, in.x0 + 72, lab.c_str(), F6x12, READ_FG[ts.at[r][45] & 15], "energy label");
    fit_text(s, in.x0 + 73, y, in.x0 + 99, val.c_str(), F6x12, READ_FG[ts.at[r][54] & 15], "energy value");
    int filled = 0;
    for (int c = 58; c < 78; c++) if (ts.ch[r][c] == 0xDB) filled++;
    int bx0 = in.x0 + 100, bx1 = in.x1 - 1;
    s.rect(bx0, y + 2, bx1 + 1, y + 10, rgb(70, 72, 86));
    if (filled) s.fill(bx0 + 1, y + 3, bx0 + 1 + (bx1 - bx0 - 1) * filled / 20, y + 9, VGA[ts.at[r][58 + filled - 1] & 15]);
  }
  int sep = in.y1 - 27;
  s.hline(in.x0, in.x1, sep, FRAME);
  cells_in(s, ts, 1, 11, 16, 12, F6x12, in.x0, sep + 1, in.x1, in.y1, 12);

  in = tile(s, 138, 124, 238, 224, "LRS");
  cells_in(s, ts, 2, 3, 16, 9, F6x12, in.x0, in.y0, in.x1, in.y1);
  // Right column (Ryan, Nov 20): the VEL bar graph is folded into DATA as its two read-outs
  // (sublight s: and hyperspace c:, beside the DATA rows they line up with), as the swapped 320-px
  // layout does, so the gold hint strip spans the whole width. Everything stays 6x12.
  in = tile(s, 238, 12, 350, 108, "DAMAGE");
  cells_in(s, ts, 60, 2, 77, 8, F6x12, in.x0, in.y0, in.x1, in.y1);
  // beside DAMAGE: the shields / tractor beam box (ss, TB) and the two status read-outs of row 1
  in = tile(s, 350, 12, 400, 108, 0);
  {
    // the box's outer columns (42 and 50) are padding; drop them when they are blank so the box
    // stays 6x12 in 46 px, else show all nine columns in 5x9 rather than cut anything
    bool pad_blank = true;
    for (int r = 14; r <= 16; r++) if (ts.ch[r][42] > ' ' || ts.ch[r][50] > ' ') pad_blank = false;
    if (pad_blank) cells_in(s, ts, 43, 14, 49, 16, F6x12, in.x0, in.y0 + 4, in.x1, in.y0 + 4 + 36, 12);
    else cells_in(s, ts, 42, 14, 50, 16, F5x9, in.x0, in.y0 + 4, in.x1, in.y0 + 4 + 36, 12);
    s.hline(in.x0, in.x1, in.y0 + 46, FRAME);
    cells_in(s, ts, 1, 1, 3, 1, F6x12, in.x0, in.y0 + 52, in.x1, in.y0 + 64, 12);
    cells_in(s, ts, 7, 1, 12, 1, F6x12, in.x0, in.y0 + 70, in.x1, in.y0 + 82, 12);
  }
  in = tile(s, 238, 108, 400, 224, "DATA");
  {
    const int lh = 14, top = in.y0 + (in.y1 - in.y0 - 7 * lh) / 2;   // 7 rows, spaced like the other tiles
    cells_in(s, ts, 60, 10, 77, 16, F6x12, in.x0, top, in.x0 + 110, top + 7 * lh, lh);
    s.vline(in.x0 + 112, in.y0 + 2, in.y1 - 2, FRAME);
    // s: and c: (VEL rows 15-16) beside DATA's last two rows (rows 15-16 too)
    cells_in(s, ts, 53, 15, 58, 16, F6x12, in.x0 + 114, top + 5 * lh, in.x1, top + 7 * lh, lh);
  }
  if (!lefty_loaded) lefty_load();
  const char* hint = alert_level(ts) == 2 ? (lefty_on ? "D-Lt/Up fire  L freeze  R+tap: explain  SELECT: what now?"
                                                      : "X/Y fire  L freeze  R+tap: explain  SELECT: what now?")
                                          : "SELECT: what now?  R+tap: explain  L: freeze  START: menu";
  if (frozen(ts)) hint = "FROZEN: the clock is stopped. Press L to carry on";
  else if (tut_finished) hint = "Tutorial done. To leave: START, then LEAVE TUTORIAL";
  // every other 8 s the strip shows the mission goal instead (plan item 4), when it fits
  std::string goal_line = mis_goal.empty() || tut_num > 0 ? "" : goal_met ? "Goal met: fly home (STARFORT) and dock" : "Goal: " + mis_goal;
  const Font* hf = &F6x12;
  if (!goal_line.empty() && (now() / 8000) % 2) {
    if (text_width(goal_line.c_str(), F6x12) <= TOP_W - 8) hint = goal_line.c_str();
    else if (text_width(goal_line.c_str(), F5x9) <= TOP_W - 8) { hint = goal_line.c_str(); hf = &F5x9; }
  }
  s.fill(0, 224, TOP_W, TOP_H, BTN);
  s.hline(0, TOP_W, 224, BTN_EDGE);
  fit_text(s, (TOP_W - text_width(hint, *hf)) / 2, 226 + (12 - hf->h) / 2, TOP_W - 2, hint, *hf, TITLE, "bridge hint");
}

// The bridge dashboard laid out for 320 px, used when the screens are swapped and it sits on the
// touch screen (swap readability rule: never squeeze). Cells use the 5x9 font this port already
// uses for full DOS screens; tile titles stay 6x12. The velocity column's bar graph is folded
// into DATA as its two read-outs (sublight s: and hyperspace c:).
static void bridge_top_narrow(Surf& s, const TextScreen& ts) {
  s.fill(0, 0, TOP_W, TOP_H, BG);
  static const int starts[8] = {2, 12, 22, 32, 42, 52, 62, 72};
  for (int i = 0; i < 8; i++) {
    int a, b; split(BOT_W, 8, i, a, b);
    std::string lab = trim(row_str(ts, 0, starts[i], starts[i] + 7));
    uint8_t at = ts.at[0][starts[i] + 1];
    s.fill(a, 0, b - 1, 12, VGA[(at >> 4) & 15]);
    uint16_t fg = (at >> 4) & 15 ? VGA[at & 15] : READ_FG[at & 15];
    fit_text(s, a + (b - a - 1 - text_width(lab.c_str(), F5x9)) / 2, 2, b - 1, lab.c_str(), F5x9, fg, "alert light");
  }
  Box in = tile(s, 0, 12, 204, 98, "COMMS");
  cells_in(s, ts, 1, 15, 39, 22, F5x9, in.x0, in.y0, in.x1, in.y1);
  in = tile(s, 204, 12, BOT_W, 98, "DAMAGE");
  cells_in(s, ts, 60, 2, 77, 8, F5x9, in.x0, in.y0, in.x1, in.y1);

  // energy: toggle, label, value, bar per row, then mission time
  in = tile(s, 0, 98, 132, TOP_H, "ENERGY");
  static const int trow[5] = {18, 19, 20, 21, 22}, erow[4] = {18, 19, 20, 22};
  for (int i = 0; i < 5; i++) {
    uint8_t at = ts.at[trow[i]][41];
    int y = in.y0 + i * 11;
    s.fill(in.x0, y, in.x0 + 17, y + 10, VGA[(at >> 4) & 15]);
    std::string t = row_str(ts, trow[i], 41, 44);
    fit_text(s, in.x0 + 1, y + 1, in.x0 + 17, t.c_str(), F5x9, (at >> 4) & 15 ? VGA[at & 15] : READ_FG[at & 15], "energy toggle");
  }
  for (int i = 0; i < 4; i++) {
    int r = erow[i], y = in.y0 + i * 14;
    std::string lab = trim(row_str(ts, r, 45, 53)), val = trim(row_str(ts, r, 54, 58));
    while (val.size() < 4) val = " " + val;
    fit_text(s, in.x0 + 20, y + 1, in.x0 + 62, lab.c_str(), F5x9, READ_FG[ts.at[r][45] & 15], "energy label");
    fit_text(s, in.x0 + 65, y + 1, in.x0 + 86, val.c_str(), F5x9, READ_FG[ts.at[r][54] & 15], "energy value");
    int filled = 0;
    for (int c = 58; c < 78; c++) if (ts.ch[r][c] == 0xDB) filled++;
    int bx0 = in.x0 + 89, bx1 = in.x1 - 1;
    s.rect(bx0, y + 2, bx1 + 1, y + 10, rgb(70, 72, 86));
    if (filled) s.fill(bx0 + 1, y + 3, bx0 + 1 + (bx1 - bx0 - 1) * filled / 20, y + 9, VGA[ts.at[r][58 + filled - 1] & 15]);
  }
  int sep = in.y1 - 23;
  s.hline(in.x0, in.x1, sep, FRAME);
  cells_in(s, ts, 1, 11, 16, 12, F5x9, in.x0, sep + 1, in.x1, in.y1, 10);

  in = tile(s, 132, 98, 212, 184, "LRS");
  cells_in(s, ts, 2, 3, 16, 9, F5x9, in.x0, in.y0, in.x1, in.y1);
  in = tile(s, 132, 184, 212, TOP_H, 0);
  cells_in(s, ts, 42, 14, 50, 16, F5x9, in.x0, in.y0, in.x1, in.y1 - 12, 10);
  cells_in(s, ts, 1, 1, 3, 1, F5x9, in.x0, in.y1 - 11, in.x0 + 30, in.y1);
  cells_in(s, ts, 7, 1, 12, 1, F5x9, in.x0 + 34, in.y1 - 11, in.x1, in.y1);

  in = tile(s, 212, 98, BOT_W, TOP_H, "DATA");
  int dsep = in.y1 - 23;
  cells_in(s, ts, 60, 10, 77, 16, F5x9, in.x0, in.y0, in.x1, dsep - 1);
  s.hline(in.x0, in.x1, dsep, FRAME);
  cells_in(s, ts, 53, 15, 58, 16, F5x9, in.x0, dsep + 1, in.x1, in.y1, 10);
}

// tactical geometry on the touch screen: cells 28..49 x 2..13 at 8x13 inside tile (0,0)-(180,166)
// (13-px rows, down from 14, leave room for the prompt strip and two 24-px button rows 8 px apart
// under it; the cells start one pixel into the title band so the 12 rows end above the frame)
static const int TAC_X = 2, TAC_Y = 9, TAC_CW = 8, TAC_LH = 13;

// ------------------------------------------------------------------ mission card (plan item 7)
// The orders screen ("COMMUNICATION FROM KRELLAN FLEET HEADQUARTERS ... Subject : Orders for
// Reconnaissance Mission into Region #1-A ... 2. Your primary objective is to discover ... 4. You
// have 10 days to complete this mission.") gives the mission goal and the days allowed.
static void read_orders(const TextScreen& ts) {
  if (!screen_has(ts, "FLEET HEADQUARTERS") || !screen_has(ts, "Subject") || !screen_has(ts, "Mission #")) return;
  std::string title, text;
  for (int r = 0; r < 24; r++) {
    std::string row = trim(row_str(ts, r, 0, 80));
    size_t k = row.find("Subject");
    if (k != std::string::npos && row.find(':') != std::string::npos) {
      title = trim(row.substr(row.find(':') + 1));
      if (title.rfind("Orders for ", 0) == 0) title = title.substr(11);
    }
    if (r > 6) text += row + " ";
  }
  // numbered paragraphs: "1. ...", "2. ..."
  std::string goal; int days = 0;
  for (size_t i = 0; i < text.size(); i++) {
    if (!(isdigit((unsigned char)text[i]) && i + 1 < text.size() && text[i + 1] == '.' && (i == 0 || text[i - 1] == ' '))) continue;
    size_t e = i + 2; while (e < text.size() && !(isdigit((unsigned char)text[e]) && e + 1 < text.size() && text[e + 1] == '.' && text[e - 1] == ' ')) e++;
    std::string para = trim(text.substr(i + 2, e - i - 2));
    std::string collapsed; bool sp = false;
    for (char c : para) { if (c == ' ') { if (!sp) collapsed += c; sp = true; } else { collapsed += c; sp = false; } }
    size_t o = collapsed.find("objective is to ");
    if (o != std::string::npos && goal.empty()) goal = collapsed.substr(o + 16);
    size_t d = collapsed.find("You have ");
    if (d != std::string::npos) days = atoi(collapsed.c_str() + d + 9);
    i = e - 1;
  }
  if (goal.empty()) return;
  if (!goal.empty() && goal.back() == '.') goal.pop_back();
  goal[0] = (char)toupper((unsigned char)goal[0]);
  if (goal == mis_goal && title == mis_title && days == mis_days) return;
  mis_title = title; mis_goal = goal; mis_days = days; mis_show_pending = true;
  save_settings();
}
static std::string mission_text() {
  std::string t = mis_goal + ".";
  if (mis_days > 0) t += " You have " + std::to_string(mis_days) + " days (Time left on the top screen).";
  t += " Orders again: ORDERS on the bridge. Stuck? Press SELECT.";
  return t;
}

// ------------------------------------------------------------------ explain mode (plan item 6)
struct Gloss { const char* key; const char* title; const char* text; };
static const Gloss GLOSSARY[] = {
  {"PHASERS", "PHASERS (X)", "Short-range beam weapons. They fire only at red alert, at an enemy within 5 points. Ready when the P under DAMAGE is green."},
  {"TORPS", "TORPEDOES (Y)", "Two tubes, longer range than phasers, limited supply (Torps in DATA). They can't hit mines."},
  {"SHLD+", "SHIELDS UP", "Raise shield power 5%. Shields soak up damage but use energy. At 100%, one more tap calls red alert."},
  {"SHLD-", "SHIELDS DOWN", "Lower shield power 5% to save energy. At 0%, one more tap cancels red alert."},
  {"SPD+", "SUBLIGHT SPEED UP", "Sublight speed +0.1 c (up to 0.9). Hyperspace speed: ZR up, ZL down. The engines must be on to move."},
  {"SPD-", "SUBLIGHT SPEED DOWN", "Sublight speed -0.1 c. To stop at once use STOP (emergency stop)."},
  {"LAUNCH", "LAUNCH", "Leave the starfort: sets sublight 0.5 if no speed is set and starts the engines."},
  {"ENGINES", "ENGINES ON", "Starts the engines (F9). If no speed is set it sets sublight 0.5 first."},
  {"ECM", "ECM (lit = on)", "Electronic countermeasures jam weapon locks, theirs AND yours: with it on, PHASERS and TORPS cannot lock. Tap to switch. Uses energy."},
  {"HOME", "HOME TO THE STARFORT", "The same as STARFORT: the ship flies to the nearest starfort by itself. The way out of a fight you are not ready for."},
  {"GO", "ENGINES ON", "Starts the engines (F9). If no speed is set it sets sublight 0.5 first."},
  {"STOP", "EMERGENCY STOP", "All engines stop (F10). The ship stops at once."},
  {"STARFORT", "AUTOPILOT TO STARFORT", "The game's own autopilot to the nearest Krellan starfort (it asks first). Dock there to resupply, repair and rest the crew."},
  {"WHAT NOW?", "WHAT NOW? (SELECT)", "Reads the situation and tells you the next one or two things to do, with the button to tap ringed."},
  {"SHIP MENUS", "SHIP MENUS", "Opens the ship's nine departments (navigation, engineering, combat...) as big buttons. B or CLOSE MENUS puts the tactical display back."},
  {"CLOSE MENUS", "CLOSE MENUS", "Puts the tactical display back."},
  {"ORDERS", "MISSION ORDERS", "Shows your mission orders again: what to do and how many days you have."},
  {"ECM ON", "ECM (countermeasures)", "Electronic countermeasures jam weapon locks, theirs AND yours: switch it off before you fire. Uses energy."},
  {"ECM OFF", "ECM (countermeasures)", "ECM is on: weapon locks are jammed, theirs and yours. Tap to switch it off before you fire, and to save energy."},
  {"BACK TO THE BRIDGE", "BACK TO THE BRIDGE", "Closes the planet's map (B does the same)."},
  {"NEXT PROBE", "NEXT PROBE", "Moves the lit row to the next probe. A probe marked Loaded can be launched; one that is out can be located or detonated."},
  {"STOP LIST", "STOP THE CHECKLIST", "Puts the checklist away. START > MANUAL has every checklist if you want one back."},
  {"RESUME", "RESUME (L)", "The clock is frozen. This (or L) starts it again."},
  {"BEAM DOWN", "BEAM DOWN", "Sends the teams in the table down to the planet (the game's Activate). With no teams it just closes the table."},
  {"BEAM UP", "BEAM UP", "Brings back every team that is not marked to stay."},
  {"ALL BACK", "ALL BACK", "Sets every row to 0 teams staying: everyone comes back when you tap BEAM UP."},
  {"CANCEL", "CANCEL", "Closes the table and forgets the plan. Nobody is sent."},
  {"+ TEAM", "ADD A TEAM", "One more team (10 people) on the lit mission. The game picks the mix of crew and shock troops."},
  {"- TEAM", "REMOVE A TEAM", "One team fewer on the lit mission."},
  {"FREEZE", "FREEZE (L)", "Stops the clock so you can think. FREEZE again (or L) carries on."},
  {"TELEPORT", "TELEPORTERS", "Send a landing party, shock troops or supplies to the planet you orbit, or bring them back."},
  {"AUTO", "TAP MODE: AUTO", "A tap on the tactical display moves the target box when it is showing, otherwise steers. This button changes the mode."},
  {"STEER", "TAP MODE: STEER", "A tap on the tactical display always turns the ship toward that spot. This button changes the mode."},
  {"TARGET", "TAP MODE: TARGET", "A tap on the tactical display always moves the target box. This button changes the mode."},
  {"STARS", "STAR MAP", "The region's star systems: choose where to travel next."},
  {"SYSTEM", "SYSTEM MAP", "Planets, ships and bases in the star system you are in. Tap one to aim at it."},
  {"WAR", "WAR MAP", "Who owns which star systems: Krellan, UGA (the enemy) or neutral. This is the map that shows starforts."},
  {"COMBAT", "COMBAT CONTROL", "Weapon settings and targets: Destroy/Disable, automatic or manual targeting."},
  {"FLEET", "FLEET COMMAND", "Escorts and fleets under your command, and their orders."},
  {"LOG", "LOG", "Your log of reports and messages, so you can read what you missed."},
  {"NAV", "NAVIGATION", "Course, speed, targets and autopilot (travel to a planet, ship or system)."},
  {"ENG", "ENGINEERING", "Damage control and repairs, power, teleporters."},
  {"CMB", "COMBAT", "Weapons settings, firing modes and targeting."},
  {"CMP", "COMPUTER", "Databases of what you have found, and the Ship Information System. Save and Quit are here: Special services."},
  {"SEC", "SECURITY", "Security status, defence inside your ship, interrogating captives."},
  {"COM", "COMMUNICATIONS", "Messages, hails and distress calls."},
  {"STG", "STRATEGIC COMMAND", "Fleets, escorts and planetary invasions."},
  {"SCI", "SCIENCE", "Scanners, Medical (crew rest and morale), Department Q."},
  {"CTL", "CONTROLS", "Hot keys (the ship's switches: AAS, CMF, rest, ECM) and the command tree."},
  {"MORE", "MORE", "Shows the rest of the game's commands on this row."},
  {"CONTINUE", "CONTINUE", "Goes on to the next screen (same as A)."},
  {"BACK", "BACK", "Backs out of the current menu or screen (same as B)."},
  {"KEYBOARD", "KEYBOARD", "Opens the on-screen keyboard for the rare times the game wants typing."},
  {"YES", "YES", "Answers the game's question with yes."},
  {"NO", "NO", "Answers the game's question with no."},
};
// Library links (learning design step 1): where in the game's own manuals a card's subject is
// explained. With the card up, R + tap the same thing again opens the manual there.
struct LibLink { const char* key; const char* section; const char* anchor; };
static const LibLink LIBLINKS[] = {
  {"PHASERS", "Ops 1.3", "Phaser Control"}, {"TORPS", "Ops 1.3", "Torpedo Control"},
  {"SHLD+", "Training 2.4", "shield"}, {"SHLD-", "Training 2.4", "shield"},
  {"SPD+", "Training 2.4", "TABLE II"}, {"SPD-", "Training 2.4", "TABLE II"},
  {"LAUNCH", "Tutorial 1", "undock"}, {"ENGINES", "Tutorial 1", "engines"}, {"STOP", "Training 2.4", "Emergency"},
  {"STARFORT", "Ops 1.1", "tarfort"}, {"ECM ON", "Ops 1.9", "ECM"}, {"ECM OFF", "Ops 1.9", "ECM"},
  {"TELEPORT", "Tutorial 2", ""}, {"STARS", "Ops 1.1", "Nav Map"}, {"SYSTEM", "Ops 1.1", "System Map"},
  {"WAR", "Ops 1.1", "War Map"}, {"COMBAT", "Ops 1.3", ""}, {"FLEET", "Ops 3.2", ""},
  {"NAV", "Ops 1.1", ""}, {"ENG", "Ops 1.2", ""}, {"CMB", "Ops 1.3", ""}, {"CMP", "Ops 1.4", ""},
  {"SEC", "Ops 1.5", ""}, {"COM", "Ops 1.6", ""}, {"STG", "Ops 1.7", ""}, {"SCI", "Ops 1.8", ""}, {"CTL", "Ops 1.9", ""},
  {"TACTICAL DISPLAY", "Training 2.2", "Tactical Display -"}, {"NAVIGATION", "Training 2.2", "Navigation Data"},
  {"FREEZE", "Training 2.4", "Freeze"},
};
static std::string explain_key;          // what the card on screen is about (for the Library link)
static void manual_wrap(struct Section& sec, int w);
static int lib_find(const char* key) {   // the manual section a card links to, or -1
  for (auto& l : LIBLINKS) if (!strcmp(l.key, key))
    for (int i = 0; i < (int)manual.size(); i++) if (manual[i].title.compare(0, strlen(l.section), l.section) == 0) return i;
  return -1;
}
static bool lib_open(const char* key) {
  int i = lib_find(key); if (i < 0) return false;
  const char* anchor = "";
  for (auto& l : LIBLINKS) if (!strcmp(l.key, key)) anchor = l.anchor;
  manual_open = true; pause_open = false; man_sel = i; man_scroll = 0; man_focus = 0;
  manual_wrap(manual[i], top_w);
  if (*anchor) for (int k = 0; k < (int)manual[i].lines.size(); k++)
    if (manual[i].lines[k].find(anchor) != std::string::npos) { man_scroll = k > 1 ? k - 1 : 0; break; }
  explain_until = 0; dirty();
  return true;
}
// Cards stay up until the player does something (a press or a tap), the situation they belong to
// passes, or a minute goes by: they were gone before a newcomer had read them (learning review, 10).
static const uint32_t CARD_MS = 60000;
static bool card_fresh = false;
static std::string prev_title, prev_text;     // the last card that was not "what now" (SELECT twice shows it again)
static void explain(const char* title, const char* text) {
  explain_title = title; explain_text = text; explain_until = now() + CARD_MS; explain_key.clear(); card_fresh = true; dirty();
  if (strcmp(title, "WHAT NOW?") && strncmp(title, "QUICK REFERENCE", 15)) { prev_title = title; prev_text = text; }
}
// Any press or tap puts the card away (and still does what it normally does).
static void card_dismiss() { if (explain_until && !quickref_up) { explain_until = 0; coach_card = -1; dirty(); } }
static void explain_link(const char* key) {   // the card says how to read more when the manual covers it
  explain_key = key;
  if (lib_find(key) >= 0) { explain_text += " (R + tap it again: the manual on this.)"; }
}
static bool explain_button(const std::string& label) {
  std::string ab = abbr_word(label);
  for (auto& g : GLOSSARY) if (label == g.key || ab == g.key) {
    if (explain_until && explain_key == g.key && lib_open(g.key)) return true;   // second tap: the Library
    explain(g.title, g.text); explain_link(g.key); return true;
  }
  return false;
}
// What the tapped spot on the bridge's touch screen is (outside the buttons).
static void explain_spot(const TextScreen& ts, int x, int y) {
  if (is_bridge(ts) && y < UP_H) {
    const char* key = x < 180 ? "TACTICAL DISPLAY" : x < 236 ? "NAVIGATION" : 0;
    if (!key) return;
    if (explain_until && explain_key == key && lib_open(key)) return;
    if (x < 180) explain("TACTICAL DISPLAY", "A close-up around your ship (in the centre, heading up). Tap to steer there. Red dots are torpedoes or mines.");
    else explain("NAVIGATION", "Where you are (region and position), the course set, your first target and its distance, and the mode: DOCKED, NORM SPC, HYPRSPCE, EMG STOP...");
    explain_link(key);
    return;
  }
  if (y >= UP_H && y < ROW1_Y0) { explain("PROMPT LINE", "The game's instructions and questions, or what to do next. Alerts show here in red or gold."); return; }
  explain("EXPLAIN", "Hold R and tap a button or a part of the touch screen to see what it does.");
}
// ------------------------------------------------------------------ checklists (learning design step 3)
static bool screen_has_ci(const TextScreen& ts, const std::string& needle) {
  if (needle.empty()) return false;
  // "A ;; B": either text proves the step (capture 3: a planet under attack never says "Survey
  // complete", only "This planet's name is ...")
  size_t alt = needle.find(" ;; ");
  if (alt != std::string::npos) return screen_has_ci(ts, needle.substr(0, alt)) || screen_has_ci(ts, needle.substr(alt + 4));
  // a proof written all in capitals is matched as written ("ENTERED" must not match "Entered orbit")
  bool lower = false; for (char c : needle) if (c >= 'a' && c <= 'z') lower = true;
  if (!lower) { for (int r = 0; r < 25; r++) if (row_str(ts, r, 0, 80).find(needle) != std::string::npos) return true; return false; }
  std::string n = needle; for (auto& c : n) c = (char)tolower((unsigned char)c);
  for (int r = 0; r < 25; r++) {
    std::string row = row_str(ts, r, 0, 80); for (auto& c : row) c = (char)tolower((unsigned char)c);
    if (row.find(n) != std::string::npos) return true;
  }
  return false;
}
static std::vector<char> chk_far;       // per step: its proof text was already on screen when the current step began
static void chk_begin_step(const TextScreen& ts) {
  chk_since = now(); chk_pressed.clear();
  chk_far.clear();
  if (chk_sec >= 0) for (auto& x : manual[chk_sec].steps) chk_far.push_back(screen_has_ci(ts, x.done));
  chk_had = chk_sec >= 0 && chk_step < (int)manual[chk_sec].steps.size() && screen_has_ci(ts, manual[chk_sec].steps[chk_step].done);
}
// A step is done when its "done" text newly appears on the game screen, or (no such text, or it
// was already there) when its button is pressed. A step with neither is done when the next one is.
static bool chk_step_done(const TextScreen& ts, const Step& st, bool current) {
  if (!st.done.empty() && !(current && chk_had) && screen_has_ci(ts, st.done)) return true;
  // (a step that names a text is ticked by the text: the game may refuse the button)
  // (A, ENTER and CONTINUE are one key to the game; so are B, BACK and EXIT)
  auto norm = [](const std::string& b) { return b == "ENTER" || b == "CONTINUE" ? std::string("A") : b == "BACK" || b == "EXIT" ? std::string("B") : b; };
  // (only the current step: a tap on the map must not tick a "tap the map" step three steps on;
  // cold read 2 saw lists run 3 to 8 steps ahead this way)
  if (current && !st.btn.empty() && norm(chk_pressed) == norm(st.btn) && (st.done.empty() || chk_had)) return true;
  return false;
}
static int chk_find(const std::string& title) {
  for (size_t i = 0; i < manual.size(); i++) if (manual[i].title == title && !manual[i].steps.empty()) return (int)i;
  return -1;
}
// A finished list says so on a card that stays (learning review 9), and the next list of a chain
// (Tutorial 1A > 1B > 1C) starts by itself.
static void chk_finish(const TextScreen& ts) {
  const Section& sec = manual[chk_sec];
  std::string text = sec.done.empty() ? "Checklist done: " + sec.title + "." : sec.done;
  int nx = sec.next.empty() ? -1 : chk_find(sec.next);
  if (nx >= 0) text += " Next: " + manual[nx].title + " (its steps follow on this screen).";
  explain("WELL DONE", text.c_str());
  if (nx < 0 && tut_num > 0) { tut_finished = true; if (tut_num == 1 && !tut1_done) { tut1_done = true; save_settings(); } }
  chk_sec = nx; chk_step = 0; if (nx >= 0) chk_begin_step(ts);
  dirty();
}
static bool chk_wait_end = false;       // the last step is done but a question is still up
static void chk_update(const TextScreen& ts) {
  if (chk_sec < 0 || !ts.text_mode) return;
  std::vector<Step>& st = manual[chk_sec].steps;
  if (chk_wait_end) {
    if (row_mode == R_YESNO || !dialog_question(ts).empty()) return;
    chk_wait_end = false; chk_step = (int)st.size(); chk_finish(ts); return;
  }
  for (int look = 0; look < 3 && chk_step + look < (int)st.size(); look++) {
    const Step& x = st[chk_step + look];
    if (chk_step_done(ts, x, look == 0)) { chk_step += look + 1; chk_begin_step(ts); dirty(); break; }
  }
  bool asking = row_mode == R_YESNO || !dialog_question(ts).empty();
  chk_pressed.clear();
  if (chk_step >= (int)st.size()) {
    if (asking) chk_step = (int)st.size() - 1, chk_wait_end = true;    // (the success card waits for the answer)
    else chk_finish(ts);
  }
}
// Where a card goes on the top screen, so it does not cover what it talks about (both reviews,
// Nov 21): on a map, inside the empty tile the map left behind; on the bridge, over ENERGY and LRS
// with COMMS, Time left and the s:/c: speeds still showing; elsewhere above the hint strip.
static bool ct_on = false; static int ct_x0, ct_y0, ct_x1, ct_y1;   // the empty map tile on the top screen (set by generic_top)
static void card_area(const TextScreen& ts, int& x0, int& x1, int& ybot, int& ytop) {
  x0 = 4; x1 = TOP_W - 4; ybot = 222; ytop = 2;
  if (!ts.text_mode) return;
  if (is_bridge(ts)) {
    if (top_w != BOT_W) {
      x1 = 350; ybot = 192; ytop = 12;
      // a ship in TARGET DATA: keep that panel (right) in view, the card stays over COMMS and
      // ENERGY (capture 3: the card hid the locks a checklist step talks about)
      if (row_str(ts, 9, 59, 79).find("TARGET DATA") != std::string::npos && row_str(ts, 11, 59, 79).find_first_of("0123456789") != std::string::npos) x1 = 236;
    }
    return;
  }
  if (ct_on) { x0 = ct_x0; x1 = ct_x1; ybot = ct_y1; ytop = ct_y0; }
}
static std::vector<std::string> wrap_words(const std::string& text, int maxc) {
  std::vector<std::string> lines; std::string cur, word;
  auto flush_word = [&]() {
    if (word.empty()) return;
    if ((int)(cur.size() + (cur.empty() ? 0 : 1) + word.size()) > maxc) { lines.push_back(cur); cur.clear(); }
    if (!cur.empty()) cur += ' ';
    cur += word; word.clear();
  };
  for (char c : text) { if (c == ' ') flush_word(); else word += c; }
  flush_word(); if (!cur.empty()) lines.push_back(cur);
  return lines;
}
static void chk_card(Surf& s, const TextScreen& ts, bool small) {
  const std::vector<Step>& st = manual[chk_sec].steps;
  const uint16_t EDGE = rgb(80, 230, 255);
  if (small) {      // away from the flight screens: one line, so the game's page stays readable
    const int x0 = 4, x1 = TOP_W - 4, y1 = TOP_H - 18, y0 = y1 - 15;
    s.fill(x0, y0, x1, y1, PROMPT_BG); s.rect(x0, y0, x1, y1, EDGE);
    char n[16]; snprintf(n, sizeof n, "%d/%d ", chk_step + 1, (int)st.size());
    std::string l = std::string(n) + st[chk_step].what;
    if (!st[chk_step].btn.empty()) l += "  [" + st[chk_step].btn + "]";
    const Font* f = text_width(l.c_str(), F6x12) <= x1 - x0 - 12 ? &F6x12 : &F5x9;
    fit_text(s, x0 + 6, y0 + 2, x1 - 6, l.c_str(), *f, BTN_TXT, "checklist line");
    return;
  }
  int x0, x1, y1, ytop; card_area(ts, x0, x1, y1, ytop);
  char cnt[16]; snprintf(cnt, sizeof cnt, "  %d/%d", chk_step + 1, (int)st.size());
  if (x1 - x0 < 300) {       // in the map's tile: the title, then this step wrapped (no room for its neighbours)
    std::string l = st[chk_step].what; if (!st[chk_step].btn.empty()) l += "  [" + st[chk_step].btn + "]";
    std::vector<std::string> lines = wrap_words(l, (x1 - x0 - 12) / F6x12.w);
    int h = 4 + 12 + 12 + (int)lines.size() * 12 + 4, y0 = y1 - h;
    s.fill(x0, y0, x1, y1, PROMPT_BG); s.rect(x0, y0, x1, y1, EDGE);
    std::string title = manual[chk_sec].title; size_t colon = title.find(':'); if (colon != std::string::npos) title = title.substr(0, colon);
    std::string head = title + cnt;
    if (text_width(head.c_str(), F5x9) > x1 - x0 - 12) head = std::string("Checklist") + cnt;
    fit_text(s, x0 + 6, y0 + 3, x1 - 6, head.c_str(), text_width(head.c_str(), F6x12) <= x1 - x0 - 12 ? F6x12 : F5x9, EDGE, "checklist card");
    fit_text(s, x0 + 6, y0 + 15, x1 - 6, "Next step:", F6x12, HINT, "checklist card");
    for (size_t i = 0; i < lines.size(); i++) fit_text(s, x0 + 6, y0 + 28 + (int)i * 12, x1 - 6, lines[i].c_str(), F6x12, BTN_TXT, "checklist card");
    return;
  }
  const int y0 = y1 - 56;
  s.fill(x0, y0, x1, y1, PROMPT_BG); s.rect(x0, y0, x1, y1, EDGE);
  std::string head = manual[chk_sec].title + cnt;
  fit_text(s, x0 + 6, y0 + 3, x1 - 6, head.c_str(), text_width(head.c_str(), F6x12) <= x1 - x0 - 12 ? F6x12 : F5x9, EDGE, "checklist card");
  for (int k = -1; k <= 1; k++) {
    int i = chk_step + k; if (i < 0 || i >= (int)st.size()) continue;
    std::string l = std::string(k < 0 ? "\xfb " : k == 0 ? "\x10 " : "  ") + st[i].what;
    if (k == 0 && !st[i].btn.empty()) l += "  [" + st[i].btn + "]";
    const Font* f = text_width(l.c_str(), F6x12) <= x1 - x0 - 12 ? &F6x12 : &F5x9;
    fit_text(s, x0 + 6, y0 + 17 + (k + 1) * 12, x1 - 6, l.c_str(), *f, k == 0 ? BTN_TXT : k < 0 ? rgb(110, 200, 120) : HINT, "checklist card");
  }
}

static void explain_card(Surf& s, const TextScreen& ts) {
  int x0, x1, y1, ytop; card_area(ts, x0, x1, y1, ytop);
  // wrap the text to the card width in the 6x12 font
  std::vector<std::string> lines = wrap_words(explain_text, (x1 - x0 - 12) / F6x12.w);
  const Font* tf = text_width(explain_title.c_str(), F8x16) <= x1 - x0 - 12 ? &F8x16 : &F6x12;
  int h = 4 + tf->h + 2 + (int)lines.size() * 12 + 4, y0 = y1 - h;
  if (y0 < ytop) { y1 = std::min(222, y1 + (ytop - y0)); y0 = y1 - h; }     // a long card grows down before it grows up
  if (y0 < 2) y0 = 2;
  s.fill(x0, y0, x1, y1, PROMPT_BG);
  s.rect(x0, y0, x1, y1, TITLE);
  fit_text(s, x0 + 6, y0 + 4, x1 - 6, explain_title.c_str(), *tf, TITLE, "explain card");
  for (size_t i = 0; i < lines.size(); i++) {
    int ly = y0 + 6 + tf->h + (int)i * 12;
    if (ly + 12 > y1) { fit_problem("explain card text is longer than its space"); break; }
    fit_text(s, x0 + 6, ly, x1 - 6, lines[i].c_str(), F6x12, BTN_TXT, "explain card");
  }
}

// ------------------------------------------------------------------ "Why can't I...?" (learning design, must)
// When an officer refuses an order, COMMS says so in the game's own terse words. The port shows
// the reason and the fix in plain words. Only phrases seen in the real game are listed (add more
// from tests/screens dumps as they turn up).
struct Refusal { const char* phrase; const char* title; const char* text; const char* button; };
#include "refusals.h"   // 60 refusals found in the game's own text files (see the notes there)
static bool ring_active = false;         // a blinking ring is on screen (needs a redraw every 500 ms)
static uint32_t refusal_sig = 0;
static const char* refusal_button = 0; static uint32_t refusal_until = 0;
static void check_refusals(const TextScreen& ts, uint32_t t) {
  if (!is_bridge(ts)) return;
  std::string comms;
  for (int r = 15; r <= 22; r++) comms += row_str(ts, r, 1, 40) + "\n";
  comms += row_str(ts, 24, 0, 24);
  uint32_t h = 2166136261u; for (char c : comms) { h ^= (uint8_t)c; h *= 16777619u; }
  if (h == refusal_sig) return;            // only when COMMS changes
  bool first = refusal_sig == 0; refusal_sig = h;
  if (first) return;                       // not for text already there when the bridge appears
  // the newest line counts: the phrase must be in the last two lines with text
  std::vector<std::string> lines;
  for (int r = 15; r <= 22; r++) { std::string l = trim(row_str(ts, r, 1, 40)); if (!l.empty()) lines.push_back(l); }
  { std::string l = trim(row_str(ts, 24, 0, 80)); if (!l.empty()) lines.push_back(l); }   // (some come on the prompt line)
  for (auto& rf : REFUSALS)
    for (size_t i = lines.size() > 3 ? lines.size() - 3 : 0; i < lines.size(); i++)
      if (lines[i].find(rf.phrase) != std::string::npos) {
        explain(rf.title, rf.text);
        refusal_button = rf.button; refusal_until = t + CARD_MS;
        return;
      }
}

// ------------------------------------------------------------------ advisor: "What now?"
// SELECT on a flight screen, or START > WHAT NOW?: the mission goal and the next one to three
// things to do, read from the situation on screen, with the first button to press ringed.
static float bridge_speed(const TextScreen& ts, int row);
static const char* advise_button = 0; static uint32_t advise_until = 0;
static bool menu_fresh = true;            // the sign-on menu has just come back (scroll it to the coach's item again)
static bool want_practice_view = false;   // the sign-on menu should scroll to show Practice
static int pct_at(const TextScreen& ts, int row, int c0, int c1) {     // "  76%" -> 76, none -> -1
  std::string v = row_str(ts, row, c0, c1); size_t p = v.find('%');
  if (p == std::string::npos) return -1;
  size_t a = p; while (a > 0 && isdigit((unsigned char)v[a - 1])) a--;
  return a == p ? -1 : atoi(v.c_str() + a);
}
// One plain line saying which screen is up (the checklist advice uses it: a step can only be
// done on its own screen).
static std::string where_am_i(const TextScreen& ts) {
  if (!ts.text_mode) return "A picture is up: BACK (or B) leaves it.";
  if (is_bridge(ts)) {
    std::string q = trim(row_str(ts, 24, 0, 24));
    if (q.find('?') != std::string::npos || !dialog_question(ts).empty()) return "The game is waiting for YES or NO.";
    if (q.find("PRESS") != std::string::npos) return "The game is waiting for CONTINUE.";
    switch (bridge_situation(ts)) {
      case SIT_DOCKED: return "You are on the bridge, docked.";
      case SIT_ORBIT: return "You are on the bridge, in orbit.";
      default: return bridge_speed(ts, 16) > 0.0f ? "You are on the bridge, under way." : "You are on the bridge, stopped.";
    }
  }
  if (screen_has(ts, "TARGETER") && screen_has(ts, "enter target")) return "The Targeter is open (ENTER loads, EXIT cancels).";
  if (screen_has(ts, "MAP OF STAR SYSTEM")) return "You are on a SYSTEM map (B: bridge).";
  if (row_str(ts, 1, 0, 80).find("COMBAT") != std::string::npos && row_str(ts, 1, 0, 80).find("INFORMATION") != std::string::npos) return "You are on the COMBAT screen (B: bridge).";
  if (screen_has(ts, "WAR STAR MAP") || screen_has(ts, "WAR MAP") || has_btn("STARFORTS")) return "You are on the WAR map (B: bridge).";
  if (in_flight(ts) && glay_kind() == 2) return "You are on the STARS map (B: bridge).";
  if (in_flight(ts)) return "One of the ship's pages is open (B: bridge).";
  return "You are outside the mission (sign-on pages).";
}
static std::string advise(const TextScreen& ts, const char** button) {
  *button = 0;
  std::vector<std::string> say_list;
  auto add = [&](const std::string& t, const char* b) { if (say_list.empty() && b) *button = b; say_list.push_back(t); };
  if (!ts.text_mode) return mission_on ? "A picture is on the top screen (a planet's map, a boarding plan). Its keys are named on it: the arrows and SPACE are on the touch screen, KEYBOARD has the letters. BACK (or B) leaves it." : "Wait for the picture to finish, or press A.";
  if (int lp = lp_table(ts)) {
    *button = lp == 1 ? "+ TEAM" : "ALL BACK";
    return lp == 1 ? "Landing parties. The lit row is a mission: the arrows change the row, + TEAM puts a team of 10 on it. For a first look: one team on eXplore/survey, then BEAM DOWN; their report comes about a day later. CANCEL sends nobody."
                   : "These teams are on the planet. The number is how many STAY: - TEAM brings one back, ALL BACK brings everyone. Then BEAM UP.";
  }
  if (screen_has(ts, "Enter file name")) return "Type a name for this save (8 letters at most) and tap ENTER.";
  if (chk_sec >= 0 && chk_step < (int)manual[chk_sec].steps.size() && !(ts.text_mode && is_bridge(ts) && alert_level(ts) == 2)) {       // a checklist is being followed: its step is the answer (a fight comes first)
    const Step& stp = manual[chk_sec].steps[chk_step];
    static std::string keep; keep = stp.btn;
    if (!stp.btn.empty() && stp.btn != "A" && stp.btn != "B") *button = keep.c_str();
    std::string stopped;
    if ((stp.what.compare(0, 4, "Wait") == 0 || stp.what.compare(0, 4, "Keep") == 0) && ts.text_mode && is_bridge(ts) && bridge_situation(ts) == SIT_CRUISE && bridge_speed(ts, 16) <= 0.0f)
      stopped = " The ship is stopped, and the game's clock only runs while it moves or orbits: tap SPD+ and ENGINES.";
    return manual[chk_sec].title + ", step " + std::to_string(chk_step + 1) + " of " + std::to_string(manual[chk_sec].steps.size()) + ": " + stp.what + ". " + where_am_i(ts) + stopped + " Off track? START has STEP BACK and SKIP STEP.";
  }
  if (tut_finished && in_flight(ts)) {
    add("This tutorial is done. To leave: press START, then tap LEAVE TUTORIAL. At sign-on, Practice (demo) mission is the next step.", 0);
  } else if (is_bridge(ts)) {
    Situation st = bridge_situation(ts);
    std::string prompt = trim(row_str(ts, 24, 0, 24));
    std::string mode = trim(row_str(ts, 13, 18, 26)), target = trim(row_str(ts, 9, 20, 26));
    float set = bridge_speed(ts, 15), cur = bridge_speed(ts, 16);
    int sup = pct_at(ts, 13, 60, 78), energy = pct_at(ts, 19, 44, 58), shields = pct_at(ts, 22, 44, 58);
    float left = (float)atof(row_str(ts, 12, 11, 17).c_str());
    bool has_left = row_str(ts, 12, 1, 11).find("Time left") != std::string::npos;
    bool asked = prompt.find('?') != std::string::npos;
    int alert = alert_level(ts);
    std::string dq = dialog_question(ts);
    if (!asked && dq.find("planetary map") != std::string::npos) {
      asked = true;
      add("The scan report is in COMMS: class, life, minerals, surveyed or not. The game asks: Display planetary map? NO is fine (YES shows a picture of the planet; BACK leaves it).", "NO");
    } else if (frozen(ts)) {
      add("The clock is FROZEN (the mission time blinks white). Look around as long as you like; L carries on.", 0);
    } else if (!asked && !dq.empty()) {
      asked = true; add("The game asks: " + dq + " Answer YES or NO below.", 0);
    } else if (asked) {
      const char* why = prompt.find("orbit") != std::string::npos ? "YES circles the planet (needed to scan it or send landing parties); NO stays beside it." :
                        prompt.find("dock") != std::string::npos ? "YES docks: the starfort repairs and resupplies the ship." :
                        prompt.find("Comply") != std::string::npos ? "A routine request from the starfort: YES is the usual answer." :
                        prompt.find("Beam landing party up") != std::string::npos ? "YES opens the list of teams on the planet; there you choose who stays." :
                        prompt.find("Answer") != std::string::npos && screen_has(ts, "disable rather than destroy", 14, 23) ? "YES is the better answer for freighters and starliners: a disabled ship can be captured and is worth more than a wreck." :
                        prompt.find("Answer") != std::string::npos ? (screen_has(ts, "landing part", 14, 23) ? "NO is fine on a first visit; YES opens the landing-party table." : "The question is in COMMS on the top screen.") : "Answer YES or NO below.";
      bool safe = prompt.find("orbit") != std::string::npos || prompt.find("dock") != std::string::npos || prompt.find("Comply") != std::string::npos;
      add("The game asks: " + prompt + " " + why, safe ? "YES" : 0);      // (YES is only ringed where it is harmless)
    } else if (prompt.find("PRESS") != std::string::npos) add("Read the message in COMMS, then tap CONTINUE.", "CONTINUE");
    if (asked) {
    } else if (alert == 2) {
      int locks = weapon_locks(ts);
      if (shields >= 0 && shields < 100) add("Red alert and the shields are not at 100%: tap SHLD+.", "SHLD+");
      if (locks & 2) add("Torpedoes are locked on (T in TARGET DATA): tap TORPS. Both tubes fire, then reload for a few seconds.", "TORPS");
      if (locks & 1) add("Phasers are locked on (P in TARGET DATA): tap PHASERS.", (locks & 2) ? 0 : "PHASERS");
      if (!locks) add("No weapon has a lock yet: the enemy is too far (its bearing and distance are in COMMS and TARGET DATA). Phasers reach 5 points, torpedoes 10; a lit PHASERS or TORPS button means a lock.", 0);
      add("Not ready to fight? HOME flies you to the starfort. FREEZE stops time while you think.", 0);
    } else if (alert == 1) {
      add("ORANGE: you are in Alliance or neutral space, nobody in range. Shields at 0% is normal here.", 0);
    }
    if (alert == 1) st = SIT_CRUISE;     // (orange flies like green)
    if (asked) {
    } else {
    if (sup >= 0 && sup < 25) add("Supplies are low: STARFORT flies you home to resupply.", st == SIT_DOCKED ? 0 : "STARFORT");
    if (energy >= 0 && energy < 25) add("Energy is low: slow down (SPD-) or go back to a starfort.", "SPD-");
    if (has_left && left > 0 && left < 1.5f && st != SIT_DOCKED) add("Under 1.5 days left: STARFORT takes you home before time runs out.", "STARFORT");
    if (st == SIT_DOCKED) {
      if (tut_num == 0 && !goal_met && (float)atof(row_str(ts, 11, 11, 17).c_str()) > 0.5f)
        add("Docked, but the mission's goal is not met yet, so the game will not end it. Go out again, or stop here: START, then LEAVE MISSION (NO keeps it for later).", 0);
      if (mis_goal.empty() && tut_num == 0) add("Read your orders first: tap ORDERS.", "ORDERS");
      if (target.empty()) { add("Pick where to go first: tap SYSTEM, tap a planet, TARGETER, ENTER, then B.", "SYSTEM"); add("Then LAUNCH leaves the starfort and the ship flies there. (STARS shows other star systems.)", 0); }
      else add("A target is loaded: tap LAUNCH. The ship undocks and flies there by itself.", "LAUNCH");
    } else if (st == SIT_ORBIT || mode.find("ORBIT") != std::string::npos) {
      add("You are in orbit: the scan report is in COMMS. Tap the planet on the touch screen to send a landing party (not on a gas giant, Cls G).", 0);
      add("ENGINES leaves orbit.", "ENGINES");
    } else if (mode.find("HYPR") != std::string::npos) {
      add("In hyperspace: nothing to do until the ship arrives.", 0);
    } else if (st == SIT_CRUISE) {
      if (cur <= 0.0f && target.empty()) add("Choose where to go: SYSTEM for a planet here, STARS for another star system.", "SYSTEM");
      else if (cur <= 0.0f) add(set > 0.0f ? "A target is set: tap ENGINES to go." : "A target is set: SPD+ for a speed, then ENGINES.", set > 0.0f ? "ENGINES" : "SPD+");
      else if (!target.empty()) add("Under way to " + target + ". The ship stops there by itself; the helm then asks to orbit or dock.", 0);
      else add("Flying by hand. SYSTEM lets you pick a destination and the ship flies itself.", "SYSTEM");
    }
    }
    if (say_list.empty()) add("Follow the prompt line above the buttons. SHIP MENUS opens the ship's departments; B backs out of them.", 0);
  } else if (!dialog_question(ts).empty()) {
    add("The game asks: " + dialog_question(ts) + " Answer YES or NO below.", 0);
  } else if (screen_has(ts, "SIGN-ON OPTIONS")) {
    if (!screen_has(ts, "T  Tutorials")) add("A message is over the menu: read it, then tap ENTER (or A).", "ENTER");
    else if (practice_on(ts)) { add("Practice is on. Tap Start a new mission.", "Start a new mission"); add("New to the game? Tutorials, then 1, comes first. For a mission that counts on your record: Change default settings, then Practice Mode off.", 0); }
    else if (tut1_done) { add("You have flown Tutorial 1. Next: tap Practice (demo) mission, answer YES, then Start a new mission: a checklist flies your first mission with you. Nothing in Practice counts on your record.", "Practice (demo) mission");
           add("Tutorials 2, 4 and 6 (landing parties, weapons, capturing ships) have checklists too.", 0); }
    else { add("New here? Tap Tutorials, then 1: a checklist walks you through your first flight.", "Tutorials");
           add("After that: Practice (demo) mission, YES, then Start a new mission. Nothing in Practice counts against your career.", 0); }
  } else if (screen_has(ts, "MISSIONS RECORD", 0, 3)) {
    if (has_btn("SELECT")) add("This chart is your career: one square per mission. The next mission is highlighted: tap the SELECT button below.", "SELECT");
    else add("This chart is your career: one square per mission. VIEW shows the orders of the lit one; B goes back.", 0);
  } else if (screen_has(ts, "Press number of desired tutorial")) {
    add("Tap a tutorial. Start with 1 (Basic navigation): its checklist follows you step by step.", 0);
  } else if (screen_has(ts, "give up on this mission")) {
    add("NO keeps this mission for later (Resume a saved game). YES throws it away.", "NO");
  } else if (in_flight(ts) && screen_has(ts, "enter target") && screen_has(ts, "TARGETER")) {
    add("The Targeter is open: SPD- and SPD+ change the speed it offers, ENTER loads the target, EXIT cancels.", "ENTER");
    add("Then B for the bridge and ENGINES to go.", 0);
  } else if (in_flight(ts) && glay_kind() == 2 && row_str(ts, 24, 0, 80).find("argeter") == std::string::npos) {
    add("A window or submenu is open on this map: use the buttons below, or B to close it.", 0);
  } else if (screen_has(ts, "MAP OF STAR SYSTEM")) {
    add("Tap a planet (a letter) or a starfort to put the target box on it.", 0);
    add("Then TARGETER, choose a speed, ENTER. Back on the bridge (B), tap ENGINES.", "TARGETER");
  } else if (in_flight(ts) && glay_kind() == 2) {
    add("Tap a star to put the target box on it, then TARGETER and ENTER.", "TARGETER");
    add("B goes back to the bridge.", 0);
  } else if (in_flight(ts)) {
    add("This is one of the ship's pages. The buttons below are its keys; B goes back to the bridge.", 0);
  } else {
    bool cont = false; for (auto& b : btns) if (b.label == "CONTINUE") cont = true;
    if (cont) add("Read the page, then tap CONTINUE (or A). B goes back.", "CONTINUE");
    else add("Use the buttons on the touch screen; B goes back.", 0);
  }
  std::string out;
  size_t say_len = 0; for (size_t i = 0; i < say_list.size() && i < 3; i++) say_len += say_list[i].size();
  if (!mis_goal.empty() && in_flight(ts) && tut_num == 0 && say_len + mis_goal.size() < 230) {
    std::string g = mis_goal; size_t inc = g.find("INCOMPLETE");      // (the orders page adds its own status line)
    if (inc != std::string::npos) g = trim(g.substr(0, inc));
    while (!g.empty() && (g.back() == '.' || g.back() == ' ')) g.pop_back();
    out = "Goal: " + g + (inc != std::string::npos ? " (not done yet). " : ". ");
  }
  for (size_t i = 0; i < say_list.size() && i < 3; i++) out += (say_list.size() > 1 ? std::to_string(i + 1) + ") " : "") + say_list[i] + " ";
  return trim(out);
}

// ------------------------------------------------------------------ coach (plan item 8)
// One card per situation, the first time it comes up, with the next button ringed in blinking
// cyan while the coach is on. The ring stays (the card goes after 9 s) until the situation changes.
enum CoachStep { CS_MENU, CS_ORDERS, CS_DOCKED, CS_CRUISE, CS_ALERT, CS_SYSMAP, CS_COUNT };
static const char* COACH_TITLE[CS_COUNT] = {"COACH: START HERE", "COACH: YOUR ORDERS", "COACH: LEAVE THE STARFORT",
                                           "COACH: FLYING", "COACH: ENEMY!", "COACH: SYSTEM MAP"};
static const char* COACH_TEXT[CS_COUNT] = {
  "New here? Tap Tutorials, then 1: a checklist walks you through your first flight. Stuck anywhere: press SELECT.",
  "These are your orders: read them, then tap CONTINUE. Stuck later? Press SELECT for what to do next.",
  "You are docked at a starfort. Pick where to go first: SYSTEM, tap a planet, TARGETER, ENTER, B. Then LAUNCH: the ship undocks and flies there.",
  "You are under way. Tap the tactical display to steer, SPD+/SPD- for speed. Tap SYSTEM to see the planets nearby.",
  "RED ALERT: an enemy is near. Fight: close in until PHASERS (5 points) or TORPS (10) lights up, then tap it. Not ready? HOME flies you to the starfort. FREEZE stops the clock while you think.",
  "Planets, ships and bases in this star system. Tap a planet or starfort: the box jumps to it. Then tap TARGETER. B goes back to the bridge."};
static const char* COACH_BUTTON[CS_COUNT] = {"Tutorials", "CONTINUE", "SYSTEM", "SYSTEM", "SHLD+", "TARGETER"};
static int coach_step(const TextScreen& ts) {
  if (!ts.text_mode) return -1;
  if (screen_has(ts, "SIGN-ON OPTIONS")) return CS_MENU;
  if (screen_has(ts, "FLEET HEADQUARTERS") && screen_has(ts, "Mission #")) return CS_ORDERS;
  if (is_bridge(ts)) {
    if (chk_sec >= 0) return -1;     // a checklist is guiding: its card is the one to read (verify 4: COACH: ENEMY! hid it)
    Situation st = bridge_situation(ts);
    // (not while the officers report in or a question is up: the card would point at buttons that
    // do nothing yet; both reviews, Nov 21)
    if (trim(row_str(ts, 24, 0, 24)).compare(0, 13, "Select option") != 0) return -1;
    if (st == SIT_DOCKED) return CS_DOCKED;
    if (st == SIT_ALERT) return alert_level(ts) == 2 ? CS_ALERT : CS_CRUISE;
    if (st == SIT_CRUISE) return CS_CRUISE;
    return -1;
  }
  if (in_flight(ts) && glay_kind() == 2 && screen_has(ts, "PLANETS")) return CS_SYSMAP;
  return -1;
}

// ------------------------------------------------------------------ bridge situation (plan item 3)
// The bridge status box (rows 15-16, columns 40-51) reads "STATUS:" over the ship's condition:
// DOCKED, GREEN, YELLOW, ORANGE, RED (and ORBIT). "s:" / "c:" beside it are the set and current
// sublight speeds. ECM's indicator (row 18, col 41) has a coloured background while it is on.
static Situation bridge_situation(const TextScreen& ts) {
  // At an alert the box reads "RED" / "ALERT" (or ORANGE) with no "STATUS:" line (both expert
  // reviews, Nov 21: alerts were not recognised, so first combat had no ECM button and no coach).
  std::string a0 = row_str(ts, 15, 40, 52);
  if (row_str(ts, 16, 40, 52).find("ALERT") != std::string::npos && (a0.find("RED") != std::string::npos || a0.find("ORANGE") != std::string::npos)) return SIT_ALERT;
  if (a0.find("STATUS") == std::string::npos) return SIT_UNKNOWN;
  std::string w = row_str(ts, 16, 40, 52);
  if (row_str(ts, 1, 26, 50).find("ORBITAL") != std::string::npos) return SIT_ORBIT;     // the display shows the planet below
  if (w.find("DOCKED") != std::string::npos) return SIT_DOCKED;
  if (w.find("ORBIT") != std::string::npos) return SIT_ORBIT;
  if (w.find("RED") != std::string::npos || w.find("ORANGE") != std::string::npos) return SIT_ALERT;
  if (w.find("GREEN") != std::string::npos || w.find("YELLOW") != std::string::npos) return SIT_CRUISE;
  return SIT_UNKNOWN;
}
static float bridge_speed(const TextScreen& ts, int row) {   // row 15: set ("s:"), row 16: current ("c:")
  std::string r = row_str(ts, row, 52, 60);
  size_t p = r.find(':');
  return p == std::string::npos ? -1.0f : (float)atof(r.c_str() + p + 1);
}
// In orbit at any alert level: the display title reads ORBITAL, or Mode reads IN ORBIT (capture 3:
// bridge_situation() says ALERT first, so LANDING PRTY was greyed in orbit at orange/red alert).
static bool in_orbit(const TextScreen& ts) {
  return row_str(ts, 1, 26, 50).find("ORBITAL") != std::string::npos || row_str(ts, 13, 18, 26).find("IN ORBIT") != std::string::npos;
}
static bool ecm_on(const TextScreen& ts) { return row_str(ts, 18, 40, 44).find("ECM") != std::string::npos && (ts.at[18][41] >> 4) != 0; }

static void bridge_bottom(Surf& s, const TextScreen& ts) {
  std::string t1 = row_str(ts, 1, 29, 50), title;
  for (char c : t1) { if ((uint8_t)c == 0xCD) c = ' '; title += c; }
  // collapse spaces
  std::string tt; bool sp = false;
  for (char c : trim(title)) { if (c == ' ') { if (!sp) tt += c; sp = true; } else { tt += c; sp = false; } }
  std::string port = trim(row_str(ts, 7, 26, 29)), stbd = trim(row_str(ts, 7, 51, 53));
  std::string full = port + "\xf8 " + tt + " " + stbd + "\xf8";
  Box in = tile(s, 0, 0, 180, UP_H, full.c_str());
  TextScreen t2 = ts;
  t2.ch[7][28] = ' '; t2.at[7][28] = 0;     // last digit of the port heading, not an axis label
  if (TAC_X + 22 * TAC_CW > in.x1 || TAC_Y + 12 * TAC_LH > UP_H - 1) fit_problem("tactical display is bigger than its tile");
  cells(s, t2, 28, 2, 49, 13, F8x14, TAC_X, TAC_Y, TAC_CW, TAC_LH);
  int tc, tr;
  (void)tc; (void)tr;
  in = tile(s, 180, 0, 236, UP_H, "NAV");
  cells_in(s, ts, 18, 2, 25, 13, F6x12, in.x0, in.y0, in.x1, in.y1);
  // the action column, x 238..314: six rows of 24 px, 8 px apart, y 6..190 (rule 11), beside the
  // prompt strip and the menu row (which stops at x 230 on the bridge); shields and speed are +/-
  // pairs of 34-px buttons, 8 px apart, so the tap mode and FREEZE still fit
  const int AX0 = 238, AX1 = 314, AXM = 272;   // halves: 238..272 and 280..314
  s.fill(236, 0, BOT_W, ROW1_Y1, BG);   // clear behind the buttons: text from the previous screen showed in the gaps
  auto ry = [](int r) { return 6 + r * 32; };
  // weapons fire only at red alert (the game answers "not energized" otherwise): greyed until then
  const int alert = alert_level(ts);
  const int locks = weapon_locks(ts);        // a weapon with a lock is lit: that is when firing does something
  add_btn(AX0, ry(0), AX1, ry(0) + 24, "PHASERS", &F8x16, key_act(K_INSERT), -1, (locks & 1) != 0);
  if (alert != 2) btns.back().off = true;
  add_btn(AX0, ry(1), AX1, ry(1) + 24, "TORPS", &F8x16, key_act(K_DELETE), -1, (locks & 2) != 0);
  if (alert != 2) btns.back().off = true;
  add_btn(AX0, ry(2), AXM, ry(2) + 24, "SHLD+", &F6x12, key_act((Key)(K_F1 + 7)));
  add_btn(AXM + GAP, ry(2), AX1, ry(2) + 24, "SHLD-", &F6x12, key_act((Key)(K_F1 + 6)));
  add_btn(AX0, ry(3), AXM, ry(3) + 24, "SPD+", &F6x12, key_act((Key)(K_F1 + 3)));
  add_btn(AXM + GAP, ry(3), AX1, ry(3) + 24, "SPD-", &F6x12, key_act((Key)(K_F1 + 2)));
  // The last two rows follow the situation (plan item 3), so the next sensible action is one tap:
  // docked: LAUNCH / ORDERS; cruising: STOP or ENGINES / STARFORT (the game's own autopilot to the
  // nearest starfort, Alt+J); alert: ECM / FREEZE; orbit: TELEPORT / ENGINES. Otherwise the tap
  // mode and FREEZE as before. L still freezes and R still changes the tap mode everywhere.
  Situation sit = bridge_situation(ts);
  float cur = bridge_speed(ts, 16);
  auto row = [&](int r, const char* lab, Act a) { add_btn(AX0, ry(r), AX1, ry(r) + 24, lab, &F8x16, a); };
  switch (sit) {
    // (docked: ORDERS is already a tab below, so the second row is the advisor; UI review 10)
    case SIT_DOCKED: row(4, "LAUNCH", kind_act(A_LAUNCH)); add_btn(AX0, ry(5), AX1, ry(5) + 24, "WHAT NOW?", &F6x12, kind_act(A_MISSION)); break;
    case SIT_CRUISE:
      if (cur > 0.0f) row(4, "STOP", key_act(K_F10)); else row(4, "ENGINES", kind_act(A_ENGINES));
      row(5, "STARFORT", key_act(K_J, M_ALT)); break;
    case SIT_ALERT:
      // red alert: ECM and FREEZE; orange (enemy space, nobody shooting yet): the engines stay in reach
      // (both reviews, Nov 21: engines must stay in reach at any alert)
      if (alert != 2) {              // orange flies like green: the same two rows
        if (cur > 0.0f) row(4, "STOP", key_act(K_F10)); else row(4, "ENGINES", kind_act(A_ENGINES));
        row(5, "STARFORT", key_act(K_J, M_ALT));
      } else {
        add_btn(AX0, ry(4), AXM, ry(4) + 24, "ECM", &F6x12, key_act(K_E, M_ALT), -1, ecm_on(ts));       // (lit while it is on)
        add_btn(AXM + GAP, ry(4), AX1, ry(4) + 24, frozen(ts) ? "RESUME" : "FREEZE", &F5x9, key_act(K_F, M_CTRL));
        if (cur > 0.0f) add_btn(AX0, ry(5), AXM, ry(5) + 24, "STOP", &F6x12, key_act(K_F10));
        else add_btn(AX0, ry(5), AXM, ry(5) + 24, "GO", &F6x12, kind_act(A_ENGINES));
        add_btn(AXM + GAP, ry(5), AX1, ry(5) + 24, "HOME", &F6x12, key_act(K_J, M_ALT));   // (the way out of a fight stays in reach)
      }
      break;
    // (ENGINES keeps its row; the teleporters are a tap on the planet, so the way home stays in reach)
    case SIT_ORBIT: row(4, "ENGINES", kind_act(A_ENGINES)); row(5, "STARFORT", key_act(K_J, M_ALT)); break;
    default:
      add_btn(AX0, ry(4), AX1, ry(4) + 24, TAP_LABEL[tap_mode], &F8x16, kind_act(A_MODE), 0, true);
      row(5, "FREEZE", key_act(K_F, M_CTRL));
  }
}

// heading under the ship ("TACTICAL  n DISPLAY") and the course setting ("set: n")
static int tactical_heading(const TextScreen& ts) {
  std::string r = row_str(ts, 1, 29, 50);
  size_t p = r.find("TACTICAL");
  return p == std::string::npos ? 0 : parse_int_after(r, p + 8);
}
static int course_set(const TextScreen& ts) {
  std::string r = row_str(ts, 7, 18, 26);
  size_t p = r.find("set:");
  return p == std::string::npos ? -1 : parse_int_after(r, p + 4);
}
static void start_drive(int col, int row, int c, int r);
static void steer_to(const TextScreen& ts, int px, int py) {
  // "Select target" (manual weapon lock, tractor beam): the tap moves the game's target box there
  if (row_str(ts, 24, 0, 24).find("Select target") != std::string::npos) {
    int col = 28 + (px - TAC_X) / TAC_CW, row = 2 + (py - TAC_Y) / TAC_LH, c, r;
    if (col < 29) col = 29; if (col > 48) col = 48; if (row < 3) row = 3; if (row > 12) row = 12;
    // an enemy ship next to the tap is what was meant
    int best = 99;  int bc = col, br = row;
    for (int y = row - 1; y <= row + 1; y++) for (int x = col - 2; x <= col + 2; x++)
      if (y >= 2 && y <= 13 && x >= 28 && x <= 49 && ts.ch[y][x] == 0x9A) { int d = abs(x - col) + 2 * abs(y - row); if (d < best) { best = d; bc = x; br = y; } }
    if (find_tdb(ts, c, r, 28, 2, 49, 13)) { start_drive(bc, br, c, r); busy_since = now(); }
    else say("No target box on the display yet", 2000);
    return;
  }
  if (row_str(ts, 1, 26, 50).find("ORBITAL") != std::string::npos) {     // in orbit a tap on the planet opens the teleporters
    { int col = 28 + (px - TAC_X) / TAC_CW, row = 2 + (py - TAC_Y) / TAC_LH;       // (an enemy ship is not the planet)
      for (int y = row - 1; y <= row + 1; y++) for (int x = col - 2; x <= col + 2; x++)
        if (y >= 2 && y <= 13 && x >= 28 && x <= 49 && ts.ch[y][x] == 0x9A) { say("Enemy ship. PHASERS / TORPS fire when lit", 3000); return; } }
    chk_pressed = "PLANET";
    press(K_P, M_ALT); busy_since = now(); say("Teleporters: landing parties and transfers", 2500); return;
  }
  chk_pressed = "TACTICAL";
  // tap -> grid units from the ship (centre cell col 39, row 7); x step is 2 columns
  float col = 28 + (px - TAC_X) / (float)TAC_CW, row = 2 + (py - TAC_Y) / (float)TAC_LH;
  float dx = (col - 39.5f) / 2.0f, dy = row - 7.5f;
  if (fabsf(dx) < 0.4f && fabsf(dy) < 0.4f) { say("Tap away from the ship to steer"); return; }
  int rel = (int)lroundf(atan2f(dx, -dy) * 180.0f / 3.14159265f);
  int heading = tactical_heading(ts), cur = course_set(ts);
  if (cur < 0) cur = heading;
  int want = ((heading + rel) % 360 + 360) % 360;
  int delta = ((want - cur) % 360 + 540) % 360 - 180;     // -180..179
  int n15 = abs(delta) / 15, n1 = abs(delta) % 15;
  Key k = delta >= 0 ? (Key)(K_F1 + 5) : (Key)(K_F1 + 4);   // F6 = +1, F5 = -1
  for (int i = 0; i < n15; i++) press(k, M_SHIFT);
  for (int i = 0; i < n1; i++) press(k);
  char msg[64]; snprintf(msg, sizeof msg, "Course %d\xf8 (%+d\xf8)", want, delta); say(msg);
}

// ------------------------------------------------------------------ generic views
// ------------------------------------------------------------------ other displays
// Every screen apart from the bridge is split by its content, so nothing appears on both
// screens: a menu becomes big buttons on the touch screen, a map goes on the touch screen to
// be tapped, and the rest of the game's screen stays on top.
enum Kind { G_PLAIN, G_MENU, G_MAP, G_WIN };   // G_WIN: a window the game opened over the bridge
struct GLine { int row, col, end; char key; std::string label; bool sel; };
struct GLayout {
  Kind kind;
  int r0, c0, r1, c1;                 // the part of the game screen shown on the touch screen
  std::vector<GLine> items;           // menu items (G_MENU)
  int ox, oy, cw, lh; const Font* f;  // where the map is drawn (G_MAP)
};
static GLayout glay;
static int glay_kind() { return glay.kind; }
static_assert(G_MENU == 1 && G_MAP == 2, "context_hint() reads glay_kind() as numbers");

static bool is_key_char(uint8_t c) { return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); }
static bool is_alpha(uint8_t c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }

// Grow a rectangle over the box drawn around it (frame lines), at most a few cells each way.
static void grow_to_box(const TextScreen& ts, int& r0, int& c0, int& r1, int& c1) {
  for (int step = 0; step < 3; step++) {
    bool any = false;
    if (c0 > 0) { for (int r = r0; r <= r1; r++) if (is_box(ts.ch[r][c0 - 1])) { c0--; any = true; break; } }
    if (c1 < 79) { for (int r = r0; r <= r1; r++) if (is_box(ts.ch[r][c1 + 1])) { c1++; any = true; break; } }
    if (!any) break;
  }
  for (int step = 0; step < 4; step++) {
    bool any = false;
    if (r0 > 0) { for (int c = c0; c <= c1; c++) if (is_box(ts.ch[r0 - 1][c])) { r0--; any = true; break; } }
    if (r1 < 23) { for (int c = c0; c <= c1; c++) if (is_box(ts.ch[r1 + 1][c])) { r1++; any = true; break; } }
    if (!any) break;
  }
}

// Menu lines of the form "N  Start a new mission": a key, two spaces, a label; 3 or more in a column.
static bool find_menu(const TextScreen& ts, GLayout& g) {
  std::vector<GLine> all;
  for (int r = 0; r < 24; r++)
    for (int c = 0; c + 4 < 80; c++) {
      uint8_t k = ts.ch[r][c];
      if (!is_key_char(k)) continue;
      if (c > 0 && ts.ch[r][c - 1] != ' ' && !is_box(ts.ch[r][c - 1])) continue;
      // ("1* Basic navigation": the tutorials menu stars the ones already run)
      if ((ts.ch[r][c + 1] != ' ' && ts.ch[r][c + 1] != '*') || ts.ch[r][c + 2] != ' ' || !is_alpha(ts.ch[r][c + 3])) continue;
      int e = c + 3;
      while (e + 1 < 80 && !is_box(ts.ch[r][e + 1]) && !(ts.ch[r][e + 1] == ' ' && (e + 2 >= 80 || ts.ch[r][e + 2] == ' '))) e++;
      std::string lab = trim(row_str(ts, r, c + 3, e + 1));
      if (lab.size() < 3) continue;
      GLine l; l.row = r; l.col = c; l.end = e; l.key = (char)k; l.label = lab;
      if (ts.ch[r][c + 1] == '*') l.label += " (done)";
      l.sel = ((ts.at[r][c + 3] >> 4) & 7) == 7;
      all.push_back(l);
      c = e;
    }
  int best_col = -1, best_n = 0;
  for (auto& l : all) {
    int n = 0;
    for (auto& m : all) if (abs(m.col - l.col) <= 1) n++;
    if (n > best_n) { best_n = n; best_col = l.col; }
  }
  if (best_n < 3) return false;
  g.items.clear();
  for (auto& l : all) if (abs(l.col - best_col) <= 1) g.items.push_back(l);
  g.r0 = 24; g.r1 = -1; g.c0 = 80; g.c1 = -1;
  for (auto& l : g.items) {
    g.r0 = std::min(g.r0, l.row); g.r1 = std::max(g.r1, l.row);
    g.c0 = std::min(g.c0, l.col); g.c1 = std::max(g.c1, l.end);
  }
  if (g.r1 - g.r0 + 1 > (int)g.items.size() * 2) return false;    // scattered: not one menu
  grow_to_box(ts, g.r0, g.c0, g.r1, g.c1);
  g.kind = G_MENU;
  return true;
}

// A map: the biggest framed area whose inside is mostly empty space with scattered symbols
// (star, system and war maps, combat control), or failing that a block of grid-dot rows.
static bool tl_corner(uint8_t c) { return c == 0xDA || c == 0xC9 || c == 0xD5 || c == 0xD6; }
static bool tr_corner(uint8_t c) { return c == 0xBF || c == 0xBB || c == 0xB8 || c == 0xB7; }
static bool bl_corner(uint8_t c) { return c == 0xC0 || c == 0xC8 || c == 0xD4 || c == 0xD3; }
static bool fit_map(GLayout& g, int r0, int c0, int r1, int c1) {
  int cols = c1 - c0 + 1, rows = r1 - r0 + 1;
  int cw = std::min(8, BOT_W / cols), lh = std::min(14, UP_H / rows);
  if (cw < 4 || lh < 7) return false;                    // too big for the touch screen
  g.f = (cw >= 8 && lh >= 14) ? &F8x14 : (cw >= 6 && lh >= 12) ? &F6x12 : &F5x9;
  g.cw = cw; g.lh = lh;                                  // (5x9 in 4-px cells: its spacing column is dropped)
  g.r0 = r0; g.r1 = r1; g.c0 = c0; g.c1 = c1;
  g.ox = (BOT_W - cols * cw) / 2; g.oy = (UP_H - rows * lh) / 2;
  g.kind = G_MAP;
  return true;
}
static bool find_map(const TextScreen& ts, GLayout& g) {
  int best = 0, br0 = 0, bc0 = 0, br1 = 0, bc1 = 0;
  bool label_frame = false;      // the best frame is one with labels over its top corners
  auto consider = [&](int r, int c, int r1, int c1) {
    int w = c1 - c - 1, h = r1 - r - 1;
    if (w < 24 || h < 6) return;
    int used = 0, letters = 0, symbols = 0;
    for (int rr = r + 1; rr < r1; rr++)
      for (int cc = c + 1; cc < c1; cc++) {
        uint8_t ch = ts.ch[rr][cc];
        if (ch > ' ' && ch != 0xFF && !is_box(ch)) { used++; if (ch != 0xFA && ch != 0xF9) symbols++; }
        if (is_alpha(ch)) letters++;
      }
    // a map is sparse, has something on it, and is not a page of words
    if (symbols < 2 || used * 100 > w * h * 45 || letters * 100 > w * h * 12) return;
    if (w * h > best) { best = w * h; br0 = r; bc0 = c; br1 = r1; bc1 = c1; label_frame = false; }
  };
  for (int r = 0; r < 23; r++)
    for (int c = 0; c < 79; c++) {
      if (!tl_corner(ts.ch[r][c])) continue;
      // (frame edges may carry labels, e.g. "0  20  40" along a map's top: mostly line is enough)
      int c1 = c + 1, line = 0;
      while (c1 < 80 && !tr_corner(ts.ch[r][c1])) { if (is_box(ts.ch[r][c1])) line++; c1++; }
      if (c1 >= 80 || line * 10 < (c1 - c - 1) * 6) continue;
      int r1 = r + 1; line = 0;
      while (r1 < 24 && !bl_corner(ts.ch[r1][c])) { if (is_box(ts.ch[r1][c])) line++; r1++; }
      if (r1 >= 24 || line * 10 < (r1 - r - 1) * 6) continue;
      consider(r, c, r1, c1);
    }
  // A frame whose top corners are overwritten by axis labels (the system map: " 0====20====...80"):
  // found from its bottom corners, going up the left edge.
  for (int r1 = 23; r1 >= 7; r1--)
    for (int c = 0; c < 56; c++) {
      if (!bl_corner(ts.ch[r1][c])) continue;
      int c1 = c + 1, line = 0;
      while (c1 < 80 && ts.ch[r1][c1] != 0xD9 && ts.ch[r1][c1] != 0xBC && ts.ch[r1][c1] != 0xBE && ts.ch[r1][c1] != 0xBD) { if (is_box(ts.ch[r1][c1])) line++; c1++; }
      if (c1 >= 80 || line * 10 < (c1 - c - 1) * 6) continue;
      int r = r1 - 1;
      while (r > 0 && is_box(ts.ch[r][c]) && !tl_corner(ts.ch[r][c])) r--;
      if (tl_corner(ts.ch[r][c])) continue;                 // an ordinary frame: found above
      if (!(ts.ch[r][c] >= '0' && ts.ch[r][c] <= '9') || !(ts.ch[r][c1] >= '0' && ts.ch[r][c1] <= '9')) continue;
      { int was = best; consider(r, c, r1, c1); if (best != was) label_frame = true; }
    }
  if (best) {
    // take in the axis labels just outside the frame
    int ec0 = bc0, ec1 = bc1;
    for (int k = 0; k < 3 && ec0 > 0; k++) { bool any = false; for (int rr = br0 + 1; rr < br1; rr++) if (ts.ch[rr][ec0 - 1] > ' ') any = true; if (!any) break; ec0--; }
    // (a label-cornered frame's right neighbour is another panel, not axis labels)
    for (int k = 0; k < 3 && ec1 < 79 && !label_frame; k++) { bool any = false; for (int rr = br0 + 1; rr < br1; rr++) if (ts.ch[rr][ec1 + 1] > ' ') any = true; if (!any) break; ec1++; }
    if (fit_map(g, br0, ec0, br1, ec1)) return true;
    if (fit_map(g, br0, bc0, br1, bc1)) return true;
    if (fit_map(g, br0 + 1, bc0 + 1, br1 - 1, bc1 - 1)) return true;      // without its frame
  }
  // grid dots (e.g. an unframed tactical grid)
  int dots[24] = {0};
  for (int r = 0; r < 24; r++)
    for (int c = 0; c < 80; c++) if (ts.ch[r][c] == 0xFA || ts.ch[r][c] == 0xF9) dots[r]++;
  int b0 = -1, b1 = -1, r = 0;
  while (r < 24) {
    if (dots[r] < 6) { r++; continue; }
    int a = r, b = r;
    while (b + 1 < 24 && (dots[b + 1] >= 6 || (b + 2 < 24 && dots[b + 2] >= 6))) b++;
    if (b - a > b1 - b0) { b0 = a; b1 = b; }
    r = b + 1;
  }
  if (b0 < 0 || b1 - b0 + 1 < 5) return false;
  int c0 = 80, c1 = -1;
  for (int rr = b0; rr <= b1; rr++)
    for (int c = 0; c < 80; c++) if (ts.ch[rr][c] == 0xFA || ts.ch[rr][c] == 0xF9) { c0 = std::min(c0, c); c1 = std::max(c1, c); }
  return fit_map(g, b0, c0, b1, c1);
}

// A window the game opened over the bridge (damage control, the log, help pages, the landing-party
// table): a double-line box, at most 53 columns wide so its text is readable on the touch screen
// in the 6x12 font. (UI review 3: these screens were a squashed 80x25 on top plus an arrow pad.)
static int win_top = 0;                 // first row shown when the window is taller than the touch screen
static const int WIN_ROWS = 11, WIN_Y0 = 4;
static bool find_window(const TextScreen& ts, GLayout& g) {
  int best = 0;
  if (row_str(ts, 1, 0, 80).find("COMBAT") != std::string::npos && row_str(ts, 1, 0, 80).find("INFORMATION") != std::string::npos) return false;   // combat control: its grid is the subject, not its menu box
  for (int r = 0; r < 22; r++)
    for (int c = 0; c < 68; c++) {
      if (ts.ch[r][c] != 0xC9) continue;
      int c2 = c + 1; while (c2 < 80 && ts.ch[r][c2] != 0xBB) c2++;
      if (c2 >= 80 || ts.ch[r][c2] != 0xBB || c2 - c < 12 || c2 - c + 1 > 53) continue;
      int r2 = r + 1; while (r2 < 25 && ts.ch[r2][c] != 0xC8) r2++;
      if (r2 >= 25 || ts.ch[r2][c2] != 0xBC || r2 - r < 3) continue;
      int sides = 0; for (int y = r + 1; y < r2; y++) if (ts.ch[y][c] == 0xBA || ts.ch[y][c] == 0xCC || ts.ch[y][c] == 0xC7) sides++;
      if (sides * 2 < r2 - r - 1) continue;
      int area = (c2 - c) * (r2 - r);
      if (area > best) { best = area; g.r0 = r; g.c0 = c; g.r1 = r2; g.c1 = c2; }
    }
  if (!best) return false;
  g.kind = G_WIN; g.f = &F6x12; g.cw = 6; g.lh = 12;
  g.ox = (BOT_W - 8 - (g.c1 - g.c0 + 1) * 6) / 2; if (g.ox < 0) g.ox = 0;
  g.oy = WIN_Y0;
  return true;
}
static void analyse(const TextScreen& ts) {
  glay.kind = G_PLAIN; glay.items.clear(); glay.r0 = glay.c0 = 0; glay.r1 = glay.c1 = -1;
  if (kb_open) return;
  if (!dialog_question(ts).empty() && !lp_table(ts)) return;   // a question is up: YES/NO, not the menu behind it
  // The Targeter window opens over a corner of the star maps, which hides the frame the map is
  // found by: keep the map where it was while that window is up (emulator, Nov 21).
  static GLayout last_map; static bool have_map = false; static uint8_t map_edge[80];
  if (dc_table(ts) && in_flight(ts) && find_window(ts, glay)) return;   // (the tactical grid behind it is not a star map)
  glay.kind = G_PLAIN; glay.r1 = glay.c1 = -1;
  if (find_map(ts, glay)) { last_map = glay; have_map = true; memcpy(map_edge, ts.ch[glay.r1], 80); return; }
  if (have_map && in_flight(ts) && screen_has(ts, "enter target") && screen_has(ts, "TARGETER")) {
    // (the same map: its bottom edge is where it was, cell for cell)
    int same = 0, n = last_map.c1 - last_map.c0 + 1;
    for (int c = last_map.c0; c <= last_map.c1; c++) if (ts.ch[last_map.r1][c] == map_edge[c]) same++;
    if (same * 10 >= n * 9) { glay = last_map; return; }
  }
  if (!in_flight(ts)) have_map = false;
  if (find_menu(ts, glay)) return;
  glay.kind = G_PLAIN; glay.r1 = glay.c1 = -1;
  if (in_flight(ts) && find_window(ts, glay)) return;
  glay.kind = G_PLAIN; glay.r1 = glay.c1 = -1;
}

// ---- touch-screen scroll area (the text reader, a long menu): drag to scroll, rule 13
struct ScrollArea { bool on; int x0, y0, x1, y1, step; int* pos; int max; };
static ScrollArea sarea = {false, 0, 0, 0, 0, 1, 0, 0};
static bool sdrag = false, sd_maybe = false; static int sd_y = 0, sd_ly = 0, sd_acc = 0; static float sd_mom = 0;
static void sarea_scroll() {          // move by whole steps; the remainder waits for more drag
  if (!sarea.on || !sarea.pos) { sd_acc = 0; return; }
  while (sd_acc >= sarea.step) { sd_acc -= sarea.step; if (*sarea.pos < sarea.max) { (*sarea.pos)++; dirty(); } else { sd_acc = 0; sd_mom = 0; } }
  while (sd_acc <= -sarea.step) { sd_acc += sarea.step; if (*sarea.pos > 0) { (*sarea.pos)--; dirty(); } else { sd_acc = 0; sd_mom = 0; } }
}
static void scroll_bar(Surf& s, int x, int y0, int y1, int pos, int max, int vis) {
  if (max <= 0) return;
  int total = max + vis, th = (y1 - y0) * vis / total; if (th < 10) th = 10;
  int ty = y0 + (y1 - y0 - th) * pos / max;
  s.fill(x, y0, x + 4, y1, rgb(40, 44, 60));
  s.fill(x, ty, x + 4, ty + th, BTN_EDGE);
}

// ---- P2-1 text reader: the page's text on the touch screen in 6x12 (frames dropped, long rows
// wrapped at 50 columns), dragged or stick-scrolled; the game's own page keys stay in a row below.
static char hotkey_at(const TextScreen& ts, int col, int row);
struct RLine { std::string ch, at; std::vector<int16_t> src;  bool small = false; };   // src: row * 80 + col per cell, -1 none
static std::vector<RLine> rlines;
static int rd_scroll = 0, rd_max = 0;
static uint32_t rd_sig = 0;
static const int RD_Y0 = 4, RD_LINES = 11, RD_COLS = 50, RD_X0 = 6;
static bool frame_cell(uint8_t c) { return c >= 0xB0 && c <= 0xDF; }   // box lines, blocks, shades
static bool blank_cell(uint8_t c) { return c <= ' ' || c == 0xFF || frame_cell(c); }
// Rows of prose that the game wrapped itself are joined into one paragraph and wrapped again at 50
// columns; short rows, indented rows and table rows (with wide gaps) stay rows of their own.
static bool rd_wide_table = false;
static void build_reader(const TextScreen& ts) {
  rlines.clear(); rd_wide_table = false;
  int minc = 80, maxe = -1;
  int first[24], last_c[24];
  for (int r = 0; r < 24; r++) {
    first[r] = -1; last_c[r] = -1;
    for (int c = 0; c < 80; c++) if (!blank_cell(ts.ch[r][c])) { if (first[r] < 0) first[r] = c; last_c[r] = c; }
    if (first[r] >= 0) { minc = std::min(minc, first[r]); maxe = std::max(maxe, last_c[r]); }
  }
  if (maxe < 0) return;
  auto table_row = [&](int r) {
    for (int c = first[r]; c + 2 <= last_c[r]; c++)
      if (blank_cell(ts.ch[r][c]) && blank_cell(ts.ch[r][c + 1]) && blank_cell(ts.ch[r][c + 2])) return true;
    return false;
  };
  struct Para { std::string ch, at; std::vector<int16_t> src; int ind; int fixed; };   // fixed: 1 a table row (never wrapped), 2 one in the small font
  std::vector<Para> paras;
  bool blank_before = true;
  // Tables (UI review 7: 80-column rows wrapped at 50 lost their header and their columns): a block
  // of table rows keeps its columns; the gaps every row shares shrink to one space, and if the
  // rows still do not fit 50 columns they are set in the 5x9 font (64 columns).
  int tb_end = -1; std::vector<int> tb_cols; int tb_fixed = 0;
  for (int r = 0; r < 24; r++) {
    if (r > tb_end && first[r] >= 0 && table_row(r) && last_c[r] - minc + 1 > RD_COLS) {
      int e = r; while (e + 1 < 24 && first[e + 1] >= 0 && (table_row(e + 1) || last_c[e + 1] - first[e + 1] < 40)) e++;
      int lo = 80, hi = -1; for (int y = r; y <= e; y++) { lo = std::min(lo, first[y]); hi = std::max(hi, last_c[y]); }
      std::vector<int> cols; bool gap = false;
      for (int c = lo; c <= hi; c++) {
        bool all_blank = true; for (int y = r; y <= e && all_blank; y++) if (!blank_cell(ts.ch[y][c])) all_blank = false;
        if (!all_blank) { cols.push_back(c); gap = false; } else if (!gap) { cols.push_back(c); gap = true; }
      }
      if (e > r && (int)cols.size() <= RD_COLS) { tb_end = e; tb_cols = cols; tb_fixed = 1; }
      else if (e > r + 1) rd_wide_table = true;     // a real table too wide to read here: the page stays whole on the top screen
    }
    if (r <= tb_end && first[r] >= 0) {
      Para p{"", "", {}, 0, tb_fixed};
      for (int c : tb_cols) { uint8_t x = ts.ch[r][c]; p.ch += (char)(blank_cell(x) ? ' ' : x); p.at += (char)ts.at[r][c]; p.src.push_back((int16_t)(r * 80 + c)); }
      while (!p.ch.empty() && p.ch.back() == ' ') { p.ch.pop_back(); p.at.pop_back(); p.src.pop_back(); }
      paras.push_back(p); blank_before = false; continue;
    }
    if (first[r] < 0) {
      if (!blank_before) paras.push_back(Para{"", "", {}, 0, 0});
      blank_before = true; continue;
    }
    int ind = first[r] - minc;
    bool join = !blank_before && !paras.empty() && r > 0 && first[r - 1] >= 0 &&
                r - 1 > tb_end && first[r - 1] == first[r] && last_c[r - 1] >= minc + (maxe - minc) * 3 / 4 &&
                last_c[r - 1] - first[r - 1] >= 40 &&   // only long rows were wrapped prose (names, credits stay rows)
                !table_row(r) && !table_row(r - 1) && (ts.at[r][first[r]] & 15) == (ts.at[r - 1][first[r - 1]] & 15);
    if (!join) paras.push_back(Para{"", "", {}, ind, 0});
    Para& p = paras.back();
    if (join) { p.ch += ' '; p.at += (char)7; p.src.push_back(-1); }
    int c0 = join ? first[r] : minc + ind;
    for (int c = c0; c <= last_c[r]; c++) {
      uint8_t x = ts.ch[r][c];
      p.ch += (char)(blank_cell(x) ? ' ' : x); p.at += (char)ts.at[r][c]; p.src.push_back((int16_t)(r * 80 + c));
    }
    blank_before = false;
  }
  while (!paras.empty() && paras.back().ch.empty()) paras.pop_back();
  for (auto& p : paras) {
    if (p.ch.empty()) { rlines.push_back(RLine()); continue; }
    if (p.fixed) { RLine l; l.ch = p.ch; l.at = p.at; l.src = p.src; l.small = p.fixed == 2; rlines.push_back(l); continue; }
    int ind = std::min(p.ind, 20), pos = 0, n = (int)p.ch.size();
    while (pos < n) {
      int lead = ind;
      int room = RD_COLS - lead;
      int len = n - pos;
      if (len > room) {
        int cut = (int)p.ch.rfind(' ', pos + room);
        len = (cut > pos) ? cut - pos : room;
      }
      RLine l;
      l.ch = std::string(lead, ' ') + p.ch.substr(pos, len);
      l.at = std::string(lead, (char)7) + p.at.substr(pos, len);
      l.src.assign(lead, -1); l.src.insert(l.src.end(), p.src.begin() + pos, p.src.begin() + pos + len);
      rlines.push_back(l);
      pos += len;
      while (pos < n && p.ch[pos] == ' ') pos++;   // the space at the break
    }
  }
}
// Only pages that are mostly words: title art drawn with block characters and star fields
// (the STAR FLEET logo pages) keep the plain controls instead of a reader full of dots.
static bool mostly_text(const TextScreen& ts) {
  int letters = 0, art = 0;
  // the page's own background colour (the orders pages are grey all over): the commonest
  // background among blank cells; only blanks in another colour count as drawn art
  int bgc[16] = {0};
  for (int r = 0; r < 24; r++)
    for (int c = 0; c < 80; c++) { uint8_t x = ts.ch[r][c]; if (x == ' ' || x == 0 || x == 0xFF) bgc[ts.at[r][c] >> 4]++; }
  int page_bg = 0;
  for (int k = 1; k < 16; k++) if (bgc[k] > bgc[page_bg]) page_bg = k;
  for (int r = 0; r < 24; r++)
    for (int c = 0; c < 80; c++) {
      uint8_t x = ts.ch[r][c];
      if ((x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z')) letters++;
      else if ((x >= 0xB0 && x <= 0xB2) || (x >= 0xDB && x <= 0xDF) || x == 0xFE || x == 0xF9 || x == 0xFA) art++;
      else if ((x == ' ' || x == 0 || x == 0xFF) && (ts.at[r][c] >> 4) != 0 && (ts.at[r][c] >> 4) != page_bg) art++;   // coloured blank cells: grids, bars (not the page's own background)
    }
  return letters >= 60 && art * 2 < letters;
}
static bool reader_wanted(const TextScreen& ts) {
  return glay.kind == G_PLAIN && !kb_open && ts.text_mode && !in_flight(ts) && mostly_text(ts);
}
static void draw_reader(Surf& s, const TextScreen& ts) {
  build_reader(ts);
  uint32_t sig = 2166136261u;          // a different page (its first lines changed) starts at the top
  for (int i = 0, k = 0; i < (int)rlines.size() && k < 3; i++) if (!rlines[i].ch.empty()) { for (char c : rlines[i].ch) { sig ^= (uint8_t)c; sig *= 16777619u; } k++; }
  if (sig != rd_sig) { rd_sig = sig; rd_scroll = 0; }
  rd_max = (int)rlines.size() - RD_LINES; if (rd_max < 0) rd_max = 0;
  if (rd_scroll > rd_max) rd_scroll = rd_max;
  for (int i = 0; i < RD_LINES && rd_scroll + i < (int)rlines.size(); i++) {
    const RLine& l = rlines[rd_scroll + i];
    for (size_t k = 0; k < l.ch.size(); k++) {
      uint8_t a = (uint8_t)l.at[k];
      int bgc = (a >> 4) & 7;
      uint16_t bg = bgc ? VGA[bgc] : BG;
      uint16_t fg = (a & 15) == bgc ? (bgc ? VGA[0] : BTN_TXT) : READ_FG[a & 15];
      if (l.small) draw_cell(s, (int)k * 5, RD_Y0 + i * 12, 5, 12, (uint8_t)l.ch[k], fg, bg, F5x9);
      else draw_cell(s, RD_X0 + (int)k * 6, RD_Y0 + i * 12, 6, 12, (uint8_t)l.ch[k], fg, bg, F6x12);
    }
  }
  scroll_bar(s, 310, RD_Y0, RD_Y0 + RD_LINES * 12, rd_scroll, rd_max, RD_LINES);
  sarea = ScrollArea{true, 0, 0, 308, RD_Y0 + RD_LINES * 12, 12, &rd_scroll, rd_max};
  // the game's own keys for stepping through pages
  static const char* labs[7] = {"PG UP", "\x1b", "\x18", "\x19", "\x1a", "PG DN", "SPACE"};
  static const Key keys[7] = {K_PGUP, K_LEFT, K_UP, K_DOWN, K_RIGHT, K_PGDN, K_SPACE};
  for (int i = 0; i < 7; i++) {
    int a, b; split_gap(6, BOT_W - 6, 7, i, a, b);   // (8-px gaps: PG UP, PG DN, SPACE drop to 6x12)
    // the up/down arrows scroll the text first, like the sticks (rule 12: dragging has a button too)
    Act act = key_act(keys[i]);
    if (keys[i] == K_UP || keys[i] == K_DOWN) { act.kind = A_RDLINE; act.arg = keys[i] == K_UP ? -1 : 1; }
    if (keys[i] == K_PGUP || keys[i] == K_PGDN) act = kind_act(A_RDPAGE, keys[i] == K_PGUP ? -1 : 1);
    add_btn(a, 140, b, 164, labs[i], &F8x16, act);
  }
}
// A tap on the reader's text: a word with a red hotkey letter (or an "X  Label" line) sends that key.
static char reader_hotkey(int x, int y) {
  int li = rd_scroll + (y - RD_Y0) / 12, k = (x - RD_X0) / 6;
  if (y < RD_Y0 || li < 0 || li >= (int)rlines.size() || k < 0) return 0;
  const RLine& l = rlines[li];
  if (l.small) k = x / 5;
  if (k >= (int)l.ch.size() || l.ch[k] == ' ' || l.src[k] < 0) return 0;
  return hotkey_at(last, l.src[k] % 80, l.src[k] / 80);
}
// Text pages (Ryan, Nov 20): the page is read on the touch screen, so the top screen does not show
// it again. It shows the house header bar with the page's title (its first line), where the reader
// is in the page (lines a-b of n, and a bar), and the controls in the gold strip.
static void reader_top(Surf& s, const char* hint) {
  s.fill(0, 0, TOP_W, TOP_H, BG);
  // the title: the page's first paragraph when it fits the header in 6x12 (a heading the reader
  // wrapped at 50 columns), else its first line
  std::string title, para;
  size_t i = 0;
  while (i < rlines.size() && trim(rlines[i].ch).empty()) i++;
  if (i < rlines.size()) title = trim(rlines[i].ch);
  for (; i < rlines.size() && !trim(rlines[i].ch).empty(); i++) para += (para.empty() ? "" : " ") + trim(rlines[i].ch);
  if (text_width(para.c_str(), F6x12) <= TOP_W - 16) title = para;
  s.fill(0, 0, TOP_W, 22, BTN);
  s.hline(0, TOP_W, 22, BTN_EDGE);
  // reader lines are at most RD_COLS (50) characters: 6x12 always fits the 384 px between the margins
  const Font* tf = text_width(title.c_str(), F8x16) <= TOP_W - 16 ? &F8x16 : &F6x12;
  fit_text(s, 8, (22 - tf->h) / 2, TOP_W - 8, title.c_str(), *tf, TITLE, "reader title");
  const int n = (int)rlines.size();
  const int a = rd_scroll + 1, b = std::min(n, rd_scroll + RD_LINES);
  const char* where = "Reading on the touch screen";
  fit_text(s, (TOP_W - text_width(where, F6x12)) / 2, 70, TOP_W - 8, where, F6x12, HINT, "reader top");
  char pos[48];
  if (rd_max <= 0) snprintf(pos, sizeof pos, "All %d lines shown", n);
  else snprintf(pos, sizeof pos, "Lines %d-%d of %d", a, b, n);
  fit_text(s, (TOP_W - text_width(pos, F8x16)) / 2, 96, TOP_W - 8, pos, F8x16, BTN_TXT, "reader position");
  // the page as a bar: the gold part is the stretch of it on the touch screen
  const int bx0 = 40, bx1 = TOP_W - 40, by0 = 128, by1 = 140;
  s.rect(bx0, by0, bx1, by1, BTN_EDGE);
  int span = bx1 - bx0 - 4;
  int ta = n ? span * rd_scroll / n : 0, tb = n ? span * b / n : span;
  if (tb - ta < 6) tb = std::min(span, ta + 6);
  s.fill(bx0 + 2 + ta, by0 + 2, bx0 + 2 + tb, by1 - 2, TITLE);
  const char* ends = rd_max <= 0 ? "" : rd_scroll >= rd_max ? "End of the page" : rd_scroll == 0 ? "Top of the page" : "";
  if (*ends) fit_text(s, (TOP_W - text_width(ends, F6x12)) / 2, 150, TOP_W - 8, ends, F6x12, HINT, "reader top");
  if (mission_on && !mis_goal.empty() && tut_num == 0) {       // (UI review 8: the top screen was nearly empty on text pages)
    std::vector<std::string> gl = wrap_words("Your mission: " + mis_goal + ".", (TOP_W - 24) / F6x12.w);
    for (size_t i = 0; i < gl.size() && i < 3; i++) fit_text(s, 12, 176 + (int)i * 12, TOP_W - 8, gl[i].c_str(), F6x12, BTN_TXT, "reader goal");
  }
  s.fill(0, 224, TOP_W, TOP_H, BTN);
  s.hline(0, TOP_W, 224, BTN_EDGE);
  fit_text(s, (TOP_W - text_width(hint, F6x12)) / 2, 226, TOP_W - 2, hint, F6x12, TITLE, "top hint");
}
// A titled window the game opened over the map (Targeter, Probes Control): the map moved to the
// touch screen, where the window is too small to read, so the top screen shows it at full size.
static bool find_window_over(const TextScreen& ts, int mr0, int mc0, int mr1, int mc1, int& wr0, int& wc0, int& wr1, int& wc1) {
  int best = 0;
  for (int r = mr0 > 2 ? mr0 - 2 : 0; r <= mr1 && r < 22; r++)
    for (int c = mc0 > 4 ? mc0 - 4 : 0; c <= mc1 && c < 66; c++) {
      if (ts.ch[r][c] != 0xC9) continue;
      int c2 = c + 1, line = 0, letters = 0;
      while (c2 < 80 && ts.ch[r][c2] != 0xBB) { if (ts.ch[r][c2] == 0xCD) line++; else if (isalpha(ts.ch[r][c2]) && ts.ch[r][c2] < 127) letters++; c2++; }
      if (c2 >= 80 || c2 - c < 14 || line < 8 || letters < 4) continue;
      if (r == mr0 && c <= mc0 + 3 && c2 >= mc1 - 3) continue;                 // the map's own frame
      int r2 = r + 1;
      while (r2 < 24 && (ts.ch[r2][c] == 0xBA || ts.ch[r2][c] == 0xCC || ts.ch[r2][c] == 0xC7)) r2++;
      if (r2 < 24 && ts.ch[r2][c] != 0xC8) r2--;                               // (it runs off under another panel)
      if (r2 - r < 2) continue;
      int area = (c2 - c) * (r2 - r);
      if (area > best) { best = area; wr0 = r; wc0 = c; wr1 = r2; wc1 = c2; }
    }
  return best > 0;
}
static void generic_top(Surf& s, const TextScreen& ts) {
  // house hint strip (look-and-feel: gold on the header colour): what to do on this screen
  const char* hint = swap_wanted ? "ZL swap is on: it applies on the bridge and in the manual."
                                 : context_hint(ts);
  ct_on = false;
  if (reader_on && !rlines.empty()) { reader_top(s, hint); return; }
  s.fill(0, 0, TOP_W, TOP_H, BG);
  cells(s, ts, 0, 0, 79, 24, F5x9, 0, 0, 5, 9);
  bool return_after_window = false;
  if (glay.kind != G_PLAIN) {        // the part on the touch screen is not repeated here
    int x0 = glay.c0 * 5, y0 = glay.r0 * 9, x1 = (glay.c1 + 1) * 5, y1 = (glay.r1 + 1) * 9;
    s.fill(x0, y0, x1, y1, BG);
    s.rect(x0, y0, x1, y1, BTN_EDGE);
    int wr0, wc0, wr1, wc1;
    if (glay.kind == G_MAP && find_window_over(ts, glay.r0, glay.c0, glay.r1, glay.c1, wr0, wc0, wr1, wc1)) {
      int cols = wc1 - wc0 + 1, rows = wr1 - wr0 + 1;
      const Font* wf = cols * 8 <= x1 - x0 - 4 && rows * 14 <= y1 - y0 - 4 ? &F8x14 : cols * 6 <= x1 - x0 - 4 && rows * 12 <= y1 - y0 - 4 ? &F6x12 : &F5x9;
      if (cols * wf->w <= x1 - x0 - 2 && rows * wf->h <= y1 - y0 - 2) {
        cells(s, ts, wc0, wr0, wc1, wr1, *wf, x0 + (x1 - x0 - cols * wf->w) / 2, y0 + (y1 - y0 - rows * wf->h) / 2, wf->w, wf->h);
        return_after_window = true;
      }
    }
    const char* note = glay.kind == G_MENU ? "Menu on the touch screen" : glay.kind == G_WIN ? "On the touch screen" : "Map on the touch screen";
    if (return_after_window) note = "";
    else if (glay.kind == G_MAP && x1 - x0 >= 194 && y1 - y0 >= 94) { ct_on = true; ct_x0 = x0 + 2; ct_y0 = y0 + 2; ct_x1 = x1 - 2; ct_y1 = y1 - 2; }
    const Font* nf = text_width(note, F6x12) <= x1 - x0 - 4 ? &F6x12 : &F5x9;
    int tw = text_width(note, *nf);
    if (tw <= x1 - x0 - 4 && y1 - y0 >= nf->h + 2)
      draw_text(s, x0 + (x1 - x0 - tw) / 2, y0 + (y1 - y0 - nf->h) / 2, note, *nf, HINT);
  }
  // (y 225, one pixel lower than the house 224: the game's row 24 ends at y 224)
  s.fill(0, 225, TOP_W, TOP_H, BTN);
  s.hline(0, TOP_W, 225, BTN_EDGE);
  fit_text(s, (TOP_W - text_width(hint, F6x12)) / 2, 227, TOP_W - 2, hint, F6x12, TITLE, "top hint");
}
static uint32_t screen_sig(const TextScreen& ts) {   // changes when a different screen appears
  uint32_t h = 2166136261u;
  for (int r = 0; r < 24; r++) for (int c = 0; c < 80; c++) { h ^= ts.ch[r][c]; h *= 16777619u; }
  return h;
}
static int menu_top = 0;               // first row shown of a menu too long for the touch screen
static void generic_bottom(Surf& s, const TextScreen& ts) {
  s.fill(0, 0, BOT_W, UP_H, BG);
  reader_on = false; menu_scrolls = false;
  if (glay.kind == G_MENU) {
    int n = (int)glay.items.size();
    int cols = n > 6 ? 2 : 1, rows = (n + cols - 1) / cols;
    // Long labels ("6 Capturing ships (tractor beam/boarding)") don't fit half the width: use one
    // full-width column in the small font when the rows are still tall enough (>= 14 px).
    if (cols == 2 && n <= 12) {
      int half = BOT_W / 2 - 6;
      for (auto& l : glay.items) if (text_width((std::string("X ") + l.label).c_str(), F6x12) > half) { cols = 1; rows = n; break; }
    }
    bool small = cols == 2 || rows > 7;
    // Rule 11: rows at least 24 px, 8 px apart. A menu with more rows than that allows shows 5 rows
    // of 24 px at a time and scrolls (drag, scroll bar; the game's own highlight is kept in view).
    bool scroll = (UP_H - 10 - GAP * (rows - 1)) / rows < 24;
    const int VIS = 5, PITCH = 32;
    if (scroll) {
      static std::string seen_first; static int seen_sel = -1;
      int sel = -1;
      for (int i = 0; i < n; i++) if (glay.items[i].sel) sel = i % rows;
      static bool practice_shown = false;
      bool signon = screen_has(ts, "SIGN-ON OPTIONS");
      if (glay.items[0].label != seen_first || want_practice_view || (signon && menu_fresh)) { seen_first = glay.items[0].label; menu_top = 0; seen_sel = -1; practice_shown = false; }
      // coach: the sign-on menu opens with Practice (and Start a new mission) in view. (Done once the
      // Practice line is there: a message box can hide it when the menu first comes back.)
      if (!practice_shown && (coach_on || want_practice_view))
        for (int i = 0; i < n; i++) if (glay.items[i].label.compare(0, 9, practice_on(ts) ? "Start a n" : tut1_done ? "Practice " : "Tutorials") == 0) {
          menu_top = i - 2 < 0 ? 0 : i - 2; if (menu_top > n - VIS) menu_top = n - VIS;
          practice_shown = true; want_practice_view = false;
          if (signon) menu_fresh = false;
        }
      if (sel >= 0 && sel != seen_sel) {
        seen_sel = sel;
        if (sel < menu_top) menu_top = sel;
        if (sel >= menu_top + VIS) menu_top = sel - VIS + 1;
      }
      if (menu_top > rows - VIS) menu_top = rows - VIS;
      if (menu_top < 0) menu_top = 0;
      scroll_bar(s, 277, 6, 6 + VIS * PITCH - GAP, menu_top, rows - VIS, VIS);
      sarea = ScrollArea{true, 0, 0, 276, 6 + VIS * PITCH, PITCH, &menu_top, rows - VIS};
      add_btn(284, 6, BOT_W - 6, 80, "\x1e", &F8x16, kind_act(A_MENUSCROLL, -VIS));   // rule 12: a button for the drag
      if (menu_top == 0) btns.back().off = true;
      add_btn(284, 88, BOT_W - 6, 6 + VIS * PITCH - GAP, "\x1f", &F8x16, kind_act(A_MENUSCROLL, VIS));
      if (menu_top >= rows - VIS) btns.back().off = true;
      menu_scrolls = true;
    }
    for (int i = 0; i < n; i++) {
      int col = i / rows, row = i % rows;
      int x0, x1, y0, y1;
      if (scroll) {
        if (row < menu_top || row >= menu_top + VIS) continue;
        split_gap(6, 274, cols, col, x0, x1);
        y0 = 6 + (row - menu_top) * PITCH; y1 = y0 + PITCH - GAP;
      } else {
        split_gap(6, BOT_W - 6, cols, col, x0, x1);
        split_gap(6, UP_H - 4, rows, row, y0, y1);   // (split_gap works for rows too)
      }
      const GLine& l = glay.items[i];
      int hot = -1;
      for (size_t k = 0; k < l.label.size(); k++)
        if (toupper((unsigned char)l.label[k]) == l.key) { hot = (int)k; break; }
      std::string lab = l.label;
      if (hot < 0) { lab = std::string(1, l.key) + " " + l.label; hot = 0; }
      // (a label that would touch both edges of its button drops to the smaller font)
      add_btn(x0, y0, x1, y1, lab, (small && !scroll) || text_width(lab.c_str(), F8x16) > x1 - x0 - 12 ? &F6x12 : &F8x16, key_act(key_for_char(l.key)), hot, l.sel);
      if (l.label == "DOS shell") btns.back().off = true;   // nothing to do at a DOS prompt on a handheld, and no easy way back
    }
  } else if (glay.kind == G_MAP) {
    { TextScreen m = ts;      // (the game's solid filler blocks beside a map's axis labels are left out)
      for (int r = glay.r0; r <= glay.r1; r++) for (int c = glay.c0; c <= glay.c1; c++) if (m.ch[r][c] == 0xDB) { m.ch[r][c] = ' '; m.at[r][c] = 0x07; }
      cells(s, m, glay.c0, glay.r0, glay.c1, glay.r1, *glay.f, glay.ox, glay.oy, glay.cw, glay.lh); }
  } else if (glay.kind == G_PLAIN && screen_has(ts, "FLEET COMMANDERS", 0, 1) && screen_has(ts, "Select Commander", 24, 24)) {
    // the commander list: each officer is a button (UI review: the rows were not tappable)
    int sel = -1; std::vector<int> rows_;
    for (int r = 3; r < 23; r++) { if (trim(row_str(ts, r, 8, 24)).empty()) continue; if ((ts.at[r][2] >> 4) == 7) sel = (int)rows_.size(); rows_.push_back(r); }
    int n = std::min((int)rows_.size(), 10);
    for (int i = 0; i < n; i++) {
      int a, b, y0, y1; split_gap(6, BOT_W - 6, 2, i % 2, a, b); split_gap(6, UP_H - 4, 5, i / 2, y0, y1);
      std::string lab = trim(row_str(ts, rows_[i], 8, 24)); const Font* lf = text_width(lab.c_str(), F8x16) <= b - a - 8 ? &F8x16 : &F6x12;
      add_btn(a, y0, b, y1, lab, lf, kind_act(A_LISTPICK, i - (sel < 0 ? 0 : sel)), -1, i == sel);
    }
  } else if (glay.kind == G_WIN) {
    int rows = glay.r1 - glay.r0 + 1, maxtop = rows > WIN_ROWS ? rows - WIN_ROWS : 0;
    static uint32_t win_sig = 0;         // another window starts at its top
    uint32_t sig = (uint32_t)(glay.r0 * 80 + glay.c0) * 131u + (uint32_t)(glay.r1 * 80 + glay.c1);
    for (int c = glay.c0; c <= glay.c1; c++) sig = sig * 16777619u ^ ts.ch[glay.r0][c];
    if (sig != win_sig) { win_sig = sig; win_top = 0; }
    if (win_top > maxtop) win_top = maxtop;
    if (win_top < 0) win_top = 0;
    int show = std::min(rows, WIN_ROWS);
    cells(s, ts, glay.c0, glay.r0 + win_top, glay.c1, glay.r0 + win_top + show - 1, F6x12, glay.ox, WIN_Y0, 6, 12);
    if (maxtop) {
      scroll_bar(s, 312, WIN_Y0, WIN_Y0 + WIN_ROWS * 12, win_top, maxtop, WIN_ROWS);
      sarea = ScrollArea{true, 0, 0, 310, WIN_Y0 + WIN_ROWS * 12, 12, &win_top, maxtop};
    }
    if (dc_table(ts)) {
      static const char* ll[6] = {"\x18", "\x19", "\x1b", "\x1a", "- TEAM", "+ TEAM"};
      const Act la[6] = {key_act(K_UP), key_act(K_DOWN), key_act(K_LEFT), key_act(K_RIGHT), key_act(K_MINUS), key_act(K_EQUALS, M_SHIFT)};
      for (int i = 0; i < 6; i++) { int a, b; split_gap(6, BOT_W - 6, 6, i, a, b); add_btn(a, 140, b, 164, ll[i], i < 4 ? &F8x16 : &F6x12, la[i]); }
    } else if (lp_table(ts)) {      // (harness, Nov 26: Up/Down pick the row, + and - change its teams; digits do nothing)
      static const char* ll[4] = {"\x18", "\x19", "- TEAM", "+ TEAM"};
      const Act la[4] = {key_act(K_UP), key_act(K_DOWN), key_act(K_MINUS), key_act(K_EQUALS, M_SHIFT)};
      for (int i = 0; i < 4; i++) { int a, b; split_gap(6, BOT_W - 6, 4, i, a, b); add_btn(a, 140, b, 164, ll[i], &F8x16, la[i]); }
    } else {
    static const char* labs[7] = {"PG UP", "\x1b", "\x18", "\x19", "\x1a", "PG DN", "SPACE"};
    static const Key keys[7] = {K_PGUP, K_LEFT, K_UP, K_DOWN, K_RIGHT, K_PGDN, K_SPACE};
    for (int i = 0; i < 7; i++) { int a, b; split_gap(6, BOT_W - 6, 7, i, a, b); add_btn(a, 140, b, 164, labs[i], &F8x16, key_act(keys[i])); }
    }
  } else if (reader_wanted(ts) && (build_reader(ts), !rlines.empty() && !rd_wide_table)) {
    reader_on = true;
    draw_reader(s, ts);
  } else {
    // keys for screens that are read or stepped through (flight windows): arrows, page keys, space
    static const char* labs[9] = {"PG UP", "\x18", "PG DN", "\x1b", "SPACE", "\x1a", "HOME", "\x19", "END"};
    static const Key keys[9] = {K_PGUP, K_UP, K_PGDN, K_LEFT, K_SPACE, K_RIGHT, K_NONE, K_DOWN, K_NONE};
    for (int i = 0; i < 9; i++) {
      if (keys[i] == K_NONE) continue;
      int x0, x1, y0, y1;
      split_gap(6, BOT_W - 6, 3, i % 3, x0, x1); split_gap(6, UP_H - 4, 3, i / 3, y0, y1);
      add_btn(x0, y0, x1, y1, labs[i], &F8x16, key_act(keys[i]));
    }
  }
}

// ------------------------------------------------------------------ manual
static void load_library_cards() {
  for (const LibCard* c = LIBCARDS; c->title; c++) {
    Section sec; sec.title = c->title; sec.shelf = c->shelf;
    std::string line;
    auto flush = [&]() {
      // "@next <card>": the checklist that follows this one; "@done <text>": its success card
      if (line.compare(0, 6, "@next ") == 0) { sec.next = trim(line.substr(6)); line.clear(); return; }
      if (line.compare(0, 6, "@done ") == 0) { sec.done = trim(line.substr(6)); line.clear(); return; }
      if (line.compare(0, 5, "STEP ") == 0) {            // checklist step: "STEP button | what | done text"
        std::string rest = line.substr(5), btn, what;
        size_t a = rest.find('|'), b = a == std::string::npos ? a : rest.find('|', a + 1);
        btn = trim(rest.substr(0, a)); what = a == std::string::npos ? "" : trim(rest.substr(a + 1, b == std::string::npos ? b : b - a - 1));
        Step st; st.btn = btn == "-" ? "" : btn; st.what = what; st.done = b == std::string::npos ? "" : trim(rest.substr(b + 1));
        if (st.done == "-") st.done.clear();
        sec.steps.push_back(st);
        line = "[ ] " + what + (btn.empty() || btn == "-" ? "" : "  (" + btn + ")");
      }
      // "{HH:C} text": the game's own symbol (CP437 code HH, VGA colour C) drawn before the text
      unsigned code = 0, col = 0; int used = 0;
      if (line[0] == '{' && sscanf(line.c_str(), "{%2x:%u}%n", &code, &col, &used) == 2 && used > 0 && code && col < 16)
        line = std::string("\x01") + (char)code + (char)(col + 0x30) + line.substr(used);
      sec.paras.push_back(line); line.clear();
    };
    for (const char* p = c->text; *p; p++) { if (*p == '\n') flush(); else line += *p; }
    flush();
    if (c->see && *c->see) {
      sec.paras.push_back("");
      std::string see; for (const char* q = c->see; *q; q++) { if (*q == '|') see += "; "; else see += *q; }
      sec.paras.push_back("More in BOOKS: " + see);
    }
    manual.push_back(sec);
  }
}
static void load_manual() {
  manual.clear();
  load_library_cards();
  FILE* f = H && H->manual_path ? fopen(H->manual_path, "rb") : 0;
  Section cur; cur.title = "About";
  if (!f) {
    cur.paras.push_back("The manual file was not found.");
    cur.paras.push_back(H && H->manual_path ? H->manual_path : "");
    manual.push_back(cur); return;
  }
  static char buf[16384];                  // a whole paragraph is one line in the file
  bool any = false;
  while (fgets(buf, sizeof buf, f)) {
    size_t n = strlen(buf); while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = 0;
    if (buf[0] == '#' && buf[1] == ' ') {
      if (any || !cur.lines.empty()) manual.push_back(cur);
      cur = Section(); cur.title = buf + 2; any = true; continue;
    }
    cur.paras.push_back(buf);     // wrapped for the screen width when drawn (manual_wrap)
  }
  fclose(f);
  manual.push_back(cur);
}
static const int MAN_LINES = 16;   // text lines on the top screen (between the header bar and the hint strip)
// Word-wrap a section for a screen this many pixels wide (63 columns at 400, 50 at 320 when
// swapped: same 6x12 text, never squeezed). The wrap is cached per width (swap readability rule 4).
static void manual_wrap(struct Section& sec, int w) {
  int cols = (w - 22) / 6;
  if (sec.wrap == cols) return;
  sec.wrap = cols; sec.lines.clear();
  for (const std::string& para : sec.paras) {
    // a key table row ("Circle Pad    Left/right: ...") wraps under its description column
    size_t ind = para.find_first_not_of(' ');
    size_t gap = para.find("  ", ind == std::string::npos ? 0 : ind);
    size_t col = gap == std::string::npos ? std::string::npos : para.find_first_not_of(' ', gap);
    std::string pad = (col != std::string::npos && col <= 24 && (int)col < cols / 2) ? std::string(col, ' ')
                    : std::string(ind == std::string::npos ? 0 : std::min<size_t>(ind, 24), ' ');
    std::string line = para;
    bool first = true;
    do {
      if (!first) line = pad + line;
      first = false;
      if ((int)line.size() <= cols) { sec.lines.push_back(line); break; }
      size_t cut = line.rfind(' ', cols);
      if (cut == std::string::npos || cut <= pad.size()) cut = cols;
      sec.lines.push_back(line.substr(0, cut));
      line = line.substr(cut + (line[cut] == ' ' ? 1 : 0));
    } while (!line.empty());
  }
}
// House top screen (look-and-feel): 22-px header bar with the section title in gold at x 8 and the
// section number right-aligned, 1:1 text from y 28, the gold hint strip at y 224.
static void manual_top(Surf& s) {
  s.fill(0, 0, TOP_W, TOP_H, BG);
  Section& sec = manual[man_sel];
  manual_wrap(sec, top_w);
  s.fill(0, 0, top_w, 22, BTN);
  s.hline(0, top_w, 22, BTN_EDGE);
  char num[24]; snprintf(num, sizeof num, "%d/%d", man_sel + 1, (int)manual.size());
  int nw = text_width(num, F6x12);
  const Font* tf = text_width(sec.title.c_str(), F8x16) <= top_w - 24 - nw ? &F8x16 : &F6x12;
  bool show_num = text_width(sec.title.c_str(), *tf) <= top_w - 24 - nw;
  fit_text(s, 8, (22 - tf->h) / 2, top_w - 8, sec.title.c_str(), *tf, TITLE, "manual title");
  if (show_num) draw_text(s, top_w - 8 - nw, 5, num, F6x12, rgb(150, 150, 165));
  Box in = {0, 28, top_w - 2, 28 + MAN_LINES * 12};
  if (!lefty_loaded) lefty_load();
  const char* hint = lefty_on ? "Sticks scroll  ABXY sections  L/R next  D-Dn close"
                              : "Sticks scroll  D-Pad sections  L/R next  B close";
  s.fill(0, 224, top_w, TOP_H, BTN);
  s.hline(0, top_w, 224, BTN_EDGE);
  fit_text(s, (top_w - text_width(hint, F6x12)) / 2, 226, top_w - 2, hint, F6x12, TITLE, "manual top hint");
  int maxs = (int)sec.lines.size() - MAN_LINES; if (maxs < 0) maxs = 0;
  if (man_scroll > maxs) man_scroll = maxs;
  if (man_scroll < 0) man_scroll = 0;
  for (int i = 0; i < MAN_LINES && man_scroll + i < (int)sec.lines.size(); i++) {
    const std::string& ml = sec.lines[man_scroll + i];
    if (ml.size() >= 3 && ml[0] == 1) {     // a game symbol, then its text three columns in
      char g[2] = {ml[1], 0};
      s.fill(in.x0 + 6, in.y0 + i * 12 - 1, in.x0 + 18, in.y0 + i * 12 + 13, rgb(0, 0, 0));
      draw_text(s, in.x0 + 8, in.y0 + i * 12 - 1, g, F8x14, READ_FG[(ml[2] - 0x30) & 15]);
      fit_text(s, in.x0 + 8 + 18, in.y0 + i * 12, in.x1 - 4, ml.c_str() + 3, F6x12, BTN_TXT, "manual text");
    } else fit_text(s, in.x0 + 8, in.y0 + i * 12, in.x1 - 4, ml.c_str(), F6x12, BTN_TXT, "manual text");
  }
  // scroll bar
  if (maxs > 0) {
    int track = in.y1 - in.y0, th = track * MAN_LINES / (int)sec.lines.size();
    if (th < 8) th = 8;
    int ty = in.y0 + (track - th) * man_scroll / maxs;
    s.fill(in.x1 - 3, in.y0, in.x1 - 1, in.y1, rgb(40, 44, 60));
    s.fill(in.x1 - 3, ty, in.x1 - 1, ty + th, BTN_EDGE);
  }
}
static void lefty_load() {
  lefty_loaded = true;
  FILE* f = H && H->lefty_path ? fopen(H->lefty_path, "rb") : 0;
  if (!f) return;
  int c = fgetc(f); fclose(f);
  lefty_on = (c == '1');
}
uint32_t lefty_remap(uint32_t m) {
  static const uint32_t pr[4][2] = {{B_Y, B_DUP}, {B_A, B_DRIGHT}, {B_B, B_DDOWN}, {B_X, B_DLEFT}};
  if (!lefty_loaded) lefty_load();
  if (!lefty_on) return m;
  uint32_t o = m;
  for (int i = 0; i < 4; i++) {
    o &= ~(pr[i][0] | pr[i][1]);
    if (m & pr[i][0]) o |= pr[i][1];
    if (m & pr[i][1]) o |= pr[i][0];
  }
  return o;
}
static void manual_bottom(Surf& s) {
  s.fill(0, 0, BOT_W, BOT_H, BG);
  // UI rule 11: buttons 24 px tall, 8 px apart, 6 px from the edges
  // Shelf tabs (learning design step 1): jump to the port's guide, the key list, the Operations
  // Manual, the Training Manual or the tutorials; the tab of the section being read is lit.
  static const char* SHELF[5] = {"GUIDE", "KEYS", "OPS", "TRAIN", "TUTOR"};
  static const char* SHELF_PRE[5] = {"", "Keyboard Quick Reference", "Ops ", "Training ", "Tutorial "};
  static const char* LSHELF[6] = {"START", "BRIDGE", "HOW TO", "FIX IT", "REF", "BOOKS"};
  if (LIBCARDS[0].title) {         // the Library's own shelves, then the books
    int first[6] = {-1, -1, -1, -1, -1, -1};
    for (int i = (int)manual.size() - 1; i >= 0; i--) first[manual[i].shelf] = i;
    for (int k = 0; k < 6; k++) {
      int a, b; split_gap(6, BOT_W - 6, 6, k, a, b, 4);
      add_btn(a, 6, b, 30, LSHELF[k], &F8x16, kind_act(A_SECTION, first[k] < 0 ? 0 : first[k]), -1, k == manual[man_sel].shelf);
      if (first[k] < 0) btns.back().off = true;
    }
  } else {
  int shelf_first[5] = {0, -1, -1, -1, -1}, cur_shelf = 0;
  for (int i = 0; i < (int)manual.size(); i++)
    for (int k = 1; k < 5; k++)
      if (manual[i].title.compare(0, strlen(SHELF_PRE[k]), SHELF_PRE[k]) == 0) {
        if (shelf_first[k] < 0) shelf_first[k] = i;
        if (i <= man_sel) cur_shelf = k;
      }
  if (shelf_first[1] >= 0 && man_sel < shelf_first[1]) cur_shelf = 0;
  for (int k = 0; k < 5; k++) {
    int a, b; split_gap(6, BOT_W - 6, 5, k, a, b, 4);
    add_btn(a, 6, b, 30, SHELF[k], &F8x16, kind_act(A_SECTION, shelf_first[k] < 0 ? 0 : shelf_first[k]), -1, k == cur_shelf);
    if (shelf_first[k] < 0) btns.back().off = true;
  }
  }
  const int rows = 4;  // section buttons visible; the list follows the highlight, and the arrows page it
  int n = (int)manual.size();
  static int seen_sel = -1;
  if (man_sel != seen_sel) {      // a new section was chosen: bring it into view (the arrows may scroll away)
    seen_sel = man_sel;
    if (man_sel < man_list_top) man_list_top = man_sel;
    if (man_sel >= man_list_top + rows) man_list_top = man_sel - rows + 1;
  }
  if (man_list_top > n - rows) man_list_top = n - rows;
  if (man_list_top < 0) man_list_top = 0;
  bool paged = n > rows;
  int lx1 = paged ? 270 : BOT_W - 6;   // section buttons; with more sections than rows: a scroll bar and arrows
  for (int i = 0; i < rows && man_list_top + i < n; i++) {
    int k = man_list_top + i;
    add_btn(6, 38 + i * 32, lx1, 62 + i * 32, manual[k].title, &F8x16, kind_act(A_SECTION, k), -1, k == man_sel);
  }
  if (paged) {                    // rule 13: show there is more, and let the stylus page the list
    int ty0 = 38, ty1 = 158, th = (ty1 - ty0) * rows / n; if (th < 10) th = 10;
    int ty = ty0 + (ty1 - ty0 - th) * man_list_top / (n - rows);
    s.fill(272, ty0, 275, ty1, rgb(40, 44, 60));
    s.fill(272, ty, 275, ty + th, BTN_EDGE);
    add_btn(279, 38, BOT_W - 6, 94, "\x1e", &F8x16, kind_act(A_SECLIST, -rows));
    if (man_list_top == 0) btns.back().off = true;
    add_btn(279, 102, BOT_W - 6, 158, "\x1f", &F8x16, kind_act(A_SECLIST, rows));
    if (man_list_top >= n - rows) btns.back().off = true;
  }
  if (!lefty_loaded) lefty_load();
  if (!manual[man_sel].steps.empty())      // a checklist card: follow it while playing (or stop)
    add_btn(6, 166, BOT_W - 6, 190, chk_sec == man_sel ? "STOP THIS CHECKLIST" : "FOLLOW THIS CHECKLIST", &F8x16, kind_act(A_FOLLOW), -1, chk_sec == man_sel);
  else
  add_btn(6, 166, BOT_W - 6, 190, lefty_on ? "LEFT HAND: ON" : "LEFT HAND: OFF", &F8x16, kind_act(A_LEFTY), -1, lefty_on);
  btns.back().focus = man_focus == -1;
  s.fill(0, 193, BOT_W, 207, PROMPT_BG);
  fit_text(s, 6, 194, BOT_W - 6, lefty_on ? "Sticks scroll \xb3 ABXY, L/R sections \xb3 D-Dn close"
                                          : "Sticks scroll \xb3 D-Pad, L/R sections \xb3 B close", F6x12, HINT, "manual hint");
  add_btn(6, 210, BOT_W - 6, 234, "CLOSE MANUAL", &F8x16, kind_act(A_CLOSE));
  btns.back().focus = man_focus == -2;
}

// ------------------------------------------------------------------ graphics (title art) screens
static void graphics_bottom(Surf& s) {
  s.fill(0, 0, BOT_W, BOT_H, BG);
  if (mission_on) {
    // A picture inside a mission: the planet's map (only Esc leaves it), the boarding plan (arrows,
    // SPACE per team, letters), the invasion screens (letters). The keys are named on the picture,
    // so the touch screen offers all of them: arrows, SPACE, ENTER, the keyboard, and the way back.
    if (kb_open) { keyboard(s); }
    else {
      const char* l1 = "The picture is on the top screen.";
      const char* l2 = "Its keys are named there; KEYBOARD has the letters.";
      fit_text(s, (BOT_W - text_width(l1, F6x12)) / 2, 6, BOT_W - 6, l1, F6x12, BTN_TXT, "picture");
      fit_text(s, (BOT_W - text_width(l2, F6x12)) / 2, 20, BOT_W - 6, l2, F6x12, HINT, "picture");
      static const char* labs[9] = {"", "\x18", "", "\x1b", "SPACE", "\x1a", "", "\x19", ""};
      static const Key keys[9] = {K_NONE, K_UP, K_NONE, K_LEFT, K_SPACE, K_RIGHT, K_NONE, K_DOWN, K_NONE};
      for (int i = 0; i < 9; i++) {
        if (keys[i] == K_NONE) continue;
        int a, b, y0, y1; split_gap(60, 260, 3, i % 3, a, b); split_gap(40, 150, 3, i / 3, y0, y1);
        add_btn(a, y0, b, y1, labs[i], &F8x16, key_act(keys[i]));
      }
    }
    int a, b;
    split_gap(6, BOT_W - 6, 3, 0, a, b); add_btn(a, 202, b, 234, "ENTER", &F8x16, key_act(K_ENTER));
    split_gap(6, BOT_W - 6, 3, 1, a, b); add_btn(a, 202, b, 234, "BACK", &F8x16, key_act(K_ESC));
    split_gap(6, BOT_W - 6, 3, 2, a, b); add_btn(a, 202, b, 234, kb_open ? "HIDE KEYS" : "KEYBOARD", &F8x16, kind_act(A_KB));
    return;
  }
  fit_text(s, (BOT_W - text_width("STAR FLEET II", F8x16)) / 2, 40, BOT_W, "STAR FLEET II", F8x16, TITLE, "title");
  fit_text(s, (BOT_W - text_width("Krellan Commander", F6x12)) / 2, 62, BOT_W, "Krellan Commander", F6x12, BTN_TXT, "title");
  add_btn(40, 100, 280, 150, "CONTINUE", &F8x16, key_act(K_ENTER));
  const char* tip = "New here? MANUAL starts with Getting started.";
  fit_text(s, (BOT_W - text_width(tip, F6x12)) / 2, 170, BOT_W - 6, tip, F6x12, HINT, "title tip");
  add_btn(6, 202, 156, 234, "BACK", &F8x16, key_act(K_ESC));             // UI rule 11: 6 px from the edges, 8 apart
  add_btn(164, 202, 314, 234, "MANUAL", &F8x16, kind_act(A_MANUAL));
}

// ------------------------------------------------------------------ START menu
static void hint_strip(Surf& s, const char* hint) {
  s.fill(0, 224, BOT_W, 240, BTN);
  s.fill(0, 224, BOT_W, 225, BTN_EDGE);
  fit_text(s, (BOT_W - text_width(hint, F6x12)) / 2, 226, BOT_W, hint, F6x12, TITLE, "hint strip");
}
static void header_bar(Surf& s, const char* title) {
  s.fill(0, 0, BOT_W, 22, BTN);
  s.fill(0, 22, BOT_W, 23, BTN_EDGE);
  fit_text(s, 8, 3, BOT_W, title, F8x16, TITLE, "header");
}
static void pause_bottom(Surf& s, const TextScreen& ts) {
  s.fill(0, 0, BOT_W, BOT_H, BG);
  if (quit_confirm) {
    header_bar(s, "EXIT GAME");
    // rule 17 (decided Nov 20 after hardware tests): no save is attempted on exit. The game autosaves every
    // half day of mission time; saving from the game's own command menu keeps the exact place.
    const bool br = ts.text_mode && is_bridge(ts);
    const char* l1 = br ? "Exit now?" : "Exit Star Fleet II?";
    const char* l2 = br ? "The game autosaves every half day" : "Anything since your last save is lost.";
    const char* l3 = br ? "of mission time. To keep your exact" : "(The game saves on the bridge only.)";
    const char* l4 = br ? "place, save from the game's command menu first." : "";
    fit_text(s, (BOT_W - text_width(l1, F8x16)) / 2, 40, BOT_W - 6, l1, F8x16, BTN_TXT, "pause");
    fit_text(s, (BOT_W - text_width(l2, F6x12)) / 2, 70, BOT_W - 6, l2, F6x12, HINT, "pause");
    fit_text(s, (BOT_W - text_width(l3, F6x12)) / 2, 84, BOT_W - 6, l3, F6x12, HINT, "pause");
    if (*l4) fit_text(s, (BOT_W - text_width(l4, F6x12)) / 2, 98, BOT_W - 6, l4, F6x12, HINT, "pause");
    add_btn(30, 130, 150, 172, "YES", &F8x16, kind_act(A_QUIT_YES));
    btns.back().focus = pause_focus == 0;
    add_btn(170, 130, 290, 172, "NO", &F8x16, kind_act(A_PAUSE));
    btns.back().focus = pause_focus != 0;
    hint_strip(s, lefty_on ? "D-Right Select  D-Down Back  ZL Swap" : "A Select  B Back  ZL Swap");
    return;
  }
  header_bar(s, "MENU");
  if (chk_sec >= 0 && chk_step < (int)manual[chk_sec].steps.size()) {     // the step the buttons below move
    char n[24]; snprintf(n, sizeof n, "Step %d/%d: ", chk_step + 1, (int)manual[chk_sec].steps.size());
    std::string l = std::string(n) + manual[chk_sec].steps[chk_step].what;
    fit_text(s, 50, 6, BOT_W - 4, l.c_str(), text_width(l.c_str(), F6x12) <= BOT_W - 54 ? F6x12 : F5x9, BTN_TXT, "pause step");
  } else
  {   // port version (git describe of starfleet2-3ds), right of the title
    const char* v = "Build " PORT_VERSION;
    fit_text(s, BOT_W - 8 - text_width(v, F6x12), 6, BOT_W, v, F6x12, HINT, "version");
  }
  bool bridge = ts.text_mode && is_bridge(ts);
  bool menu = ts.text_mode && screen_has(ts, "Change default settings", 0, 24);
  // (Saving is the game's own: its command menu on the touch screen; the port does not drive it.
  // Hardware, Nov 20: at mission start every injected key only advanced the XO's COMMS messages.)
  // seven items on six rows (24 px tall, 4 px apart): COACH and SAVE share the fourth row
  // (inside a mission the sixth row leaves it: the port walks the game's own Quit menu. Cold read,
  // Nov 27: a finished tutorial could not be left by touch.)
  bool flying = ts.text_mode && in_flight(ts);
  const char* lab[7] = {"RESUME", "WHAT NOW?", "MANUAL", coach_on ? "COACH: ON" : "COACH: OFF", "SAVE",
                        flying ? (tut_num > 0 ? "LEAVE TUTORIAL" : "LEAVE MISSION") : "GAME SETTINGS", "EXIT GAME"};
  Act act[7] = {kind_act(A_PAUSE), kind_act(A_MISSION), kind_act(A_MANUAL), kind_act(A_COACH), kind_act(A_SAVE), flying ? kind_act(A_LEAVE) : kind_act(A_SETTINGS), kind_act(A_QUIT)};
  for (int i = 0; i < 7; i++) {
    int row = i < 4 ? i : i - 1, x0 = i == 4 ? 164 : 50, x1 = i == 3 ? 156 : 270;
    add_btn(x0, 28 + row * 28, x1, 52 + row * 28, lab[i], &F8x16, act[i]);
    if ((i == 4 && !bridge) || (i == 5 && !menu && !flying) || (i == 6 && !(H && H->quit))) btns.back().off = true;
    btns.back().focus = i == pause_focus;
  }
  const char* note = bridge ? "SAVE re-saves a mission you saved by name." :
                     menu ? "Saving works on the bridge." : "Saving: on the bridge. Settings: main menu.";
  if (chk_sec >= 0) {       // a checklist is running: the manual tick (learning review 7)
    int a, b;
    split_gap(6, BOT_W - 6, 3, 0, a, b); add_btn(a, 196, b, 220, "STEP BACK", &F6x12, kind_act(A_STEPBACK));
    if (chk_step == 0) btns.back().off = true;
    split_gap(6, BOT_W - 6, 3, 1, a, b); add_btn(a, 196, b, 220, "SKIP STEP", &F6x12, kind_act(A_STEPSKIP));
    split_gap(6, BOT_W - 6, 3, 2, a, b); add_btn(a, 196, b, 220, "STOP LIST", &F6x12, kind_act(A_STOPLIST));
  } else
  fit_text(s, (BOT_W - text_width(note, F6x12)) / 2, 200, BOT_W, note, F6x12, HINT, "pause");
  hint_strip(s, lefty_on ? "D-Right Select  D-Down Back  ZL Swap" : "A Select  B Back  ZL Swap");
}

// ------------------------------------------------------------------ public API

void init(const Host* host) {
  H = host;
  next_key_ms = now();
  memset(&last, 0, sizeof last);
  load_manual();
  load_officer();
  load_settings();
  dirty();
}

bool modal() { return manual_open; }
void set_swapped(bool on) { if (on != swap_wanted) { swap_wanted = on; dirty(); } }
// Principle (Ryan, Nov 17 2026): everything has to be readable. Only screens with a readable
// 320-px layout swap (the bridge and the manual); other 80-column text screens stay unswapped.
bool swap_ok(const TextScreen& ts) { return manual_open || (ts.text_mode && is_bridge(ts)); }
void set_top_width(int w) { if (w != top_w) { top_w = w; for (auto& sec : manual) sec.wrap = 0; dirty(); } }
static bool top_native = false;   // the last top panel was laid out at top_w (manual, bridge)
int top_width() { return top_native ? top_w : TOP_W; }


std::vector<std::string> fit_problems() { return fit_log; }
void fit_clear() { fit_log.clear(); }
void force_redraw() { dirty(); }

// Enter on the keyboard: remember what was typed at the sign-on prompts.
static void typed_enter() {
  if (at_name_prompt(last)) pend_name = typed;
  else if (at_pass_prompt(last)) pend_pass = typed;
  typed.clear();
}

static void fire(const Act& a) {
  switch (a.kind) {
    case A_KEY:
      if (last.text_mode && screen_has(last, "Press number of desired tutorial")) for (int i = 1; i <= 8; i++) if (a.key == key_for_char((char)('0' + i))) tut_num = i;
      if (a.key != K_NONE) press(a.key, a.mods);
      if (a.key == K_ENTER) typed_enter();
      busy_since = now();
      break;
    case A_VIEW: {
      view_goal = a.arg; view_esc = 0; view_next = now(); busy_since = now();
      // P2-5: the first two taps on each view say in one line what it is (at most 38 characters,
      // so it fits the prompt line beside the bridge's action buttons)
      static const char* meaning[7] = {"Star map: tap a star to aim at it", "System map: inside this star system",
                                       "War map: the whole war at a glance", "Combat control: direct your ships",
                                       "Fleet: the state of your ships", "Orders: your mission orders again",
                                       "Log: the messages so far"};
      static int told[7] = {0};
      if (a.arg >= 0 && a.arg < 7 && told[a.arg] < 2) { told[a.arg]++; say(meaning[a.arg], 3000); }
      break;
    }
    case A_TYPE:
      if (last.cursor_y != typed_y) { typed.clear(); typed_y = last.cursor_y; }
      press(a.key, a.mods);
      if (a.arg == 8) { if (!typed.empty()) typed.erase(typed.size() - 1); }
      else if (a.arg == 13) typed_enter();
      else typed += (char)(a.arg >= 'A' && a.arg <= 'Z' ? a.arg + 32 : a.arg);
      break;
    case A_SIGNON:
      // A second tap before the screen redraws (or a tap that lands after sign-on) would type the
      // name and password again into the next screen, where the letters pick menu items
      // (seen Nov 18 2026: the "p" of a password chose Practice). Only sign on once, at the prompt.
      if (autologin != 0) break;
      { int mv = 0;
        if (signon_list(last, &mv)) {        // commander list: go to the officer's row, pick it; the password follows
          for (int i = 0; i < (mv < 0 ? -mv : mv); i++) press(mv < 0 ? K_UP : K_DOWN);
          press(K_ENTER); autologin = 1; pass_tries = 0; pend_name = off_name; say("Signing on...", 1500); break;
        }
        if (signon_pass(last)) { autologin = 1; pass_tries = 0; pend_name = off_name; say("Signing on...", 1500); break; }
      }
      if (!at_name_prompt(last)) break;
      typed.clear(); type_text(off_name); press(K_ENTER); autologin = 1; pass_tries = 0; pend_name = off_name; say("Signing on...", 1500);
      break;
    case A_FOLLOW:
      if (chk_sec == man_sel) { chk_sec = -1; break; }
      chk_sec = man_sel; chk_step = 0; chk_begin_step(last);
      manual_open = false; man_focus = 0; manual_closed(); save_settings();
      say("Checklist on the top screen", 2500);
      break;
    case A_STEPBACK: if (chk_sec >= 0 && chk_step > 0) { chk_step--; chk_begin_step(last); chk_had = true; chk_wait_end = false; } break;      // (the menu stays open: several steps in a row)
    case A_STOPLIST: chk_sec = -1; chk_wait_end = false; pause_open = false; say("Checklist stopped (START > MANUAL has them all)", 3000); break;
    case A_STEPSKIP: if (chk_sec >= 0) { chk_step++; if (chk_step >= (int)manual[chk_sec].steps.size()) { pause_open = false; chk_finish(last); } else chk_begin_step(last); } break;
    case A_GRID: grid_open = !grid_open; break;
    case A_LEAVE:
      pause_open = false;
      if (row_mode == R_YESNO) { say("Answer the question first (YES or NO), then LEAVE", 4000); break; }   // (verify 4: it did nothing, silently)
      leave_until = now() + 20000; leave_next = now(); say(tut_num > 0 ? "Leaving the tutorial..." : "Leaving the mission...", 4000); break;
    case A_RDPAGE: rd_page(a.arg); break;
    case A_LISTPICK:        // a row of the commander list: move the game's highlight there, then Enter
      for (int i = 0; i < abs(a.arg); i++) press(a.arg > 0 ? K_DOWN : K_UP);
      press(K_ENTER); busy_since = now(); break;
    case A_COACH: coach_on = !coach_on; if (coach_on) coach_seen = 0; save_settings(); break;
    case A_MISSION: {
      bool pause_was = pause_open;
      pause_open = false;
      // SELECT again while "what now" is up: the card before it (a refusal, the coach, the mission)
      if (explain_until && explain_title == "WHAT NOW?" && !prev_title.empty() && !pause_was) {
        std::string pt = prev_title, px = prev_text; explain(pt.c_str(), px.c_str()); advise_button = 0; break;
      }
      { const char* b = 0; std::string a = advise(last, &b);
        explain("WHAT NOW?", a.c_str());
        advise_button = b; advise_until = now() + CARD_MS;
        if (b && (!strcmp(b, "Tutorials") || !strcmp(b, "Start a new mission"))) want_practice_view = true; }
      break;
    }
    case A_LAUNCH: case A_ENGINES: {
      // the helm refuses to start the engines with no speed set ("Velocity must be set"):
      // set 0.5 sublight first when it is 0, then start them (F9)
      if (bridge_speed(last, 15) <= 0.0f) for (int i = 0; i < 5; i++) press((Key)(K_F1 + 3));
      press(K_F9); busy_since = now();
      say(a.kind == A_LAUNCH ? "Launching: engines on at sublight 0.5" : "Engines on");
      break;
    }
    case A_SAVE: save_pending = true; say("Quick save (needs a named save first)", 2500); pause_open = false; break;   // (sent after the clock is running again)
    case A_PAUSE: pause_open = false; quit_confirm = false; break;
    case A_QUIT: quit_confirm = true; pause_focus = 1; break;
    case A_QUIT_YES: if (H && H->quit) H->quit(); break;
    case A_SETTINGS: pause_open = false; press(K_C); break;
    case A_MODE:
      tap_mode = (TapMode)((tap_mode + 1) % 3);
      save_settings();
      say(tap_mode == TM_AUTO ? "Taps target when the target box is up, else steer" :
          tap_mode == TM_STEER ? "Taps always steer" : "Taps always move the target box");
      break;
    case A_MANUAL: {
      // The first time in a session the manual opens on the section the player last read (rule 16,
      // remembered across launches), or on "Getting started" if there is none (newcomers); after
      // that it keeps the section the player last read.
      static bool opened_before = false;
      if (!opened_before) {
        int want = -1, gs = -1;
        for (int i = 0; i < (int)manual.size(); i++) {
          if (!set_manual.empty() && manual[i].title == set_manual) want = i;
          if (manual[i].title == "Getting started") gs = i;
        }
        if (want < 0) want = gs;
        if (want >= 0) man_sel = want;
      }
      opened_before = true;
      manual_open = true; man_scroll = 0; man_focus = 0; pause_open = false; break;
    }
    case A_KB:
      if (kb_open) { kb_open = false; kb_user_hidden = true; kb_forced = false; }
      else { kb_open = true; kb_forced = true; kb_user_hidden = false; }
      break;
    case A_SECTION:      // (a card that is off the list, e.g. picked by its shelf tab, goes to the top row)
      if (a.arg < man_list_top || a.arg >= man_list_top + 4) man_list_top = a.arg;
      man_sel = a.arg; man_scroll = 0; man_focus = 0; break;
    case A_SECLIST: man_list_top += a.arg; break;   // (clamped when the list is drawn)
    case A_ROWPAGE: row_page++; break;              // (wraps when the row is laid out)
    case A_MENUSCROLL: menu_top += a.arg; break;     // (clamped when the menu is laid out)
    case A_RDLINE:
      if (reader_on && a.arg > 0 && rd_scroll < rd_max) rd_scroll++;
      else if (reader_on && a.arg < 0 && rd_scroll > 0) rd_scroll--;
      else if (reader_on && screen_has(last, "ANY KEY")) say("End of the text: tap CONTINUE", 2500);
      else { press(a.arg > 0 ? K_DOWN : K_UP); busy_since = now(); }
      break;
    case A_CLOSE: manual_open = false; man_focus = 0; manual_closed(); save_settings(); break;
    case A_LEFTY:
      lefty_on = !lefty_on;
      if (H && H->lefty_path) {
        const char* v = lefty_on ? "1" : "0";
        if (H->save) H->save(H->lefty_path, v, 1);
        else if (FILE* f = fopen(H->lefty_path, "wb")) { fputs(v, f); fclose(f); }
      }
      break;
    default: break;
  }
  dirty();
}

Redraw frame(const TextScreen& ts, uint16_t* top, uint16_t* bottom) {
  find_popup(ts);
  pump_keys();
  uint32_t t = now();
  Redraw rd = {false, false, 0, BOT_H};

  // Very first launch (no marker yet): open the manual on "Getting started" once the title art
  // is up, so a newcomer sees it without hunting for MANUAL (Ryan, Nov 20). Never again after.
  {
    static int first_run = -1;   // -1 unknown, 1 show it, 0 done
    static const char* kSeen = "sdmc:/3ds/starfleet2/.manual_seen";
    if (first_run < 0) { FILE* f = fopen(kSeen, "rb"); first_run = f ? 0 : 1; if (f) fclose(f); }
    if (first_run == 1 && !ts.text_mode && !manual_open && !pause_open && !manual.empty()) {
      first_run = 0;
      if (FILE* f = fopen(kSeen, "wb")) { fputc('1', f); fclose(f); }
      fire(kind_act(A_MANUAL)); dirty();
    }
  }

  // keyboard opens by itself when the game asks for typed text
  if (ts.text_mode && !manual_open) {
    bool want = wants_text(ts);
    if (want && !kb_open && !kb_user_hidden) { kb_open = true; dirty(); }
    if (!want) { kb_user_hidden = false; if (kb_open && !kb_forced) { kb_open = false; dirty(); } }
    // a keyboard the player opened goes away when the game moves between the bridge and its other
    // pages (seen Nov 21: opened on the sign-on menu, it was still covering the bridge in the mission)
    static int kb_bridge = -1; int br = is_bridge(ts) ? 1 : 0;
    if (kb_open && kb_forced && !want && kb_bridge >= 0 && br != kb_bridge) { kb_open = kb_forced = false; dirty(); }
    kb_bridge = br;
  }

  // auto sign-on: send the password when its prompt appears
  // The game clears keys typed ahead when its password prompt comes up (sometimes well after the
  // prompt is drawn, while it reads the officer files), so wait until the screen has been still for
  // a moment, and if the same prompt is still waiting a few seconds after the password went in, send
  // it again (up to 3 tries).
  bool pass_up = (autologin == 1 || autologin == 2) && queue.empty() && ts.text_mode && at_pass_prompt(ts) &&
                 !screen_has(ts, "SIGN-ON OPTIONS");
  uint32_t sg = screen_sig(ts);
  if (!pass_up) pass_seen = 0;
  else if (!pass_seen || sg != pass_sig) { pass_seen = t; pass_sig = sg; }    // restart the wait on any change
  bool still = pass_up && t - pass_seen >= 1200;
  if (still && (autologin == 1 || (autologin == 2 && t - pass_sent >= 2500 && pass_tries < 3 && pass_waiting(ts)))) {
    if (autologin == 2) for (int i = 0; i < 12; i++) press(K_BACKSPACE);   // clear a half-taken first try
    type_text(off_pass); press(K_ENTER); autologin = 2; pass_sent = t; pass_tries++; typed.clear();
    pend_pass = off_pass;
  }
  if (autologin && ts.text_mode && (screen_has(ts, "SIGN-ON OPTIONS") || (autologin == 2 && pass_tries >= 3 && t - pass_sent > 5000)))
    autologin = 0;
  // SF-06: the name prompt is back after a password was sent: that sign-on failed (wrong password).
  // Forget what was typed, so a wrong password is never remembered.
  if (ts.text_mode && !pend_pass.empty() && at_name_prompt(ts)) { pend_name.clear(); pend_pass.clear(); }
  // a successful sign-on with a newly typed officer: remember it
  if (ts.text_mode && screen_has(ts, "SIGN-ON OPTIONS") && !screen_has(ts, "ncorrect") && !screen_has(ts, "nvalid") && !screen_has(ts, "Wrong") && !pend_name.empty() && !pend_pass.empty()) {
    if (pend_name != off_name || pend_pass != off_pass) { save_officer(pend_name, pend_pass); say("Officer remembered for one-tap sign-on"); }
    pend_name.clear(); pend_pass.clear();
  }

  // drive the target box toward a tapped cell, one arrow at a time
  if (tdb_drive && queue.empty() && (int32_t)(t - tdb_next) >= 0) {
    int c, r;
    if (!find_tdb(ts, c, r) && (int32_t)(tdb_show_until - t) > 0) {
      tdb_next = t + 150;                 // the first arrow only brings the box up: wait for it
    } else if (!find_tdb(ts, c, r) || ++tdb_steps > 80) {
      tdb_drive = false;
    } else if (tdb_steps > 0 && c == tdb_lc && r == tdb_lr && t - tdb_press_t < 1500) {
      tdb_steps--; tdb_next = t + 60;     // the game has not drawn the last move yet (slow on the 3DS): wait, don't judge
    } else {
      tdb_show_until = 0;
      int dc = tdb_tc - c, dr = tdb_tr - r, dist = abs(dc) + abs(dr) * 2;
      bool stuck = dist >= tdb_best && tdb_steps > 1;     // the last press did not bring it closer
      if (dist < tdb_best) tdb_best = dist;
      if (dist == 0 || stuck) {
        tdb_drive = false;
        if (tdb_enter_after && dist <= 1) press(K_ENTER);
      } else if (dr) {
        press(dr > 0 ? K_DOWN : K_UP); tdb_lc = c; tdb_lr = r; tdb_press_t = t;
      } else {
        press(dc > 0 ? K_RIGHT : K_LEFT); tdb_lc = c; tdb_lr = r; tdb_press_t = t;
      }
      tdb_next = t + 120;
    }
  }

  // jump to a view: Esc out of whatever is open first (at most 4 times), then the view's key
  if (view_goal >= 0 && queue.empty() && (int32_t)(t - view_next) >= 0) {
    bool open_thing = ts.text_mode && (window_hints(ts) || cursor_question(ts) || wants_text(ts) ||
                      screen_has(ts, "PRESS ANY KEY", 0, 24) || screen_has(ts, "PRESS KEY", 0, 24));
    if (!open_thing || view_esc >= 4) { press(VIEW_KEYS[view_goal], VIEW_MODS[view_goal]); view_goal = -1; }
    else { press(K_ESC); view_esc++; view_next = t + 900; }
  }

  // Rule 14: watch the flight screens for alert lights coming on and new COMMS lines.
  {
    static uint32_t flight_since = 0, light_last[8];
    static uint8_t light_at[8];
    static std::vector<std::string> comms_seen; static std::string comms_pending; static uint32_t comms_t = 0;
    static const int starts[8] = {2, 12, 22, 32, 42, 52, 62, 72};
    if (ts.text_mode && in_flight(ts)) {
      if (!flight_since) {
        flight_since = t; comms_seen.clear(); comms_pending.clear();
        for (int i = 0; i < 8; i++) { light_at[i] = ts.at[0][starts[i] + 1]; light_last[i] = 0; }
      }
      bool settled = t - flight_since > 1500;   // nothing is announced as the screen first appears
      // A light counts once it has stayed lit for 1.2 s: the game's lamp test sweeps one lit lamp
      // across the row (about 6 a second; seen Nov 21 as a false "ALERT: LOW SUP" at 100% supplies).
      // The sweep's colour (green unless the player changed it) is learned from any lamp that goes
      // out again within 0.6 s, and lamps in that colour are never announced: the sweep can stand
      // still on one lamp while the game waits at a prompt.
      static uint32_t light_on_since[8]; static uint8_t sweep_at = 0x20, light_prev[8];
      for (int i = 0; i < 8; i++) {
        uint8_t at = ts.at[0][starts[i] + 1];
        bool on = at != 0x08 && at != 0x00;
        if (at != light_at[i]) {
          if (!on && light_on_since[i] && t - light_on_since[i] < 600) sweep_at = light_prev[i];
          light_at[i] = at; light_prev[i] = at; light_on_since[i] = on ? t : 0; continue;
        }
        if (on && at != sweep_at && light_on_since[i] && t - light_on_since[i] >= 1200) {
          light_on_since[i] = 0;                 // (once per time it comes on)
          if (settled && (!light_last[i] || t - light_last[i] > 10000)) {
            light_last[i] = t;
            std::string lab = trim(row_str(ts, 0, starts[i], starts[i] + 7));
            int fg = at & 15, bgc = (at >> 4) & 7;
            alert_red = fg == 4 || fg == 12 || bgc == 4;
            alert_text = "ALERT: " + lab; alert_until = t + 3000; dirty();
          }
        }
      }
      if (is_bridge(ts)) {                      // newest COMMS line, once the game has finished printing it
        std::string line;
        for (int r = 22; r >= 15 && line.empty(); r--) line = trim(row_str(ts, r, 1, 40));
        if (line != comms_pending) { comms_pending = line; comms_t = t; }
        bool known = false;
        for (auto& k : comms_seen) if (k == line) known = true;
        if (!line.empty() && !known && t - comms_t >= 700) {
          comms_seen.push_back(line);
          if (comms_seen.size() > 24) comms_seen.erase(comms_seen.begin());
          if (settled && (alert_text.empty() || (int32_t)(alert_until - t) <= 0 || !alert_red)) {
            alert_text = "MSG: " + line; alert_red = false; alert_until = t + 3000; dirty();
          }
          static int firsts = 0;
          if (settled && coach_on && chk_sec < 0 && !explain_until) {
            if (!(firsts & 4) && line.find("Enemy ship disabled") != std::string::npos) { firsts |= 4; explain("SHIP DISABLED", "It cannot move or fight now. Stop firing: a captured ship is worth far more than a wreck. Board it with SHIP MENUS, ENGINEERING, TELEPORTERS, Boarding party."); }
            else if (!(firsts & 1) && line.find("Entered orbit") != std::string::npos) { firsts |= 1; explain("IN ORBIT", "Your first orbit. This is where scans and landing parties happen: the scan report comes up in COMMS. ENGINES leaves orbit."); }
            else if (!(firsts & 2) && line.find("Docking procedure complete") != std::string::npos) { firsts |= 2; explain("DOCKED", "Docked. The starfort repairs and resupplies the ship, and the mission clock stands still here."); }
          }
        }
      }
    } else flight_since = 0;
    if (!alert_text.empty() && (int32_t)(alert_until - t) <= 0) { alert_text.clear(); dirty(); }
  }

  bool changed = memcmp(&ts, &last, sizeof ts) != 0;
  // Busy feedback: after a press, if the game hasn't changed the screen within 0.35 s, the prompt
  // strip shows a moving "Working" bar until it does (or 60 s pass), so a slow step never looks dead.
  if (busy_since && changed && view_goal < 0) busy_since = 0;
  if (busy_since && (int32_t)(t - busy_since) > 60000) busy_since = 0;
  // the game is waiting at a question or a "press key" on the bridge: it is not working on anything
  if (busy_since && view_goal < 0 && (int32_t)(t - busy_since) > 1500 && ts.text_mode && is_bridge(ts)) {
    std::string pr = row_str(ts, 24, 0, 24);
    if (pr.find('?') != std::string::npos || pr.find("PRESS") != std::string::npos) busy_since = 0;
  }
  bool was_busy = busy_now;
  busy_now = busy_since && (int32_t)(t - busy_since) > 350;
  if (was_busy && !busy_now) dirty();
  static uint32_t last_anim = 0;
  // (-50: keep redrawing a little past the end so the button's released look gets drawn)
  if (flash_on && (int32_t)(flash_until - t) <= -50) flash_on = false;
  bool toast_anim = !toast.empty() && (int32_t)(toast_until - t) > -50;
  bool scroll_due = (marquee || busy_now) && t - last_anim >= 50;
  if (explain_until && (int32_t)(t - explain_until) >= 0) { explain_until = 0; dirty(); }
  { static uint32_t blink = 0; if (ring_active && t / 500 != blink) { blink = t / 500; dirty(); } }
  if (ts.text_mode) read_orders(ts);
  if (mis_show_pending && ts.text_mode && is_bridge(ts) && !manual_open && !pause_open) {   // the card once, on the bridge
    mis_show_pending = false;
    explain(mis_title.empty() ? "MISSION" : mis_title.c_str(), mission_text().c_str());
    }
  {   // the bridge hint strip alternates with the mission goal every 8 s
    static uint32_t phase = 0;
    uint32_t ph = t / 8000;
    if (ph != phase) { phase = ph; if (!mis_goal.empty() && ts.text_mode && is_bridge(ts)) dirty(); }
  }
  bool full = changed || drawn_version != ui_version;
  if (!full && !flash_on && !toast_anim && !scroll_due) return rd;
  if (flash_on || toast_anim || scroll_due) last_anim = t;

  Surf ts_top = {top, TOP_W, TOP_H}, ts_bot = {bottom, BOT_W, BOT_H};
  // Only the scrolling prompt moved: redraw just that strip (keeps idle screens cheap).
  if (!full && !flash_on && !toast_anim && ts.text_mode && !manual_open && !pause_open) {
    std::string prompt; parse_menu(ts, prompt);
    prompt_strip(ts_bot, ts, prompt);
    rd.bottom = true; rd.by0 = UP_H; rd.by1 = ROW1_Y0;   // (the alert box reaches down to the menu row)
    return rd;
  }
  bool draw_top = full;      // a button flash or toast only touches the touch screen
  btns.clear();
  sarea.on = false;          // (generic_bottom sets it again for the reader or a long menu)
  if (manual_open) {
    if (draw_top) { manual_top(ts_top); top_native = true; }
    manual_bottom(ts_bot); rd.top = draw_top; rd.bottom = true;
  } else if (pause_open) {
    pause_bottom(ts_bot, ts); rd.bottom = true;
  } else if (!ts.text_mode) {
    graphics_bottom(ts_bot); rd.bottom = true;
  } else if (is_bridge(ts)) {
    // The TARGET ID box (manual targeting) lies over COMMS and the panels: the top screen keeps
    // what was under it; its text is in the strip (capture 3: garbage on the dashboard).
    bool tid = row_str(ts, 14, 30, 50).find("TARGET ID") != std::string::npos && bridge_kept;
    static TextScreen tmix;
    if (tid) {
      memcpy(&tmix, &ts, sizeof ts);
      for (int r = 14; r <= 16; r++) for (int c = 15; c <= 63; c++) { tmix.ch[r][c] = bridge_keep.ch[r][c]; tmix.at[r][c] = bridge_keep.at[r][c]; }
    }
    const TextScreen& tv = tid ? tmix : ts;
    if (draw_top) { ct_on = false; if (top_w == BOT_W) bridge_top_narrow(ts_top, tv); else bridge_top(ts_top, tv); top_native = true; }
    memcpy(&bridge_keep, &tv, sizeof ts); bridge_kept = true;
    reader_on = false;
    if (kb_open) keyboard(ts_bot); else bridge_bottom(ts_bot, ts);
    bottom_rows(ts_bot, ts);
    rd.top = draw_top; rd.bottom = true;
  } else {
    analyse(ts);
    if (kb_open) { reader_on = false; keyboard(ts_bot); }   // (the top must not keep a stale reader page)
    else generic_bottom(ts_bot, ts);
    bottom_rows(ts_bot, ts);      // (before the top: it decides row_mode, which the top's hint reads)
    // A window over the bridge: the top screen stays the dashboard. The cells under the window
    // come from the last full bridge screen, the rest is live. (UI review 8: it was the whole
    // 80x25 screen shrunk.)
    bool dash = false;
    if (draw_top && glay.kind == G_WIN && bridge_kept && top_w != BOT_W) {
      static TextScreen mix; memcpy(&mix, &ts, sizeof ts);
      for (int r = glay.r0; r <= glay.r1; r++) for (int c = glay.c0; c <= glay.c1; c++) { mix.ch[r][c] = bridge_keep.ch[r][c]; mix.at[r][c] = bridge_keep.at[r][c]; }
      // (only when what is still visible around the window is that same bridge: its frame lines match)
      int frame = 0, same = 0;
      for (int r = 0; r < 24; r++) for (int c = 0; c < 80; c++) {
        if (r >= glay.r0 && r <= glay.r1 && c >= glay.c0 && c <= glay.c1) continue;
        if (is_box(bridge_keep.ch[r][c])) { frame++; if (ts.ch[r][c] == bridge_keep.ch[r][c]) same++; }
      }
      if (frame > 40 && same * 20 >= frame * 19 && is_bridge(mix)) { ct_on = false; bridge_top(ts_top, mix); top_native = true; dash = true; }
    }
    if (draw_top && !dash) { generic_top(ts_top, ts); top_native = false; }
    rd.top = draw_top; rd.bottom = true;
  }
  if (ts.text_mode && !manual_open && !pause_open) check_refusals(ts, t);
  if (ts.text_mode && !goal_met && tut_num == 0 && screen_has(ts, "COMPLETED THE MINIMUM REQUIREMENTS")) {
    goal_met = true;
    explain("OBJECTIVE COMPLETE", "The game says the mission's minimum is met. Answer its questions, then fly home: tap STARFORT, YES, and dock. Docking lets you end the mission.");
  }
  { // saving by name: say what the keyboard is for (the game's own box is under the bridge panels)
    static bool save_card = false; bool sv = ts.text_mode && screen_has(ts, "Enter file name");
    if (sv && !save_card && !manual_open && !pause_open) { explain("SAVE THE MISSION", "Type a name of up to 8 letters and tap ENTER. If a name is already shown, ENTER alone saves over that file. Resume it later from the sign-on menu: Resume a saved game."); }
    save_card = sv;
  }
  {   // the game's message box, in full, on the top screen (the bridge panels can only show slices)
    static std::string shown;
    if (pop_text != shown && !manual_open && !pause_open) {
      shown = pop_text;
      if (!pop_text.empty() && is_bridge(ts) && pop_text.find('?') == std::string::npos) {
        std::string m = pop_text;
        if (m.find("NOT AVAILABLE IN TUTORIALS") != std::string::npos) m += ". Tutorials switch this off; it works in a mission or in Practice.";
        else if (m.find("NO INFORMATION ON RECORD") != std::string::npos) m += ". The ship's information system has no entry for that word; pick a topic from its index.";
        explain("THE GAME SAYS", m.c_str());
      }
    }
  }
  // The game left to DOS (LEAVE after a finished tutorial) and the start script runs it again:
  // nothing of the old mission is left (verify 4: the title picture got the mission's keypad).
  if (ts.text_mode && (screen_has(ts, "Please stand by") || screen_has(ts, "C:\\>"))) {
    mission_on = false; tut_num = 0; tut_started = false; tut_finished = false; goal_met = false; chk_sec = -1; chk_wait_end = false; kb_open = false;
  }
  if (ts.text_mode && screen_has(ts, "SIGN-ON OPTIONS")) { tut_num = 0; tut_started = false; tut_finished = false; goal_met = false; leave_until = 0; leave_xdos = false; chk_sec = -1; chk_wait_end = false; }
  else menu_fresh = true;
  {   // the first Practice mission: its checklist follows by itself, like Tutorial 1's (cold read: nobody found it)
    static std::string started_for;
    if (coach_on && chk_sec < 0 && tut_num == 0 && ts.text_mode && is_bridge(ts) && bridge_situation(ts) == SIT_DOCKED && !goal_met &&
        mis_goal.find("mineral-rich") != std::string::npos && started_for != mis_title + mis_goal &&
        trim(row_str(ts, 24, 0, 24)).compare(0, 13, "Select option") == 0 && (float)atof(row_str(ts, 11, 11, 17).c_str()) < 0.05f) {
      started_for = mis_title + mis_goal;
      int first = chk_find("Your first mission (1-A)");
      if (first >= 0) { chk_sec = first; chk_step = 0; chk_begin_step(ts); dirty(); }
    }
  }
  if (tut_num >= 1 && !tut_started && ts.text_mode && is_bridge(ts)) {      // Tutorial 1: its checklist follows by itself
    tut_started = true;
    // (the card whose title begins "Tutorial <number>": 1 has three parts and starts at 1A)
    int first = -1; std::string want = "Tutorial " + std::to_string(tut_num);
    for (size_t i = 0; i < manual.size() && first < 0; i++)
      if (!manual[i].steps.empty() && manual[i].title.compare(0, want.size(), want) == 0 && (manual[i].title[want.size()] == ':' || manual[i].title[want.size()] == 'A')) first = (int)i;
    if (first >= 0) { chk_sec = first; chk_step = 0; chk_begin_step(ts); dirty(); }
  }
  // The shield and speed buttons answer where the stylus is: the new value shows on the prompt
  // strip (UI review 12: the value was only on the top screen).
  if (ts.text_mode && is_bridge(ts) && !manual_open && !pause_open) {
    static int was_sh = -2; static float was_set = -2.0f; static bool was_bridge = false;
    int sh = pct_at(ts, 22, 44, 58); float set = bridge_speed(ts, 15);
    if (was_bridge && (int32_t)(t - busy_since) < 2500) {
      char m[48];
      if (sh >= 0 && was_sh >= 0 && sh != was_sh) { snprintf(m, sizeof m, "Shields %d%%", sh); say(m, 2000); }
      else if (set >= 0.0f && was_set >= 0.0f && set != was_set) { snprintf(m, sizeof m, set >= 1.0f ? "Speed set: %.0f (hyperspace)" : "Speed set: %.1f", set); say(m, 2000); }
    }
    was_sh = sh; was_set = set; was_bridge = true;
  }
  // START > LEAVE: Esc back to the bridge menu, then Computer, Special services, Quit, YES. In a
  // tutorial the last question (give up?) is answered NO for the player; in a mission it is theirs.
  if (leave_until && (int32_t)(leave_until - t) <= 0) leave_until = 0;
  if (leave_until && ts.text_mode && queue.empty() && (int32_t)(t - leave_next) >= 0) {
    std::string p24 = row_str(ts, 24, 0, 24), m24 = row_str(ts, 24, 24, 80);
    if (leave_xdos) {
      // (capture 3: after a kill, Tutorial 4's Quit only answers "return to starfort". Answering NO
      // says "TO EXIT, USE EXIT COMMAND [Ctrl][X]"; its two questions lead out to DOS, and the start
      // script runs Star Fleet II again, so the player lands on the sign-on screen.)
      if (screen_has(ts, "Exit to DOS?") || screen_has(ts, "EXIT TO DOS?")) press(K_Y);
      else if (screen_has(ts, "USE EXIT COMMAND")) press(K_ENTER);
      else if (in_flight(ts)) press(K_X, M_CTRL);
      else { leave_xdos = false; leave_until = 0; }
    }
    // (between "want to stop?" and "give up?" the screen is blank for a moment: wait, don't stop.
    // Verify 4: Tutorial 2's give-up question was left unanswered 2 of 2 times.)
    else if (!in_flight(ts) && !screen_has(ts, "give up on this mission")) { if (screen_has(ts, "SIGN-ON OPTIONS")) leave_until = 0; }
    else if (screen_has(ts, "give up on this mission")) { if (tut_num > 0) press(K_N); leave_until = 0; }
    else if (p24.find("want to stop") != std::string::npos) press(K_Y);
    else if (p24.find("claim victory") != std::string::npos) {
      // (harness, Nov 27: once the goal is met the game's Quit only says "return to starfort")
      if (tut_num > 0) { press(K_N); leave_xdos = true; leave_until = now() + 30000; say("Leaving the tutorial: the game starts again at sign-on...", 8000); }
      else { press(K_Y); leave_until = 0; explain("GOAL MET", "The mission's goal is met, so the game ends it at the starfort: tap STARFORT, answer YES, then YES to dock and YES to conclude the mission."); }
    }
    else if (is_bridge(ts) && m24.find("Quit") != std::string::npos) press(K_Q);
    else if (is_bridge(ts) && m24.find("Spcl srvcs") != std::string::npos) press(K_S);
    else if (is_bridge(ts) && m24.find(" Cmp ") != std::string::npos) press(K_C);
    else press(K_ESC);
    leave_next = t + 900; busy_since = t;
  }
  {   // a "wait" step while the ship stands still in open space: the clock is not running (cold read 1 and 2)
    static uint32_t still_since = 0, warned = 0;
    bool waiting = chk_sec >= 0 && chk_step < (int)manual[chk_sec].steps.size() &&
                   (manual[chk_sec].steps[chk_step].what.compare(0, 4, "Wait") == 0 || manual[chk_sec].steps[chk_step].what.compare(0, 4, "Keep") == 0);
    if (waiting && ts.text_mode && is_bridge(ts) && bridge_situation(ts) == SIT_CRUISE && bridge_speed(ts, 16) <= 0.0f && !frozen(ts) &&
        trim(row_str(ts, 24, 0, 24)).compare(0, 13, "Select option") == 0) {
      if (!still_since) still_since = t;
      if (t - still_since > 10000 && t - warned > 20000) { warned = t; say("Clock stopped: tap SPD+, then ENGINES", 6000); }
    } else still_since = 0;
  }
  // The manual and the START menu stop the game's clock while they are open (cold read 2: a day
  // and a third of the supplies went by in orbit while the manual was being read).
  {
    static bool was_open = false, we_froze = false; static uint32_t froze_t = 0;
    bool open_now = manual_open || pause_open;
    if (open_now && !was_open && ts.text_mode && is_bridge(ts) && !frozen(ts) && !leave_until) { press(K_F, M_CTRL); we_froze = true; froze_t = t; }
    if (!open_now && was_open && we_froze) { if (ts.text_mode && is_bridge(ts) && frozen(ts) && !leave_until) press(K_F, M_CTRL); we_froze = false; }
    if (leave_until && we_froze && ts.text_mode && is_bridge(ts) && frozen(ts) && t - froze_t > 500) { press(K_F, M_CTRL); we_froze = false; }
    was_open = open_now;
    if (save_pending && !open_now && queue.empty()) { save_pending = false; press(K_Q, M_CTRL); }
  }
  bool chk_moved = false;
  if (!manual_open && !pause_open) { int was = chk_step, ws = chk_sec; chk_update(ts); if (was != chk_step || ws != chk_sec) chk_moved = true; }
  if (refusal_button && (int32_t)(t - refusal_until) >= 0) { refusal_button = 0; dirty(); }
  ring_active = false;
  if (refusal_button) for (auto& b : btns) if (b.label == refusal_button) { b.coach = true; ring_active = true; }
  if (advise_button && (int32_t)(t - advise_until) >= 0) { advise_button = 0; dirty(); }
  if (advise_button && !pause_open && !manual_open) for (auto& b : btns) if (b.label == advise_button) { b.coach = true; ring_active = true; }
  if (coach_card >= 0 && (!explain_until || explain_title != COACH_TITLE[coach_card])) coach_card = -1;
  if (coach_card >= 0 && coach_step(ts) != coach_card) { coach_card = -1; explain_until = 0; dirty(); }   // its moment has passed
  if (coach_on && !manual_open && !pause_open && !kb_open) {
    int cs = coach_step(ts);
    if (cs >= 0) {
      // (one card at a time: the mission card, a refusal or "what now" is read first)
      if (!(coach_seen & (1 << cs)) && !mis_show_pending && (chk_sec < 0 || cs == CS_ALERT) && !explain_until && (tut_num == 0 || cs == CS_ALERT || cs == CS_MENU)) {
        coach_seen |= 1 << cs;
        explain(COACH_TITLE[cs], cs == CS_MENU && practice_on(ts) ? "Practice is on. Tap Start a new mission. Stuck anywhere: press SELECT." :
                cs == CS_MENU && tut1_done ? "Next: tap Practice (demo) mission, answer YES, then Start a new mission. Stuck anywhere: press SELECT." : COACH_TEXT[cs]);
        coach_card = cs;
        if (!draw_top) dirty();     // the next frame redraws the top with the card
      }
      // While the game waits for an answer or a key on the bridge, that is the next thing to tap
      // (emulator, Nov 21: LAUNCH was ringed while "Comply with the request?" swallowed every key).
      const char* want = COACH_BUTTON[cs];
      if (cs == CS_MENU && practice_on(ts)) want = "Start a new mission";
      else if (cs == CS_MENU && tut1_done) want = "Practice (demo) mission";
      if (cs == CS_DOCKED && !trim(row_str(ts, 9, 20, 26)).empty()) want = "LAUNCH";   // a target is loaded
      if (chk_sec >= 0 || tut_num > 0) want = 0;                                         // (the checklist rings its own step)
      if (is_bridge(ts)) {
        std::string pr = row_str(ts, 24, 0, 24);
        if (pr.find('?') != std::string::npos) want = 0;
        else if (pr.find("PRESS") != std::string::npos) want = "CONTINUE";
      }
      if (want) for (auto& b : btns) if (b.label == want) { b.coach = true; ring_active = true; }
    }
  }
  if (chk_sec >= 0 && !manual_open && !pause_open) {
    const std::vector<Step>& cst = manual[chk_sec].steps;
    bool ring = chk_step < (int)cst.size() && !cst[chk_step].btn.empty() && !chk_wait_end;
    if (ring && (cst[chk_step].btn == "YES" || cst[chk_step].btn == "NO")) {
      // the step names its question ("Establish orbit? Answer YES"): its last two words must be on screen
      std::string q = cst[chk_step].what; size_t qm = q.find('?');
      if (qm != std::string::npos) {
        q = q.substr(0, qm); size_t sp = q.rfind(' '); if (sp != std::string::npos) { size_t sp2 = q.rfind(' ', sp - 1); q = q.substr(sp2 == std::string::npos ? 0 : sp2 + 1); }
        for (auto& c : q) c = (char)tolower((unsigned char)c);
        if (!screen_has_ci(ts, q)) ring = false;
      }
    }
    if (ring)
      for (auto& b : btns) if (b.label == cst[chk_step].btn) { b.coach = true; ring_active = true; }
    if (draw_top && ts.text_mode && !explain_until) { chk_card(ts_top, ts, !is_bridge(ts) && glay.kind != G_MAP); rd.top = true; }
  }
  if (draw_top && explain_until && ts.text_mode && !manual_open && !pause_open) { explain_card(ts_top, ts); rd.top = true; }
  if (focus_mode && !manual_open && !pause_open) {     // ring the button nearest the last focus point
    int best = -1, bd = 1 << 30;
    for (int i = 0; i < (int)btns.size(); i++) {
      if (btns[i].off) continue;
      int dx = (btns[i].x0 + btns[i].x1) / 2 - fx, dy = (btns[i].y0 + btns[i].y1) / 2 - fy, d = dx * dx + dy * dy;
      if (d < bd) { bd = d; best = i; }
    }
    if (best < 0) focus_mode = false;
    else btns[best].focus = true;
  }
  for (int i = 0; i < (int)btns.size(); i++) draw_btn(ts_bot, i);
  {   // (cold read: a tap meant for TARGETER landed on SPD-, the button that had just taken its place)
    uint32_t sig = 2166136261u;
    for (auto& b : btns) { for (char c : b.label) sig = (sig ^ (uint8_t)c) * 16777619u; sig = (sig ^ (uint32_t)b.x0) * 16777619u; }
    if (sig != btn_sig) { btn_sig = sig; btn_sig_t = t; }
  }
  {   // a card goes when the screen it was shown on goes (bridge, map, page, picture)
    int ctx = !ts.text_mode ? 1 : is_bridge(ts) ? 2 + 16 * alert_level(ts) + 64 * (int)bridge_situation(ts) : in_flight(ts) ? 3 + glay.kind : 8;
    static int card_ctx = 0;
    if (card_fresh) { card_ctx = ctx; card_fresh = false; }
    else if (explain_until && ctx != card_ctx && !quickref_up && !manual_open && !pause_open) { explain_until = 0; coach_card = -1; dirty(); }
    if (explain_until && explain_title == "THE GAME SAYS" && pop_text.empty()) { explain_until = 0; dirty(); }
    if (explain_until && explain_title == "SAVE THE MISSION" && !(ts.text_mode && screen_has(ts, "Enter file name"))) { explain_until = 0; dirty(); }
  }
  {   // the keyboard opened on a picture (boarding: the letter A) closes when the picture does (capture 3)
    static bool kb_pic = false;
    if (!ts.text_mode && kb_open) kb_pic = true;
    if (!kb_open) kb_pic = false;
    if (ts.text_mode && !last_text_mode && kb_pic) { kb_open = false; kb_pic = false; dirty(); }
  }
  memcpy(&last, &ts, sizeof ts);
  last_text_mode = ts.text_mode;
  drawn_version = ui_version;
  if (chk_moved) dirty();      // (after the line above: the next frame draws the new step and looks again)
  return rd;
}

// PG UP / PG DN on a reader page: page the port's own text first. The game's key goes out only
// when the text is at its end, and never on a "press any key" page (cold read, Nov 27: PG DN on
// the orders threw them away).
static void rd_page(int dir) {
  if (reader_on && dir > 0 && rd_scroll < rd_max) { rd_scroll = std::min(rd_max, rd_scroll + RD_LINES - 1); dirty(); return; }
  if (reader_on && dir < 0 && rd_scroll > 0) { rd_scroll = std::max(0, rd_scroll - (RD_LINES - 1)); dirty(); return; }
  if (reader_on && screen_has(last, "ANY KEY")) { say(dir > 0 ? "End of the text: tap CONTINUE" : "Top of the text", 2500); return; }
  press(dir > 0 ? K_PGDN : K_PGUP); busy_since = now();
}
// ------------------------------------------------------------------ touch
static bool touch_was_down = false;
static int tap_kind = 0;   // 0 none, 1 tactical, 2 loupe
static int hit(int x, int y) {
  for (int i = (int)btns.size() - 1; i >= 0; i--)
    if (x >= btns[i].x0 && x < btns[i].x1 && y >= btns[i].y0 && y < btns[i].y1) return btns[i].off ? -2 : i;
  return -1;
}

// Snap a tapped cell to the nearest visible symbol within 2 columns / 1 row (map cells are
// two columns wide and a stylus tap is rarely exact). Grid dots and box lines don't count.
static void snap_to_object(const TextScreen& ts, int& col, int& row, int c0, int r0, int c1, int r1) {
  // A tap on the target box's own frame moves the box that way by one: whatever lies under the
  // frame (a star next to your ship on the star map) can only be reached like that.
  if (row >= 0 && row < 25 && col >= 0 && col < 80 && (ts.at[row][col] & 15) == 5 && is_box(ts.ch[row][col])) return;
  int best = 99, bc = col, br = row;
  for (int dr = -1; dr <= 1; dr++)
    for (int dc = -2; dc <= 2; dc++) {
      int r = row + dr, c = col + dc;
      if (r < r0 || r > r1 || c < c0 || c > c1) continue;
      uint8_t ch = ts.ch[r][c];
      bool sym = ch > ' ' && ch != 0xF9 && ch != 0xFA && ch != '.' && !is_box(ch) && ch != 0xDB;
      bool box_part = (ts.at[r][c] & 15) == 5 && is_box(ch);
      if (!sym || box_part) continue;
      int d = abs(dc) + abs(dr) * 2;
      if (d < best) { best = d; bc = c; br = r; }
    }
  // a planet's letter stands right of the planet itself ("oA"): the box belongs on the planet
  if (ts.ch[br][bc] >= 'A' && ts.ch[br][bc] <= 'Z' && bc > c0) {
    uint8_t l = ts.ch[br][bc - 1];
    if (l > ' ' && !(l >= 'A' && l <= 'Z') && l != 0xF9 && l != 0xFA && l != '.' && !is_box(l) && l != 0xDB) bc--;
  }
  col = bc; row = br;
}
static void start_drive(int col, int row, int c, int r) {
  bool same = abs(col - c) <= 1 && row == r;           // a second tap on the box locks it in
  tdb_drive = true; tdb_tc = col; tdb_tr = row; tdb_steps = 0; tdb_best = 9999; tdb_lc = tdb_lr = -1;
  tdb_enter_after = same; tdb_next = 0;
}

static void tactical_tap(int x, int y) {
  int col = 28 + (x - TAC_X) / TAC_CW, row = 2 + (y - TAC_Y) / TAC_LH;
  int c, r;
  bool box = find_tdb(last, c, r, 29, 2, 49, 13);
  bool target = tap_mode == TM_TARGET || (tap_mode == TM_AUTO && box);
  if (!target) { steer_to(last, x, y); return; }
  if (!box) {
    say("No target box: set Weapons to Manual targeting");
    return;
  }
  snap_to_object(last, col, row, 29, 2, 49, 12);
  start_drive(col, row, c, r);
}

// A tap on a menu line ("N  Start a new mission") or on a word with a red hotkey letter sends that key.
static char hotkey_at(const TextScreen& ts, int col, int row) {
  if (row < 0 || row > 24) return 0;
  // red hotkey letter inside the tapped word
  int a = col, b = col;
  while (a > 0 && ts.ch[row][a - 1] > ' ' && !is_box(ts.ch[row][a - 1])) a--;
  while (b < 79 && ts.ch[row][b + 1] > ' ' && !is_box(ts.ch[row][b + 1])) b++;
  for (int c = a; c <= b; c++) if ((ts.at[row][c] & 15) == 12 && ts.ch[row][c] > ' ') return (char)ts.ch[row][c];
  // "X  Label" menu line: a single capital letter followed by two spaces, left of the tap
  for (int c = col; c >= 1; c--) {
    uint8_t ch = ts.ch[row][c];
    if (ch >= 'A' && ch <= 'Z' && (c == 0 || ts.ch[row][c - 1] == ' ' || is_box(ts.ch[row][c - 1])) &&
        c + 2 < 80 && ts.ch[row][c + 1] == ' ' && ts.ch[row][c + 2] == ' ')
      return (char)ch;
    if (is_box(ch)) break;
  }
  return 0;
}

static void map_tap(int x, int y) {
  if (glay.kind != G_MAP) return;
  int col = glay.c0 + (x - glay.ox) / glay.cw, row = glay.r0 + (y - glay.oy) / glay.lh;
  if (col < glay.c0 || col > glay.c1 || row < glay.r0 || row > glay.r1) return;
  chk_pressed = "MAP";
  int c, r;
  snap_to_object(last, col, row, glay.c0, glay.r0, glay.c1, glay.r1);
  if (!find_tdb(last, c, r)) {
    // The maps hide the target box until an arrow key is pressed (seen in Tutorial 1): bring it
    // up, then drive it to the tapped spot.
    if (!in_flight(last)) { say("No target box: choose a targeting option first"); return; }
    press(K_RIGHT);
    tdb_drive = true; tdb_tc = col; tdb_tr = row; tdb_steps = 0; tdb_best = 9999; tdb_enter_after = false; tdb_lc = tdb_lr = -1;
    tdb_show_until = now() + 2500; tdb_next = now() + 300;
    return;
  }
  start_drive(col, row, c, r);
}

static float man_mom = 0;   // manual drag momentum, px per frame (run in buttons())
static int man_acc = 0;     // drag remainder below one 12 px line

void touch(int x, int y, bool down) {
  // Manual: a touch that moves more than 6 px drags the text (rule 13) and presses nothing.
  if (manual_open) {
    if (down && !touch_was_down) { drag_x = x; drag_y = y; drag_ly = y; dragging = false; man_acc = 0; man_mom = 0; }
    else if (down && touch_was_down) {
      if (!dragging && (abs(x - drag_x) > 6 || abs(y - drag_y) > 6)) { dragging = true; pressed_id.valid = false; dirty(); }
      if (dragging) {
        man_acc += drag_ly - y; man_mom = (float)(drag_ly - y);
        while (man_acc >= 12) { man_scroll++; man_acc -= 12; dirty(); }
        while (man_acc <= -12) { man_scroll--; man_acc += 12; dirty(); }
      }
      drag_ly = y;
      if (dragging) { touch_was_down = down; return; }
    } else if (!down && touch_was_down && dragging) { dragging = false; touch_was_down = down; return; }
  }
  // Reader text or a long menu: a touch that moves more than 6 px up or down drags the list and
  // presses nothing (rule 12/13).
  if (sarea.on && !manual_open && !pause_open) {
    if (down && !touch_was_down) {
      sd_maybe = x >= sarea.x0 && x < sarea.x1 && y >= sarea.y0 && y < sarea.y1;
      sdrag = false; sd_y = y; sd_ly = y; sd_acc = 0; sd_mom = 0;
    } else if (down && touch_was_down && sd_maybe) {
      if (!sdrag && abs(y - sd_y) > 6) { sdrag = true; pressed_id.valid = false; tap_kind = 0; dirty(); }
      if (sdrag) { sd_acc += sd_ly - y; sd_mom = (float)(sd_ly - y); sd_ly = y; sarea_scroll(); touch_was_down = down; return; }
      sd_ly = y;
    } else if (!down && touch_was_down && sdrag) { sdrag = false; sd_maybe = false; tap_kind = 0; touch_was_down = down; return; }
  }
  if (r_held && !manual_open && !pause_open && last.text_mode) {   // explain, press nothing
    if (down && !touch_was_down) {
      r_used = true;
      int i = -1;
      for (int k = (int)btns.size() - 1; k >= 0 && i < 0; k--)
        if (x >= btns[k].x0 && x < btns[k].x1 && y >= btns[k].y0 && y < btns[k].y1) i = k;
      if (i >= 0 && explain_button(btns[i].label)) {
        if (btns[i].off) { explain_text += " (Greyed out: not available on this screen.)"; }
      } else if (i >= 0 && !btns[i].label.empty() && btns[i].label.size() > 1) {
        explain(btns[i].label.c_str(), "One of the game's own commands on this screen; tapping it chooses it.");
      } else explain_spot(last, x, y);
    }
    touch_was_down = down; return;
  }
  if (down && !touch_was_down) {
    tap_kind = 0;
    card_dismiss();
    if (advise_button) { advise_button = 0; dirty(); }
    int i = hit(x, y);
    if (i >= 0 && !manual_open && !pause_open && !kb_open && (uint32_t)(now() - btn_sig_t) < 350 && btn_sig_t) { touch_was_down = true; return; }
    if (i >= 0) {
      pressed_id = id_of(btns[i]);
      if (H && H->click) H->click();
      dirty();
    } else if (!manual_open && !pause_open && !kb_open && last.text_mode) {
      if (is_bridge(last) && grid_shown) {}                 // (the menu grid covers the tactical display)
      else if (is_bridge(last) && x < 180 && y < UP_H) { tap_kind = 1; tap_x = x; tap_y = y; }
      else if (!is_bridge(last) && y < UP_H && glay.kind == G_MAP) { tap_kind = 2; tap_x = x; tap_y = y; }
      else if (reader_on && y < RD_Y0 + RD_LINES * 12) { tap_kind = 3; tap_x = x; tap_y = y; }
      else if (glay.kind == G_WIN && y >= WIN_Y0 && y < WIN_Y0 + WIN_ROWS * 12) { tap_kind = 4; tap_x = x; tap_y = y; }
    }
  } else if (down && touch_was_down) {

  } else if (!down && touch_was_down) {
    if (pressed_id.valid) {
      BtnId p = pressed_id; pressed_id.valid = false;
      int i = -1;
      for (int k = 0; k < (int)btns.size(); k++) if (same(p, btns[k])) i = k;
      if (i >= 0 && x >= p.x0 - 8 && x < p.x1 + 8 && y >= p.y0 - 8 && y < p.y1 + 8) {
        flash_id = p; flash_until = now() + 110; flash_on = true;
        chk_pressed = btns[i].label;
        if (btns[i].label == "ALL BACK") say("Everyone is marked to return: now tap BEAM UP", 4000);
        fire(btns[i].act);
      }
      dirty();
    } else if (tap_kind == 1) {
      tactical_tap(tap_x, tap_y);
    } else if (tap_kind == 2) {
      map_tap(tap_x, tap_y);
    } else if (tap_kind == 3) {
      char k = reader_hotkey(tap_x, tap_y);
      if (k) { Key kk = key_for_char(k); if (kk != K_NONE) { press(kk); busy_since = now(); } }
    } else if (tap_kind == 4) {          // a word in the window with a hotkey letter
      int col = glay.c0 + (tap_x - glay.ox) / 6, row = glay.r0 + win_top + (tap_y - WIN_Y0) / 12;
      char k = col >= glay.c0 && col <= glay.c1 && row <= glay.r1 ? hotkey_at(last, col, row) : 0;
      if (k) { Key kk = key_for_char(k); if (kk != K_NONE) { press(kk); busy_since = now(); } }
    }
    tap_kind = 0;
  }
  touch_was_down = down;
}

// ------------------------------------------------------------------ buttons
struct Repeat { uint32_t bit; uint32_t next; };
static Repeat reps[16];
static int nreps = 0;
static bool repeat_due(uint32_t bit, uint32_t held, uint32_t pressed_now) {
  uint32_t t = now();
  Repeat* r = 0;
  for (int i = 0; i < nreps; i++) if (reps[i].bit == bit) r = &reps[i];
  if (!r && nreps < 16) { r = &reps[nreps++]; r->bit = bit; r->next = 0; }
  if (pressed_now & bit) { r->next = t + 320; return true; }
  if ((held & bit) && (int32_t)(t - r->next) >= 0) { r->next = t + 70; return true; }
  return false;
}

void buttons(uint32_t held, uint32_t pr, uint32_t rel) {
  (void)rel;
  if (manual_open) {
    // the standard manual controls: sticks scroll, D-Pad sections, L/R prev/next, B/START close
    if (repeat_due(B_CPAD_DOWN, held, pr) || repeat_due(B_CSTICK_DOWN, held, pr)) { man_scroll++; dirty(); }
    if (repeat_due(B_CPAD_UP, held, pr) || repeat_due(B_CSTICK_UP, held, pr)) { man_scroll--; dirty(); }
    int n = (int)manual.size();
    // D-Pad: through the sections, then LEFT HAND and CLOSE MANUAL (gold ring; A presses them), then
    // round to the first section. L/R always page the sections.
    if (pr & B_R) { man_sel = (man_sel + 1) % n; man_scroll = 0; man_focus = 0; dirty(); }
    if (pr & B_L) { man_sel = (man_sel + n - 1) % n; man_scroll = 0; man_focus = 0; dirty(); }
    if (repeat_due(B_DDOWN, held, pr)) {
      if (man_focus == -1) man_focus = -2;
      else if (man_focus == -2) { man_focus = 0; man_sel = 0; man_scroll = 0; }
      else if (man_sel == n - 1) man_focus = -1;
      else { man_sel++; man_scroll = 0; }
      dirty();
    }
    if (repeat_due(B_DUP, held, pr)) {
      if (man_focus == -2) man_focus = -1;
      else if (man_focus == -1) man_focus = 0;
      else if (man_sel == 0) man_focus = -2;
      else { man_sel--; man_scroll = 0; }
      dirty();
    }
    if ((pr & B_A) && man_focus < 0) {
      if (H && H->click) H->click();
      flash_until = now() + 110; flash_on = true;
      for (auto& b : btns) if (b.focus) flash_id = id_of(b);
      fire(kind_act(man_focus == -1 ? (manual[man_sel].steps.empty() ? A_LEFTY : A_FOLLOW) : A_CLOSE));
    }
    if (pr & (B_B | B_START)) { manual_open = false; man_focus = 0; manual_closed(); save_settings(); dirty(); }
    if (!dragging && (man_mom > 0.5f || man_mom < -0.5f)) {   // coast a little after a drag
      man_acc += (int)(man_mom * 2.0f); man_mom *= 0.88f;
      while (man_acc >= 12) { man_scroll++; man_acc -= 12; dirty(); }
      while (man_acc <= -12) { man_scroll--; man_acc += 12; dirty(); }
    }
    return;
  }
  if (pause_open) {
    if (pr & (B_B | B_START)) {
      H->click();
      if (quit_confirm) { quit_confirm = false; pause_focus = 6; } else pause_open = false;
      dirty(); return;
    }
    if (quit_confirm) {
      if (pr & (B_DLEFT | B_DRIGHT)) { pause_focus = !pause_focus; dirty(); }
      if (pr & B_A) { H->click(); if (pause_focus == 0) fire(kind_act(A_QUIT_YES)); quit_confirm = false; pause_focus = 6; dirty(); }
      return;
    }
    bool bridge = is_bridge(last);
    bool menu = last.text_mode && screen_has(last, "Change default settings", 0, 24);
    (void)bridge;
    bool usable[7] = {true, true, true, true, bridge, menu, H && H->quit};
    int step = repeat_due(B_DDOWN, held, pr) ? 1 : repeat_due(B_DUP, held, pr) ? 6 : 0;
    if (!step && (pr & (B_DLEFT | B_DRIGHT)) && (pause_focus == 3 || pause_focus == 4) && usable[7 - pause_focus]) { pause_focus = 7 - pause_focus; dirty(); }
    if (step) {
      do pause_focus = (pause_focus + step) % 7; while (!usable[pause_focus]);
      dirty();
    }
    if ((pr & B_A) && usable[pause_focus]) {
      static const ActKind k[7] = {A_PAUSE, A_MISSION, A_MANUAL, A_COACH, A_SAVE, A_SETTINGS, A_QUIT};
      H->click(); fire(kind_act(k[pause_focus])); dirty();
    }
    return;
  }
  if (sarea.on && !sdrag && (sd_mom > 0.5f || sd_mom < -0.5f)) {   // a dragged list coasts a little
    sd_acc += (int)(sd_mom * 2.0f); sd_mom *= 0.88f; sarea_scroll();
  } else if (!sdrag) sd_mom = 0;
  if (pr & ~(B_R | B_SELECT | B_START | B_ZR)) card_dismiss();
  if (pr & B_START) { pause_open = true; quit_confirm = false; pause_focus = 0; focus_mode = false; H->click(); dirty(); return; }
  if (pr & B_ZR) {
    focus_mode = !focus_mode;
    if (focus_mode) say(lefty_on ? "ABXY: move  D-Rt: press  D-Dn/ZR: done" : "D-Pad: move  A: press  B/ZR: done", 3000);
    dirty(); return;
  }
  if (focus_mode) {
    const uint32_t PAD = B_DUP | B_DDOWN | B_DLEFT | B_DRIGHT | B_A | B_B;
    int cur = -1;
    for (int i = 0; i < (int)btns.size(); i++) if (btns[i].focus) cur = i;
    if (cur >= 0) {
      int cx = (btns[cur].x0 + btns[cur].x1) / 2, cy = (btns[cur].y0 + btns[cur].y1) / 2;
      static const uint32_t dirs[4] = {B_DRIGHT, B_DLEFT, B_DDOWN, B_DUP};
      for (int d = 0; d < 4; d++) {
        if (!repeat_due(dirs[d], held, pr)) continue;
        int best = -1, bc = 1 << 30;
        for (int i = 0; i < (int)btns.size(); i++) {
          if (i == cur || btns[i].off) continue;
          int bx = (btns[i].x0 + btns[i].x1) / 2, by = (btns[i].y0 + btns[i].y1) / 2;
          int along = d == 0 ? bx - cx : d == 1 ? cx - bx : d == 2 ? by - cy : cy - by;
          int across = d < 2 ? abs(by - cy) : abs(bx - cx);
          if (along <= 0) continue;
          int cost = along + 2 * across;
          if (cost < bc) { bc = cost; best = i; }
        }
        if (best >= 0) { fx = (btns[best].x0 + btns[best].x1) / 2; fy = (btns[best].y0 + btns[best].y1) / 2; dirty(); }
      }
      if (pr & B_A) {
        if (H && H->click) H->click();
        flash_id = id_of(btns[cur]); flash_until = now() + 110; flash_on = true;
        chk_pressed = btns[cur].label;
        fire(btns[cur].act);
      }
    }
    if (pr & B_B) { focus_mode = false; dirty(); }
    held &= ~PAD; pr &= ~PAD;
  }
  if (pr & B_SELECT) fire(kind_act(last.text_mode && !in_flight(last) && (kb_open || wants_text(last)) ? A_KB : A_MISSION));   // "What now?" in flight (re-save is in the START menu)
  if (pr & B_A) { press(K_ENTER); typed_enter(); chk_pressed = "A"; }
  if (pr & B_B) chk_pressed = "B";
  if (pr & B_L) chk_pressed = "L";
  if ((pr & B_B) && row_mode == R_YESNO && last.text_mode && !kb_open) press(K_N);     // (B is NO: Esc does nothing at the game's questions)
  else if (pr & B_B) press(K_ESC);
  if (pr & B_X) press(K_INSERT);           // fire phasers
  if (pr & B_Y) press(K_DELETE);           // fire torpedoes
  if (pr & B_L) press(K_F, M_CTRL);        // freeze / unfreeze
  // R: held + tap = explain (plan item 6); held alone = quick reference. (A short press used to
  // cycle the tap mode: newcomers told to "hold R" ended up in TARGET mode with steering dead.)
  if (pr & B_R) { r_held = true; r_used = false; r_since = now(); }
  if (r_held && !r_used && !quickref_up && (int32_t)(now() - r_since) >= 600 && last.text_mode) {
    // quick reference for this screen (learning design): what every button does here
    quickref_up = true; r_used = true;
    bool br = is_bridge(last), fl = in_flight(last);
    const char* body = br ?
      (lefty_on ? "D-Left phasers, D-Up torpedoes. L freeze. ZR/ZL hyperspace speed up/down. Circle Pad: left/right heading, up/down sublight speed. Tap the display to steer. D-Right Enter, D-Down back. SELECT: what now? START menu. R + tap: explain."
                : "X phasers, Y torpedoes. L freeze. ZR/ZL hyperspace speed up/down. Circle Pad: left/right heading, up/down sublight speed. Tap the display to steer. A Enter, B back. SELECT: what now? START menu. R + tap: explain.")
      : fl ? "Tap an item or a spot on the map. A Enter, B back to the bridge. X/Y still fire, Circle Pad still steers. The tabs at the bottom jump straight to another view. START menu. R + tap: explain."
           : "Tap an item to choose it. A Enter (or CONTINUE), B back. Sticks or drag scroll long pages. ZR: move a ring over the buttons with the D-Pad. SELECT keyboard. START menu. R + tap: explain.";
    explain(br ? "QUICK REFERENCE: BRIDGE" : fl ? "QUICK REFERENCE: THIS VIEW" : "QUICK REFERENCE", body);
    explain_until = now() + 600000;      // stays while R is held
  }
  if (rel & B_R) {
    if (r_held && !r_used) say("Hold R and tap something: explain. Hold R: all buttons", 3000);
    if (quickref_up) { quickref_up = false; explain_until = 0; dirty(); }
    r_held = false;
  }
  if (repeat_due(B_DUP, held, pr)) press(K_UP);
  if (repeat_due(B_DDOWN, held, pr)) press(K_DOWN);
  if (repeat_due(B_DLEFT, held, pr)) press(K_LEFT);
  if (repeat_due(B_DRIGHT, held, pr)) press(K_RIGHT);
  // Off the flight screens (orders, reports, long text pages) the sticks read, like the manual:
  // up/down scroll the page a line at a time (hold to keep going), left/right page up/down.
  // They never send the helm keys there.
  // With the text reader up they scroll it first, and only page the game when it is at its end.
  if (last.text_mode && !in_flight(last)) {
    if (repeat_due(B_CPAD_DOWN, held, pr) || repeat_due(B_CSTICK_DOWN, held, pr)) {
      if (reader_on && rd_scroll < rd_max) { rd_scroll++; dirty(); } else press(K_DOWN);
    }
    if (repeat_due(B_CPAD_UP, held, pr) || repeat_due(B_CSTICK_UP, held, pr)) {
      if (reader_on && rd_scroll > 0) { rd_scroll--; dirty(); } else press(K_UP);
    }
    if (repeat_due(B_CPAD_RIGHT, held, pr) || repeat_due(B_CSTICK_RIGHT, held, pr)) rd_page(1);
    if (repeat_due(B_CPAD_LEFT, held, pr) || repeat_due(B_CSTICK_LEFT, held, pr)) rd_page(-1);
    return;
  }
  if (repeat_due(B_CPAD_LEFT, held, pr)) press((Key)(K_F1 + 4));   // heading -1
  if (repeat_due(B_CPAD_RIGHT, held, pr)) press((Key)(K_F1 + 5));  // heading +1
  if (repeat_due(B_CPAD_UP, held, pr)) press((Key)(K_F1 + 3));     // sublight +0.1
  if (repeat_due(B_CPAD_DOWN, held, pr)) press((Key)(K_F1 + 2));   // sublight -0.1
  if (repeat_due(B_CSTICK_UP, held, pr)) press((Key)(K_F1 + 1));   // hyperspeed +1 (ZL is the screen swap)
  if (repeat_due(B_CSTICK_DOWN, held, pr)) press(K_F1);             // hyperspeed -1
  bool bridge = is_bridge(last);
  if (bridge) {
    if (repeat_due(B_CSTICK_LEFT, held, pr)) press(K_LEFT, M_CTRL);   // earlier messages
    if (repeat_due(B_CSTICK_RIGHT, held, pr)) press(K_RIGHT, M_CTRL);
  }
}

}  // namespace sf2ui
