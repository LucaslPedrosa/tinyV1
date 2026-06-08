# Developer Tooling

Pinned local tooling:

- Godot: `4.6.3-stable`
- Godot Linux x86_64 SHA-256: `d0bc2113065e481c9c2c2b2c37daa4e8be3fe9e27f0ab9ab0b6096e9a37907f3`
- Godot Windows x86_64 SHA-256: `e39986a178d585ce7ac198fb8de6ea436366dc0cc00e594810c2e3e104c04b90`
- `godot-cpp`: `7e316302f71a7d51bf460fe7e73596764f3a3d94`

All scripts auto-detect Linux or Windows and use the correct Godot binary, platform suffix, and Python venv layout.

Scripts:

- `./dev/setup_dev.sh`: creates local Python/SCons tooling, downloads Godot for the current OS, and checks out pinned `godot-cpp`.
- `./dev/build_debug.sh`: builds the debug GDExtension for the current OS (`platform=linux` or `platform=windows`).
- `./dev/run.sh`: runs the main scene with the OS-appropriate Godot binary.
- `./dev/check_headless.sh`: runs a headless scene load check with the OS-appropriate Godot binary.

## Prerequisites

- **Linux**: Python 3, curl, unzip, sha256sum (or openssl), git, a C++23 compiler
- **Windows** (Git Bash): Python 3, curl, unzip, sha256sum (or openssl), git, MSVC or MinGW C++23 compiler

Ignored local/generated paths:

- `.venv/`
- `tools/godot/`
- `external/godot-cpp/`
- `bin/`
- `.godot/`
