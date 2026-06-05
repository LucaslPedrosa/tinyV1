# AGENTS

Instructions for AI agents working on this repository.

## Read First

Before making non-trivial changes, read these files:

- `ARCHITECTURE.md`: current module responsibilities, runtime flow, data ownership, and communication rules.
- `CPP_GUIDELINES.md`: C++ style, ownership, ID/reference rules, and Godot GDExtension patterns.
- `DESIGN.md`: game direction, prototype scope, and gameplay intent.
- `ROADMAP.md`: current refactor phase, priorities, and known architectural debt.
- `README.md`: setup, controls, and current player-facing behavior.

For dev tooling details, also read:

- `dev/README.md`

## Current Architecture Rules

- Keep gameplay mutations in `GameSimulation`, not `TinyGame`.
- Keep local UI/input state in `LocalPlayerState`, not `GameSimulation`.
- Keep rendering, Godot input, texture loading, and HUD bridge code in `TinyGame` or `scripts/main.gd`.
- GDScript should talk to `TinyGame`, not directly to simulation internals.
- Player input should create `GameCommand` values and submit them through `GameSimulation::apply_command`.
- Bot behavior lives in `BotController` and should stay command-driven; do not move strategy decisions back into `GameSimulation`.
- Prefer read-only `GameSimulation` queries for external systems instead of direct access to simulation vectors/state.
- Do not use vector indices as persistent gameplay references. Use stable IDs such as `UnitId`, `BuildingId`, and `ResourceId`.
- Do not reintroduce authoritative selection flags such as `Unit::selected`; selection is local state.

## Editing Guidelines

- Prefer the smallest correct change.
- Preserve current behavior unless the task explicitly asks for a gameplay change.
- Avoid broad rewrites while the prototype is still being modularized.
- Keep Godot-facing code at the edges when practical.
- Do not add major gameplay features before checking `ROADMAP.md` for the active architectural priority.
- Do not commit generated build outputs such as `.os`, `.so`, `.godot/`, or files under local tool directories.

## Verification

After C++ or scene changes, run:

```sh
./dev/build_debug.sh
./dev/check_headless.sh
```

For manual playtesting, run:

```sh
./dev/run.sh
```

## Git Notes

- The working tree may contain user changes or generated files.
- Do not revert unrelated changes.
- Do not commit unless explicitly asked.
- Avoid staging generated files.
