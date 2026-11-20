// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 the starfleet2-3ds authors
// Background SD card writer for the 3DS build.
//
// On the 3DS each small file write can take seconds (the SD card's file system is slow to
// update), and the game writes its data files on almost every screen change. Writes are
// therefore queued and done by a worker thread; the game carries on at once. Until a
// queued write lands, reads of that file are served from the queued copy.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vector>

void sf2w_start();                                         // start the worker (once)
void sf2w_queue(const char* path, const uint8_t* data, size_t n);   // replace the file's contents
bool sf2w_pending(const char* path, std::vector<uint8_t>& out);     // latest queued contents, if any
void sf2w_wait(const char* path);                          // until the file's writes have landed (0 = all)
void sf2w_cancel(const char* path);                        // the file is being deleted: drop its writes
int  sf2w_rename(const char* from, const char* to);        // rename on the card; queued writes follow the file
void sf2w_wait_busy(const char* path);                     // until a write in progress of this file ends
bool sf2w_pending_size(const char* path, size_t* n);      // size of the latest queued contents, if any
