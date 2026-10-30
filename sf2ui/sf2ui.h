// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 the starfleet2-3ds authors
// sf2ui: Star Fleet II split-cockpit frontend for the New 3DS XL.
//
// Platform independent. Each frame the platform hands in the game's 80x25 text
// screen; sf2ui draws the top (400x240) and touch (320x240) screens into RGB565
// buffers (row-major, x to the right, y down) and turns taps and button presses
// into the game's own keystrokes through the Host callbacks.
#pragma once
#include <stdint.h>

namespace sf2ui {

// Keys sf2ui can send. The platform maps them to its keyboard codes.
enum Key {
  K_NONE = 0,
  K_A, K_B, K_C, K_D, K_E, K_F, K_G, K_H, K_I, K_J, K_K, K_L, K_M,
  K_N, K_O, K_P, K_Q, K_R, K_S, K_T, K_U, K_V, K_W, K_X, K_Y, K_Z,
  K_0, K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9,
  K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10,
  K_ESC, K_ENTER, K_SPACE, K_BACKSPACE, K_TAB,
  K_UP, K_DOWN, K_LEFT, K_RIGHT,
  K_INSERT, K_DELETE, K_PGUP, K_PGDN,
  K_LSHIFT, K_LCTRL, K_LALT,
  K_MINUS, K_EQUALS, K_LBRACKET, K_RBRACKET, K_COMMA, K_PERIOD, K_SLASH,
  K_GRAVE, K_SEMICOLON, K_QUOTE,
  K_COUNT
};

// 3DS buttons (same bit layout as libctru's KEY_* so the 3DS glue can pass hidKeys* through).
enum Button : uint32_t {
  B_A = 1u << 0, B_B = 1u << 1, B_SELECT = 1u << 2, B_START = 1u << 3,
  B_DRIGHT = 1u << 4, B_DLEFT = 1u << 5, B_DUP = 1u << 6, B_DDOWN = 1u << 7,
  B_R = 1u << 8, B_L = 1u << 9, B_X = 1u << 10, B_Y = 1u << 11,
  B_ZL = 1u << 14, B_ZR = 1u << 15,
  B_CSTICK_RIGHT = 1u << 24, B_CSTICK_LEFT = 1u << 25, B_CSTICK_UP = 1u << 26, B_CSTICK_DOWN = 1u << 27,
  B_CPAD_RIGHT = 1u << 28, B_CPAD_LEFT = 1u << 29, B_CPAD_UP = 1u << 30, B_CPAD_DOWN = 1u << 31,
};

struct Host {
  void (*key)(Key k, bool down);      // inject one key event into the game
  void (*click)();                    // play the touch-button click sound
  uint32_t (*ms)();                   // monotonic milliseconds
  const char* manual_path;            // text file for the built-in manual
  const char* profile_path;           // remembered officer for one-tap sign-on
  void (*save)(const char* path, const char* data, unsigned n);   // write a small file (0: plain fopen)
};

// The game's text screen, as read from video memory.
struct TextScreen {
  bool text_mode;          // false while the game shows a graphics screen (title art)
  uint8_t ch[25][80];      // CP437 character codes
  uint8_t at[25][80];      // VGA attributes (fg = low nibble, bg = high nibble)
  int cursor_x, cursor_y;  // BIOS text cursor
};

enum { TOP_W = 400, TOP_H = 240, BOT_W = 320, BOT_H = 240 };

void init(const Host* host);

// Called every frame by the platform. Draws into top/bottom when it returns true
// for that screen (a screen is only redrawn when something on it changed).
// When ts.text_mode is false, sf2ui leaves the top screen to the platform
// (the game's own graphics) and only draws the touch screen.
struct Redraw { bool top, bottom; int by0, by1; };  // by0..by1: touch-screen rows that changed
Redraw frame(const TextScreen& ts, uint16_t* top, uint16_t* bottom);

// Input. Touch coordinates are touch-screen pixels; down=false on release.
void touch(int x, int y, bool down);
void buttons(uint32_t held, uint32_t pressed, uint32_t released);

// True while sf2ui wants the whole input stream (manual or keyboard open).
bool modal();

}  // namespace sf2ui

#include <string>
#include <vector>
namespace sf2ui {
// Text that did not fit its space since start-up (should always be empty).
std::vector<std::string> fit_problems();
void fit_clear();     // test harness: start a fresh fit report
void force_redraw();  // test harness: redraw both screens on the next frame

}  // namespace sf2ui
