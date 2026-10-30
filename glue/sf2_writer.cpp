// Background SD card writer (see sf2_writer.h).
#ifdef SF2_3DS
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <deque>
#include "sf2_writer.h"
#include "sf2_ctr.h"
#include <strings.h>

// The SD card's file system ignores case, and DOSBox can spell one file two ways
// (as created vs. as found in its directory cache): compare paths without case.
static bool same_path(const std::string& a, const char* b) { return b && strcasecmp(a.c_str(), b) == 0; }

struct Job { std::string path; std::vector<uint8_t> data; };
static std::deque<Job> jobs;          // queued writes, oldest first (at most one per file)
static std::string busy_path;         // the write in progress
static LightLock lock;
static LightEvent wake, done;
static Thread worker = 0;
static bool started = false;

static void write_file(const Job& j) {
  u64 t0 = svcGetSystemTick();
  // Rewrite in place ("r+b") rather than truncating to zero first: freeing and re-allocating the
  // file's clusters is the slow part on the 3DS.
  FILE* f = fopen(j.path.c_str(), "r+b");
  if (!f) f = fopen(j.path.c_str(), "wb");
  if (!f) { sf2ctr_log("write failed: %s", j.path.c_str()); return; }
  fseek(f, 0, SEEK_END);
  long old = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (!j.data.empty()) fwrite(j.data.data(), 1, j.data.size(), f);
  fflush(f);
  if (old > (long)j.data.size()) { if (ftruncate(fileno(f), (off_t)j.data.size()) != 0) {} }
  fclose(f);
  unsigned ms = (unsigned)((svcGetSystemTick() - t0) / (SYSCLOCK_ARM11 / 1000));
  if (ms > 300) sf2ctr_log("slow write: %s (%u bytes) %u ms", j.path.c_str(), (unsigned)j.data.size(), ms);
}

static void run(void*) {
  for (;;) {
    LightEvent_Wait(&wake);
    for (;;) {
      Job j;
      LightLock_Lock(&lock);
      if (jobs.empty()) { busy_path.clear(); LightLock_Unlock(&lock); LightEvent_Signal(&done); break; }
      j = std::move(jobs.front()); jobs.pop_front(); busy_path = j.path;
      LightLock_Unlock(&lock);
      write_file(j);
      LightLock_Lock(&lock); busy_path.clear(); LightLock_Unlock(&lock);
      LightEvent_Signal(&done);
    }
  }
}

static void drain_at_exit() { sf2w_wait(0); }

void sf2w_start() {
  if (started) return;
  started = true;
  LightLock_Init(&lock);
  LightEvent_Init(&wake, RESET_ONESHOT);
  LightEvent_Init(&done, RESET_PULSE);
  // the system core first (the game's own core stays free); else the game's core at the same priority
  s32 prio = 0x30; svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
  worker = threadCreate(run, 0, 32 * 1024, prio, 1, true);
  if (!worker) worker = threadCreate(run, 0, 32 * 1024, prio, -2, true);
  atexit(drain_at_exit);
}

static bool trace = false;                 // (test builds: log what the writer does)
void sf2w_queue(const char* path, const uint8_t* data, size_t n) {
  if (trace) sf2ctr_log("wq %s %u", path, (unsigned)n);
  if (!started || !worker) {                 // no worker: write now
    Job j; j.path = path; j.data.assign(data, data + n); write_file(j); return;
  }
  LightLock_Lock(&lock);
  bool replaced = false;
  for (auto& j : jobs) if (same_path(j.path, path)) { j.data.assign(data, data + n); replaced = true; break; }
  if (!replaced) { Job j; j.path = path; j.data.assign(data, data + n); jobs.push_back(std::move(j)); }
  LightLock_Unlock(&lock);
  LightEvent_Signal(&wake);
}

bool sf2w_pending(const char* path, std::vector<uint8_t>& out) {
  if (!started) return false;
  bool found = false;
  LightLock_Lock(&lock);
  for (auto& j : jobs) if (same_path(j.path, path)) { out = j.data; found = true; }
  LightLock_Unlock(&lock);
  if (trace && found) sf2ctr_log("wr %s %u", path, (unsigned)out.size());
  if (!found) {                              // being written right now: wait for it, then read the card
    for (;;) {
      LightLock_Lock(&lock); bool b = same_path(busy_path, path); LightLock_Unlock(&lock);
      if (!b) break;
      svcSleepThread(2 * 1000 * 1000);
    }
  }
  return found;
}

void sf2w_wait(const char* path) {
  if (!started || !worker) return;
  for (;;) {
    LightLock_Lock(&lock);
    bool b = path ? same_path(busy_path, path) : !busy_path.empty();
    if (!b) for (auto& j : jobs) if (!path || same_path(j.path, path)) { b = true; break; }
    LightLock_Unlock(&lock);
    if (!b) return;
    LightEvent_Signal(&wake);
    svcSleepThread(2 * 1000 * 1000);
  }
}

static void wait_not_busy(const char* path) {
  for (;;) {
    LightLock_Lock(&lock); bool b = same_path(busy_path, path); LightLock_Unlock(&lock);
    if (!b) return;
    svcSleepThread(2 * 1000 * 1000);
  }
}

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
  // rename and re-target under the lock, so the worker never writes the old name after the move
  for (;;) {
    LightLock_Lock(&lock);
    if (!same_path(busy_path, from)) break;
    LightLock_Unlock(&lock);
    svcSleepThread(2 * 1000 * 1000);
  }
  int r = rename(from, to);
  if (r == 0) for (auto& j : jobs) if (same_path(j.path, from)) j.path = to;
  LightLock_Unlock(&lock);
  return r;
}

bool sf2w_pending_size(const char* path, size_t* n) {
  if (!started) return false;
  bool found = false;
  LightLock_Lock(&lock);
  for (auto& j : jobs) if (same_path(j.path, path)) { *n = j.data.size(); found = true; }
  LightLock_Unlock(&lock);
  return found;
}
#endif  // SF2_3DS
