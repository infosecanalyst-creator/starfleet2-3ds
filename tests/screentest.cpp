// SPDX-License-Identifier: GPL-2.0-or-later
// Screen regression test: every real screen dump in tests/screens runs through sf2ui on the PC.
// For each one it writes a short report (what kind of screen the port thinks it is, the hint,
// every touch button and whether it is greyed, any text that does not fit) and compares it with
// tests/expected/<name>.txt. A new game version or a port change that breaks a screen shows up
// as a diff here. Any text that does not fit fails the test even when it matches.
//
//   screentest <screens dir> <expected dir> [--update] [--png <dir>]
//
// The test includes sf2ui.cpp directly so it can read the frontend's own state.
#include "../sf2ui/sf2ui.cpp"
#include <dirent.h>
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace sf2ui;

static uint32_t clock_ms = 1000;
static void k_key(Key, bool) {}
static void k_click() {}
static uint32_t k_ms() { return clock_ms; }
static const Host test_host = {k_key, k_click, k_ms, "../assets/manual-port.txt", 0, 0, 0, 0, 0, 0};

// Dump format (Linux DOSBox harness / port's dump hook): "#vgamode=N ...", 25 printable rows,
// "#attrs" + 25 rows of 80 hex attributes, "#raw" + 25 rows of 80 hex character codes.
static bool load_dump(const std::string& path, TextScreen& ts) {
  std::ifstream in(path);
  if (!in) return false;
  memset(&ts, 0, sizeof ts);
  std::string line; int mode = -1, section = 0, row = 0;
  while (std::getline(in, line)) {
    if (line.rfind("#vgamode=", 0) == 0) { mode = atoi(line.c_str() + 9); continue; }
    if (line == "#attrs") { section = 1; row = 0; continue; }
    if (line == "#raw") { section = 2; row = 0; continue; }
    if (line.rfind("#cursor=", 0) == 0) { sscanf(line.c_str() + 8, "%d,%d", &ts.cursor_x, &ts.cursor_y); continue; }
    if (section == 0 || row >= 25) continue;
    for (int c = 0; c < 80 && (size_t)(c * 2 + 1) < line.size(); c++) {
      unsigned v = 0; sscanf(line.substr(c * 2, 2).c_str(), "%x", &v);
      (section == 1 ? ts.at : ts.ch)[row][c] = (uint8_t)v;
    }
    row++;
  }
  ts.text_mode = mode == 9 || mode == 2 ? mode == 9 : true;
  if (!ts.cursor_y && !ts.cursor_x) ts.cursor_y = -1;
  return true;
}

static const char* kind_name(const TextScreen& ts) {
  if (!ts.text_mode) return "graphics";
  if (is_bridge(ts)) return "bridge";
  if (reader_on) return "reader";
  switch (glay.kind) { case G_MENU: return "menu"; case G_MAP: return "map"; default: return "plain"; }
}

static void save_ppm(const std::string& fn, const uint16_t* b, int w, int h) {
  FILE* f = fopen(fn.c_str(), "wb"); if (!f) return;
  fprintf(f, "P6 %d %d 255\n", w, h);
  for (int i = 0; i < w * h; i++) {
    uint16_t p = b[i];
    unsigned char px[3] = {(unsigned char)((p >> 11) << 3), (unsigned char)(((p >> 5) & 63) << 2), (unsigned char)((p & 31) << 3)};
    fwrite(px, 1, 3, f);
  }
  fclose(f);
}

static std::string report(const std::string& name, const TextScreen& ts, const std::string& png_dir) {
  static uint16_t top[TOP_W * TOP_H], bot[BOT_W * BOT_H];
  // each screen on its own: nothing open from the previous one
  manual_open = kb_open = kb_forced = kb_user_hidden = false; pause_open = quit_confirm = false;
  reader_on = menu_scrolls = focus_mode = false; tdb_drive = false; view_goal = -1; busy_since = 0;
  row_page = 0; menu_top = 0; rd_sig = 0; rd_scroll = 0; toast.clear(); toast_until = 0;
  memset(&last, 0, sizeof last);
  fit_clear(); force_redraw();
  for (int i = 0; i < 6; i++) { clock_ms += 40; frame(ts, top, bot); }
  std::ostringstream o;
  o << "kind: " << kind_name(ts) << "\n";
  if (ts.text_mode) o << "hint: " << context_hint(ts) << "\n";
  o << "buttons:";
  for (auto& b : btns) o << " [" << b.label << (b.off ? " (grey)" : "") << "]";
  o << "\n";
  // Explain mode (hold R + tap): every button gets a card; each card must fit the top screen
  if (ts.text_mode) {
    std::vector<Btn> bs = btns; int cards = 0;
    buttons(B_R, B_R, 0);
    for (auto& b : bs) {
      int x = (b.x0 + b.x1) / 2, y = (b.y0 + b.y1) / 2;
      explain_until = 0;
      touch(x, y, true); clock_ms += 40; frame(ts, top, bot); touch(x, y, false); clock_ms += 40; frame(ts, top, bot);
      if (explain_until) cards++;
    }
    buttons(0, 0, B_R); explain_until = 0; clock_ms += 40; frame(ts, top, bot);
    o << "explain cards: " << cards << "/" << bs.size() << "\n";
  }
  for (auto& p : fit_problems()) o << "FIT: " << p << "\n";
  if (!png_dir.empty()) { save_ppm(png_dir + "/" + name + "_top.ppm", top, TOP_W, TOP_H); save_ppm(png_dir + "/" + name + "_bot.ppm", bot, BOT_W, BOT_H); }
  return o.str();
}

int main(int argc, char** argv) {
  if (argc < 3) { fprintf(stderr, "usage: screentest <screens> <expected> [--update] [--png dir]\n"); return 2; }
  std::string sdir = argv[1], edir = argv[2], png;
  bool update = false;
  for (int i = 3; i < argc; i++) {
    if (!strcmp(argv[i], "--update")) update = true;
    else if (!strcmp(argv[i], "--png") && i + 1 < argc) png = argv[++i];
  }
  std::vector<std::string> names;
  if (DIR* d = opendir(sdir.c_str())) {
    while (dirent* e = readdir(d)) { std::string n = e->d_name; if (n.size() > 4 && n.substr(n.size() - 4) == ".txt") names.push_back(n.substr(0, n.size() - 4)); }
    closedir(d);
  }
  std::sort(names.begin(), names.end());
  init(&test_host);
  int bad = 0, fits = 0;
  for (auto& n : names) {
    TextScreen ts;
    if (!load_dump(sdir + "/" + n + ".txt", ts)) { printf("FAIL %s: unreadable\n", n.c_str()); bad++; continue; }
    std::string got = report(n, ts, png);
    if (got.find("FIT: ") != std::string::npos) { printf("FIT  %s\n%s", n.c_str(), got.c_str()); fits++; }
    std::string ef = edir + "/" + n + ".txt";
    if (update) { std::ofstream(ef) << got; continue; }
    std::ifstream ei(ef); std::stringstream want; want << ei.rdbuf();
    if (!ei.good() && want.str().empty()) { printf("NEW  %s (no expected file; run with --update)\n%s", n.c_str(), got.c_str()); bad++; continue; }
    if (want.str() != got) { printf("DIFF %s\n--- expected\n%s--- got\n%s", n.c_str(), want.str().c_str(), got.c_str()); bad++; }
  }
  printf("%zu screens, %d differ, %d with text that does not fit%s\n", names.size(), bad, fits, update ? " (expected files rewritten)" : "");
  return (bad || fits) ? 1 : 0;
}
