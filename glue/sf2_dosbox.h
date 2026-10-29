// Glue between sf2ui and DOSBox internals (shared by the 3DS build and the desktop harness).
#pragma once
#include "sf2ui.h"

// Fill ts from DOSBox's emulated video memory (80x25 colour text at B800).
void sf2_read_screen(sf2ui::TextScreen& ts);
// Inject a key event into the emulated keyboard.
void sf2_key(sf2ui::Key k, bool down);
