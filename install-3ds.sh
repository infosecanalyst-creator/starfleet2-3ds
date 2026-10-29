#!/usr/bin/env bash
# Copy the app, settings, manual and your GOG game files to the 3DS over Wi-Fi (ftpd).
#   ./install-3ds.sh 3DS-IP /path/to/GOG/STAR\ FLEET\ II...   (ftpd listens on port 5000)
# Uploads:  /3ds/starfleet2/starfleet2.3dsx, starfleet2.conf, manual.txt
#           /3ds/starfleet2/game/  <- the contents of the GOG install's "base" folder
#           (DOSBox's own Windows files are skipped)
set -euo pipefail
IP="${1:?3DS address}"; GOG="${2:?GOG install folder}"; PORT="${PORT:-5000}"
URL="ftp://$IP:$PORT"
cd "$(dirname "$0")"
put() { curl -sS --ftp-create-dirs -T "$1" "$URL$2" && echo "  $2"; }
python3 tools/mkmanual.py assets/manual-port.txt "$GOG" out/manual.txt
put out/starfleet2.3dsx /3ds/starfleet2/starfleet2.3dsx
put sd/3ds/starfleet2/starfleet2.conf /3ds/starfleet2/starfleet2.conf
put out/manual.txt /3ds/starfleet2/manual.txt
find "$GOG/base" -maxdepth 1 -type f ! -iname 'dosbox*' ! -iname '*.dll' ! -iname '*.conf' ! -iname '*.bat' | while read -r f; do
  put "$f" "/3ds/starfleet2/game/$(basename "$f")"
done
echo "Done. Start Star Fleet II from the Homebrew Launcher."
