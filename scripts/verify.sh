#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d /tmp/esp32-smart-light-controller-verify.XXXXXX)"
PYCACHE="$(mktemp -d /tmp/esp32-smart-light-controller-pycache.XXXXXX)"
cleanup() { rm -rf -- "$WORK" "$PYCACHE"; }
trap cleanup EXIT

if git -C "$ROOT" rev-parse --verify HEAD >/dev/null 2>&1; then
  git -C "$ROOT" archive HEAD | tar -x -C "$WORK"
else
  tar -C "$ROOT" --exclude=.git --exclude=.pio --exclude=.vscode --exclude=.idea \
    --exclude=.vs --exclude=build --exclude=dist --exclude=__pycache__ -cf - . | tar -x -C "$WORK"
fi

export PYTHONPYCACHEPREFIX="$PYCACHE"
python3 "$WORK/scripts/secret_scan.py" --root "$WORK"
python3 "$WORK/scripts/check_repo.py" --root "$WORK"
(cd "$WORK" && python3 -m unittest discover -s tests -p 'test_*.py' -v)
pio run -d "$WORK/firmware" -e esp32dev
python3 "$WORK/scripts/secret_scan.py" --root "$WORK"
python3 "$WORK/scripts/check_repo.py" --root "$WORK"
echo 'Verification: PASS'
