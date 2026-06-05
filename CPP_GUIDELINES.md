# Modern C++ Guidelines For tinyV1

This project uses C++ through Godot GDExtension. The goal is to write clear, modern, boring C++ that is easy to change as the game grows.

The style target is practical modern C++: value types, clear ownership, small functions, explicit state, strong types where useful, and minimal cleverness.

## Core Principles

- Prefer simple code over clever code.
- Prefer value types for simulation data.
- Prefer explicit ownership over implicit ownership.
- Prefer small data structures with clear invariants.
- Prefer standard library facilities over hand-written utilities.
- Prefer compile-time checks and strong types where they reduce bugs.
- Keep Godot-facing code at the edges when possible.

## Language Baseline

Use modern C++ features when they make the code clearer:

- `enum class`
- `constexpr`
- range-based `for`
- structured bindings when useful
- `std::optional`
- `std::variant` only when it genuinely models alternatives well
- `std::span` when passing non-owning contiguous views, if available
- `std::unique_ptr` for unique ownership
- `std::vector` for contiguous storage

Avoid features that add complexity without clear value:

- deep template metaprogramming
- macros for regular logic
- global mutable state
- inheritance-heavy gameplay hierarchies
- raw owning pointers

## Ownership

Default ownership rules:

- Simulation objects are owned by the simulation.
- Godot nodes/resources are owned by Godot or `Ref<T>`.
- Do not store raw owning pointers.
- Raw pointers are allowed as temporary non-owning references only when lifetime is obvious.
- Prefer IDs for persistent references between game objects.

Good:

```cpp
Unit *unit = find_unit(unit_id); // temporary lookup
if (unit == nullptr) {
    return;
}
```

Better long-term:

```cpp
UnitId target_id;
```

Avoid:

```cpp
Unit *target; // stored long-term across frames
```

## IDs Instead Of Indices

Do not use vector indices as persistent gameplay references.

Bad long-term:

```cpp
int32_t target_barracks_index;
```

Preferred:

```cpp
BuildingId target_building_id;
```

Reason:

- Vector indices change when objects are erased.
- IDs remain stable.
- IDs serialize better for networking.
- IDs are safer for bots and commands.

## Strong Types

Use small wrapper types for IDs once the simulation is split:

```cpp
struct UnitId {
    int32_t value = -1;
};

struct PlayerId {
    int32_t value = -1;
};
```

This prevents accidentally passing a unit ID where a player ID was expected.

## Const Correctness

Use `const` aggressively for read-only functions.

Good:

```cpp
const Unit *find_unit(UnitId id) const;
Unit *find_unit(UnitId id);
```

Avoid `const_cast` except as a short-term bridge during refactors. If a `const_cast` appears, it is usually a sign the API should be split into const and mutable overloads.

## Function Design

Prefer functions that do one thing.

Good:

```cpp
void update_training(double delta);
void update_units(double delta);
void remove_dead_units();
```

Avoid functions that mix unrelated responsibilities:

```cpp
void update_everything_and_draw_and_handle_input();
```

When a function grows because it has many cases, consider introducing a command or system boundary.

## Commands

Input should create commands. Commands should mutate simulation state after validation.

Example direction:

```cpp
struct MoveCommand {
    PlayerId player_id;
    std::vector<UnitId> units;
    godot::Vector2 target;
};
```

Command handlers should:

- validate ownership
- validate target exists when needed
- validate resources/costs
- mutate simulation only after validation

## Data-Driven Values

Avoid growing hardcoded constants forever.

Acceptable during prototype:

```cpp
constexpr float WORKER_TRAIN_TIME = 4.0f;
```

Long-term direction:

```text
data/units/worker.json
data/units/hoplite.json
data/buildings/barracks.json
```

Gameplay values that should become data:

- HP
- speed
- damage
- attack range
- damage type
- resistance profile
- build time
- train time
- cost
- sprite paths
- portrait paths

## Godot Boundary

Keep Godot-specific rendering and input code near the Godot-facing layer.

Prefer:

- `TinyGame` handles Godot input, drawing, and HUD calls.
- `GameSimulation` handles gameplay state.
- Simulation code should not need to know about UI node paths.

Avoid:

- gameplay logic inside HUD code
- network logic inside rendering code
- bots mutating arbitrary vectors directly

## Error Handling

For gameplay validation, prefer early returns.

Good:

```cpp
if (unit == nullptr) {
    return;
}
if (unit->owner != command.player_id) {
    return;
}
```

For development-only impossible states, use assertions if available and appropriate.

Do not crash the game for normal invalid player commands. Invalid commands should simply be rejected.

## Naming

Current code follows Godot-style parameter prefixes in places, such as `p_delta` and `p_owner`. Keep consistency inside existing files.

Suggested conventions:

- Types: `PascalCase`
- Functions: `snake_case` or existing Godot-compatible style, but be consistent per file
- Constants: `UPPER_SNAKE_CASE` or `constexpr` names already used in file
- Members: clear names over prefixes

Examples:

```cpp
constexpr float WORKER_TRAIN_TIME = 4.0f;
void update_training(double p_delta);
int32_t player_gold = 0;
```

## Includes

Keep includes minimal.

In headers:

- Prefer forward declarations when possible.
- Include only what is required for member fields or base classes.

In `.cpp` files:

- Include concrete dependencies.

Avoid using broad includes to hide dependency problems.

## Allocation And Performance

Do not optimize prematurely, but avoid obvious per-frame waste.

Good:

- Store simulation objects in vectors.
- Reuse loaded textures.
- Avoid loading resources every frame.
- Avoid unnecessary string building inside hot loops.

Acceptable for now:

- Small vector scans for prototype object lookup.

Improve later:

- ID lookup tables
- spatial queries
- pathfinding structures

## Rendering

Rendering should read state, not decide gameplay.

Good:

```cpp
draw_units();
draw_buildings();
draw_hud_markers();
```

Avoid:

```cpp
if (drawing_unit) {
    decide_attack_target();
}
```

## Networking Readiness

Write gameplay code as if a client will eventually send commands to a host.

This means:

- avoid direct UI-to-state mutations long-term
- keep commands serializable
- validate all commands
- avoid relying on local-only object addresses
- use seeds for procedural generation

## Refactoring Rule

Refactor in small, behavior-preserving steps.

Preferred process:

1. Move types out.
2. Build and check.
3. Move one system out.
4. Build and check.
5. Introduce commands for one action.
6. Build and check.

Avoid giant rewrites that change architecture and gameplay at the same time.

Always run:

```bash
./dev/build_debug.sh
./dev/check_headless.sh
```

## Practical Checklist

Before merging a C++ gameplay change:

- Does it compile cleanly?
- Does headless scene loading pass?
- Did it avoid tracking generated files?
- Is persistent state referenced by stable IDs or at least prepared for that refactor?
- Are resources/costs validated before mutation?
- Could a bot use the same action?
- Could this later become a network command?
- Is rendering separate from gameplay decisions?
