# tinyV1 Design Notes

## High Concept

tinyV1 is a short-session 2D micro-RTS for 2 to 4 players, using humans or bots. Each player starts near an edge or corner of a square map, gathers resources, builds a small base, trains units, and fights while the playable area becomes more dangerous over time.

The goal is to make a complete RTS match fit into a compact, intense session.

Target feeling:

> RTS economy + mythological civilizations + battle royale pressure + 10 minute matches.

## Player Count

- Minimum players: 2
- Maximum players: 4
- Empty slots can be filled by bots
- First prototype target: 1 human vs 1 bot

## Theme And Civilizations

The long-term civilization direction is mythological, not generic fantasy.

Example civilization families:

- Greeks: Zeus, Poseidon, Hades
- Egyptians: Ra, Horus, Anubis, Isis, Set
- Norse: Odin, Thor, Loki, Freyja
- Other pantheons can be added later if the core game works

Each civilization or god should eventually have:

- One economic identity
- One military identity
- One special power or passive
- One unique unit or building
- One weakness

For the first playable version, use only one generic starter civilization. The goal is to prove the game loop before adding multiple factions.

## Core Match Loop

1. Players enter a lobby.
2. Players choose a civilization or god. In the first prototype this can be fixed.
3. Players spawn near map edges or corners.
4. Workers gather resources from nearby nodes.
5. Players build basic structures.
6. Players train combat units.
7. Players fight for center resources and survival.
8. The map border becomes dangerous and pushes everyone inward.
9. The winner is the last player alive or the last main base standing.

## Initial Resource Model

Start with one resource only.

Possible names:

- Favor
- Tribute
- Crystal
- Essence

Recommendation for prototype: use `Essence` as a neutral resource name until civilization-specific economy is added.

Later versions can add more resources, such as food, stone, gold, or favor, but the first version should avoid economy complexity.

## Initial Units

Prototype units:

- Worker: gathers resources, builds structures, repairs buildings
- Fighter: cheap melee combat unit

Later units:

- Ranged unit
- Heavy unit
- Scout
- Mythic unique unit per civilization or god

## Initial Buildings

Prototype buildings:

- Main Base: player survival anchor and worker production
- Resource Dropoff: optional if resources are far from the main base
- Barracks: trains fighters

Later buildings:

- Tower
- Wall or gate
- Temple
- God-specific unique building

## Map Pressure Mechanic

The game is called tinyV1 because the match space should become smaller and more dangerous.

Preferred mechanic:

- A dangerous border slowly advances from the outside of the square map toward the center.
- Units and buildings inside the danger zone take damage over time.
- The border encourages expansion, scouting, center control, and final fights.

Theme options for the border:

- Divine storm
- Void fog
- World collapse
- Wrath of the gods

Prototype name: `Divine Storm`.

## Win Conditions

First prototype:

- Destroy the enemy Main Base.

Later options:

- Last player alive
- Hold center shrine for a fixed time
- Sudden death when the storm reaches the final circle
- Score victory after a time limit

## Controls

Use classic RTS controls, simplified where possible.

Expected controls:

- Left click selects a unit or building
- Drag box selects multiple units
- Right click issues contextual commands: move, attack, gather
- Build menu from worker selection
- Production menu from building selection

## Bots

Bots should run on the host simulation.

First bot behavior:

- Gather resources
- Build barracks
- Train fighters
- Attack the enemy base when it has enough units

Later bot behavior:

- Scout
- Expand toward center
- Retreat from storm
- Choose enemy targets
- Use civilization powers

## Online Approach

Use a host-authoritative model first, not true fully decentralized P2P.

Reasoning:

- Easier to build and debug
- Good enough for friends/private matches
- Bots can run on the host
- Avoids deterministic lockstep complexity at the start

Networking model:

- One player hosts the match.
- Other players connect as clients.
- Clients send commands such as move, gather, build, train, and attack.
- Host validates commands and owns the game state.
- Host replicates relevant state to clients.

Long-term competitive play may require dedicated servers, but that is not an MVP requirement.

## Engine And Language

Engine target:

- Godot 4.x

Preferred language direction:

- C++ through Godot GDExtension for core gameplay systems if desired
- GDScript can still be useful for UI, glue code, and fast iteration

Practical note:

- Building everything in C++ is possible, but Godot C++ development has more setup than GDScript.
- A pragmatic approach is to prototype gameplay quickly, then move performance-critical or stable systems to C++.

## Major Systems

- GameManager: match state, players, win conditions
- PlayerState: resources, civilization, owned units and buildings
- Unit: health, movement, commands, combat
- Building: health, production, construction state
- ResourceNode: resource amount and gathering behavior
- CommandSystem: move, attack, gather, build, train
- BorderSystem: shrinking dangerous border
- BotController: AI decisions
- NetworkManager: host, join, command replication, state sync
- SelectionController: mouse selection and command targeting

## MVP Roadmap

### MVP 1: Local Core Loop

- Godot project boots
- Square 2D map
- 1 human player and 1 bot
- Main base, workers, one resource, barracks, fighters
- Gather, build, train, attack
- Destroy enemy base to win

### MVP 2: Arena Pressure

- Add Divine Storm border
- Damage units and buildings outside the safe area
- Tune match duration toward 10 minutes

### MVP 3: 2 To 4 Players

- Add up to 4 spawn positions
- Add bot slots
- Add basic lobby configuration locally

### MVP 4: Online Host/Join

- Host-authoritative multiplayer
- Clients send commands to host
- Host replicates state
- Bots run on host

### MVP 5: Mythological Civilizations

- Add Greek god prototype, likely Zeus first
- Add Egyptian god prototype later
- Add unique powers, units, or buildings gradually

## First Implementation Target

The first playable target is:

> A local 1v1 prototype where both sides use the same generic civilization. Each player starts with a Main Base and workers, gathers Essence, builds a Barracks, trains Fighters, and wins by destroying the enemy Main Base.
