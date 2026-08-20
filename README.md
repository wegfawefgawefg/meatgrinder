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
- Click another node to assault along the route, capturing every hostile stop.
- Control-click the target to rush directly past intermediate positions.
- Right-drag from your HQ to establish a persistent reinforcement rally route.
- Right-click the HQ to cancel its rally route.
- Press H, then click an owned node, to relocate your headquarters.
- WASD or arrow keys pan; the mouse wheel zooms. Escape pauses or backs out.
- F1 opens developer tools; F11 toggles fullscreen.

The simulation advances at a fixed 60 Hz. Rendering interpolates moving troops
between completed simulation states, so presentation remains smooth without
putting frame-rate-dependent behavior into the rules.

## Current first-pass rules

Each side has one headquarters. Every owned node contributes to the discrete
GEN payout shown in the HUD, but recruits appear only at headquarters. A rally
order immediately forwards each new batch toward its target. Friendly arrivals
merge and hostile arrivals resolve against the local garrison. Assault orders
capture intermediate positions; direct orders ignore them. Opposing packets
still pass on roads. The deliberately plain rules live in `src/game.cpp`.

See [docs/DESIGN.md](docs/DESIGN.md), [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md),
and [docs/LEVELS.md](docs/LEVELS.md) for the current rules, ownership, and
provenance boundaries.
