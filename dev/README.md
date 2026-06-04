# Developer Tooling

Pinned local tooling:

- Godot: `4.6.3-stable`
- Godot Linux x86_64 SHA-256: `d0bc2113065e481c9c2c2b2c37daa4e8be3fe9e27f0ab9ab0b6096e9a37907f3`
- `godot-cpp`: `7e316302f71a7d51bf460fe7e73596764f3a3d94`

Scripts:

- `./dev/setup_dev.sh`: creates local Python/SCons tooling, downloads Godot, and checks out pinned `godot-cpp`.
- `./dev/build_debug.sh`: builds the Linux debug GDExtension.
- `./dev/run.sh`: runs the main scene.
- `./dev/check_headless.sh`: runs a headless scene load check.

Ignored local/generated paths:

- `.venv/`
- `tools/godot/`
- `external/godot-cpp/`
- `bin/`
- `.godot/`
