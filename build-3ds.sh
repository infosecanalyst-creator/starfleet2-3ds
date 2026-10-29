#!/usr/bin/env bash
# Build out/starfleet2.3dsx with devkitARM + libctru + 3ds-sdl (SDL 1.2).
#   devkitPro MSYS2 shell (Windows) or Linux:  ./build-3ds.sh
set -euo pipefail
cd "$(dirname "$0")"
: "${DEVKITPRO:=/opt/devkitpro}"; export DEVKITPRO
export DEVKITARM="${DEVKITARM:-$DEVKITPRO/devkitARM}"
export PATH="$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH"
echo "== toolchain: $("$DEVKITARM/bin/arm-none-eabi-gcc" --version | head -1)"
[ -f "$DEVKITPRO/libctru/lib/libctru.a" ] || { echo "libctru is missing: install 3ds-dev"; exit 1; }
if [ ! -f "$DEVKITPRO/portlibs/3ds/lib/libSDL.a" ]; then
  echo "== SDL 1.2 for 3DS not found; installing 3ds-sdl"
  (pacman -S --noconfirm --needed 3ds-sdl || dkp-pacman -S --noconfirm --needed 3ds-sdl)
fi
echo "== SDL: $(ls "$DEVKITPRO/portlibs/3ds/lib/libSDL.a")"
if command -v python3 >/dev/null 2>&1; then python3 tools/mkfont.py; else echo "== python3 not found: using the generated sf2ui/fonts.h"; fi
JOBS=$(nproc 2>/dev/null || echo 4)
echo "== building with $JOBS jobs"
make -C dosbox -f Makefile.ctr -j"$JOBS"
mkdir -p out && cp dosbox/starfleet2.3dsx out/
ls -la out/starfleet2.3dsx
echo "== BUILD OK"
