#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

detect_platform() {
  local os
  os="$(uname -s)"
  case "${os}" in
    Linux)                  PLATFORM="linux";; 
    MINGW64_NT-*|MSYS_NT-*|CYGWIN_NT-*) PLATFORM="windows";;
    *) printf 'Unsupported OS: %s\n' "${os}" >&2; exit 1;;
  esac
}
detect_platform

if [ -f ".venv/bin/scons" ]; then
  SCONS_PATH=".venv/bin/scons"
elif [ -f ".venv/Scripts/scons" ]; then
  SCONS_PATH=".venv/Scripts/scons"
else
  SCONS_PATH=""
fi

if [ -z "${SCONS_PATH}" ] || [ ! -d "external/godot-cpp" ]; then
  printf 'Missing local build tooling. Run ./dev/setup_dev.sh first.\n' >&2
  exit 1
fi

"${SCONS_PATH}" platform="${PLATFORM}" target=template_debug arch=x86_64 compiledb=yes compiledb_file=build/compile_commands.json "$@"
