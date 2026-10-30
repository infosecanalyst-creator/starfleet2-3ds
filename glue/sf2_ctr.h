// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 the starfleet2-3ds authors
// 3DS platform layer for sf2ui (see sf2_ctr.cpp).
#pragma once
#ifdef SF2_3DS
void sf2ctr_events();      // input + frontend redraw; replaces DOSBox-3DS's GetInput()
bool sf2ctr_owns_top();    // true while the frontend draws the top screen (skip SDL's flip)
void sf2ctr_log(const char* fmt, ...);
void sf2ctr_check_install();  // before DOSBox starts: settings and game present?
void sf2_dosbox_redraw();      // (ctr_main.cpp) make DOSBox repaint its whole top-screen picture
void sf2ctr_screen_reset();   // DOSBox changed video mode: SDL may repaint the top screen
#endif
