# Architecture

This document describes how the current code is organized and how the modules communicate today. It is not a future-feature roadmap; use `ROADMAP.md` for planned work.

## Current Summary

The project is a Godot 4 scene backed by a C++ GDExtension.

- `scripts/main.gd` owns the Godot camera, bottom HUD scene, HUD texture cache, native `TinyGame` instantiation, and action button wiring.
- `TinyGame` is the native Godot-facing bridge: lifecycle, input dispatch, simulation/bot ownership, renderer ownership, and HUD callbacks.
- `GameRenderer` owns match drawing orchestration for map, resources, buildings, units, selection/command markers, placement preview, and win overlay.
- `GreeceRenderer` owns the current Greek civilization sprite loading and humanoid/building/unit drawing details.
- `asset_contracts.hpp` defines path contracts for humanoids, faces, items, buildings, and world resources.
- `texture_loader.hpp` loads textures through Godot `ResourceLoader` when imported resources exist, with raw PNG fallback for fast asset iteration.
- `HudPresenter` formats HUD strings, portraits, action button labels, icons, and enabled state from simulation/local state.
- `LocalInputController` translates local mouse/keyboard/button actions into `GameCommand` values and local UI state changes.
- `LocalPlayerState` stores local-only UI/input state such as selection, drag selection, placement preview, and command marker.
- `GameCommand` describes player-issued gameplay requests.
- `BotController` owns current bot decision timing and emits bot `GameCommand` values.
- `GameSimulation` stores authoritative match state and applies gameplay rules.
- `GameTypes` contains plain gameplay data structures and enums.
- `game_constants.hpp` contains shared constants and small gameplay/math helpers.

Current state detail:

- `Unit` composes a `GameObject` for identity, owner, transform, and health.
- Unit action state is split across `MovementComponent`, `CombatComponent`, `GatherComponent`, and `BuildComponent`.
- `GatherComponent` uses per-resource `GatherRule` entries, so unit archetypes can support different resource types without adding capability flags.
- `UnitSummary` now exposes `order` for future animation selection in the rendering layer.
- Unit movement uses local steering/avoidance against static obstacles and nearby units; full pathfinding is not implemented yet.
- Unit rendering uses a humanoid contract: every humanoid unit folder has the same eight body-part PNG names plus a standardized face file.
- Pixel-art presentation currently relies on nearest filtering, camera snapping, and render-time texture position snapping.

## Modules

### `scripts/main.gd`

Responsibilities:

- Load `tinyv1.gdextension` if `TinyGame` is not registered yet.
- Instantiate `TinyGame` through `ClassDB`.
- Create and update the `Camera2D` for the playable world.
- Keep camera zoom/panning local to Godot presentation code.
- Own the bottom HUD scene nodes.
- Pull HUD text, portraits, button text, button icons, and button enabled state from `TinyGame`.
- Load/cache HUD portraits and action icons by path returned from `TinyGame`, using Godot resource loading with raw image fallback.
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
- Own one `GameRenderer` instance.
- Handle Godot lifecycle methods: `_ready`, `_process`, `_draw`, `_unhandled_input`.
- Set nearest texture filtering on the native canvas item.
- Load renderer textures through `GameRenderer::load_textures`.
- Delegate drawing to `GameRenderer`.
- Delegate local mouse/keyboard handling to `LocalInputController`, except match restart after victory.
- Expose HUD callbacks to `scripts/main.gd` and delegate HUD data formatting to `HudPresenter`.

Communication:

- Reads simulation render state through read-only render queries.
- Reads/writes local UI state through `LocalPlayerState`.
- Passes read-only simulation/local state to `GameRenderer` for drawing.
- Calls `GameSimulation::apply_command` for player gameplay actions.
- Asks `BotController` for bot commands each frame and submits them to `GameSimulation::apply_command`.

### Rendering

Files:

- `src/rendering/game_renderer.hpp`
- `src/rendering/game_renderer.cpp`
- `src/rendering/greece_renderer.hpp`
- `src/rendering/greece_renderer.cpp`
- `src/rendering/render_constants.hpp`
- `src/rendering/asset_contracts.hpp`
- `src/rendering/texture_loader.hpp`

Responsibilities:

- `GameRenderer` coordinates drawing from simulation render queries and local presentation state.
- `GameRenderer` draws the map, resources, building overlays, HP/progress bars, rally points, command marker, drag rectangle, placement preview, and win overlay.
- `GreeceRenderer` loads and draws the current Greek civilization sprites for Town Center, Barracks, Worker, and Hoplite.
- `GreeceRenderer` owns humanoid pose evaluation, body-part draw order, attachment sockets, and unit sprite sizing.
- `render_constants.hpp` stores sprite sizes, unit radii, HP-bar dimensions, selection-ring offsets, and placement preview sizes.
- `asset_contracts.hpp` defines file/path contracts instead of one-off paths for every texture.
- `texture_loader.hpp` loads imported `Texture2D` resources through `ResourceLoader` when available and falls back to raw `Image` + `ImageTexture` loading for unimported PNGs.

Asset contracts:

- Greek humanoids live under `assets/civilizations/greece/units/humanoid/<unit_folder>/`.
- Every humanoid unit folder must provide the same eight body parts: `1head.png`, `2torso.png`, `3left_shoulder.png`, `4left_arm.png`, `5left_leg.png`, `6right_shoulder.png`, `7right_arm.png`, `8right_leg.png`.
- Humanoid faces use `_<unit_folder>_face.png` in the unit folder.
- Humanoid held items live under `assets/civilizations/greece/units/humanoid/_tools/<category>/<item>.png`.
- Buildings live under `assets/civilizations/greece/b_<building_name>/` and use `<building_name>.png` plus `<building_name>_face.png`.
- World resources live under `assets/world/<resource_name>/` and use `<resource_name>.png` plus optional `<resource_name>_face.png`.

Communication:

- Rendering reads read-only summaries/render models from `GameSimulation`.
- Rendering reads local-only presentation state from `LocalPlayerState`.
- Rendering does not mutate simulation state or decide gameplay behavior.
- HUD/icon portrait paths are produced by `HudPresenter` using the same asset contracts.

### `LocalInputController`

Files:

- `src/local_input_controller.hpp`
- `src/local_input_controller.cpp`

Responsibilities:

- Translate local mouse clicks, drag selection, double-click selection, right-click commands, hotkeys, and HUD action button clicks into `GameCommand` values.
- Mutate `LocalPlayerState` for local-only selection, drag, placement preview, and command marker feedback.
- Return whether an event changed visible/local state so `TinyGame` can redraw.

Communication:

- Reads and writes `LocalPlayerState`.
- Reads `GameSimulation` through query methods.
- Calls `GameSimulation::apply_command` for player-issued commands.
- Does not own Godot node lifecycle, rendering, HUD formatting, or simulation state.

### `HudPresenter`

Files:

- `src/hud_presenter.hpp`
- `src/hud_presenter.cpp`

Responsibilities:

- Format top status/resource text from read-only match state.
- Format selected object name, action text, detail text, portrait color, and portrait path.
- Format action button text, icon path, and enabled state.

Communication:

- Reads `GameSimulation` through read-only queries.
- Reads `LocalPlayerState` for local selection and placement state.
- Does not mutate simulation or local state.

Important temporary coupling:

- `HudPresenter` currently maps unit types to asset folders such as `worker` and `hoplite`.
- `GreeceRenderer` currently maps `UnitType::WORKER` and fighter units to concrete humanoid texture sets.
- Sprite names and folders are still hardcoded contracts in C++ instead of external data files.

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
- `GameObject`
- `OwnerComponent`
- `TransformComponent`
- `HealthComponent`
- `MovementComponent`
- `CombatComponent`
- `GatherRule`
- `GatherComponent`
- `BuildComponent`
- `CommandFeedback`
- `SelectionResult`

Notes:

- IDs are currently aliases over `int32_t`, not strong wrapper types.
- `Barracks` has a stable `BuildingId`.
- `Unit` targets buildings and enemies through componentized stable IDs, not vector indices.
- Unit visual state is still owned by rendering code, but the simulation now exposes enough order/state data for a modular animation layer.

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

- `src/game_simulation/game_simulation.hpp`

Implementation files:

- `src/game_simulation/game_simulation.cpp`
- `src/game_simulation/game_simulation_queries.cpp`
- `src/game_simulation/game_simulation_update.cpp`
- `src/game_simulation/game_simulation_production.cpp`
- `src/game_simulation/game_simulation_commands.cpp`
- `src/game_simulation/game_simulation_selection.cpp`
- `src/game_simulation/game_simulation_lifecycle.cpp`

Responsibilities:

- Own authoritative match state.
- Own resources, bases, Barracks, units, player/bot gold, next IDs, and winner text.
- Reset the match.
- Update unit movement, gathering, returning, construction, attacks, and timers.
- Move units with local avoidance around resources, bases, Barracks, and nearby units.
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
scripts/main.gd creates the Camera2D and configures HUD texture filtering
scripts/main.gd instantiates TinyGame
TinyGame::_ready configures native texture filtering, loads renderer textures, and resets match
```

### Frame Update

`scripts/main.gd::_process` updates camera panning and refreshes HUD labels/icons when `TinyGame` exists.

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

`TinyGame::_draw` delegates to `GameRenderer::draw`, which reads simulation render queries and local state.

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

Rendering state used for drawing:

- asset path contracts
- loaded `Texture2D` references
- sprite sizes and render constants
- humanoid pose and socket offsets

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
- camera position/zoom
- labels
- action buttons
- texture caches

Rendering resource state:

- `GameRenderer` owns world/resource texture references.
- `GreeceRenderer` owns Greek building, humanoid body-part, and held-item texture references.
- `texture_loader.hpp` owns no state; it only resolves a path to a `Texture2D`.

## Communication Rules

Current rules:

- GDScript talks only to `TinyGame`.
- `TinyGame` owns `GameSimulation`, `BotController`, `LocalPlayerState`, and `GameRenderer`.
- `TinyGame` may ask `GameSimulation` to calculate selections.
- `TinyGame` stores selection results locally.
- `TinyGame` asks `GameSimulation` to apply player commands.
- `TinyGame` asks `BotController` for bot commands and submits them to `GameSimulation`.
- `GameSimulation` must not store mouse drag, placement preview, command marker, or visual selection.
- `GameRenderer` and `GreeceRenderer` must only render from summaries/local state; they must not mutate gameplay.
- Asset path contracts belong in rendering/data-facing code, not in simulation code.

Desired discipline for current code:

- Put gameplay mutations in `GameSimulation`, not `TinyGame`.
- Put local UI/input state in `LocalPlayerState`, not `GameSimulation`.
- Put rendering-only logic in `GameRenderer`/`GreeceRenderer` or `TinyGame`, not `GameSimulation`.
- Put Godot scene/HUD node handling in `scripts/main.gd`, not simulation code.

## Known Temporary Couplings

- `GameCommand` is still a thin command envelope over existing simulation methods.
- Bases are still referenced by owner instead of a stable `BuildingId`.
- ID aliases are still plain `int32_t` aliases.
- Stats and costs are still hardcoded in C++.
- Asset contracts are hardcoded in C++ and are not yet data-driven.

## What Not To Do

- Do not add new gameplay rules directly inside `TinyGame`.
- Do not put mouse drag or command marker state into `GameSimulation`.
- Do not reintroduce `Unit::selected`.
- Do not use vector indices as persistent gameplay references.
- Do not make GDScript mutate simulation state directly.
- Do not make bots bypass the same gameplay rules as players.
- Do not make renderers mutate simulation state.
- Do not put gameplay rules into asset contracts or texture loading.
- Do not add major features before the command path exists unless the feature is explicitly experimental.
