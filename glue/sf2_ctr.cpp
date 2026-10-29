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
// fb[x * 240 + (239 - y)].
static void blit(gfxScreen_t scr, const uint16_t* img, int w, int h) {
  u16 fw, fh;
  u8* fb = gfxGetFramebuffer(scr, GFX_LEFT, &fw, &fh);
  GSPGPU_FramebufferFormat fmt = gfxGetScreenFormat(scr);
  if (fmt == GSP_RGB565_OES) {
    u16* p = (u16*)fb;
    for (int x = 0; x < w; x++) {
      u16* col = p + x * 240;
      for (int y = 0; y < h; y++) col[239 - y] = img[y * w + x];
    }
  } else {                                      // BGR8 (3 bytes per pixel)
    for (int x = 0; x < w; x++)
      for (int y = 0; y < h; y++) {
        uint16_t c = img[y * w + x];
        u8* d = fb + (x * 240 + (239 - y)) * 3;
        d[0] = (u8)((c & 31) << 3); d[1] = (u8)(((c >> 5) & 63) << 2); d[2] = (u8)((c >> 11) << 3);
      }
  }
}
// Write both buffers of the screen's double buffer so whichever one is shown is current.
static void present(gfxScreen_t scr, const uint16_t* img, int w, int h) {
  blit(scr, img, w, h);
  gfxFlushBuffers();
  gfxScreenSwapBuffers(scr, false);
  blit(scr, img, w, h);
  gfxFlushBuffers();
}

// ------------------------------------------------------------------ main hook
static uint32_t ms() { return (uint32_t)osGetTime(); }
static sf2ui::Host host = {sf2_key, click, ms, "sdmc:/3ds/starfleet2/manual.txt", "sdmc:/3ds/starfleet2/officer.txt"};

static void init() {
  gfxSet3D(false);
  click_init();
  sf2ui::init(&host);
  inited = true;
}

bool sf2ctr_owns_top() { return owns_top; }

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
    for (auto& p : sf2ui::fit_problems()) sf2ctr_log("FIT: %s", p.c_str());
    perf_t = now; perf_ticks = sf2_emu_ticks; perf_under = sf2_mixer_underruns; draw_us = 0; draws = 0;
  }

  // redraw at most ~30 times a second
  if (now - last_frame < 33) return;
  last_frame = now;
  u64 t0 = svcGetSystemTick();
  sf2ui::TextScreen ts;
  sf2_read_screen(ts);
  owns_top = ts.text_mode || sf2ui::modal();
  sf2ui::Redraw rd = sf2ui::frame(ts, top_img, bot_img);
  if (rd.top && owns_top) present(GFX_TOP, top_img, sf2ui::TOP_W, sf2ui::TOP_H);
  if (rd.bottom) present(GFX_BOTTOM, bot_img, sf2ui::BOT_W, sf2ui::BOT_H);
  if (rd.top || rd.bottom) { draws++; draw_us += (svcGetSystemTick() - t0) / (SYSCLOCK_ARM11 / 1000000); }
}
#endif  // SF2_3DS
