# Architecture

Meatgrinder follows the direct architecture used by Adventures with Chickens
Remastered: plain state, authored data loaded up front, one visible fixed-step
loop, and domain functions that mutate the state they receive.

`State` owns worlds, levels, results, current selection, input, options, and the
mutable match. `Level` and `World` are immutable authored content. `Match` is
created from a level and contains changing garrisons, armies, cannon shots, gold
shipments, AI clocks, and statistics.
There is no scene graph, event bus, ECS, service locator, or hidden scheduler.

The host pumps SDL events, executes `step()` in 1/60-second increments, then
calls `draw()` with the remaining accumulator fraction. Armies and the camera
retain previous and current states, and only drawing interpolates between them.
Simulation decisions therefore remain deterministic for a given command
sequence.

Drawing always targets a 1280x720 RGBA framebuffer. The host then copies that
texture to the largest aspect-correct external rectangle using nearest-neighbor
sampling. Mouse coordinates apply the inverse presentation transform. Thin
world geometry is rasterized at two or three internal pixels before scaling, so
routes and selection boxes survive non-native window sizes.

Modes are explicit: main menu, options, world select/unlock/transition, level
select/transition/card, playing, pause, defeat, score, and campaign complete.
Selection changes use a two-sided horizontal wipe which swaps scenes at full
cover, and all UI motion evaluates at render-interpolated time. ImGui is a
developer overlay only.

Files remain split by concrete domain:

- `level.*`: JSON decoding and authored validation.
- `campaign.*`: unlock rules, map navigation, and result updates.
- `progress.*`: small ID-keyed best score/time save file.
- `mechanics.*`: ownership-gated routes, cannon fire, and mine shipments.
- `camera.*`: tile-world framing, manual movement, zoom, and interpolation.
- `ai.*`: weighted enemy decisions, soft objectives, threats, and supply movement.
- `game.*`: campaign/match creation, commands, movement, combat, and outcomes.
- `input.*`: SDL event translation and pointer gestures.
- `draw.*`: battles, specialist silhouettes, troops, and interpolation.
- `draw_campaign.*`: world/level maps and two-sided wipe transitions.
- `debug.*`: optional ImGui inspection and rule tuning.
- `main.cpp`: SDL ownership and the fixed-step loop.
