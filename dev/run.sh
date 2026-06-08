#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

detect_platform() {
  local os
  os="$(uname -s)"
  case "${os}" in
    Linux)                  GODOT_BINARY_NAME="Godot_v4.6.3-stable_linux.x86_64";;
    MINGW64_NT-*|MSYS_NT-*|CYGWIN_NT-*) GODOT_BINARY_NAME="Godot_v4.6.3-stable_win64.exe";;
    *) printf 'Unsupported OS: %s\n' "${os}" >&2; exit 1;;
  esac
}
detect_platform

GODOT_BINARY="${ROOT_DIR}/tools/godot/${GODOT_BINARY_NAME}"

if [ ! -f "${GODOT_BINARY}" ]; then
  printf 'Missing Godot binary. Run ./dev/setup_dev.sh first.\n' >&2
  exit 1
fi

"${GODOT_BINARY}" --path "${ROOT_DIR}" "res://scenes/main.tscn" "$@"
