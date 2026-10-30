#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 the starfleet2-3ds authors
# Stage the manual and your GOG game files in sd/3ds/starfleet2/, then upload the whole folder
# to the 3DS over Wi-Fi (ftpd, port 5000) and check every file's size on the 3DS.
#   ./install-3ds.sh <3DS-IP> "/path/to/STAR FLEET II - Krellan Commander Version 2.0"
# (or set THREEDS_IP and pass "" as the address; the IP is shown by ftpd on the 3DS)
set -euo pipefail
IP="${1:-${THREEDS_IP:-}}"; [ -n "$IP" ] || { echo "usage: $0 <3DS-IP> [GOG folder]  (or set THREEDS_IP)" >&2; exit 1; }; GOG="${2:-}"; PORT="${PORT:-5000}"
cd "$(dirname "$0")"
SD=sd/3ds/starfleet2
if [ -n "$GOG" ]; then
  python3 tools/mkmanual.py assets/manual-port.txt "$GOG" "$SD/manual.txt"
  mkdir -p "$SD/game"
  find "$GOG/base" -maxdepth 1 -type f ! -iname 'dosbox*' ! -iname '*.dll' ! -iname '*.conf' ! -iname '*.bat' -exec cp {} "$SD/game/" \;
fi
python3 tools/ftp_upload.py "$IP" "$PORT" sd
