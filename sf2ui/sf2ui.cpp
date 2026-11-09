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

namespace sf2ui {

// ------------------------------------------------------------------ colours
static const uint16_t BG = rgb(8, 10, 18);
static const uint16_t FRAME = rgb(130, 36, 36);
static const uint16_t TITLE = rgb(235, 90, 80);
static const uint16_t BTN = rgb(22, 30, 52), BTN_EDGE = rgb(70, 90, 130), BTN_TXT = rgb(230, 236, 245);
static const uint16_t BTN_DOWN = rgb(90, 110, 160), BTN_SEL = rgb(64, 70, 92), HOT = rgb(255, 120, 100);
static const uint16_t PROMPT_BG = rgb(34, 30, 10), HINT = rgb(140, 150, 170), LOUPE = rgb(255, 210, 80);

// ------------------------------------------------------------------ state
static const Host* H = 0;

struct Ev { Key k; bool down; };
static std::vector<Ev> queue;          // pending key events
static uint32_t next_key_ms = 0;
static const uint32_t KEY_GAP_MS = 35; // spacing between injected key events

enum ActKind { A_NONE, A_KEY, A_MODE, A_MANUAL, A_KB, A_SECTION, A_CLOSE, A_TYPE, A_SIGNON, A_SAVE, A_VIEW, A_PAUSE, A_QUIT, A_QUIT_YES, A_SETTINGS, A_LEFTY };
struct Act { ActKind kind; Key key; uint8_t mods; int arg; };
enum { M_SHIFT = 1, M_CTRL = 2, M_ALT = 4 };

struct Btn {
  int x0, y0, x1, y1;
  std::string label; int hot; bool sel;
  const Font* font; Act act; bool on_top;
  bool off;                            // can't be used on this screen: greyed out, taps do nothing
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

// loupe (generic views): top-left cell of the magnified window
static int lx = 13, ly = 5; static bool follow = true;
static bool dragging = false; static int drag_x, drag_y, drag_lx, drag_ly, drag_moved;
static int tap_x = -1, tap_y = -1;

// target box driver (closed loop: press an arrow, look where the box went, repeat)
// START menu (pause): resume, manual, quick save, game settings, exit.
static bool pause_open = false, quit_confirm = false;
// Busy feedback: a button was pressed and the game hasn't answered yet (see frame()).
static uint32_t busy_since = 0; static bool busy_now = false;
// Jumping straight to another view: close what's open (Esc) until the game is back at a view, then switch.
static int view_goal = -1, view_esc = 0; static uint32_t view_next = 0;
static bool tdb_drive = false; static int tdb_tc, tdb_tr, tdb_steps; static uint32_t tdb_next = 0;
static bool tdb_enter_after = false; static int tdb_best = 9999;

static TextScreen last;                // last screen drawn (for change detection)
static uint32_t ui_version = 1, drawn_version = 0;
static bool last_text_mode = false;

// manual
struct Section { std::string title; std::vector<std::string> lines; };
static std::vector<Section> manual;
static int man_sel = 0, man_scroll = 0, man_list_top = 0;

static void dirty() { ui_version++; }

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
// The screen as it was when the player last sent a key: the magnifier frames everything that
// changed since then (the game prints slowly, so frame-to-frame changes are only the latest few
// characters).
static TextScreen base_ts; static bool base_ok = false;
static void mark_base();
static void press(Key k, uint8_t mods = 0) {
  mark_base();
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

// ---- remembered officer (one-tap sign-on). Saved after a successful sign-on.
static std::string typed;                       // characters typed on the on-screen keyboard
static int typed_y = -1;                        // screen row they were typed on (a new prompt starts afresh)
static uint32_t pass_seen = 0;                  // when the password prompt appeared (auto sign-on waits)
static std::string off_name, off_pass;          // remembered officer
static std::string pend_name, pend_pass;        // typed at this sign-on, saved once it succeeds
static int autologin = 0;                       // 1 = name sent, waiting for the password prompt; 2 = password sent
static uint32_t pass_sent = 0; static int pass_tries = 0; static uint32_t pass_sig = 0;
static void load_officer() {
  off_name.clear(); off_pass.clear();
  FILE* f = H && H->profile_path ? fopen(H->profile_path, "rb") : 0;
  if (!f) return;
  char a[64] = "", b[64] = "";
  if (fgets(a, sizeof a, f) && !fgets(b, sizeof b, f)) b[0] = 0;
  fclose(f);
  for (char* p : {a, b}) { size_t n = strlen(p); while (n && (p[n - 1] == '\n' || p[n - 1] == '\r')) p[--n] = 0; }
  off_name = a; off_pass = b;
}
static void save_officer(const std::string& n, const std::string& p) {
  if (!H || !H->profile_path) return;
  std::string text = n + "\n" + p + "\n";
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
  return has(r0, "LOW PWR") && has(r1, "NAV") && has(r1, "TACTICAL") && has(r1, "VEL") &&
         has(r1, "DAMAGE") && has(r2, "LRS") && has(r9, "DATA") && has(r14, "ss") && has(r17, "ENERGY");
}
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

// ------------------------------------------------------------------ widgets
static void cells(Surf& s, const TextScreen& ts, int c0, int r0, int c1, int r1, const Font& f,
                  int x, int y, int cw, int lh) {
  for (int r = r0; r <= r1; r++)
    for (int c = c0; c <= c1; c++) {
      uint8_t a = ts.at[r][c];
      uint16_t bg = VGA[(a >> 4) & 15];
      uint16_t fg = ((a >> 4) & 15) == (a & 15) ? VGA[a & 15] : READ_FG[a & 15];
      draw_cell(s, x + (c - c0) * cw, y + (r - r0) * lh, cw, lh, ts.ch[r][c], fg, bg, f);
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
  b.font = f; b.act = a; b.on_top = false; b.off = false;
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
  // label: pick the biggest font that fits, never cut off
  const Font* f = b.font;
  if (text_width(b.label.c_str(), *f) > b.x1 - b.x0 - 4 && f != &F6x12) f = &F6x12;
  std::string lab = b.label;
  if (text_width(lab.c_str(), *f) > b.x1 - b.x0 - 4) {
    fit_problem("button \"" + lab + "\"");
    while (!lab.empty() && text_width(lab.c_str(), *f) > b.x1 - b.x0 - 4) lab.erase(lab.size() - 1);
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
  for (auto& m : items) if (m.label.size() > 12) dividers = 0;   // long words are prose, not a menu
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
    if (h.size() >= 2) { items = h; break; }
  }
  return items;
}
static bool window_hints(const TextScreen& ts) {     // a window with its own keys is open
  for (int r = 23; r >= 1; r--) { std::vector<MenuItem> h; parse_hints(row_str(ts, r, 0, 80), h, 0); if (h.size() >= 2) return true; }
  return false;
}

static bool cursor_question(const TextScreen& ts);
// Prompt strip (y 180..194): the game's question, a toast, or the line being typed; scrolls when long.
static void prompt_strip(Surf& s, const TextScreen& ts, const std::string& prompt) {
  s.fill(0, 180, BOT_W, 194, PROMPT_BG);
  if (busy_now) {                                // moving bar: the game is still working on the press
    draw_text(s, 4, 181, "Working...", F6x12, LOUPE);
    int x0 = 74, x1 = BOT_W - 6, w = 40, span = x1 - x0 - w;
    int pos = (int)((now() / 12) % (uint32_t)(2 * span)); if (pos > span) pos = 2 * span - pos;
    s.fill(x0, 185, x1, 189, BTN);
    s.fill(x0 + pos, 184, x0 + pos + w, 190, LOUPE);
    marquee = false;
    return;
  }
  bool showing_toast = !toast.empty() && (int32_t)(toast_until - now()) > 0;
  std::string line = showing_toast ? toast : prompt;
  if (!showing_toast && kb_open && ts.cursor_y >= 0 && ts.cursor_y < 25) line = trim(row_str(ts, ts.cursor_y, 0, 80));
  uint16_t pc = showing_toast ? LOUPE : READ_FG[ts.at[24][0] & 15 ? ts.at[24][0] & 15 : 14];
  int lw = text_width(line.c_str(), F6x12);
  if (lw <= BOT_W - 8) {
    draw_text(s, 4, 181, line.c_str(), F6x12, pc);
    marquee = false;
  } else {
    // scroll: pause at the start, glide to the end, pause, repeat
    int travel = lw - (BOT_W - 8), pause_ms = 1200, glide_ms = travel * 25;
    int cycle = (int)(now() % (uint32_t)(2 * pause_ms + glide_ms)), off;
    off = cycle < pause_ms ? 0 : cycle < pause_ms + glide_ms ? (cycle - pause_ms) / 25 : travel;
    Surf clip = s; (void)clip;
    for (size_t k = 0; k < line.size(); k++) {
      int cx = 4 - off + (int)k * 6;
      if (cx < 0 || cx + 6 > BOT_W - 4) continue;   // whole characters only
      char one[2] = {line[k], 0};
      draw_text(s, cx, 181, one, F6x12, pc);
    }
    marquee = true;
  }
}
static void bottom_rows(Surf& s, const TextScreen& ts) {
  // prompt strip (y 180..194), menu row (194..217), views row (217..240)
  std::string prompt;
  std::vector<MenuItem> items = parse_menu(ts, prompt);
  prompt_strip(s, ts, prompt);

  bool yesno = (prompt.find('?') != std::string::npos && items.empty()) || cursor_question(ts);
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
  bool anykey = items.empty() && !yesno &&
                (prompt.find("PRESS") != std::string::npos || screen_has(ts, "PRESS ANY KEY", 18, 24) || screen_has(ts, "PRESS KEY", 18, 24));
  if (!items.empty()) {
    int n = (int)items.size();
    for (int i = 0; i < n; i++) {
      int a, b; split(BOT_W, n, i, a, b);
      Key k = items[i].key == 27 ? K_ESC : items[i].key == 13 ? K_ENTER : key_for_char(items[i].key);
      add_btn(a, 194, b, 217, items[i].label, &F8x16, key_act(k, items[i].mods), items[i].hot, items[i].sel);
    }
  } else if (yesno) {
    add_btn(0, 194, 160, 217, "YES", &F8x16, key_act(K_Y), 0);
    add_btn(160, 194, 320, 217, "NO", &F8x16, key_act(K_N), 0);
  } else if (kb_open && at_name_prompt(ts) && !off_name.empty() && autologin == 0) {
    std::string up = off_name;
    for (auto& ch2 : up) if (ch2 >= 'a' && ch2 <= 'z') ch2 -= 32;
    std::string lab = "SIGN ON AS " + up;
    if (text_width(lab.c_str(), F8x16) > 213 - 4) lab = "SIGN ON";
    add_btn(0, 194, 213, 217, lab, &F8x16, kind_act(A_SIGNON), -1, true);
    add_btn(213, 194, 320, 217, "HIDE KEYS", &F8x16, kind_act(A_KB));
  } else {
    add_btn(0, 194, 107, 217, anykey ? "CONTINUE" : "ENTER", &F8x16, key_act(K_ENTER));
    add_btn(107, 194, 213, 217, "BACK", &F8x16, key_act(K_ESC));
    add_btn(213, 194, 320, 217, kb_open ? "HIDE KEYS" : "KEYBOARD", &F8x16, kind_act(A_KB));
  }
  static const char* views[8] = {"STARS", "SYSTEM", "WAR", "COMBAT", "FLEET", "ORDERS", "LOG", "MANUAL"};
  // The views only exist during a mission (the status lights are on the top row then).
  bool in_mission = row_str(ts, 0, 0, 80).find("LOW PWR") != std::string::npos;
  for (int i = 0; i < 8; i++) {
    int a, b; split(BOT_W, 8, i, a, b);
    add_btn(a, 217, b, 240, views[i], &F6x12, i == 7 ? kind_act(A_MANUAL) : kind_act(A_VIEW, i));
    if (i < 7 && !in_mission) btns.back().off = true;
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
         screen_has(ts, "Enter new password") || screen_has(ts, "NOTEPAD") || screen_has(ts, "Notepad");
}
static void keyboard(Surf& s) {
  static const char* rows[4] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL-", "ZXCVBNM,./"};
  s.fill(0, 0, BOT_W, 180, BG);
  for (int r = 0; r < 4; r++)
    for (int k = 0; k < 10; k++) {
      int a, b; split(BOT_W, 10, k, a, b);
      std::string lab(1, rows[r][k]);
      Act ka = key_act(key_for_char(rows[r][k])); ka.kind = A_TYPE; ka.arg = rows[r][k];
      add_btn(a, r * 36, b, r * 36 + 36, lab, &F8x16, ka);
    }
  add_btn(0, 144, 64, 180, "ESC", &F8x16, key_act(K_ESC));
  Act sp = key_act(K_SPACE); sp.kind = A_TYPE; sp.arg = ' ';
  Act del = key_act(K_BACKSPACE); del.kind = A_TYPE; del.arg = 8;
  Act ent = key_act(K_ENTER); ent.kind = A_TYPE; ent.arg = 13;
  add_btn(64, 144, 192, 180, "SPACE", &F8x16, sp);
  add_btn(192, 144, 256, 180, "DEL", &F8x16, del);
  add_btn(256, 144, 320, 180, "ENTER", &F8x16, ent);
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
  Box in = tile(s, 0, 12, 238, 138, "COMMS");
  cells_in(s, ts, 1, 15, 39, 22, F6x12, in.x0, in.y0, in.x1, in.y1);

  // energy + mission time
  in = tile(s, 0, 138, 138, 240, "ENERGY");
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

  in = tile(s, 138, 138, 238, 240, "LRS");
  cells_in(s, ts, 2, 3, 16, 9, F6x12, in.x0, in.y0, in.x1, in.y1);
  in = tile(s, 238, 12, 356, 108, "DAMAGE");
  cells_in(s, ts, 60, 2, 77, 8, F6x12, in.x0, in.y0, in.x1, in.y1);
  in = tile(s, 238, 108, 356, 204, "DATA");
  cells_in(s, ts, 60, 10, 77, 16, F6x12, in.x0, in.y0, in.x1, in.y1);
  in = tile(s, 238, 204, 356, 240, 0);
  cells_in(s, ts, 42, 14, 50, 16, F6x12, in.x0, in.y0, in.x1, in.y1, 10);
  in = tile(s, 356, 12, 400, 204, "VEL");
  cells_in(s, ts, 53, 2, 58, 16, F6x12, in.x0, in.y0, in.x1, in.y1);
  in = tile(s, 356, 204, 400, 240, 0);
  cells_in(s, ts, 1, 1, 3, 1, F6x12, in.x0, in.y0, in.x1, in.y0 + 16);
  cells_in(s, ts, 7, 1, 10, 1, F6x12, in.x0, in.y0 + 16, in.x1, in.y1);
}

// tactical geometry on the touch screen: cells 28..49 x 2..13 at 8x14 inside tile (0,0)-(180,180)
static const int TAC_X = 2, TAC_Y = 10, TAC_CW = 8, TAC_LH = 14;

static void bridge_bottom(Surf& s, const TextScreen& ts) {
  std::string t1 = row_str(ts, 1, 29, 50), title;
  for (char c : t1) { if ((uint8_t)c == 0xCD) c = ' '; title += c; }
  // collapse spaces
  std::string tt; bool sp = false;
  for (char c : trim(title)) { if (c == ' ') { if (!sp) tt += c; sp = true; } else { tt += c; sp = false; } }
  std::string port = trim(row_str(ts, 7, 26, 29)), stbd = trim(row_str(ts, 7, 51, 53));
  std::string full = port + "\xf8 " + tt + " " + stbd + "\xf8";
  Box in = tile(s, 0, 0, 180, 180, full.c_str());
  TextScreen t2 = ts;
  t2.ch[7][28] = ' '; t2.at[7][28] = 0;     // last digit of the port heading, not an axis label
  if (22 * TAC_CW > in.x1 - in.x0 || 12 * TAC_LH > in.y1 - in.y0) fit_problem("tactical display is bigger than its tile");
  cells(s, t2, 28, 2, 49, 13, F8x14, in.x0, in.y0, TAC_CW, TAC_LH);
  int tc, tr;
  (void)tc; (void)tr;
  in = tile(s, 180, 0, 236, 156, "NAV");
  cells_in(s, ts, 18, 2, 25, 13, F6x12, in.x0, in.y0, in.x1, in.y1);
  add_btn(180, 156, 236, 180, TAP_LABEL[tap_mode], &F8x16, kind_act(A_MODE), 0, true);
  static const char* acts[7] = {"PHASERS", "TORPS", "SHLD +", "SHLD -", "SPEED +", "SPEED -", "FREEZE"};
  static const Key akeys[7] = {K_INSERT, K_DELETE, (Key)(K_F1 + 7), (Key)(K_F1 + 6), (Key)(K_F1 + 3), (Key)(K_F1 + 2), K_F};
  static const uint8_t amods[7] = {0, 0, 0, 0, 0, 0, M_CTRL};
  for (int i = 0; i < 7; i++) {
    int a, b; split(180, 7, i, a, b);
    add_btn(236, a, 320, b, acts[i], &F8x16, key_act(akeys[i], amods[i]));
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
static void steer_to(const TextScreen& ts, int px, int py) {
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
static const int LP_COLS = 53, LP_ROWS = 15;
static void clamp_loupe() {
  if (lx < 0) lx = 0;
  if (lx > 80 - LP_COLS) lx = 80 - LP_COLS;
  if (ly < 0) ly = 0;
  if (ly > 25 - LP_ROWS) ly = 25 - LP_ROWS;
}
// ------------------------------------------------------------------ other displays
// Every screen apart from the bridge is split by its content, so nothing appears on both
// screens: a menu becomes big buttons on the touch screen, a map goes on the touch screen to
// be tapped, and the rest of the game's screen stays on top.
enum Kind { G_PLAIN, G_MENU, G_MAP };
struct GLine { int row, col, end; char key; std::string label; bool sel; };
struct GLayout {
  Kind kind;
  int r0, c0, r1, c1;                 // the part of the game screen shown on the touch screen
  std::vector<GLine> items;           // menu items (G_MENU)
  int ox, oy, cw, lh; const Font* f;  // where the map is drawn (G_MAP)
};
static GLayout glay;

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
      if (ts.ch[r][c + 1] != ' ' || ts.ch[r][c + 2] != ' ' || !is_alpha(ts.ch[r][c + 3])) continue;
      int e = c + 3;
      while (e + 1 < 80 && !is_box(ts.ch[r][e + 1]) && !(ts.ch[r][e + 1] == ' ' && (e + 2 >= 80 || ts.ch[r][e + 2] == ' '))) e++;
      std::string lab = trim(row_str(ts, r, c + 3, e + 1));
      if (lab.size() < 3) continue;
      GLine l; l.row = r; l.col = c; l.end = e; l.key = (char)k; l.label = lab;
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
  int cw = std::min(8, BOT_W / cols), lh = std::min(14, 180 / rows);
  if (cw < 4 || lh < 8) return false;                    // too big for the touch screen
  g.f = (cw >= 8 && lh >= 14) ? &F8x14 : (cw >= 6 && lh >= 12) ? &F6x12 : &F5x9;
  g.cw = cw; g.lh = lh;                                  // (5x9 in 4-px cells: its spacing column is dropped)
  g.r0 = r0; g.r1 = r1; g.c0 = c0; g.c1 = c1;
  g.ox = (BOT_W - cols * cw) / 2; g.oy = (180 - rows * lh) / 2;
  g.kind = G_MAP;
  return true;
}
static bool find_map(const TextScreen& ts, GLayout& g) {
  int best = 0, br0 = 0, bc0 = 0, br1 = 0, bc1 = 0;
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
      int w = c1 - c - 1, h = r1 - r - 1;
      if (w < 24 || h < 6) continue;
      int used = 0, letters = 0, symbols = 0;
      for (int rr = r + 1; rr < r1; rr++)
        for (int cc = c + 1; cc < c1; cc++) {
          uint8_t ch = ts.ch[rr][cc];
          if (ch > ' ' && ch != 0xFF && !is_box(ch)) { used++; if (ch != 0xFA && ch != 0xF9) symbols++; }
          if (is_alpha(ch)) letters++;
        }
      // a map is sparse, has something on it, and is not a page of words
      if (symbols < 2 || used * 100 > w * h * 45 || letters * 100 > w * h * 12) continue;
      if (w * h > best) { best = w * h; br0 = r; bc0 = c; br1 = r1; bc1 = c1; }
    }
  if (best) {
    // take in the axis labels just outside the frame
    int ec0 = bc0, ec1 = bc1;
    for (int k = 0; k < 3 && ec0 > 0; k++) { bool any = false; for (int rr = br0 + 1; rr < br1; rr++) if (ts.ch[rr][ec0 - 1] > ' ') any = true; if (!any) break; ec0--; }
    for (int k = 0; k < 3 && ec1 < 79; k++) { bool any = false; for (int rr = br0 + 1; rr < br1; rr++) if (ts.ch[rr][ec1 + 1] > ' ') any = true; if (!any) break; ec1++; }
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

static void analyse(const TextScreen& ts) {
  glay.kind = G_PLAIN; glay.items.clear(); glay.r0 = glay.c0 = 0; glay.r1 = glay.c1 = -1;
  if (kb_open) return;
  if (find_map(ts, glay)) return;
  if (find_menu(ts, glay)) return;
  glay.kind = G_PLAIN; glay.r1 = glay.c1 = -1;
}

static void generic_top(Surf& s, const TextScreen& ts) {
  s.fill(0, 0, TOP_W, TOP_H, BG);
  cells(s, ts, 0, 0, 79, 24, F5x9, 0, 0, 5, 9);
  const char* hint = kb_open ? "Type on the touch screen keyboard."
                   : glay.kind == G_MENU ? "Tap a menu item on the touch screen."
                   : glay.kind == G_MAP ? "The map is on the touch screen: tap a star or ship to aim."
                   : "Touch screen: keys for this screen.";
  if (glay.kind != G_PLAIN) {        // the part on the touch screen is not repeated here
    int x0 = glay.c0 * 5, y0 = glay.r0 * 9, x1 = (glay.c1 + 1) * 5, y1 = (glay.r1 + 1) * 9;
    s.fill(x0, y0, x1, y1, BG);
    s.rect(x0, y0, x1, y1, BTN_EDGE);
    const char* note = glay.kind == G_MENU ? "Menu on the touch screen" : "Map on the touch screen";
    const Font* nf = text_width(note, F6x12) <= x1 - x0 - 4 ? &F6x12 : &F5x9;
    int tw = text_width(note, *nf);
    if (tw <= x1 - x0 - 4 && y1 - y0 >= nf->h + 2)
      draw_text(s, x0 + (x1 - x0 - tw) / 2, y0 + (y1 - y0 - nf->h) / 2, note, *nf, HINT);
  }
  s.fill(0, 225, TOP_W, TOP_H, BG);
  fit_text(s, 4, 226, TOP_W - 2, hint, F6x12, HINT, "top hint");
}
static uint32_t user_pan_ms = 0;
static uint32_t screen_sig(const TextScreen& ts) {   // changes when a different screen appears
  uint32_t h = 2166136261u;
  for (int r = 0; r < 24; r++) for (int c = 0; c < 80; c++) { h ^= ts.ch[r][c]; h *= 16777619u; }
  return h;
}
static void frame_content(const TextScreen& ts) {
  int r0 = 25, r1 = -1, c0 = 80, c1 = -1;
  for (int r = 0; r < 25; r++)
    for (int c = 0; c < 80; c++)
      if (ts.ch[r][c] > ' ' && ts.ch[r][c] != 0xFF) {
        if (r < r0) r0 = r;
        if (r > r1) r1 = r;
        if (c < c0) c0 = c;
        if (c > c1) c1 = c;
      }
  if (r1 < 0) return;
  ly = (r1 - r0 + 1 <= LP_ROWS) ? r0 - (LP_ROWS - (r1 - r0 + 1)) / 2 : r0;
  lx = (c1 - c0 + 1 <= LP_COLS) ? c0 - (LP_COLS - (c1 - c0 + 1)) / 2 : c0;
}
// Aim at the cells that changed since the previous screen (a dialog that popped up, a panel
// that was replaced); fall back to framing the whole content when most of the screen changed.
static void mark_base() { if (!base_ok || queue.empty()) { base_ts = last; base_ok = true; } }
static bool frame_changes(const TextScreen& ts, const TextScreen& before) {
  int r0 = 25, r1 = -1, c0 = 80, c1 = -1, n = 0;
  for (int r = 0; r < 24; r++)                 // row 24 is the prompt line, shown separately
    for (int c = 0; c < 80; c++)
      if (ts.ch[r][c] != before.ch[r][c] || ts.at[r][c] != before.at[r][c]) {
        n++;
        if (r < r0) r0 = r;
        if (r > r1) r1 = r;
        if (c < c0) c0 = c;
        if (c > c1) c1 = c;
      }
  if (n == 0 || n > 80 * 24 * 6 / 10) return false;   // nothing, or a whole new screen
  int h = r1 - r0 + 1, w = c1 - c0 + 1;
  ly = h <= LP_ROWS ? r0 - (LP_ROWS - h) / 2 : r0;
  lx = w <= LP_COLS ? c0 - (LP_COLS - w) / 2 : c0;
  return true;
}
static void generic_bottom(Surf& s, const TextScreen& ts) {
  s.fill(0, 0, BOT_W, 180, BG);
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
    for (int i = 0; i < n; i++) {
      int col = i / rows, row = i % rows;
      int x0, x1, y0, y1;
      split(BOT_W, cols, col, x0, x1);
      split(180, rows, row, y0, y1);
      const GLine& l = glay.items[i];
      int hot = -1;
      for (size_t k = 0; k < l.label.size(); k++)
        if (toupper((unsigned char)l.label[k]) == l.key) { hot = (int)k; break; }
      std::string lab = l.label;
      if (hot < 0) { lab = std::string(1, l.key) + " " + l.label; hot = 0; }
      add_btn(x0, y0, x1, y1, lab, small ? &F6x12 : &F8x16, key_act(key_for_char(l.key)), hot, l.sel);
    }
  } else if (glay.kind == G_MAP) {
    cells(s, ts, glay.c0, glay.r0, glay.c1, glay.r1, *glay.f, glay.ox, glay.oy, glay.cw, glay.lh);
  } else {
    // keys for screens that are read or stepped through: arrows, page keys, space
    static const char* labs[9] = {"PG UP", "\x18", "PG DN", "\x1b", "SPACE", "\x1a", "HOME", "\x19", "END"};
    static const Key keys[9] = {K_PGUP, K_UP, K_PGDN, K_LEFT, K_SPACE, K_RIGHT, K_NONE, K_DOWN, K_NONE};
    for (int i = 0; i < 9; i++) {
      if (keys[i] == K_NONE) continue;
      int x0, x1, y0, y1;
      split(BOT_W, 3, i % 3, x0, x1); split(180, 3, i / 3, y0, y1);
      add_btn(x0, y0, x1, y1, labs[i], &F8x16, key_act(keys[i]));
    }
  }
}

// ------------------------------------------------------------------ manual
static void load_manual() {
  manual.clear();
  FILE* f = H && H->manual_path ? fopen(H->manual_path, "rb") : 0;
  Section cur; cur.title = "About";
  if (!f) {
    cur.lines.push_back("The manual file was not found.");
    cur.lines.push_back(H && H->manual_path ? H->manual_path : "");
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
    // word-wrap to 63 columns (leaves room for the margin and scroll bar)
    std::string line = buf;
    do {
      if ((int)line.size() <= 63) { cur.lines.push_back(line); break; }
      size_t cut = line.rfind(' ', 63); if (cut == std::string::npos || cut == 0) cut = 63;
      cur.lines.push_back(line.substr(0, cut));
      line = line.substr(cut + (line[cut] == ' ' ? 1 : 0));
    } while (!line.empty());
  }
  fclose(f);
  manual.push_back(cur);
}
static const int MAN_LINES = 18;   // text lines on the top screen
static void manual_top(Surf& s) {
  s.fill(0, 0, TOP_W, TOP_H, BG);
  const Section& sec = manual[man_sel];
  Box in = tile(s, 0, 0, TOP_W, TOP_H, sec.title.c_str());
  int maxs = (int)sec.lines.size() - MAN_LINES; if (maxs < 0) maxs = 0;
  if (man_scroll > maxs) man_scroll = maxs;
  if (man_scroll < 0) man_scroll = 0;
  for (int i = 0; i < MAN_LINES && man_scroll + i < (int)sec.lines.size(); i++)
    fit_text(s, in.x0 + 3, in.y0 + 1 + i * 12, in.x1 - 4, sec.lines[man_scroll + i].c_str(), F6x12, BTN_TXT, "manual text");
  // scroll bar
  if (maxs > 0) {
    int track = in.y1 - in.y0, th = track * MAN_LINES / (int)sec.lines.size();
    if (th < 8) th = 8;
    int ty = in.y0 + (track - th) * man_scroll / maxs;
    s.fill(in.x1 - 3, in.y0, in.x1 - 1, in.y1, rgb(40, 44, 60));
    s.fill(in.x1 - 3, ty, in.x1 - 1, ty + th, BTN_EDGE);
  }
}
static void manual_bottom(Surf& s) {
  s.fill(0, 0, BOT_W, BOT_H, BG);
  const int rows = 7;  // section buttons visible (each 24px); the Left hand button follows
  if (man_sel < man_list_top) man_list_top = man_sel;
  if (man_sel >= man_list_top + rows) man_list_top = man_sel - rows + 1;
  for (int i = 0; i < rows && man_list_top + i < (int)manual.size(); i++) {
    int k = man_list_top + i;
    add_btn(0, i * 24, BOT_W, i * 24 + 24, manual[k].title, &F8x16, kind_act(A_SECTION, k), -1, k == man_sel);
  }
  if (!lefty_loaded) lefty_load();
  add_btn(0, 168, BOT_W, 192, lefty_on ? "LEFT HAND: ON" : "LEFT HAND: OFF", &F8x16, kind_act(A_LEFTY), -1, lefty_on);
  s.fill(0, 192, BOT_W, 205, PROMPT_BG);
  fit_text(s, 4, 193, BOT_W - 4, "Sticks scroll \xb3 D-Pad, L/R sections \xb3 B close", F6x12, HINT, "manual hint");
  add_btn(0, 205, BOT_W, 240, "CLOSE MANUAL", &F8x16, kind_act(A_CLOSE));
}

// ------------------------------------------------------------------ graphics (title art) screens
static void graphics_bottom(Surf& s) {
  s.fill(0, 0, BOT_W, BOT_H, BG);
  fit_text(s, (BOT_W - text_width("STAR FLEET II", F8x16)) / 2, 40, BOT_W, "STAR FLEET II", F8x16, TITLE, "title");
  fit_text(s, (BOT_W - text_width("Krellan Commander", F6x12)) / 2, 62, BOT_W, "Krellan Commander", F6x12, BTN_TXT, "title");
  add_btn(40, 100, 280, 150, "CONTINUE", &F8x16, key_act(K_ENTER));
  add_btn(0, 194, 160, 217, "BACK", &F8x16, key_act(K_ESC));
  add_btn(160, 194, 320, 217, "MANUAL", &F8x16, kind_act(A_MANUAL));
}

// ------------------------------------------------------------------ START menu
static void pause_bottom(Surf& s, const TextScreen& ts) {
  s.fill(0, 0, BOT_W, BOT_H, BG);
  if (quit_confirm) {
    fit_text(s, (BOT_W - text_width("Exit Star Fleet II?", F8x16)) / 2, 50, BOT_W, "Exit Star Fleet II?", F8x16, TITLE, "pause");
    const char* l2 = "Anything since your last save is lost.";
    fit_text(s, (BOT_W - text_width(l2, F6x12)) / 2, 80, BOT_W, l2, F6x12, BTN_TXT, "pause");
    add_btn(20, 130, 155, 175, "EXIT GAME", &F8x16, kind_act(A_QUIT_YES));
    add_btn(165, 130, 300, 175, "KEEP PLAYING", &F8x16, kind_act(A_PAUSE));
    return;
  }
  fit_text(s, (BOT_W - text_width("MENU", F8x16)) / 2, 6, BOT_W, "MENU", F8x16, TITLE, "pause");
  bool bridge = ts.text_mode && is_bridge(ts);
  bool menu = ts.text_mode && screen_has(ts, "Change default settings", 0, 24);
  const char* lab[5] = {"RESUME", "MANUAL", "QUICK SAVE", "GAME SETTINGS", "EXIT GAME"};
  Act act[5] = {kind_act(A_PAUSE), kind_act(A_MANUAL), kind_act(A_SAVE), kind_act(A_SETTINGS), kind_act(A_QUIT)};
  for (int i = 0; i < 5; i++) {
    add_btn(40, 28 + i * 36, 280, 60 + i * 36, lab[i], &F8x16, act[i]);
    if ((i == 2 && !bridge) || (i == 3 && !menu) || (i == 4 && !(H && H->quit))) btns.back().off = true;
  }
  const char* note = bridge ? "B or START: back to the game" :
                     menu ? "Quick save works on the bridge." : "Quick save: on the bridge. Settings: main menu.";
  fit_text(s, (BOT_W - text_width(note, F6x12)) / 2, 216, BOT_W, note, F6x12, HINT, "pause");
}

// ------------------------------------------------------------------ public API
void init(const Host* host) {
  H = host;
  next_key_ms = now();
  memset(&last, 0, sizeof last);
  load_manual();
  load_officer();
  dirty();
}

bool modal() { return manual_open; }

static bool lefty_on = false, lefty_loaded = false;
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
    case A_KEY: if (a.key != K_NONE) press(a.key, a.mods); if (a.key == K_ENTER) typed_enter(); busy_since = now(); break;
    case A_VIEW: view_goal = a.arg; view_esc = 0; view_next = now(); busy_since = now(); break;
    case A_TYPE:
      if (last.cursor_y != typed_y) { typed.clear(); typed_y = last.cursor_y; }
      press(a.key, a.mods);
      if (a.arg == 8) { if (!typed.empty()) typed.erase(typed.size() - 1); }
      else if (a.arg == 13) typed_enter();
      else typed += (char)(a.arg >= 'A' && a.arg <= 'Z' ? a.arg + 32 : a.arg);
      break;
    case A_SIGNON:
      typed.clear(); type_text(off_name); press(K_ENTER); autologin = 1; pass_tries = 0; pend_name = off_name; say("Signing on...", 1500);
      break;
    case A_SAVE: press(K_Q, M_CTRL); say("Quick save", 1500); pause_open = false; break;
    case A_PAUSE: pause_open = false; quit_confirm = false; break;
    case A_QUIT: quit_confirm = true; break;
    case A_QUIT_YES: if (H && H->quit) H->quit(); break;
    case A_SETTINGS: pause_open = false; press(K_C); break;
    case A_MODE:
      tap_mode = (TapMode)((tap_mode + 1) % 3);
      say(tap_mode == TM_AUTO ? "Taps target when the target box is up, else steer" :
          tap_mode == TM_STEER ? "Taps always steer" : "Taps always move the target box");
      break;
    case A_MANUAL: manual_open = true; man_scroll = 0; pause_open = false; break;
    case A_KB:
      if (kb_open) { kb_open = false; kb_user_hidden = true; kb_forced = false; }
      else { kb_open = true; kb_forced = true; kb_user_hidden = false; }
      break;
    case A_SECTION: man_sel = a.arg; man_scroll = 0; break;
    case A_CLOSE: manual_open = false; break;
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
  pump_keys();
  uint32_t t = now();
  Redraw rd = {false, false, 0, BOT_H};

  // keyboard opens by itself when the game asks for typed text
  if (ts.text_mode && !manual_open) {
    bool want = wants_text(ts);
    if (want && !kb_open && !kb_user_hidden) { kb_open = true; dirty(); }
    if (!want) { kb_user_hidden = false; if (kb_open && !kb_forced) { kb_open = false; dirty(); } }
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
  if (still && (autologin == 1 || (autologin == 2 && t - pass_sent >= 2500 && pass_tries < 3))) {
    if (autologin == 2) for (int i = 0; i < 12; i++) press(K_BACKSPACE);   // clear a half-taken first try
    type_text(off_pass); press(K_ENTER); autologin = 2; pass_sent = t; pass_tries++; typed.clear();
    pend_pass = off_pass;
  }
  if (autologin && ts.text_mode && (screen_has(ts, "SIGN-ON OPTIONS") || (autologin == 2 && pass_tries >= 3 && t - pass_sent > 5000)))
    autologin = 0;
  // a successful sign-on with a newly typed officer: remember it
  if (ts.text_mode && screen_has(ts, "SIGN-ON OPTIONS") && !pend_name.empty() && !pend_pass.empty()) {
    if (pend_name != off_name || pend_pass != off_pass) { save_officer(pend_name, pend_pass); say("Officer remembered for one-tap sign-on"); }
    pend_name.clear(); pend_pass.clear();
  }

  // drive the target box toward a tapped cell, one arrow at a time
  if (tdb_drive && queue.empty() && (int32_t)(t - tdb_next) >= 0) {
    int c, r;
    if (!find_tdb(ts, c, r) || ++tdb_steps > 80) {
      tdb_drive = false;
    } else {
      int dc = tdb_tc - c, dr = tdb_tr - r, dist = abs(dc) + abs(dr) * 2;
      bool stuck = dist >= tdb_best && tdb_steps > 1;     // the last press did not bring it closer
      if (dist < tdb_best) tdb_best = dist;
      if (dist == 0 || stuck) {
        tdb_drive = false;
        if (tdb_enter_after && dist <= 1) press(K_ENTER);
      } else if (dr) {
        press(dr > 0 ? K_DOWN : K_UP);
      } else {
        press(dc > 0 ? K_RIGHT : K_LEFT);
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

  bool changed = memcmp(&ts, &last, sizeof ts) != 0;
  // Busy feedback: after a press, if the game hasn't changed the screen within 0.35 s, the prompt
  // strip shows a moving "Working" bar until it does (or 60 s pass), so a slow step never looks dead.
  if (busy_since && changed && view_goal < 0) busy_since = 0;
  if (busy_since && (int32_t)(t - busy_since) > 60000) busy_since = 0;
  bool was_busy = busy_now;
  busy_now = busy_since && (int32_t)(t - busy_since) > 350;
  if (was_busy && !busy_now) dirty();
  static uint32_t last_anim = 0;
  // (-50: keep redrawing a little past the end so the button's released look gets drawn)
  if (flash_on && (int32_t)(flash_until - t) <= -50) flash_on = false;
  bool toast_anim = !toast.empty() && (int32_t)(toast_until - t) > -50;
  bool scroll_due = (marquee || busy_now) && t - last_anim >= 50;
  bool full = changed || drawn_version != ui_version;
  if (!full && !flash_on && !toast_anim && !scroll_due) return rd;
  if (flash_on || toast_anim || scroll_due) last_anim = t;

  Surf ts_top = {top, TOP_W, TOP_H}, ts_bot = {bottom, BOT_W, BOT_H};
  // Only the scrolling prompt moved: redraw just that strip (keeps idle screens cheap).
  if (!full && !flash_on && !toast_anim && ts.text_mode && !manual_open && !pause_open) {
    std::string prompt; parse_menu(ts, prompt);
    prompt_strip(ts_bot, ts, prompt);
    rd.bottom = true; rd.by0 = 180; rd.by1 = 194;
    return rd;
  }
  bool draw_top = full;      // a button flash or toast only touches the touch screen
  btns.clear();
  if (manual_open) {
    if (draw_top) manual_top(ts_top);
    manual_bottom(ts_bot); rd.top = draw_top; rd.bottom = true;
  } else if (pause_open) {
    pause_bottom(ts_bot, ts); rd.bottom = true;
  } else if (!ts.text_mode) {
    graphics_bottom(ts_bot); rd.bottom = true;
  } else if (is_bridge(ts)) {
    if (draw_top) bridge_top(ts_top, ts);
    if (kb_open) keyboard(ts_bot); else bridge_bottom(ts_bot, ts);
    bottom_rows(ts_bot, ts);
    rd.top = draw_top; rd.bottom = true;
  } else {
    analyse(ts);
    if (kb_open) keyboard(ts_bot); else generic_bottom(ts_bot, ts);
    if (draw_top) generic_top(ts_top, ts);
    bottom_rows(ts_bot, ts);
    rd.top = draw_top; rd.bottom = true;
  }
  for (int i = 0; i < (int)btns.size(); i++) draw_btn(ts_bot, i);
  memcpy(&last, &ts, sizeof ts);
  last_text_mode = ts.text_mode;
  drawn_version = ui_version;
  return rd;
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
  col = bc; row = br;
}
static void start_drive(int col, int row, int c, int r) {
  bool same = abs(col - c) <= 1 && row == r;           // a second tap on the box locks it in
  tdb_drive = true; tdb_tc = col; tdb_tr = row; tdb_steps = 0; tdb_best = 9999;
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
  int c, r;
  if (!find_tdb(last, c, r)) { say("No target box: choose a targeting option first"); return; }
  snap_to_object(last, col, row, glay.c0, glay.r0, glay.c1, glay.r1);
  start_drive(col, row, c, r);
}

void touch(int x, int y, bool down) {
  if (down && !touch_was_down) {
    tap_kind = 0;
    int i = hit(x, y);
    if (i >= 0) {
      pressed_id = id_of(btns[i]);
      if (H && H->click) H->click();
      dirty();
    } else if (!manual_open && !pause_open && !kb_open && last.text_mode) {
      if (is_bridge(last) && x < 180 && y < 180) { tap_kind = 1; tap_x = x; tap_y = y; }
      else if (!is_bridge(last) && y < 180 && glay.kind == G_MAP) { tap_kind = 2; tap_x = x; tap_y = y; }
    }
  } else if (down && touch_was_down) {

  } else if (!down && touch_was_down) {
    if (pressed_id.valid) {
      BtnId p = pressed_id; pressed_id.valid = false;
      int i = -1;
      for (int k = 0; k < (int)btns.size(); k++) if (same(p, btns[k])) i = k;
      if (i >= 0 && x >= p.x0 - 8 && x < p.x1 + 8 && y >= p.y0 - 8 && y < p.y1 + 8) {
        flash_id = p; flash_until = now() + 110; flash_on = true;
        fire(btns[i].act);
      }
      dirty();
    } else if (tap_kind == 1) {
      tactical_tap(tap_x, tap_y);
    } else if (tap_kind == 2) {
      map_tap(tap_x, tap_y);
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
    if (repeat_due(B_DDOWN, held, pr) || (pr & B_R)) { man_sel = (man_sel + 1) % n; man_scroll = 0; dirty(); }
    if (repeat_due(B_DUP, held, pr) || (pr & B_L)) { man_sel = (man_sel + n - 1) % n; man_scroll = 0; dirty(); }
    if (pr & (B_B | B_START)) { manual_open = false; dirty(); }
    return;
  }
  if (pause_open) {
    if (pr & (B_B | B_START)) { if (quit_confirm) quit_confirm = false; else pause_open = false; dirty(); }
    return;
  }
  if (pr & B_START) { pause_open = true; quit_confirm = false; dirty(); return; }
  if (pr & B_SELECT) fire(kind_act(is_bridge(last) ? A_SAVE : A_KB));   // quick save on the bridge
  if (pr & B_A) { press(K_ENTER); typed_enter(); }
  if (pr & B_B) press(K_ESC);
  if (pr & B_X) press(K_INSERT);           // fire phasers
  if (pr & B_Y) press(K_DELETE);           // fire torpedoes
  if (pr & B_L) press(K_F, M_CTRL);        // freeze / unfreeze
  if (pr & B_R) fire(kind_act(A_MODE));    // tap mode: auto / steer / target
  if (repeat_due(B_DUP, held, pr)) press(K_UP);
  if (repeat_due(B_DDOWN, held, pr)) press(K_DOWN);
  if (repeat_due(B_DLEFT, held, pr)) press(K_LEFT);
  if (repeat_due(B_DRIGHT, held, pr)) press(K_RIGHT);
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
