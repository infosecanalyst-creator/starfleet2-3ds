#!/usr/bin/env bash
# Build starfleet2.3dsx (Star Fleet II split-cockpit port) with devkitARM + libctru + 3ds-sdl (SDL 1.2).
#   devkitPro shell (Windows MSYS2) or Linux:  ./build-3ds.sh
# Output: out/starfleet2.3dsx
set -euo pipefail
cd "$(dirname "$0")"
: "${DEVKITPRO:=/opt/devkitpro}"; export DEVKITPRO
export DEVKITARM="${DEVKITARM:-$DEVKITPRO/devkitARM}"
[ -x "$DEVKITARM/bin/arm-none-eabi-gcc" ] || { echo "devkitARM not found under $DEVKITPRO"; exit 1; }
[ -f "$DEVKITPRO/portlibs/3ds/lib/libSDL.a" ] || { echo "3ds-sdl (SDL 1.2) is missing: dkp-pacman -S 3ds-sdl"; exit 1; }
python3 tools/mkfont.py
make -C dosbox -f Makefile.ctr -j"$(nproc 2>/dev/null || echo 4)" 3dsx
mkdir -p out && cp dosbox/starfleet2.3dsx out/
echo "Built out/starfleet2.3dsx"
