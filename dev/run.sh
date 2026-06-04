#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GODOT_BINARY="${ROOT_DIR}/tools/godot/Godot_v4.6.3-stable_linux.x86_64"

if [ ! -x "${GODOT_BINARY}" ]; then
  printf 'Missing Godot binary. Run ./dev/setup_dev.sh first.\n' >&2
  exit 1
fi

"${GODOT_BINARY}" --path "${ROOT_DIR}" "res://scenes/main.tscn" "$@"
