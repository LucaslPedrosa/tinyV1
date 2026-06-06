# Architecture

This document describes how the current code is organized and how the modules communicate today. It is not a future-feature roadmap; use `ROADMAP.md` for planned work.

## Current Summary

The project is a Godot 4 scene backed by a C++ GDExtension.

- `scripts/main.gd` owns the Godot HUD scene and instantiates the native `TinyGame` node.
- `TinyGame` is the Godot-facing bridge: input, drawing, texture loading, and HUD callbacks.
- `LocalPlayerState` stores local-only UI/input state such as selection, drag selection, placement preview, and command marker.
- `GameCommand` describes player-issued gameplay requests.
- `BotController` owns current bot decision timing and emits bot `GameCommand` values.
- `GameSimulation` stores authoritative match state and applies gameplay rules.
- `GameTypes` contains plain gameplay data structures and enums.
- `game_constants.hpp` contains shared constants and small gameplay/math helpers.

Current state detail:

- Unit action state is split across `MovementComponent`, `CombatComponent`, `GatherComponent`, and `BuildComponent`.

## Modules

### `scripts/main.gd`

Responsibilities:

- Load `tinyv1.gdextension` if `TinyGame` is not registered yet.
- Instantiate `TinyGame` through `ClassDB`.
- Own the bottom HUD scene nodes.
- Pull HUD text, portraits, button text, button icons, and button enabled state from `TinyGame`.
- Forward action button clicks to `TinyGame::perform_action_button`.

Communication:

- Calls bound C++ methods on `TinyGame` using `game.call(...)`.
- Does not mutate simulation state directly.

### `TinyGame`

Files:

- `src/tiny_game.hpp`
- `src/tiny_game.cpp`

Responsibilities:

- Register methods callable from GDScript.
- Own one `GameSimulation` instance.
- Own one `BotController` instance.
- Own one `LocalPlayerState` instance.
- Handle Godot lifecycle methods: `_ready`, `_process`, `_draw`, `_unhandled_input`.
- Load raw PNG textures with `Image` + `ImageTexture`.
- Draw the map, resources, buildings, units, selection rings, HP bars, rally points, drag rectangle, placement preview, command marker, and win overlay.
- Translate raw mouse/keyboard input into `GameCommand` values and submit them to `GameSimulation`.
- Provide HUD strings and action button data to `scripts/main.gd`.

Communication:

- Reads simulation render state through read-only render queries.
- Reads/writes local UI state through `LocalPlayerState`.
- Calls `GameSimulation::apply_command` for player gameplay actions.
- Asks `BotController` for bot commands each frame and submits them to `GameSimulation::apply_command`.

Important temporary coupling:

- `TinyGame` still owns rendering details and sprite sizing for simulation objects.

### `LocalPlayerState`

File:

- `src/local_player_state.hpp`

Responsibilities:

- Store state that belongs to the local player/UI, not the authoritative simulation.
- Track selected unit IDs.
- Track selected base owner.
- Track selected building ID.
- Track Barracks placement preview.
- Track drag selection state.
- Track command marker position and timer.

Communication:

- `TinyGame` writes this state from input events.
- `TinyGame` reads this state for drawing and HUD.
- `GameSimulation` returns `SelectionResult` and `CommandFeedback`; `TinyGame` applies those results to `LocalPlayerState`.

Rules:

- Local-only state should stay here, not in `GameSimulation`.
- Mouse drag, placement preview, command marker, and visual selection are not authoritative game state.

### `GameTypes`

File:

- `src/game_types.hpp`

Responsibilities:

- Define plain gameplay enums and structs.
- Define ID aliases.
- Define command/selection return structs currently used between modules.

Current IDs:

- `PlayerId`
- `UnitId`
- `BuildingId`
- `ResourceId`

Current data structs:

- `ResourceNode`
- `Base`
- `Barracks`
- `Unit`
- `OwnerComponent`
- `TransformComponent`
- `HealthComponent`
- `MovementComponent`
- `CombatComponent`
- `GatherComponent`
- `BuildComponent`
- `CommandFeedback`
- `SelectionResult`

Notes:

- IDs are currently aliases over `int32_t`, not strong wrapper types.
- `Barracks` has a stable `BuildingId`.
- `Unit` targets buildings and enemies through componentized stable IDs, not vector indices.

### `GameCommand`

File:

- `src/game_command.hpp`

Responsibilities:

- Define the first command envelope used by player input and bot decisions.
- Carry stable IDs and positions from input code to simulation code.
- Return command-side results through `GameCommandResult`.

Current command types:

- `COMMAND_SELECTED_TO`
- `TRAIN_UNIT`
- `PLACE_BARRACKS`
- `DELETE_BARRACKS`

Current result fields:

- `feedback`: local/presentation command marker feedback.
- `placed_building_id`: ID of a successfully placed Barracks.

Notes:

- This is an initial command path, not the final command model.
- Bot decisions live in `BotController` and are routed through commands.

### `BotController`

Files:

- `src/bot_controller.hpp`
- `src/bot_controller.cpp`

Responsibilities:

- Own bot decision timing.
- Inspect current simulation state through read-only query methods for temporary bot decisions.
- Emit `GameCommand` values for bot gathering recovery, training, Barracks construction, and attack waves.

Communication:

- `TinyGame::_process` calls `BotController::update(sim, delta)`.
- `BotController` returns a list of `GameCommand` values.
- `TinyGame` submits those commands to `GameSimulation::apply_command`.

Important temporary coupling:

- `BotController` does not mutate simulation state directly.
- Future query/encapsulation work should continue reducing direct reads in `TinyGame` and HUD code.

### `game_constants.hpp`

Responsibilities:

- Store shared gameplay constants.
- Store small inline helpers used by both rendering and simulation.

Examples:

- Player constants: `PLAYER`, `BOT`.
- Costs: `WORKER_COST`, `FIGHTER_COST`, `BARRACKS_COST`.
- Timings: `WORKER_TRAIN_TIME`, `FIGHTER_TRAIN_TIME`, `BARRACKS_BUILD_TIME`, `GATHER_TIME`.
- Distances/radii: `RESOURCE_RADIUS`, `BASE_RADIUS`, `BARRACKS_RADIUS`, `BUILD_DISTANCE`, `GATHER_DISTANCE`, `RETURN_DISTANCE`.
- Helpers: `distance_to`, `move_toward`, `unit_speed`, `unit_radius`, `unit_attack_range`, `unit_attack_damage`, `formation_offset`.

Notes:

- These are still hardcoded C++ constants.
- Balance/data-driven work belongs in a later phase.

### `GameSimulation`

Header:

- `src/game_simulation.hpp`

Implementation files:

- `src/game_simulation.cpp`
- `src/game_simulation_queries.cpp`
- `src/game_simulation_update.cpp`
- `src/game_simulation_production.cpp`
- `src/game_simulation_commands.cpp`
- `src/game_simulation_selection.cpp`
- `src/game_simulation_lifecycle.cpp`

Responsibilities:

- Own authoritative match state.
- Own resources, bases, Barracks, units, player/bot gold, next IDs, and winner text.
- Reset the match.
- Update unit movement, gathering, returning, construction, attacks, and timers.
- Keep unit action state componentized, with per-action handlers in `game_simulation_update.cpp`.
- Update production queues and spawning.
- Place and delete Barracks.
- Resolve selection queries into `SelectionResult`.
- Apply current right-click style selected-unit commands.
- Apply human- and bot-issued `GameCommand` values.
- Remove dead units and destroyed Barracks.
- Check win condition.

Communication:

- Called through `GameSimulation::apply_command` for human and bot gameplay input.
- Called directly by `TinyGame` for render and HUD-related queries.
- Called internally by update logic and production logic.
- Returns local/presentation results through `SelectionResult` and `CommandFeedback`.

Important temporary coupling:

- The command path is still a thin wrapper over existing simulation methods.

## GameSimulation Files

### `game_simulation.cpp`

Responsibilities:

- Reset match state.
- Initialize map bounds.
- Create starting bases.
- Create starting gold resources.
- Create starting workers.
- Reset unit and building ID counters.

### `game_simulation_queries.cpp`

Responsibilities:

- Find units, resources, bases, and Barracks.
- Resolve Barracks by `BuildingId`.
- Find nearest resource.
- Find enemy units or buildings at a clicked position.
- Find available bot worker.
- Provide read-only query helpers used by `BotController` and player input/HUD code.
- Provide summary queries for units, bases, and buildings.
- Provide render read models for resources, bases, buildings, and units.
- Provide command-availability queries for current player actions.

### `game_simulation_update.cpp`

Responsibilities:

- Update all units each frame.
- Resolve unit orders: `MOVE`, `GATHER`, `RETURN`, `BUILD`, `ATTACK`.
- Handle worker mining and returning gold.
- Handle Barracks construction progress.
- Handle attacks against units, Barracks, and bases.
- Handle fighter auto-aggro.

### `game_simulation_production.cpp`

Responsibilities:

- Update worker and fighter training timers.
- Start training when queue is non-empty.
- Spawn units when training completes.
- Add units to production queues through `train_unit`.
- Place Barracks and assign builder workers.
- Delete Barracks by `BuildingId` and refund gold.

### `game_simulation_commands.cpp`

Responsibilities:

- Apply the current direct right-click command behavior.
- Dispatch initial `GameCommand` values through `GameSimulation::apply_command`.
- Set rally points for selected production buildings.
- Convert selected units and a clicked position into unit orders.
- Return `CommandFeedback` for local command marker drawing.

Current command behavior:

- Right-click resource with workers: gather.
- Right-click unfinished friendly Barracks with workers: build.
- Right-click enemy unit/building/base: attack.
- Right-click empty ground with fighters: attack-move.
- Right-click empty ground with workers: move.
- Right-click while base/Barracks selected: set rally point.

### `game_simulation_selection.cpp`

Responsibilities:

- Calculate click selection.
- Calculate box selection.
- Calculate same-type selection.
- Return `SelectionResult` without storing local selection in simulation state.

Rules:

- Selection result contains selected unit IDs, selected base owner, or selected building ID.
- Selection is local/presentation state after `TinyGame` stores it in `LocalPlayerState`.

### `game_simulation_lifecycle.cpp`

Responsibilities:

- Remove dead units.
- Remove destroyed Barracks.
- Clear unit building targets when a targeted Barracks is removed.
- Check player or bot win condition.

## Runtime Flow

### Startup

```text
Godot loads scenes/main.tscn
scripts/main.gd runs
scripts/main.gd loads tinyv1.gdextension if needed
scripts/main.gd instantiates TinyGame
TinyGame::_ready loads textures and resets match
```

### Frame Update

`TinyGame::_process` runs this order:

```text
if winner exists:
  redraw only

sim.update_units(delta)
local.command_marker_timer -= delta
sim.update_training(delta)
bot.update(sim, delta) -> commands
sim.apply_command(command) for each bot command
sim.remove_dead_units()
sim.remove_destroyed_barracks()
sim.check_win_condition()
queue_redraw()
```

### Drawing

`TinyGame::_draw` reads simulation state and local state.

Simulation state used for drawing:

- map bounds
- resources
- bases
- Barracks
- units
- HP/progress/timers

Local state used for drawing:

- selected unit IDs
- selected base owner
- selected building ID
- command marker
- drag rectangle
- placement preview

### HUD Update

`scripts/main.gd` updates every frame and calls C++ methods on `TinyGame`.

The HUD does not access `GameSimulation` directly. It only talks to `TinyGame`.

## Input Flow

### Left Click

If placing Barracks:

```text
TinyGame creates GameCommandType::PLACE_BARRACKS
TinyGame -> sim.apply_command(command)
if success:
  local selected building = new BuildingId
```

If double-clicking a unit:

```text
TinyGame -> sim.select_all_units_of_type(...)
TinyGame -> local.set_selection(result)
```

Otherwise starts local drag selection.

### Left Release

If drag area is large:

```text
TinyGame -> sim.select_units_in_rect(...)
TinyGame -> local.set_selection(result)
```

If drag area is small:

```text
TinyGame -> sim.select_at(...)
TinyGame -> local.set_selection(result)
```

### Right Click

```text
TinyGame clears placement preview
TinyGame creates GameCommandType::COMMAND_SELECTED_TO
TinyGame -> sim.apply_command(command)
sim mutates unit/building orders
sim returns GameCommandResult with CommandFeedback
TinyGame stores command marker in LocalPlayerState
```

### Keyboard

- `Delete`: cancel placement or delete selected Barracks by `BuildingId`.
- `F`: train fighter from selected Barracks.
- `W`: train worker from selected base.
- `B`: start Barracks placement if worker selected and gold is enough.

## Data Ownership

Authoritative simulation state:

- resources
- bases
- Barracks
- units
- gold amounts
- training queues
- build progress
- HP
- current unit orders
- target IDs
- winner text

Local presentation/input state:

- current selection
- drag selection state
- placement preview state
- command marker state

Godot/HUD state:

- scene nodes
- labels
- action buttons
- texture caches

## Communication Rules

Current rules:

- GDScript talks only to `TinyGame`.
- `TinyGame` owns `GameSimulation` and `LocalPlayerState`.
- `TinyGame` may ask `GameSimulation` to calculate selections.
- `TinyGame` stores selection results locally.
- `TinyGame` asks `GameSimulation` to apply player commands.
- `TinyGame` asks `BotController` for bot commands and submits them to `GameSimulation`.
- `GameSimulation` must not store mouse drag, placement preview, command marker, or visual selection.

Desired discipline for current code:

- Put gameplay mutations in `GameSimulation`, not `TinyGame`.
- Put local UI/input state in `LocalPlayerState`, not `GameSimulation`.
- Put rendering-only logic in `TinyGame`, not `GameSimulation`.
- Put Godot scene/HUD node handling in `scripts/main.gd`, not simulation code.

## Known Temporary Couplings

- `GameCommand` is still a thin command envelope over existing simulation methods.
- Bases are still referenced by owner instead of a stable `BuildingId`.
- ID aliases are still plain `int32_t` aliases.
- Stats and costs are still hardcoded in C++.

## What Not To Do

- Do not add new gameplay rules directly inside `TinyGame`.
- Do not put mouse drag or command marker state into `GameSimulation`.
- Do not reintroduce `Unit::selected`.
- Do not use vector indices as persistent gameplay references.
- Do not make GDScript mutate simulation state directly.
- Do not make bots bypass the same gameplay rules as players.
- Do not add major features before the command path exists unless the feature is explicitly experimental.
