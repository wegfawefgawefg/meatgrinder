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

The default desktop window is a centered, resizable 1280x720 utility window,
which makes tiling window managers such as i3 float it. `--width` and `--height`
can override only the external window; the internal framebuffer remains
1280x720.

Controls:

- Click a friendly stronghold to select it.
- Clicking a different friendly stronghold immediately replaces the selection.
- Drag a box around several friendly strongholds to select them.
- Shift-drag adds boxed strongholds to the selection; Shift-click toggles one stronghold.
- Click another node to assault along the route, capturing every hostile stop.
- Control-click a target to rush directly past intermediate positions; this also
  permits an immediate transfer into a friendly target instead of selecting it.
- Select one or more owned bases, press R, then click a target to establish their rally.
- Hold Control on the target click to make that rally direct instead of assault.
- Pressing R immediately clears the selected sources' previous rallies; Escape leaves them cleared.
- Press C to clear all selected rallies, or press C with no selection and click one base to clear it.
- Press H, then click an owned node, to relocate your headquarters.
- Number keys select a commitment: 1 sends one, 2 sends half, and 3 sends all but one.
- The active commitment applies to immediate orders and is stored in newly assigned rallies.
- Middle-drag or Space-left-drag pans the map; WASD and arrow keys also pan.
- The mouse wheel zooms. Escape pauses or backs out.
- F1 opens developer tools; F11 toggles fullscreen.

Options select weak, normal, or hard AI difficulty independently of each
level's balanced, aggressive, turtle, or swarm personality.

The simulation advances at a fixed 60 Hz. The game rasterizes into a 1280x720
internal framebuffer and nearest-neighbor presents it into the external window.
Rendering interpolates moving troops and the player-controlled camera without
putting frame-rate-dependent behavior into the rules.

## Current first-pass rules

Map letters preserve recovered location kinds: C castle, S stable, P port, A
cannon, F fort, M mine, and + route. They currently share the same production
and combat rules.

Every owned base receives one soldier on the discrete GEN shown in the HUD. A
rally order forwards its stored commitment from that base on every GEN toward
its target and remains visibly drawn until cleared. Friendly arrivals merge and
hostile arrivals resolve against the local garrison. Assault orders capture
intermediate positions; direct orders ignore them. Enemy personalities rotate
between balanced, aggressive, turtle, and small-packet swarm strategies. Their
weighted decisions can attack, expand, wait, or feed rear surplus through
friendly routes to a promising or threatened frontline.

See [docs/DESIGN.md](docs/DESIGN.md), [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md),
and [docs/LEVELS.md](docs/LEVELS.md) for the current rules, ownership, and
provenance boundaries.
