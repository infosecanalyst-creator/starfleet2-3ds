// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 the starfleet2-3ds authors
#include "draw.h"
#include "fonts.h"

namespace sf2ui {

const Font F6x12 = {&ter_6x12[0][0], 6, 12, 2};
const Font F8x14 = {&ter_8x14[0][0], 8, 14, 2};
const Font F8x16 = {&ter_8x16[0][0], 8, 16, 2};
const Font F5x9  = {&tam_5x9[0][0], 5, 9, 1};

#define C(r, g, b) (uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
const uint16_t VGA[16] = {
  C(0, 0, 0),     C(0, 0, 170),   C(0, 170, 0),   C(0, 170, 170),
  C(170, 0, 0),   C(170, 0, 170), C(170, 85, 0),  C(170, 170, 170),
  C(85, 85, 85),  C(85, 85, 255), C(85, 255, 85), C(85, 255, 255),
  C(255, 85, 85), C(255, 85, 255), C(255, 255, 85), C(255, 255, 255)};
// Dark blue / dark red / magenta / dark grey text is hard to read on black;
// these are brightened for text only (backgrounds keep the game's colours).
const uint16_t READ_FG[16] = {
  C(0, 0, 0),     C(96, 128, 255), C(0, 170, 0),   C(0, 170, 170),
  C(220, 70, 64), C(200, 80, 210), C(170, 85, 0),  C(170, 170, 170),
  C(120, 120, 130), C(85, 85, 255), C(85, 255, 85), C(85, 255, 255),
  C(255, 85, 85), C(255, 85, 255), C(255, 255, 85), C(255, 255, 255)};
#undef C

void Surf::fill(int x0, int y0, int x1, int y1, uint16_t c) {
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > w) x1 = w;
  if (y1 > h) y1 = h;
  for (int y = y0; y < y1; y++) {
    uint16_t* row = p + y * w;
    for (int x = x0; x < x1; x++) row[x] = c;
  }
}

void Surf::rect(int x0, int y0, int x1, int y1, uint16_t c) {
  hline(x0, x1, y0, c); hline(x0, x1, y1 - 1, c);
  vline(x0, y0, y1, c); vline(x1 - 1, y0, y1, c);
}

void Surf::round_rect(int x0, int y0, int x1, int y1, uint16_t fc, uint16_t edge) {
  fill(x0 + 1, y0 + 1, x1 - 1, y1 - 1, fc);
  hline(x0 + 2, x1 - 2, y0, edge); hline(x0 + 2, x1 - 2, y1 - 1, edge);
  vline(x0, y0 + 2, y1 - 2, edge); vline(x1 - 1, y0 + 2, y1 - 2, edge);
  px(x0 + 1, y0 + 1, edge); px(x1 - 2, y0 + 1, edge); px(x0 + 1, y1 - 2, edge); px(x1 - 2, y1 - 2, edge);
}

// CP437 line characters: up, down, left, right (1 single, 2 double).
static const uint8_t BOX[0xDB - 0xB3][4] = {
  {1,1,0,0},{1,1,1,0},{1,1,2,0},{2,2,1,0},{0,2,1,0},{0,1,2,0},{2,2,2,0},{2,2,0,0},{0,2,2,0},{2,0,2,0},
  {2,0,1,0},{1,0,2,0},{0,1,1,0},{1,0,0,1},{1,0,1,1},{0,1,1,1},{1,1,0,1},{0,0,1,1},{1,1,1,1},{1,1,0,2},
  {2,2,0,1},{2,0,0,2},{0,2,0,2},{2,0,2,2},{0,2,2,2},{2,2,0,2},{0,0,2,2},{2,2,2,2},{1,0,2,2},{2,0,1,1},
  {0,1,2,2},{0,2,1,1},{2,0,0,1},{1,0,0,2},{0,1,0,2},{0,2,0,1},{2,2,1,1},{1,1,2,2},{1,0,1,0},{0,1,0,1},
};

void draw_cell(Surf& s, int x, int y, int cw, int ch, uint8_t code, uint16_t fg, uint16_t bg, const Font& f) {
  s.fill(x, y, x + cw, y + ch, bg);
  switch (code) {
    case 0xDB: s.fill(x, y, x + cw, y + ch, fg); return;
    case 0xDC: s.fill(x, y + ch / 2, x + cw, y + ch, fg); return;
    case 0xDF: s.fill(x, y, x + cw, y + ch / 2, fg); return;
    case 0xDD: s.fill(x, y, x + (cw + 1) / 2, y + ch, fg); return;
    case 0xDE: s.fill(x + cw / 2, y, x + cw, y + ch, fg); return;
    case 0xB0: case 0xB1: case 0xB2:
      for (int yy = 0; yy < ch; yy++)
        for (int xx = 0; xx < cw; xx++) {
          bool on = code == 0xB1 ? ((xx + yy) % 2 == 0)
                  : code == 0xB0 ? ((xx % 2 == 0) && (yy % 2 == 0))
                                 : !((xx % 2 == 1) && (yy % 2 == 1));
          if (on) s.px(x + xx, y + yy, fg);
        }
      return;
  }
  if (code >= 0xB3 && code <= 0xDA) {
    const uint8_t* b = BOX[code - 0xB3];
    int mx = cw / 2, my = ch / 2;
    // vertical arms
    if (b[0] == 1) s.vline(x + mx, y, y + my + 1, fg);
    if (b[0] == 2) { s.vline(x + mx - 1, y, y + my + 1, fg); s.vline(x + mx + 1, y, y + my + 1, fg); }
    if (b[1] == 1) s.vline(x + mx, y + my, y + ch, fg);
    if (b[1] == 2) { s.vline(x + mx - 1, y + my, y + ch, fg); s.vline(x + mx + 1, y + my, y + ch, fg); }
    // horizontal arms
    if (b[2] == 1) s.hline(x, x + mx + 1, y + my, fg);
    if (b[2] == 2) { s.hline(x, x + mx + 1, y + my - 1, fg); s.hline(x, x + mx + 1, y + my + 1, fg); }
    if (b[3] == 1) s.hline(x + mx, x + cw, y + my, fg);
    if (b[3] == 2) { s.hline(x + mx, x + cw, y + my - 1, fg); s.hline(x + mx, x + cw, y + my + 1, fg); }
    return;
  }
  int ox = (cw - f.w) / 2, oy = (ch - f.h) / 2;
  const uint8_t* g = f.rows + code * f.h;
  for (int r = 0; r < f.h; r++) {
    int yy = oy + r;
    if (yy < 0 || yy >= ch) continue;
    uint8_t bits = g[r];
    if (!bits) continue;
    for (int c = 0; c < f.w; c++)
      if (bits & (0x80 >> c)) {
        int xx = ox + c;
        if (xx >= 0 && xx < cw) s.px(x + xx, y + yy, fg);
      }
  }
}

int text_width(const char* str, const Font& f) { return (int)strlen(str) * f.w; }

int draw_text(Surf& s, int x, int y, const char* str, const Font& f, uint16_t fg) {
  int n = 0;
  for (const uint8_t* p = (const uint8_t*)str; *p; p++, n++) {
    const uint8_t* g = f.rows + (*p) * f.h;
    for (int r = 0; r < f.h; r++) {
      uint8_t bits = g[r];
      for (int c = 0; c < f.w; c++)
        if (bits & (0x80 >> c)) s.px(x + n * f.w + c, y + r, fg);
    }
  }
  return n * f.w;
}

}  // namespace sf2ui
