# Meatgrinder

Meatgrinder is a small real-time territorial tactics game. Select one or more
strongholds, send troops through the route graph, take ground, and survive the
counterattack. It begins with the useful mechanical shape of *Hills and Rivers
Remain*, then intentionally develops its own rules and presentation.

This repository is private and experimental. It contains no original game art,
audio, dialogue, executable code, or menu design. The first campaign preserves
only recovered main-story battle topology: node placement, links, base kinds,
starting sides, and garrisons. Background tiles are generated Meatgrinder data.

## Build and play

SDL3 must be installed, or CMake will fetch it. ImGui and nlohmann/json are
found locally when available and fetched otherwise.

```sh
./scripts/run.sh
./scripts/test.sh
```

Controls:

- Click a friendly stronghold to select it.
- Drag a box around several friendly strongholds to select them.
- Click another node to send half the soldiers from every selection.
- Shift-click a friendly stronghold to spend 20 soldiers promoting it.
- Right-click clears the selection. Escape pauses or backs out.
- F1 opens developer tools; F11 toggles fullscreen.

The simulation advances at a fixed 60 Hz. Rendering interpolates moving troops
between completed simulation states, so presentation remains smooth without
putting frame-rate-dependent behavior into the rules.

## Current first-pass rules

Every owned node produces locally. Promotion increases that node's production
and defense; owning more castles does **not** globally strengthen every soldier.
Friendly arrivals merge. Hostile arrivals resolve against the local garrison,
with a small defender advantage from promotion. Opposing packets pass on roads.
The deliberately plain rule set lives in `src/game.cpp` and is meant to change.

See [docs/DESIGN.md](docs/DESIGN.md), [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md),
and [docs/LEVELS.md](docs/LEVELS.md) for the current rules, ownership, and
provenance boundaries.
