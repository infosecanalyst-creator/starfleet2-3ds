// 3DS platform layer for sf2ui: screens, touch, buttons and the click sound.
//
// While the game shows its 80x25 text screen, sf2ui owns both 3DS screens: DOSBox's own
// top-screen output is skipped and the frontend's images are written straight to the
// framebuffers. While the game shows graphics (title art), DOSBox draws the top screen
// as usual and sf2ui only draws the touch screen.
#ifdef SF2_3DS
#include <3ds.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "sf2ui.h"
#include "sf2_dosbox.h"
#include "sf2_ctr.h"
#include "SDL.h"

static uint16_t top_img[sf2ui::TOP_W * sf2ui::TOP_H];
static uint16_t bot_img[sf2ui::BOT_W * sf2ui::BOT_H];
static bool inited = false, owns_top = false;
static u64 last_frame = 0;

// ------------------------------------------------------------------ click sound
static const int CLICK_CH = 22;               // a channel the emulator's audio does not use
static s16* click_pcm = 0;
static ndspWaveBuf click_buf;
static bool click_ok = false;

static void click_init() {
  if (R_FAILED(ndspInit())) return;           // no DSP firmware: buttons still animate, just silently
  const int rate = 22050, n = rate * 22 / 1000; // 22 ms
  click_pcm = (s16*)linearAlloc(n * sizeof(s16));
  if (!click_pcm) return;
  for (int i = 0; i < n; i++) {               // short decaying tick
    float t = (float)i / rate, env = expf(-t * 260.0f);
    click_pcm[i] = (s16)(env * 9000.0f * sinf(2.0f * 3.14159265f * 1900.0f * t));
  }
  DSP_FlushDataCache(click_pcm, n * sizeof(s16));
  ndspChnReset(CLICK_CH);
  ndspChnSetInterp(CLICK_CH, NDSP_INTERP_LINEAR);
  ndspChnSetRate(CLICK_CH, (float)rate);
  ndspChnSetFormat(CLICK_CH, NDSP_FORMAT_MONO_PCM16);
  float mix[12] = {0}; mix[0] = mix[1] = 0.8f;
  ndspChnSetMix(CLICK_CH, mix);
  memset(&click_buf, 0, sizeof click_buf);
  click_buf.data_vaddr = click_pcm;
  click_buf.nsamples = n;
  click_ok = true;
}
static void click() {
  if (!click_ok) return;
  if (click_buf.status == NDSP_WBUF_QUEUED || click_buf.status == NDSP_WBUF_PLAYING) ndspChnWaveBufClear(CLICK_CH);
  ndspChnWaveBufAdd(CLICK_CH, &click_buf);
}

// ------------------------------------------------------------------ framebuffers
// 3DS framebuffers are stored rotated: pixel (x, y) of the landscape screen lives at
// fb[x * 240 + (239 - y)]. Rows y0..y1 only (a strip redraw touches a few rows).
static void blit_to(u8* fb, GSPGPU_FramebufferFormat fmt, const uint16_t* img, int w, int y0, int y1) {
  if (fmt == GSP_RGB565_OES) {
    u16* p = (u16*)fb;
    for (int x0 = 0; x0 < w; x0 += 8) {         // 8 columns at a time: sequential reads, 8 write streams
      int xe = x0 + 8 < w ? x0 + 8 : w;
      for (int y = y0; y < y1; y++) {
        const uint16_t* src = img + y * w;
        for (int x = x0; x < xe; x++) p[x * 240 + 239 - y] = src[x];
      }
    }
  } else {                                      // BGR8 (3 bytes per pixel)
    for (int x = 0; x < w; x++)
      for (int y = y0; y < y1; y++) {
        uint16_t c = img[y * w + x];
        u8* d = fb + (x * 240 + (239 - y)) * 3;
        d[0] = (u8)((c & 31) << 3); d[1] = (u8)(((c >> 5) & 63) << 2); d[2] = (u8)((c >> 11) << 3);
      }
  }
}
static int bytes_per_px(GSPGPU_FramebufferFormat fmt) { return fmt == GSP_RGB565_OES ? 2 : 3; }

// Top screen: SDL (DOSBox's own output) double-buffers it, so draw into the back buffer, show it,
// and copy the finished picture into the other buffer too, so whichever one is shown is current.
static void present_top(const uint16_t* img) {
  u16 fw, fh;
  GSPGPU_FramebufferFormat fmt = gfxGetScreenFormat(GFX_TOP);
  u8* drawn = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &fw, &fh);
  blit_to(drawn, fmt, img, sf2ui::TOP_W, 0, sf2ui::TOP_H);
  gfxScreenSwapBuffers(GFX_TOP, false);
  u8* other = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &fw, &fh);
  if (other != drawn) memcpy(other, drawn, sf2ui::TOP_W * 240 * bytes_per_px(fmt));
  gfxFlushBuffers();
}
// Touch screen: only sf2ui draws it, single-buffered (set up in init), so write just the changed rows.
static void present_bottom(const uint16_t* img, int y0, int y1) {
  u16 fw, fh;
  GSPGPU_FramebufferFormat fmt = gfxGetScreenFormat(GFX_BOTTOM);
  u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &fw, &fh);
  blit_to(fb, fmt, img, sf2ui::BOT_W, y0, y1);
  gfxFlushBuffers();
  gfxScreenSwapBuffers(GFX_BOTTOM, false);      // single-buffered: re-presents the same buffer, and
                                               // hands its current format to the display
}

// ------------------------------------------------------------------ main hook
// Milliseconds since start-up. (osGetTime() counts from 1900, so its low 32 bits can have the
// top bit set; sf2ui compares times with signed differences against zero-initialised values.)
static u64 ms_base = 0;
static uint32_t ms() { if (!ms_base) ms_base = osGetTime(); return (uint32_t)(osGetTime() - ms_base) + 1000; }
static int keys_logged = 0;
static void key_logged(sf2ui::Key k, bool down) {
  if (keys_logged < 300) { keys_logged++; sf2ctr_log("key %d %s", (int)k, down ? "down" : "up"); }
  sf2_key(k, down);
}
static sf2ui::Host host = {key_logged, click, ms, "sdmc:/3ds/starfleet2/manual.txt", "sdmc:/3ds/starfleet2/officer.txt"};

static void init() {
  gfxSet3D(false);
  gfxSetDoubleBuffering(GFX_BOTTOM, false);    // nothing else draws the touch screen
  click_init();
  sf2ui::init(&host);
  inited = true;
}

bool sf2ctr_owns_top() { return owns_top; }

// SDL draws the top screen from its own thread (citro3d), and a video mode change can repaint it
// after our last frame. While we own the top screen, re-present it for a while afterwards.
static int top_refresh = 0;
void sf2ctr_screen_reset() { top_refresh = 15; if (inited) sf2ui::force_redraw(); }  // (a mode change can also reformat the touch screen)

// Before DOSBox starts: make sure the settings and the game are on the SD card, and say
// exactly what is missing if not (instead of booting a bare DOS prompt).
void sf2ctr_check_install() {
  const char* need[] = {"sdmc:/3ds/starfleet2/starfleet2.conf", "sdmc:/3ds/starfleet2/game/SF2.EXE"};
  char missing[300] = "";
  for (const char* f : need) {
    FILE* t = fopen(f, "rb");
    if (t) fclose(t);
    else { strncat(missing, "\n", sizeof missing - strlen(missing) - 1); strncat(missing, f + 5, sizeof missing - strlen(missing) - 1); }
  }
  if (!missing[0]) return;
  char msg[512];
  snprintf(msg, sizeof msg, "Star Fleet II is not fully installed on the SD card. Missing:%s\n\n"
           "Copy the starfleet2 folder (settings, manual and game files) to /3ds/starfleet2/.", missing);
  sf2ctr_log("%s", msg);
  extern void ctr_sys_error(bool fatal, const char* error);
  ctr_sys_error(true, msg);
}

// ------------------------------------------------------------------ logs (sdmc:/3ds/starfleet2/)
static const char* LOG_PATH = "sdmc:/3ds/starfleet2/debug.log";
static int log_lines = 0;
void sf2ctr_log(const char* fmt, ...) {
  if (log_lines > 2000) return;                 // keep the file small
  char buf[256];
  va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
  svcOutputDebugString(buf, strlen(buf));       // also visible in emulator logs
  if (!log_lines) { mkdir("sdmc:/3ds", 0777); mkdir("sdmc:/3ds/starfleet2", 0777); }
  FILE* f = fopen(LOG_PATH, log_lines ? "a" : "w");
  if (f) { fprintf(f, "%8llu %s\n", (unsigned long long)osGetTime() % 100000000ull, buf); fclose(f); }
  log_lines++;
}

unsigned long long sf2_now_us() { return svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000000); }

unsigned int sf2_emu_ticks = 0;                 // emulated milliseconds (GFX_Events runs once per emulated ms)
extern unsigned int sf2_mixer_underruns;        // counted in mixer.cpp

// Called from GFX_Events() in place of DOSBox-3DS's own input handling.
void sf2ctr_events() {
  if (!inited) { init(); sf2ctr_log("Star Fleet II 3DS started"); }
  sf2_emu_ticks++;

  // Polling HID/APT every emulated millisecond is expensive on the 3DS; 8 ms (125 Hz) is plenty.
  static u64 last_poll = 0;
  u64 tick = svcGetSystemTick();
  if (tick - last_poll < (u64)(SYSCLOCK_ARM11 / 125)) return;
  last_poll = tick;

  SDL_Event e;
  while (SDL_PollEvent(&e)) if (e.type == SDL_QUIT) throw(0);
  if (!aptMainLoop()) throw(0);
  hidScanInput();

  // SDL's event pump also calls hidScanInput(), so hidKeysDown()/hidKeysUp() miss presses:
  // derive presses and releases from the held state ourselves (same fix as the Ultima III port).
  static u32 prev_held = 0;
  u32 held = hidKeysHeld();
  u32 down = held & ~prev_held, up = prev_held & ~held;
  prev_held = held;
  sf2ui::buttons(held, down, up);

  static bool touching = false;
  static touchPosition last_t;
  if (held & KEY_TOUCH) {
    touchPosition t; hidTouchRead(&t);
    sf2ui::touch(t.px, t.py, true);
    touching = true; last_t = t;
  } else if (touching) {
    sf2ui::touch(last_t.px, last_t.py, false);
    touching = false;
  }

  u64 now = osGetTime();

  // performance log every 10 s: emulation speed vs real time, audio underruns, frontend cost
  static u64 perf_t = 0; static unsigned int perf_ticks = 0, perf_under = 0;
  static u64 draw_us = 0; static unsigned int draws = 0;
  if (!perf_t) { perf_t = now; perf_ticks = sf2_emu_ticks; perf_under = sf2_mixer_underruns; }
  else if (now - perf_t >= 10000) {
    unsigned int speed = (unsigned int)((sf2_emu_ticks - perf_ticks) * 100 / (now - perf_t));
    sf2ctr_log("speed %u%% of real time, audio underruns %u, %u redraws, avg %u us",
               speed, sf2_mixer_underruns - perf_under, draws, draws ? (unsigned int)(draw_us / draws) : 0);
    { char st[128]; sf2_debug_state(st, sizeof st); sf2ctr_log("state %s", st); }
    { extern unsigned int sf2_io_ops, sf2_io_disk; static unsigned int o0 = 0, d0 = 0;
      if (sf2_io_ops != o0) sf2ctr_log("files: %u operations, %u on the SD card", sf2_io_ops - o0, sf2_io_disk - d0);
      o0 = sf2_io_ops; d0 = sf2_io_disk; }
    { extern const char* sf2_prof_name[12]; extern unsigned long long sf2_prof_us[12]; extern unsigned int sf2_prof_n[12];
      char line[240]; int n = 0;
      for (int k = 0; k < 12; k++)
        if (sf2_prof_n[k]) n += snprintf(line + n, sizeof line - n, " %s %ux %llums", sf2_prof_name[k], sf2_prof_n[k], sf2_prof_us[k] / 1000);
      if (n) sf2ctr_log("dos:%s", line);
      memset(sf2_prof_us, 0, sizeof sf2_prof_us); memset(sf2_prof_n, 0, sizeof sf2_prof_n); }
    for (auto& p : sf2ui::fit_problems()) sf2ctr_log("FIT: %s", p.c_str());
    perf_t = now; perf_ticks = sf2_emu_ticks; perf_under = sf2_mixer_underruns; draw_us = 0; draws = 0;
  }

  // redraw at most ~30 times a second
  if (now - last_frame < 33) return;
  last_frame = now;
  u64 t0 = svcGetSystemTick();
  sf2ui::TextScreen ts;
  sf2_read_screen(ts);
  bool was_owner = owns_top;
  owns_top = ts.text_mode || sf2ui::modal();
  if (owns_top && !was_owner) top_refresh = 15;
  if (!owns_top && was_owner) sf2_dosbox_redraw();   // e.g. manual closed over the title art
  sf2ui::Redraw rd = sf2ui::frame(ts, top_img, bot_img);
  if (owns_top && (rd.top || top_refresh > 0)) present_top(top_img);
  if (top_refresh > 0) top_refresh--;
  if (rd.bottom) present_bottom(bot_img, rd.by0, rd.by1);
  if (rd.top || rd.bottom) { draws++; draw_us += (svcGetSystemTick() - t0) / (SYSCLOCK_ARM11 / 1000000); }
}
#endif  // SF2_3DS
