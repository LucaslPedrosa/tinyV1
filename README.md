# tinyV1

tinyV1 is a 2D micro-RTS prototype built with Godot 4 and C++ GDExtension.

See `DESIGN.md` for the current game design notes and MVP roadmap.

## Local Setup

Local tools and dependencies are project-local and ignored by Git. For a fresh checkout, run:

```bash
./dev/setup_dev.sh
```

This creates `.venv/`, downloads Godot to `tools/godot/`, installs SCons, and checks out the pinned `godot-cpp` dependency.

Pinned versions are documented in `dev/README.md`.

Run the editor:

```bash
./tools/godot/Godot_v4.6.3-stable_linux.x86_64 --path .
```

Run the main scene:

```bash
./dev/run.sh
```

Run a headless project load check:

```bash
./dev/check_headless.sh
```

## C++ Build

The project uses Godot GDExtension for native C++ code.

Local tooling and dependencies are ignored by Git:

- `.venv/`
- `external/godot-cpp/`
- `bin/`

Build the debug GDExtension library:

```bash
./dev/build_debug.sh
```

The built library should appear at:

```text
bin/libtinyv1.linux.template_debug.x86_64.so
```

## Team Workflow

Typical loop:

```bash
./dev/setup_dev.sh
./dev/build_debug.sh
./dev/run.sh
```

Before sharing changes, run:

```bash
./dev/build_debug.sh
./dev/check_headless.sh
```

Do not commit local/generated paths such as `.venv/`, `tools/godot/`, `external/godot-cpp/`, `bin/`, or `.godot/`.

## Prototype Controls

The current prototype is a local 1v1 test against a simple bot.

- Left click a blue worker/fighter to select it.
- Left click and drag a box around blue units to select multiple units.
- Drag starting on a blue unit filters the box selection to that unit type.
- Double click a blue unit to select all owned units of that type.
- Left click the blue base to select the base.
- Select a worker and click `Barracks` or press `B`, then left click the map to place it.
- Right click while placing a Barracks to cancel placement.
- Right click with selected units to move in a small formation.
- Right click an Essence node with workers selected to gather.
- Right click ground with fighters selected to attack-move in formation.
- Right click the enemy base with fighters selected to attack it.
- Press `W` with the base selected to train a worker.
- Press `F` with a Barracks selected to train a fighter.
- Click the base action buttons in the center HUD to train units.
- Destroy the red base to win.
- If the match ends, press any key to restart.

Current prototype pieces:

- Blue human player
- Red bot player
- Main bases
- Barracks built by workers
- Workers
- Fighters
- Essence nodes
- Greece sprites for Worker, Hoplite, Town Center, Barracks, and Goldmine
- Bottom HUD with Food, Wood, and Gold resource display
- Selection HUD with actions, portrait placeholder, name, and details
- Clickable base action buttons for training workers and fighters
- Main Base trains workers, Barracks trains fighters
- Playable map area reserved above the bottom HUD
- Worker gathering and return behavior
- Bot economy, unit training, and attack waves
