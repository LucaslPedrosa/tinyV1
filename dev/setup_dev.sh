#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GODOT_VERSION="4.6.3-stable"
GODOT_CPP_COMMIT="7e316302f71a7d51bf460fe7e73596764f3a3d94"

detect_platform() {
  local os
  os="$(uname -s)"
  case "${os}" in
    Linux)
      PLATFORM="linux"
      GODOT_BINARY_NAME="Godot_v${GODOT_VERSION}_linux.x86_64"
      GODOT_SHA256="d0bc2113065e481c9c2c2b2c37daa4e8be3fe9e27f0ab9ab0b6096e9a37907f3"
      ;;
    MINGW64_NT-*|MSYS_NT-*|CYGWIN_NT-*)
      PLATFORM="windows"
      GODOT_BINARY_NAME="Godot_v${GODOT_VERSION}_win64.exe"
      GODOT_SHA256="e39986a178d585ce7ac198fb8de6ea436366dc0cc00e594810c2e3e104c04b90"
      ;;
    *)
      printf 'Unsupported OS: %s\n' "${os}" >&2
      exit 1
      ;;
  esac
  GODOT_ZIP="${GODOT_BINARY_NAME}.zip"
  GODOT_URL="https://github.com/godotengine/godot/releases/download/${GODOT_VERSION}/${GODOT_ZIP}"
}

compute_sha256() {
  if command -v sha256sum > /dev/null 2>&1; then
    sha256sum "$1" | cut -d ' ' -f 1
  elif command -v openssl > /dev/null 2>&1; then
    openssl dgst -sha256 "$1" | cut -d ' ' -f 2
  else
    printf 'No sha256 tool found (sha256sum or openssl required).\n' >&2
    exit 1
  fi
}

detect_platform

cd "${ROOT_DIR}"

PYTHON_BIN="python3"
if ! command -v "${PYTHON_BIN}" > /dev/null 2>&1; then
  PYTHON_BIN="python"
fi

if [ ! -d ".venv" ]; then
  "${PYTHON_BIN}" -m venv ".venv"
fi

# Detect venv layout (bin/ on Linux and MSYS2, Scripts/ on native Windows Python)
if [ -f ".venv/bin/python" ]; then
  VENV_PYTHON=".venv/bin/python"
elif [ -f ".venv/Scripts/python" ]; then
  VENV_PYTHON=".venv/Scripts/python"
else
  printf 'Could not find python in .venv. Is the virtualenv correct?\n' >&2
  exit 1
fi

"${VENV_PYTHON}" -m pip install --upgrade pip scons

mkdir -p "tools/godot" "external"

if [ ! -f "tools/godot/${GODOT_BINARY_NAME}" ]; then
  curl -L "${GODOT_URL}" -o "tools/godot/${GODOT_ZIP}"
  ACTUAL_SHA256="$(compute_sha256 "tools/godot/${GODOT_ZIP}")"
  if [ "${ACTUAL_SHA256}" != "${GODOT_SHA256}" ]; then
    printf 'Godot checksum mismatch. Expected %s, got %s\n' "${GODOT_SHA256}" "${ACTUAL_SHA256}" >&2
    exit 1
  fi
  unzip -o "tools/godot/${GODOT_ZIP}" -d "tools/godot"
  if [ "${PLATFORM}" = "linux" ]; then
    chmod +x "tools/godot/${GODOT_BINARY_NAME}"
  fi
fi

if [ ! -d "external/godot-cpp/.git" ]; then
  git clone "https://github.com/godotengine/godot-cpp.git" "external/godot-cpp"
fi

git -C "external/godot-cpp" fetch --depth 1 origin "${GODOT_CPP_COMMIT}"
git -C "external/godot-cpp" checkout --detach "${GODOT_CPP_COMMIT}"

printf 'Dev setup complete. Platform: %s\n' "${PLATFORM}"
printf 'Godot: tools/godot/%s\n' "${GODOT_BINARY_NAME}"
printf 'godot-cpp: %s\n' "${GODOT_CPP_COMMIT}"
