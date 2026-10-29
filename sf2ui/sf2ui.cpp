// sf2ui: Star Fleet II split-cockpit frontend. See sf2ui.h.
#include "sf2ui.h"
#include "draw.h"
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

enum ActKind { A_NONE, A_KEY, A_MODE, A_MANUAL, A_KB, A_SECTION, A_CLOSE, A_TYPE };
struct Act { ActKind kind; Key key; uint8_t mods; int arg; };
enum { M_SHIFT = 1, M_CTRL = 2, M_ALT = 4 };

struct Btn {
  int x0, y0, x1, y1;
  std::string label; int hot; bool sel;
  const Font* font; Act act; bool on_top;
};
static std::vector<Btn> btns;          // buttons drawn on the touch screen this frame
struct BtnId { int x0, y0, x1, y1; std::string label; bool valid; };
static BtnId pressed_id = {0, 0, 0, 0, "", false};   // button under the stylus
static BtnId flash_id = {0, 0, 0, 0, "", false};     // button just released (brief press look)
static uint32_t flash_until = 0;
static BtnId id_of(const Btn& b) { BtnId i = {b.x0, b.y0, b.x1, b.y1, b.label, true}; return i; }
static bool same(const BtnId& i, const Btn& b) { return i.valid && i.x0 == b.x0 && i.y0 == b.y0 && i.x1 == b.x1 && i.y1 == b.y1 && i.label == b.label; }

static bool target_mode = false;       // tactical tap: steer (false) or move target box (true)
static bool manual_open = false, kb_open = false, kb_forced = false, kb_user_hidden = false;
static std::string toast; static uint32_t toast_until = 0;
static bool marquee = false;      // prompt strip is scrolling (needs regular redraws)

// loupe (generic views): top-left cell of the magnified window
static int lx = 13, ly = 5; static bool follow = true;
static bool dragging = false; static int drag_x, drag_y, drag_lx, drag_ly, drag_moved;
static int tap_x = -1, tap_y = -1;

// target box driver (closed loop: press an arrow, look where the box went, repeat)
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
  std::string r0 = row_str(ts, 0, 0, 80), r1 = row_str(ts, 1, 0, 80);
  return r0.find("LOW PWR") != std::string::npos && r1.find("NAV") != std::string::npos &&
         r1.find("TACTICAL") != std::string::npos && r1.find("DAMAGE") != std::string::npos;
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
    s.fill(tx - 2, y0, tx + tw + 2, y0 + 10, BG);
    title_text(s, tx, y0, title, TITLE);
    return Box{x0 + 2, y0 + 10, x1 - 2, y1 - 2};
  }
  return Box{x0 + 2, y0 + 2, x1 - 2, y1 - 2};
}

static void add_btn(int x0, int y0, int x1, int y1, const std::string& label, const Font* f, Act a,
                    int hot = -1, bool sel = false) {
  Btn b; b.x0 = x0; b.y0 = y0; b.x1 = x1; b.y1 = y1; b.label = label; b.hot = hot; b.sel = sel;
  b.font = f; b.act = a; b.on_top = false;
  btns.push_back(b);
}
static Act key_act(Key k, uint8_t mods = 0) { Act a; a.kind = A_KEY; a.key = k; a.mods = mods; a.arg = 0; return a; }
static Act kind_act(ActKind k, int arg = 0) { Act a; a.kind = k; a.key = K_NONE; a.mods = 0; a.arg = arg; return a; }

static void draw_btn(Surf& s, int i) {
  const Btn& b = btns[i];
  bool down = same(pressed_id, b) || (same(flash_id, b) && (int32_t)(flash_until - now()) > 0);
  s.fill(b.x0, b.y0, b.x1, b.y1, BG);
  s.round_rect(b.x0 + 1, b.y0 + 1, b.x1 - 1, b.y1 - 1, down ? BTN_DOWN : (b.sel ? BTN_SEL : BTN), down ? BTN_TXT : BTN_EDGE);
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
    draw_text(s, tx + (int)k * f->w, ty, one, *f, (int)k == b.hot ? HOT : BTN_TXT);
  }
}

static void split(int total, int n, int i, int& a, int& b) {
  int base = total / n, extra = total % n;
  a = i * base + (i < extra ? i : extra);
  b = a + base + (i < extra ? 1 : 0);
}

// ------------------------------------------------------------------ shared bottom rows
struct MenuItem { std::string label; int hot; bool sel; char key; };
static std::vector<MenuItem> parse_menu(const TextScreen& ts, std::string& prompt) {
  std::vector<MenuItem> items;
  int c = 0;
  // prompt: text before the first item separator
  prompt = trim(row_str(ts, 24, 0, 24));
  c = 24;
  while (c < 80) {
    uint8_t a = ts.at[24][c], ch = ts.ch[24][c];
    if (ch != ' ' && ch != 0 && ch != 0xB3 && ch != 0xBA && ch != 0xDB && a != 0x08) {
      int s = c;
      while (c < 80 && ts.ch[24][c] != 0xB3 && ts.ch[24][c] != 0xBA && ts.ch[24][c] != 0xDB && ts.at[24][c] != 0x08) c++;
      std::string w = trim(row_str(ts, 24, s, c));
      if (!w.empty()) {
        MenuItem m; m.label = w; m.hot = 0; m.sel = false; m.key = 0;
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
  if (items.size() < 2 || dividers < 2) {
    items.clear();
    std::string row = row_str(ts, 24, 0, 80);
    prompt = trim(row);
    // key hints in brackets: "[M]: menu  [Esc]: exit"
    size_t p = 0;
    while ((p = row.find('[', p)) != std::string::npos) {
      size_t q = row.find(']', p);
      if (q == std::string::npos || q - p > 6) break;
      std::string key = row.substr(p + 1, q - p - 1), lab;
      size_t t = q + 1;
      while (t < row.size() && (row[t] == ':' || row[t] == ' ')) t++;
      while (t < row.size() && row[t] != '[' && (uint8_t)row[t] >= ' ' && (uint8_t)row[t] < 0x7F &&
             !(row[t] == ' ' && t + 1 < row.size() && row[t + 1] == ' ')) lab += row[t++];
      MenuItem m; m.hot = -1; m.sel = false; m.key = 0;
      if (key == "Esc" || key == "ESC") m.key = 27;
      else if (key == "Enter" || key == "CR") m.key = 13;
      else if (key.size() == 1) m.key = key[0];
      if (m.key) {
        m.label = trim(lab).empty() ? key : trim(lab);
        for (auto& ch2 : m.label) if (ch2 >= 'a' && ch2 <= 'z') ch2 -= 32;
        items.push_back(m);
        if (p == 0 || prompt.find('[') != std::string::npos) prompt = trim(row.substr(0, row.find('[')));
      }
      p = q + 1;
    }
  }
  return items;
}

static void bottom_rows(Surf& s, const TextScreen& ts) {
  // prompt strip (y 180..194), menu row (194..217), views row (217..240)
  std::string prompt;
  std::vector<MenuItem> items = parse_menu(ts, prompt);
  s.fill(0, 180, BOT_W, 194, PROMPT_BG);
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

  bool yesno = prompt.find('?') != std::string::npos && items.empty();
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
      add_btn(a, 194, b, 217, items[i].label, &F8x16, key_act(k), items[i].hot, items[i].sel);
    }
  } else if (yesno) {
    add_btn(0, 194, 160, 217, "YES", &F8x16, key_act(K_Y), 0);
    add_btn(160, 194, 320, 217, "NO", &F8x16, key_act(K_N), 0);
  } else {
    add_btn(0, 194, 107, 217, anykey ? "CONTINUE" : "ENTER", &F8x16, key_act(K_ENTER));
    add_btn(107, 194, 213, 217, "BACK", &F8x16, key_act(K_ESC));
    add_btn(213, 194, 320, 217, kb_open ? "HIDE KEYS" : "KEYBOARD", &F8x16, kind_act(A_KB));
  }
  static const char* views[8] = {"STARS", "SYSTEM", "WAR", "COMBAT", "FLEET", "ORDERS", "LOG", "MANUAL"};
  static const Key vkeys[8] = {K_N, K_S, K_W, K_X, K_F, K_O, K_L, K_NONE};
  static const uint8_t vmods[8] = {M_ALT, M_ALT, M_ALT, M_ALT, M_ALT, M_CTRL, M_CTRL, 0};
  for (int i = 0; i < 8; i++) {
    int a, b; split(BOT_W, 8, i, a, b);
    add_btn(a, 217, b, 240, views[i], &F6x12, i == 7 ? kind_act(A_MANUAL) : key_act(vkeys[i], vmods[i]));
  }
}

// ------------------------------------------------------------------ on-screen keyboard
static bool wants_text(const TextScreen& ts) {
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
      add_btn(a, r * 36, b, r * 36 + 36, lab, &F8x16, key_act(key_for_char(rows[r][k])));
    }
  add_btn(0, 144, 64, 180, "ESC", &F8x16, key_act(K_ESC));
  add_btn(64, 144, 192, 180, "SPACE", &F8x16, key_act(K_SPACE));
  add_btn(192, 144, 256, 180, "DEL", &F8x16, key_act(K_BACKSPACE));
  add_btn(256, 144, 320, 180, "ENTER", &F8x16, key_act(K_ENTER));
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
    draw_text(s, a + (b - a - 1 - text_width(lab.c_str(), F6x12)) / 2, 0, lab.c_str(), F6x12, fg);
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
    draw_text(s, in.x0 + 1, y, t.c_str(), F6x12, (at >> 4) & 15 ? VGA[at & 15] : READ_FG[at & 15]);
  }
  for (int i = 0; i < 4; i++) {
    int r = erow[i], y = in.y0 + i * 15;
    std::string lab = trim(row_str(ts, r, 45, 53)), val = trim(row_str(ts, r, 54, 58));
    while (val.size() < 4) val = " " + val;
    draw_text(s, in.x0 + 23, y, lab.c_str(), F6x12, READ_FG[ts.at[r][45] & 15]);
    draw_text(s, in.x0 + 73, y, val.c_str(), F6x12, READ_FG[ts.at[r][54] & 15]);
    int filled = 0;
    for (int c = 58; c < 78; c++) if (ts.ch[r][c] == 0xDB) filled++;
    int bx0 = in.x0 + 100, bx1 = in.x1 - 1;
    s.rect(bx0, y + 2, bx1 + 1, y + 10, rgb(70, 72, 86));
    if (filled) s.fill(bx0 + 1, y + 3, bx0 + 1 + (bx1 - bx0 - 1) * filled / 20, y + 9, VGA[ts.at[r][58 + filled - 1] & 15]);
  }
  int sep = in.y1 - 27;
  s.hline(in.x0, in.x1, sep, FRAME);
  cells(s, ts, 1, 11, 16, 12, F6x12, in.x0 + (in.x1 - in.x0 - 96) / 2, sep + 2, 6, 12);

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
  cells(s, t2, 28, 2, 49, 13, F8x14, in.x0, in.y0, TAC_CW, TAC_LH);
  int tc, tr;
  if (target_mode && find_tdb(ts, tc, tr, 29, 2, 49, 13)) { /* box already drawn by the game */ }
  in = tile(s, 180, 0, 236, 156, "NAV");
  cells_in(s, ts, 18, 2, 25, 13, F6x12, in.x0, in.y0, in.x1, in.y1);
  add_btn(180, 156, 236, 180, target_mode ? "TARGET" : "STEER", &F8x16, kind_act(A_MODE), 0, true);
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
static void generic_top(Surf& s, const TextScreen& ts) {
  s.fill(0, 0, TOP_W, TOP_H, BG);
  cells(s, ts, 0, 0, 79, 24, F5x9, 0, 0, 5, 9);
  // where the magnifier is looking
  s.rect(lx * 5 - 1, ly * 9 - 1, (lx + LP_COLS) * 5 + 1, (ly + LP_ROWS) * 9 + 1, LOUPE);
  s.fill(0, 225, TOP_W, TOP_H, BG);
  draw_text(s, 4, 226, "Touch screen: magnified view. Drag to look around, tap to aim.", F6x12, HINT);
}
static uint32_t user_pan_ms = 0;
static uint32_t screen_sig(const TextScreen& ts) {   // changes when a different screen appears
  uint32_t h = 2166136261u;
  for (int r = 0; r < 25; r += 6) for (int c = 0; c < 80; c++) { h ^= ts.ch[r][c]; h *= 16777619u; }
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
static void generic_bottom(Surf& s, const TextScreen& ts) {
  int tc, tr;
  static uint32_t sig = 0;
  uint32_t sg = screen_sig(ts);
  if (sg != sig) { sig = sg; if (now() - user_pan_ms > 4000) { follow = true; frame_content(ts); } }
  if (follow && find_tdb(ts, tc, tr)) { lx = tc - LP_COLS / 2; ly = tr - LP_ROWS / 2; }
  clamp_loupe();
  s.fill(0, 0, BOT_W, 180, BG);
  cells(s, ts, lx, ly, lx + LP_COLS - 1, ly + LP_ROWS - 1, F6x12, 1, 0, 6, 12);
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
  char buf[1024];
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
  const int rows = 8;  // section buttons visible (each 24px)
  if (man_sel < man_list_top) man_list_top = man_sel;
  if (man_sel >= man_list_top + rows) man_list_top = man_sel - rows + 1;
  for (int i = 0; i < rows && man_list_top + i < (int)manual.size(); i++) {
    int k = man_list_top + i;
    add_btn(0, i * 24, BOT_W, i * 24 + 24, manual[k].title, &F8x16, kind_act(A_SECTION, k), -1, k == man_sel);
  }
  s.fill(0, 192, BOT_W, 205, PROMPT_BG);
  fit_text(s, 4, 193, BOT_W - 4, "Sticks scroll \xb3 D-Pad, L/R sections \xb3 B close", F6x12, HINT, "manual hint");
  add_btn(0, 205, BOT_W, 240, "CLOSE MANUAL", &F8x16, kind_act(A_CLOSE));
}

// ------------------------------------------------------------------ graphics (title art) screens
static void graphics_bottom(Surf& s) {
  s.fill(0, 0, BOT_W, BOT_H, BG);
  draw_text(s, (BOT_W - text_width("STAR FLEET II", F8x16)) / 2, 40, "STAR FLEET II", F8x16, TITLE);
  draw_text(s, (BOT_W - text_width("Krellan Commander", F6x12)) / 2, 62, "Krellan Commander", F6x12, BTN_TXT);
  add_btn(40, 100, 280, 150, "CONTINUE", &F8x16, key_act(K_ENTER));
  add_btn(0, 194, 160, 217, "BACK", &F8x16, key_act(K_ESC));
  add_btn(160, 194, 320, 217, "MANUAL", &F8x16, kind_act(A_MANUAL));
}

// ------------------------------------------------------------------ public API
void init(const Host* host) {
  H = host;
  memset(&last, 0, sizeof last);
  load_manual();
  dirty();
}

bool modal() { return manual_open; }

std::vector<std::string> fit_problems() { return fit_log; }

static void fire(const Act& a) {
  switch (a.kind) {
    case A_KEY: if (a.key != K_NONE) press(a.key, a.mods); break;
    case A_MODE: target_mode = !target_mode; say(target_mode ? "Tap the tactical display to move the target box" : "Tap the tactical display to steer"); break;
    case A_MANUAL: manual_open = true; man_scroll = 0; break;
    case A_KB:
      if (kb_open) { kb_open = false; kb_user_hidden = true; kb_forced = false; }
      else { kb_open = true; kb_forced = true; kb_user_hidden = false; }
      break;
    case A_SECTION: man_sel = a.arg; man_scroll = 0; break;
    case A_CLOSE: manual_open = false; break;
    default: break;
  }
  dirty();
}

Redraw frame(const TextScreen& ts, uint16_t* top, uint16_t* bottom) {
  pump_keys();
  uint32_t t = now();
  Redraw rd = {false, false};

  // keyboard opens by itself when the game asks for typed text
  if (ts.text_mode && !manual_open) {
    bool want = wants_text(ts);
    if (want && !kb_open && !kb_user_hidden) { kb_open = true; dirty(); }
    if (!want) { kb_user_hidden = false; if (kb_open && !kb_forced) { kb_open = false; dirty(); } }
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

  bool changed = memcmp(&ts, &last, sizeof ts) != 0;
  static uint32_t last_anim = 0;
  bool anim = (int32_t)(flash_until - t) > 0 || (!toast.empty() && (int32_t)(toast_until - t) > -50) ||
              (marquee && t - last_anim >= 50);
  if (anim) last_anim = t;
  if (!changed && drawn_version == ui_version && !anim) return rd;

  Surf ts_top = {top, TOP_W, TOP_H}, ts_bot = {bottom, BOT_W, BOT_H};
  btns.clear();
  if (manual_open) {
    manual_top(ts_top); manual_bottom(ts_bot); rd.top = rd.bottom = true;
  } else if (!ts.text_mode) {
    graphics_bottom(ts_bot); rd.bottom = true;
  } else if (is_bridge(ts)) {
    bridge_top(ts_top, ts);
    if (kb_open) keyboard(ts_bot); else bridge_bottom(ts_bot, ts);
    bottom_rows(ts_bot, ts);
    rd.top = rd.bottom = true;
  } else {
    if (kb_open) keyboard(ts_bot); else generic_bottom(ts_bot, ts);
    generic_top(ts_top, ts);
    bottom_rows(ts_bot, ts);
    rd.top = rd.bottom = true;
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
    if (x >= btns[i].x0 && x < btns[i].x1 && y >= btns[i].y0 && y < btns[i].y1) return i;
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
  if (!target_mode) { steer_to(last, x, y); return; }
  int col = 28 + (x - TAC_X) / TAC_CW, row = 2 + (y - TAC_Y) / TAC_LH;
  int c, r;
  if (!find_tdb(last, c, r, 29, 2, 49, 13)) {
    say("No target box: choose WEAPONS > Manual targeting first");
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

static void loupe_tap(int x, int y) {
  int col = lx + (x - 1) / 6, row = ly + y / 12;
  int c, r;
  if (!find_tdb(last, c, r)) {
    char k = hotkey_at(last, col, row);
    std::string prompt; std::vector<MenuItem> items = parse_menu(last, prompt);
    if (k) { press(key_for_char(k)); char m[32]; snprintf(m, sizeof m, "Sent %c", k); say(m, 900); }
    else if (items.empty()) press(K_ENTER);
    else say("Use the menu buttons below");
    return;
  }
  snap_to_object(last, col, row, 1, 1, 78, 23);
  start_drive(col, row, c, r);
  follow = true;
}

void touch(int x, int y, bool down) {
  if (down && !touch_was_down) {
    tap_kind = 0;
    int i = hit(x, y);
    if (i >= 0) {
      pressed_id = id_of(btns[i]);
      if (H && H->click) H->click();
      dirty();
    } else if (!manual_open && !kb_open && last.text_mode) {
      if (is_bridge(last) && x < 180 && y < 180) { tap_kind = 1; tap_x = x; tap_y = y; }
      else if (!is_bridge(last) && y < 180) {
        tap_kind = 2; dragging = true; drag_x = x; drag_y = y; drag_lx = lx; drag_ly = ly; drag_moved = 0;
      }
    }
  } else if (down && touch_was_down) {
    if (dragging) {
      int dx = x - drag_x, dy = y - drag_y;
      if (abs(dx) + abs(dy) > 6) drag_moved = 1;
      if (drag_moved) {
        lx = drag_lx - dx / 6; ly = drag_ly - dy / 12; clamp_loupe(); follow = false; user_pan_ms = now(); dirty();
      }
    }
  } else if (!down && touch_was_down) {
    if (pressed_id.valid) {
      BtnId p = pressed_id; pressed_id.valid = false;
      int i = -1;
      for (int k = 0; k < (int)btns.size(); k++) if (same(p, btns[k])) i = k;
      if (i >= 0 && x >= p.x0 - 8 && x < p.x1 + 8 && y >= p.y0 - 8 && y < p.y1 + 8) {
        flash_id = p; flash_until = now() + 110;
        fire(btns[i].act);
      }
      dirty();
    } else if (tap_kind == 1) {
      tactical_tap(tap_x, tap_y);
    } else if (tap_kind == 2) {
      dragging = false;
      if (!drag_moved) loupe_tap(drag_x, drag_y);
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
  if (pr & B_START) { manual_open = true; man_scroll = 0; dirty(); return; }
  if (pr & B_SELECT) fire(kind_act(A_KB));
  if (pr & B_A) press(K_ENTER);
  if (pr & B_B) press(K_ESC);
  if (pr & B_X) press(K_INSERT);           // fire phasers
  if (pr & B_Y) press(K_DELETE);           // fire torpedoes
  if (pr & B_L) press(K_F, M_CTRL);        // freeze / unfreeze
  if (pr & B_R) fire(kind_act(A_MODE));    // steer <-> target
  if (repeat_due(B_DUP, held, pr)) press(K_UP);
  if (repeat_due(B_DDOWN, held, pr)) press(K_DOWN);
  if (repeat_due(B_DLEFT, held, pr)) press(K_LEFT);
  if (repeat_due(B_DRIGHT, held, pr)) press(K_RIGHT);
  if (repeat_due(B_CPAD_LEFT, held, pr)) press((Key)(K_F1 + 4));   // heading -1
  if (repeat_due(B_CPAD_RIGHT, held, pr)) press((Key)(K_F1 + 5));  // heading +1
  if (repeat_due(B_CPAD_UP, held, pr)) press((Key)(K_F1 + 3));     // sublight +0.1
  if (repeat_due(B_CPAD_DOWN, held, pr)) press((Key)(K_F1 + 2));   // sublight -0.1
  if (repeat_due(B_ZR, held, pr)) press((Key)(K_F1 + 1));          // hyperspeed +1
  if (repeat_due(B_ZL, held, pr)) press(K_F1);                      // hyperspeed -1
  bool bridge = is_bridge(last);
  if (bridge) {
    if (repeat_due(B_CSTICK_LEFT, held, pr)) press(K_LEFT, M_CTRL);   // earlier messages
    if (repeat_due(B_CSTICK_RIGHT, held, pr)) press(K_RIGHT, M_CTRL);
  } else {
    if (repeat_due(B_CSTICK_LEFT, held, pr)) { lx -= 2; follow = false; clamp_loupe(); dirty(); }
    if (repeat_due(B_CSTICK_RIGHT, held, pr)) { lx += 2; follow = false; clamp_loupe(); dirty(); }
    if (repeat_due(B_CSTICK_UP, held, pr)) { ly -= 1; follow = false; clamp_loupe(); dirty(); }
    if (repeat_due(B_CSTICK_DOWN, held, pr)) { ly += 1; follow = false; clamp_loupe(); dirty(); }
  }
}

}  // namespace sf2ui
