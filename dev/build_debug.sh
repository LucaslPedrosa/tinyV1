#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

if [ ! -x ".venv/bin/scons" ] || [ ! -d "external/godot-cpp" ]; then
  printf 'Missing local build tooling. Run ./dev/setup_dev.sh first.\n' >&2
  exit 1
fi

".venv/bin/scons" platform=linux target=template_debug arch=x86_64 compiledb=yes compiledb_file=build/compile_commands.json "$@"
