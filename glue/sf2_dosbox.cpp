// Glue between sf2ui and DOSBox internals (shared by the 3DS build and the desktop harness).
#include "sf2_dosbox.h"
#include "dosbox.h"
#include "mem.h"
#include "vga.h"
#include "keyboard.h"
#include "regs.h"
#include "cpu.h"
#include <stdio.h>
#include <string.h>

using namespace sf2ui;

// BIOS data area (segment 0x40)
static const PhysPt BDA_COLS = 0x44A, BDA_CURSOR = 0x450;

void sf2_read_screen(TextScreen& ts) {
  memset(&ts, 0, sizeof ts);
  // 80-column colour text only; anything else (title art) is left to DOSBox's normal output
  Bit16u cols = mem_readw(BDA_COLS);
  ts.text_mode = (vga.mode == M_TEXT) && cols == 80;
  if (!ts.text_mode) return;
  PhysPt base = 0xB8000 + vga.config.display_start * 2;
  for (int r = 0; r < 25; r++)
    for (int c = 0; c < 80; c++) {
      ts.ch[r][c] = mem_readb(base + (r * 80 + c) * 2);
      ts.at[r][c] = mem_readb(base + (r * 80 + c) * 2 + 1);
    }
  Bit16u pos = mem_readw(BDA_CURSOR);  // page 0
  ts.cursor_x = pos & 0xFF; ts.cursor_y = pos >> 8;
}

static KBD_KEYS map_key(Key k) {
  static const KBD_KEYS letters[26] = {KBD_a, KBD_b, KBD_c, KBD_d, KBD_e, KBD_f, KBD_g, KBD_h, KBD_i,
    KBD_j, KBD_k, KBD_l, KBD_m, KBD_n, KBD_o, KBD_p, KBD_q, KBD_r, KBD_s, KBD_t, KBD_u, KBD_v, KBD_w,
    KBD_x, KBD_y, KBD_z};
  static const KBD_KEYS digits[10] = {KBD_0, KBD_1, KBD_2, KBD_3, KBD_4, KBD_5, KBD_6, KBD_7, KBD_8, KBD_9};
  if (k >= K_A && k <= K_Z) return letters[k - K_A];
  if (k >= K_0 && k <= K_9) return digits[k - K_0];
  if (k >= K_F1 && k <= K_F10) return (KBD_KEYS)(KBD_f1 + (k - K_F1));
  switch (k) {
    case K_ESC: return KBD_esc;           case K_ENTER: return KBD_enter;
    case K_SPACE: return KBD_space;       case K_BACKSPACE: return KBD_backspace;
    case K_TAB: return KBD_tab;           case K_UP: return KBD_up;
    case K_DOWN: return KBD_down;         case K_LEFT: return KBD_left;
    case K_RIGHT: return KBD_right;       case K_INSERT: return KBD_insert;
    case K_DELETE: return KBD_delete;     case K_PGUP: return KBD_pageup;
    case K_PGDN: return KBD_pagedown;     case K_LSHIFT: return KBD_leftshift;
    case K_LCTRL: return KBD_leftctrl;    case K_LALT: return KBD_leftalt;
    case K_MINUS: return KBD_minus;       case K_EQUALS: return KBD_equals;
    case K_LBRACKET: return KBD_leftbracket; case K_RBRACKET: return KBD_rightbracket;
    case K_COMMA: return KBD_comma;       case K_PERIOD: return KBD_period;
    case K_SLASH: return KBD_slash;       case K_GRAVE: return KBD_grave;
    case K_SEMICOLON: return KBD_semicolon; case K_QUOTE: return KBD_quote;
    default: return KBD_NONE;
  }
}

void sf2_key(Key k, bool down) {
  KBD_KEYS kk = map_key(k);
  if (kk != KBD_NONE) KEYBOARD_AddKey(kk, down);
}

// One-line snapshot for the debug log: video mode, CPU position, BIOS keyboard buffer.
void sf2_debug_state(char* buf, int n) {
  snprintf(buf, n, "mode %d cols %u cs:ip %04X:%04X kbbuf head %04X tail %04X if %d",
           (int)vga.mode, (unsigned)mem_readw(BDA_COLS), (unsigned)SegValue(cs), (unsigned)reg_eip,
           (unsigned)mem_readw(0x41A), (unsigned)mem_readw(0x41C), GETFLAG(IF) ? 1 : 0);
}
