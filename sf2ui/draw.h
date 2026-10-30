// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 the starfleet2-3ds authors
// Drawing primitives for sf2ui: RGB565 surfaces, bitmap fonts, CP437 cells.
#pragma once
#include <stdint.h>
#include <string.h>

namespace sf2ui {

static inline uint16_t rgb(int r, int g, int b) {
  return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

struct Surf {
  uint16_t* p; int w, h;
  inline void px(int x, int y, uint16_t c) { if ((unsigned)x < (unsigned)w && (unsigned)y < (unsigned)h) p[y * w + x] = c; }
  void fill(int x0, int y0, int x1, int y1, uint16_t c);   // half-open box
  void rect(int x0, int y0, int x1, int y1, uint16_t c);   // 1px outline, half-open box
  void hline(int x0, int x1, int y, uint16_t c) { fill(x0, y, x1, y + 1, c); }
  void vline(int x, int y0, int y1, uint16_t c) { fill(x, y0, x + 1, y1, c); }
  void round_rect(int x0, int y0, int x1, int y1, uint16_t fillc, uint16_t edge);
};

struct Font { const uint8_t* rows; int w, h; int ink_top; };

extern const Font F6x12, F8x14, F8x16, F5x9;

// VGA palette (game colours) and the readability palette used for text.
extern const uint16_t VGA[16];
extern const uint16_t READ_FG[16];

// One text cell: background, then the glyph (or a drawn shape for box/block chars).
void draw_cell(Surf& s, int x, int y, int cw, int ch, uint8_t code, uint16_t fg, uint16_t bg, const Font& f);

// A string of CP437 bytes in one colour, transparent background. Returns width in pixels.
int draw_text(Surf& s, int x, int y, const char* str, const Font& f, uint16_t fg);
int text_width(const char* str, const Font& f);

}  // namespace sf2ui
