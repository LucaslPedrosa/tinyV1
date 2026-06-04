#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GODOT_VERSION="4.6.3-stable"
GODOT_BINARY="Godot_v${GODOT_VERSION}_linux.x86_64"
GODOT_ZIP="${GODOT_BINARY}.zip"
GODOT_URL="https://github.com/godotengine/godot/releases/download/${GODOT_VERSION}/${GODOT_ZIP}"
GODOT_SHA256="d0bc2113065e481c9c2c2b2c37daa4e8be3fe9e27f0ab9ab0b6096e9a37907f3"
GODOT_CPP_COMMIT="7e316302f71a7d51bf460fe7e73596764f3a3d94"

cd "${ROOT_DIR}"

if [ ! -d ".venv" ]; then
  python3 -m venv ".venv"
fi

".venv/bin/python" -m pip install --upgrade pip scons

mkdir -p "tools/godot" "external"

if [ ! -x "tools/godot/${GODOT_BINARY}" ]; then
  curl -L "${GODOT_URL}" -o "tools/godot/${GODOT_ZIP}"
  ACTUAL_SHA256="$(sha256sum "tools/godot/${GODOT_ZIP}" | cut -d ' ' -f 1)"
  if [ "${ACTUAL_SHA256}" != "${GODOT_SHA256}" ]; then
    printf 'Godot checksum mismatch. Expected %s, got %s\n' "${GODOT_SHA256}" "${ACTUAL_SHA256}" >&2
    exit 1
  fi
  unzip -o "tools/godot/${GODOT_ZIP}" -d "tools/godot"
  chmod +x "tools/godot/${GODOT_BINARY}"
fi

if [ ! -d "external/godot-cpp/.git" ]; then
  git clone "https://github.com/godotengine/godot-cpp.git" "external/godot-cpp"
fi

git -C "external/godot-cpp" fetch --depth 1 origin "${GODOT_CPP_COMMIT}"
git -C "external/godot-cpp" checkout --detach "${GODOT_CPP_COMMIT}"

printf 'Dev setup complete.\n'
printf 'Godot: tools/godot/%s\n' "${GODOT_BINARY}"
printf 'godot-cpp: %s\n' "${GODOT_CPP_COMMIT}"
