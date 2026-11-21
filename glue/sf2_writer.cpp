// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 the starfleet2-3ds authors
// Background SD card writer (see sf2_writer.h).
#ifdef SF2_3DS
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <deque>
#include "sf2_writer.h"
#include "sf2_ctr.h"
#include <strings.h>

// The SD card's file system ignores case, and DOSBox can spell one file two ways
// (as created vs. as found in its directory cache): compare paths without case.
static bool same_path(const std::string& a, const char* b) { return b && strcasecmp(a.c_str(), b) == 0; }

struct Job { std::string path; std::vector<uint8_t> data; bool own = false, del = false; };
static std::deque<Job> jobs;          // queued writes, oldest first (at most one per file)
static std::string busy_path;         // the write in progress
static Job cur;                       // its contents: the worker only reads them while busy_path is set
static LightLock lock;
static LightEvent wake, done;
static Thread worker = 0;
static bool started = false;
static bool trace = false;                 // log what the writer does (slowsd.txt test mode)
static int slow_ms = 0;                    // test mode: make every write as slow as the real SD card
static std::string fail_name;              // SF-01: the last write that failed, for a toast (under lock)
static bool lock_ready = false;
static void ensure_lock() { if (!lock_ready) { LightLock_Init(&lock); lock_ready = true; } }   // main thread, before the worker

static void note_failure(const std::string& path) {
  size_t s = path.find_last_of('/');
  ensure_lock();
  LightLock_Lock(&lock); fail_name = s == std::string::npos ? path : path.substr(s + 1); LightLock_Unlock(&lock);
}

// SF-01: the port's own files are written to <name>.tmp (every write and the close checked) and
// swapped in through <name>.bak: rename over an existing file fails on the 3DS SD card. On any
// failure the old file stays (or is put back). A delete job just removes the file.
static bool write_own(const Job& j) {
  if (j.del) {
    if (remove(j.path.c_str()) == 0 || errno == ENOENT) return true;
    sf2ctr_note("delete failed: %s (errno %d)", j.path.c_str(), errno);
    return false;
  }
  std::string tmp = j.path + ".tmp", bak = j.path + ".bak";
  FILE* f = fopen(tmp.c_str(), "wb");
  if (!f) { sf2ctr_note("write failed (open): %s (errno %d)", tmp.c_str(), errno); return false; }
  bool ok = j.data.empty() || fwrite(j.data.data(), 1, j.data.size(), f) == j.data.size();
  if (fflush(f) != 0) ok = false;
  if (fclose(f) != 0) ok = false;
  if (!ok) { sf2ctr_note("write failed: %s", tmp.c_str()); remove(tmp.c_str()); return false; }
  FILE* old = fopen(j.path.c_str(), "rb");
  bool had = old != 0;
  if (old) fclose(old);
  if (had) {
    remove(bak.c_str());
    if (rename(j.path.c_str(), bak.c_str()) != 0) { sf2ctr_note("write failed (old aside): %s", j.path.c_str()); remove(tmp.c_str()); return false; }
  }
  if (rename(tmp.c_str(), j.path.c_str()) != 0) {
    sf2ctr_note("write failed (rename): %s", j.path.c_str());
    if (had) rename(bak.c_str(), j.path.c_str());
    remove(tmp.c_str());
    return false;
  }
  if (had) remove(bak.c_str());
  return true;
}

static void write_file(const Job& j) {
  u64 t0 = svcGetSystemTick();
  if (j.own) {
    if (!write_own(j)) note_failure(j.path);
    if (slow_ms) svcSleepThread((s64)slow_ms * 1000000);
    return;
  }
  // Rewrite in place ("r+b") rather than truncating to zero first: freeing and re-allocating the
  // file's clusters is the slow part on the 3DS.
  // The real 3DS refuses to open a file the game has open: try again for a few seconds.
  FILE* f = 0;
  for (int tries = 0; !f && tries < 30; tries++) {
    f = fopen(j.path.c_str(), "r+b");
    if (!f) f = fopen(j.path.c_str(), "wb");
    if (!f) { if (tries == 0) sf2ctr_log("write refused, retrying: %s (errno %d)", j.path.c_str(), errno); svcSleepThread(200 * 1000 * 1000); }
  }
  if (!f) { sf2ctr_note("write failed: %s", j.path.c_str()); note_failure(j.path); return; }
  fseek(f, 0, SEEK_END);
  long old = ftell(f);
  fseek(f, 0, SEEK_SET);
  bool ok = j.data.empty() || fwrite(j.data.data(), 1, j.data.size(), f) == j.data.size();
  if (fflush(f) != 0) ok = false;
  if (old > (long)j.data.size() && ftruncate(fileno(f), (off_t)j.data.size()) != 0) ok = false;
  if (fclose(f) != 0) ok = false;
  if (!ok) { sf2ctr_note("write failed: %s", j.path.c_str()); note_failure(j.path); }
  if (slow_ms) svcSleepThread((s64)slow_ms * 1000000);
  unsigned ms = (unsigned)((svcGetSystemTick() - t0) / (SYSCLOCK_ARM11 / 1000));
  if (ms > 300) sf2ctr_log("slow write: %s (%u bytes) %u ms", j.path.c_str(), (unsigned)j.data.size(), ms);
}

static void run(void*) {
  for (;;) {
    LightEvent_Wait(&wake);
    for (;;) {
      LightLock_Lock(&lock);
      if (jobs.empty()) { busy_path.clear(); LightLock_Unlock(&lock); LightEvent_Signal(&done); break; }
      cur = std::move(jobs.front()); jobs.pop_front(); busy_path = cur.path;
      LightLock_Unlock(&lock);
      write_file(cur);                       // (seconds on a real SD card)
      LightLock_Lock(&lock); busy_path.clear(); cur.data.clear(); cur.data.shrink_to_fit(); LightLock_Unlock(&lock);
      LightEvent_Signal(&done);
    }
  }
}

static void drain_at_exit() { sf2w_wait(0); }

// Called from every wait loop: if a wait drags on, say so once in the debug log.
static void waited(int& n, const char* what) {
  if (++n == 200) sf2ctr_note("still waiting for the background writer (%s)", what);   // ~10 s
}

// Sleep until the writer finishes a file (or 50 ms pass). Waking every couple of milliseconds to
// check instead slowed the writer from 0.3 s to ~16 s per file on a real 3DS.
static void block_until_done() { LightEvent_WaitTimeout(&done, 50LL * 1000 * 1000); }

void sf2w_start() {
  if (started) return;
  started = true;
  ensure_lock();
  LightEvent_Init(&wake, RESET_ONESHOT);
  LightEvent_Init(&done, RESET_PULSE);
  // On the game's own core, one step above the game's priority: it wakes as soon as a write is
  // queued and spends nearly all its time waiting on the SD card, when the game runs. (A thread on
  // the system core is not guaranteed any time on real hardware: there the writer never ran and
  // the game hung waiting for it.)
  // Test mode for the emulator, whose SD card is fast: sdmc:/3ds/starfleet2/slowsd.txt makes each
  // write take as long as on a real 3DS and logs every queue operation.
  if (FILE* t = fopen("sdmc:/3ds/starfleet2/slowsd.txt", "r")) { fclose(t); slow_ms = 4000; trace = true; sf2ctr_log("slow SD test mode"); }
  s32 prio = 0x30; svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
  worker = threadCreate(run, 0, 64 * 1024, prio > 0x19 ? prio - 1 : prio, -2, true);
  sf2ctr_note("background writer %s", worker ? "started" : "not started (writing directly)");
  atexit(drain_at_exit);
}

static void queue_job(const char* path, const uint8_t* data, size_t n, bool own) {
  if (trace) sf2ctr_log("wq %s %u%s", path, (unsigned)n, own ? (data ? " own" : " delete") : "");
  Job nj; nj.path = path; nj.own = own; nj.del = own && !data;
  if (data) nj.data.assign(data, data + n);
  if (!started || !worker) { write_file(nj); return; }   // no worker: write now
  LightLock_Lock(&lock);
  bool replaced = false;
  for (auto& j : jobs) if (same_path(j.path, path)) { j = std::move(nj); replaced = true; break; }
  if (!replaced) jobs.push_back(std::move(nj));
  LightLock_Unlock(&lock);
  LightEvent_Signal(&wake);
}

void sf2w_queue(const char* path, const uint8_t* data, size_t n) { queue_job(path, data, n, false); }
void sf2w_queue_own(const char* path, const uint8_t* data, size_t n) { queue_job(path, data, n, true); }

bool sf2w_take_failure(char* name, size_t cap) {
  ensure_lock();
  LightLock_Lock(&lock);
  bool any = !fail_name.empty();
  if (any) { snprintf(name, cap, "%s", fail_name.c_str()); fail_name.clear(); }
  LightLock_Unlock(&lock);
  return any;
}

bool sf2w_pending(const char* path, std::vector<uint8_t>& out) {
  if (!started) return false;
  bool found = false;
  LightLock_Lock(&lock);
  for (auto& j : jobs) if (same_path(j.path, path) && !j.del) { out = j.data; found = true; }
  // Being written right now: the card holds a half-written file, so serve what is being written.
  if (!found && same_path(busy_path, path) && !cur.del) { out = cur.data; found = true; }
  LightLock_Unlock(&lock);
  if (trace && found) sf2ctr_log("wr %s %u", path, (unsigned)out.size());
  return found;
}

void sf2w_wait(const char* path) {
  if (!started || !worker) return;
  int n = 0;
  for (;;) {
    waited(n, path ? path : "all");
    LightLock_Lock(&lock);
    bool b = path ? same_path(busy_path, path) : !busy_path.empty();
    if (!b) for (auto& j : jobs) if (!path || same_path(j.path, path)) { b = true; break; }
    LightLock_Unlock(&lock);
    if (!b) return;
    LightEvent_Signal(&wake);
    block_until_done();
  }
}

static void wait_not_busy(const char* path) {
  int n = 0;
  for (;;) {
    waited(n, path);
    LightLock_Lock(&lock); bool b = same_path(busy_path, path); LightLock_Unlock(&lock);
    if (!b) return;
    block_until_done();
  }
}

void sf2w_wait_busy(const char* path) { if (started) wait_not_busy(path); }

void sf2w_cancel(const char* path) {
  if (!started) return;
  if (trace) sf2ctr_log("wx %s", path);
  LightLock_Lock(&lock);
  for (auto it = jobs.begin(); it != jobs.end();) { if (same_path(it->path, path)) it = jobs.erase(it); else ++it; }
  LightLock_Unlock(&lock);
  wait_not_busy(path);
}

int sf2w_rename(const char* from, const char* to) {
  if (!started) return rename(from, to);
  if (trace) sf2ctr_log("wm %s -> %s", from, to);
  // Under the lock, and never while the worker is writing either file.
  int n = 0;
  for (;;) {
    waited(n, from);
    LightLock_Lock(&lock);
    if (!same_path(busy_path, from) && !same_path(busy_path, to)) break;
    LightLock_Unlock(&lock);
    block_until_done();
  }
  // whatever was queued for the target is replaced by this rename
  for (auto it = jobs.begin(); it != jobs.end();) { if (same_path(it->path, to)) it = jobs.erase(it); else ++it; }
  bool queued = false, was_del = false;
  for (auto& j : jobs) if (same_path(j.path, from)) { queued = true; was_del = j.del; if (!j.del) j.path = to; }
  int r;
  if (queued && was_del) { errno = ENOENT; r = -1; }          // the file was deleted
  else if (queued) {
    // Its contents are still on their way to the card: they will be written under the new name
    // (no waiting for the card), and an older copy under the old name is removed.
    Job d; d.path = from; d.own = true; d.del = true; jobs.push_back(std::move(d));
    r = 0;
  } else r = rename(from, to);
  LightLock_Unlock(&lock);
  LightEvent_Signal(&wake);
  return r;
}

// A delete of this file is queued or in progress: the file is gone as far as the game is concerned.
bool sf2w_pending_delete(const char* path) {
  if (!started) return false;
  bool found = false;
  LightLock_Lock(&lock);
  for (auto& j : jobs) if (same_path(j.path, path)) found = j.del;
  if (!found && same_path(busy_path, path) && cur.del) {
    found = true;
    for (auto& j : jobs) if (same_path(j.path, path)) found = j.del;    // (a newer write is queued behind it)
  }
  LightLock_Unlock(&lock);
  return found;
}

bool sf2w_pending_size(const char* path, size_t* n) {
  if (!started) return false;
  bool found = false;
  LightLock_Lock(&lock);
  for (auto& j : jobs) if (same_path(j.path, path) && !j.del) { *n = j.data.size(); found = true; }
  if (!found && same_path(busy_path, path) && !cur.del) { *n = cur.data.size(); found = true; }   // mid-write
  LightLock_Unlock(&lock);
  return found;
}
#endif  // SF2_3DS
