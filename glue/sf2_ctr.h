// 3DS platform layer for sf2ui (see sf2_ctr.cpp).
#pragma once
#ifdef SF2_3DS
void sf2ctr_events();      // input + frontend redraw; replaces DOSBox-3DS's GetInput()
bool sf2ctr_owns_top();    // true while the frontend draws the top screen (skip SDL's flip)
void sf2ctr_log(const char* fmt, ...);
void sf2ctr_check_install();  // before DOSBox starts: settings and game present?   // append a line to sdmc:/3ds/starfleet2/debug.log
#endif
