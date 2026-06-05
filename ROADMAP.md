# tinyV1 Roadmap

This roadmap is meant to prevent the prototype from becoming unmaintainable as we add procedural maps, bots, online play, damage types, heroes, and magic.

## Current State

The game is currently a playable local prototype with:

- Human player vs bot
- Workers, Hoplites, Town Center, Barracks, Gold
- Selection, box selection, same-type selection
- Gathering with mining time
- Barracks construction
- Unit training queues
- Rally points
- Basic combat and building destruction
- Bottom HUD and action panel

The first major refactor moved most gameplay behavior out of `TinyGame` and into `GameSimulation`.

Current split:

- `TinyGame` mostly handles Godot integration, drawing, raw input, texture loading, and HUD callbacks.
- `LocalPlayerState` owns local selection, drag selection, placement preview, and command marker presentation state.
- `BotController` owns temporary bot decision timing and emits bot commands.
- `GameSimulation` owns authoritative match state, unit updates, gathering, combat, production, Barracks lifecycle, command application helpers, cleanup, and win checks.
- `GameTypes.hpp` contains the basic enums and structs.
- `game_constants.hpp` contains shared constants and small math/stat helpers.

Current architectural problem:

- `TinyGame` is no longer the main monolith.
- `GameSimulation` has been split into focused files, but it is still the main gameplay class.
- Player input now uses the first `GameCommand` path for commands, training, placement, and deletion.
- Bot behavior now emits `GameCommand` values for gathering recovery, training, building, and attacks.
- Bot decisions now live in `BotController` and read simulation state through initial read-only queries.
- Simulation internals are still too public and `TinyGame` still has a temporary reference bridge for rendering/HUD.

## Guiding Direction

Before adding major systems, move toward this separation:

- `GameTypes`: plain data types and enums
- `GameSimulation`: authoritative game state and update logic
- `CommandSystem`: player/bot/network commands
- `BotController`: bot decisions using the same commands as players
- `MapGenerator`: map creation from seed and rules
- `TinyGame`: Godot bridge, drawing, input, HUD methods

The long-term rule is:

> Input, bots, and networking should issue commands. They should not directly mutate game state.

## Immediate Priority

Do not add major gameplay features until these are done:

- Add the first `GameCommand` path and route player input through it.
- Convert bot behavior to emit commands instead of mutating state directly.
- Start encapsulating simulation internals behind read-only queries.

Completed immediate-priority items:

- `GameSimulation.cpp` was split into focused files without behavior changes.
- Shared constants were moved into `game_constants.hpp`.
- Local UI/input state was moved into `LocalPlayerState`.
- Barracks/building references were moved from vector indices to `BuildingId`.
- Initial `GameCommand` path exists for player input.
- Bot behavior was routed through the initial `GameCommand` path.
- Bot decisions were extracted from `GameSimulation` into `BotController`.
- Initial read-only queries exist for `BotController`.

## Phase 1: Completed Extraction From TinyGame

Goal: keep current gameplay working while moving core behavior out of Godot-facing code.

Completed tasks:

- Move structs and enums out of `TinyGame` into `GameTypes.hpp`. Done: initial `GameTypes.hpp` extraction exists.
- Create `GameSimulation` as the owner of match state. Done: initial `GameSimulation` owns current state and reset flow through a temporary bridge in `TinyGame`.
- Move state lookup helpers into `GameSimulation`. Done: unit, resource, base, Barracks, enemy target, and nearest-resource lookups now live in `GameSimulation`.
- Move cleanup and win-condition logic into `GameSimulation`. Done: dead unit removal, destroyed Barracks cleanup, and win condition checks now live in `GameSimulation`.
- Move production logic into `GameSimulation`. Done: training progress, unit queueing, and unit spawning now live in `GameSimulation`.
- Move Barracks placement into `GameSimulation`. Done: Barracks creation, cost payment, builder assignment, and placement selection updates now live in `GameSimulation`.
- Move Barracks deletion and selected commands into `GameSimulation`. Done: Barracks refunds/removal, rally point commands, and selected unit move/gather/build/attack command assignment now live in `GameSimulation`.
- Move selection logic into `GameSimulation`. Done: click selection, box selection, double-click select-by-type, selected counts, and first-selected lookup now live in `GameSimulation`.
- Move bot decisions into `GameSimulation`. Done: bot gathering recovery, worker/fighter production, Barracks placement, and attack wave assignment now live in `GameSimulation`.
- Move unit simulation into `GameSimulation`. Done: movement, gathering/returning, Barracks construction, auto-aggro, attacks, combat damage, and per-unit timers now live in `GameSimulation`.

Definition of done:

- Game still builds and runs.
- Existing controls still work.
- `TinyGame` is smaller and mostly acts as Godot-facing glue.
- Core gameplay behavior lives in `GameSimulation` instead of `TinyGame`.

Status: done.

## Phase 2: Break Up GameSimulation Without Behavior Changes

Goal: reduce `GameSimulation.cpp` from a single large file into focused modules while keeping one `GameSimulation` class for now.

Tasks:

- Create `game_constants.hpp` and move shared constants out of duplicated anonymous namespaces. Done.
- Split lookup/query functions into `game_simulation_queries.cpp`. Done.
- Split unit update logic into `game_simulation_update.cpp`. Done.
- Split production and building lifecycle into `game_simulation_production.cpp`. Done.
- Split selected-unit command assignment into `game_simulation_commands.cpp`. Done.
- Split selection logic into `game_simulation_selection.cpp`. Done.
- Split bot decision logic into `game_simulation_bot.cpp` temporarily. Done, then replaced by `BotController`.
- Keep `game_simulation.cpp` focused on construction/reset/high-level tick orchestration. Done: it now contains match reset only.

Definition of done:

- `GameSimulation` behavior is unchanged.
- `./dev/build_debug.sh` passes.
- `./dev/check_headless.sh` passes.
- Each file has a clear responsibility.
- No new gameplay feature is added in this phase.

Status: done.

## Phase 3: Separate Local UI State From Authoritative Simulation

Goal: keep network/replay-relevant state separate from local-only state.

Local-only state to move out of authoritative simulation:

- `is_placing_barracks`
- `is_drag_selecting`
- `drag_has_unit_type_filter`
- `drag_unit_type_filter`
- `drag_start`
- `drag_current`
- `command_marker_position`
- `command_marker_timer`

Selection state decision:

- Short term: keep selection in a `LocalPlayerState` owned by `TinyGame` or a small local controller.
- Long term: authoritative simulation should not need `Unit::selected`.

Tasks:

- Add `local_player_state.hpp` for local selection, drag, placement preview, and command marker data. Done: initial `LocalPlayerState` owns drag, placement preview, and command marker state.
- Move drag selection state from `GameSimulation` to `LocalPlayerState`. Done.
- Move command marker state from `GameSimulation` to `LocalPlayerState`. Done: selected commands now return `CommandFeedback` for local rendering.
- Move placement-preview state from `GameSimulation` to `LocalPlayerState`. Done.
- Decide whether selected units are tracked as unit IDs instead of `Unit::selected` flags. Done: local selection stores selected unit IDs and `Unit::selected` was removed.
- Make rendering/HUD read local selection state separately from simulation data. Done: `TinyGame` uses `LocalPlayerState` for selected units, base selection, Barracks selection, drag, placement, and command marker rendering.

Definition of done:

- A headless simulation tick does not contain mouse drag or command marker state.
- Rendering-only state cannot affect combat, economy, production, or bot decisions.
- Current controls and HUD still work.

Status: done.

## Phase 4: Stable IDs And References

Goal: remove fragile vector indices for persistent gameplay references and prepare commands/networking.

IDs to introduce:

- `PlayerId`
- `UnitId`
- `BuildingId`
- `ResourceId`

Original problem fixed:

- `Unit::target_barracks_index` is a persistent reference into `std::vector<Barracks>`.
- `selected_barracks_index` is also index-based.
- Destroying or deleting Barracks currently requires manual index repair.

Current result:

- Barracks/building references now use `BuildingId`.
- `Unit::target_building_id` replaced `target_barracks_index`.
- `LocalPlayerState::selected_building_id` replaced local Barracks selection by index.
- Barracks still live in `std::vector<Barracks>`, but persistent references no longer depend on vector position.
- `PlayerId`, `UnitId`, `BuildingId`, and `ResourceId` are currently aliases, not strong wrapper types.

Tasks:

- Add strongly named ID aliases or small structs in `game_types.hpp`. Done: initial `PlayerId`, `UnitId`, `BuildingId`, and `ResourceId` aliases exist.
- Add `id` to `Barracks`. Done.
- Add `next_building_id` to `GameSimulation`. Done.
- Replace `target_barracks_index` with `target_building_id`. Done.
- Replace `selected_barracks_index` with selected building ID in local selection state. Done.
- Add lookup helpers by ID. Done: `find_barracks_by_id` exists.
- Update Barracks deletion/destruction to not repair unrelated indices. Done: unit building targets are cleared by `BuildingId` and no index repair is needed.
- Keep vector storage initially; only references change to IDs. Done.

Definition of done:

- Deleting buildings no longer requires fragile index repair.
- Unit orders reference buildings by stable ID.
- Selection references buildings by stable ID.
- Existing gameplay still works.

Status: done for Barracks/building references. Later phases can tighten ID aliases into stronger types if needed.

## Phase 5: Command System

Goal: make player input, bot behavior, and future online play use the same path.

Status: done for the initial command path. Human input and current bot behavior both route gameplay actions through `GameSimulation::apply_command`; deeper validation and command serialization are still future work.

Initial commands:

- `MoveCommand`
- `GatherCommand`
- `AttackCommand`
- `BuildCommand`
- `TrainCommand`
- `SetRallyPointCommand`
- `DeleteBuildingCommand`

Command requirements:

- Commands reference stable IDs, not raw pointers or vector indices.
- Commands are validated before changing state.
- Commands can be issued by human input or bots.
- Commands should be serializable later for networking.

Implementation tasks:

- Add `game_command.hpp` with command structs and a command variant or enum payload. Done: initial enum-based command envelope exists.
- Add `GameSimulation::apply_command(const GameCommand &command)`. Done: initial dispatcher exists.
- Move validation into command application methods.
- Convert keyboard action buttons to create commands. Done.
- Convert mouse right-click commands to create commands. Done.
- Convert placement confirmation to create `BuildCommand`. Done as `PLACE_BARRACKS`.
- Convert Delete key to create `DeleteBuildingCommand`. Done as `DELETE_BARRACKS`.
- Convert bot decisions to create commands. Done for gathering recovery, training, Barracks construction, and attack waves.
- Preserve current behavior before adding new command types. Done.

Definition of done:

- Mouse/keyboard input creates commands.
- Bot decisions create commands.
- Simulation applies commands.
- Current gameplay still works.

## Phase 6: BotController

Goal: move bot strategy out of `GameSimulation` now that bots use the same command path as humans.

Tasks:

- Add `bot_controller.hpp` and `bot_controller.cpp`. Done.
- Move bot decision state and logic out of `GameSimulation`. Done.
- Make bot read simulation state through queries. Done for current bot behavior.
- Make bot output `GameCommand` values. Done in `BotController`.
- Apply bot commands through `GameSimulation::apply_command`. Done in `TinyGame::_process` after `BotController::update`.
- Keep simple current behavior first: gather, train workers, build Barracks, train fighters, attack.

Definition of done:

- `GameSimulation` does not contain bot strategy decisions. Done.
- Bot cannot mutate state except through commands. Done for current behavior.
- Human and bot training/building/attack use the same command validation path. Done for current behavior.

Status: done for initial extraction. Remaining debt: render/HUD code still reads public simulation internals directly until Phase 7 query/encapsulation work.

## Phase 7: Simulation Queries And Encapsulation

Goal: stop exposing mutable simulation internals to rendering, HUD, input, and bot code.

Tasks:

- Reduce public access to `units`, `bases`, `barracks`, and `resources` over time.
- Add read-only query methods for bot decisions. Done for current `BotController` behavior.
- Add read-only query methods for rendering.
- Add read-only query methods for HUD selection/details/action availability.
- Replace `const_cast<TinyGame *>(this)->sim.find_base(...)` patterns with const-safe queries.
- Move HUD action availability checks into simulation or command validation queries.
- Remove the temporary reference bridge in `TinyGame` when practical.

Definition of done:

- External code can inspect state but not freely mutate it.
- `TinyGame` does not hold reference aliases to every simulation field.
- HUD does not duplicate command validation rules.

## Phase 8: Data-Driven Units And Buildings

Goal: avoid hardcoding every stat in C++.

Move these values into data definitions over time:

- HP
- Cost
- Training time
- Build time
- Movement speed
- Attack damage
- Attack range
- Damage type
- Armor/resistance profile
- Sprite paths
- Face/portrait paths

First data targets:

- Worker
- Hoplite
- Town Center
- Barracks
- Gold

Possible format:

- Start with JSON for simplicity.
- Later consider Godot resources if editor integration becomes useful.

Definition of done:

- At least Worker, Hoplite, Town Center, and Barracks stats load from data.
- Balance changes do not require recompiling C++.

## Phase 9: Semi-Random Maps

Goal: generate replayable bot matches without requiring handmade maps first.

Start simple:

- Square map
- 2 to 4 spawn positions
- Guaranteed Gold near each spawn
- Contested Gold near center
- Seeded random placement
- Optional symmetric layout

Avoid initially:

- Complex terrain generation
- Pathfinding-heavy obstacle fields
- Fancy biome systems

Definition of done:

- A seed creates the same map every time.
- Bot and player starts are fair enough for testing.
- Map generation does not depend on rendering.

## Phase 10: Combat System

Goal: support physical and magical damage without special-case chaos.

Initial damage types:

- `Slash`
- `Pierce`
- `Blunt`
- `Fire`
- `Arcane`
- `Lightning`

Initial model:

- Attacks have `base_damage` and `damage_type`.
- Units/buildings have resistance multipliers per damage type.
- Final damage starts simple:

```text
final_damage = base_damage * resistance_multiplier
```

Definition of done:

- Hoplite attack uses a real damage type.
- Targets apply resistances.
- Damage logic is separate from movement/input/rendering.

## Phase 11: Heroes And Abilities

Goal: add heroes without creating a separate one-off system.

Start with one hero:

- More HP than regular units
- One active ability
- Cooldown
- Action panel button

Example:

- Hero: Zeus Champion
- Ability: Lightning Strike
- Target: enemy unit or building
- Damage type: Lightning
- Cooldown: 12 seconds

Definition of done:

- Ability is command-driven.
- Ability uses the damage system.
- HUD can display and trigger ability cooldown.

## Phase 12: Better Bots

Goal: bots should play the same game systems as humans.

Bot improvements:

- Build order states
- Scout/expand decisions
- Army grouping
- Attack timing
- Retreat from danger
- Use hero abilities later

Rule:

> Bots must use commands, not direct state mutation.

Definition of done:

- Bot can execute a basic build order through commands.
- Bot can recover if a Barracks is destroyed.

## Phase 13: Online Host-Authoritative Multiplayer

Goal: support online play without requiring deterministic lockstep at first.

Architecture:

- Host owns simulation.
- Clients send commands.
- Host validates commands.
- Host replicates state.
- Bots run on host.

Do before online:

- Stable IDs
- Command system
- Clear simulation boundary
- Seeded map generation

Definition of done:

- Local host/client can connect.
- Client can issue commands.
- Host simulates and replicates results.

## Anti-Vibecoding Checklist

Before adding a big feature, ask:

- Does this go through commands?
- Does it use stable IDs instead of vector indices?
- Can the bot use the same system?
- Would this work if the player was a network client?
- Are tunable values in data, or hardcoded in C++?
- Is rendering separate from gameplay decisions?
- Can we verify it with `./dev/build_debug.sh` and `./dev/check_headless.sh`?

If the answer is mostly no, refactor before expanding.
