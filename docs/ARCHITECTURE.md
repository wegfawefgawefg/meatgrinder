# Architecture

Meatgrinder follows the direct architecture used by Adventures with Chickens
Remastered: plain state, authored data loaded up front, one visible fixed-step
loop, and domain functions that mutate the state they receive.

`State` owns the current mode, campaign position, input, options, and mutable
match. `Level` is immutable authored content. `Match` is created from a level
and contains changing garrisons, moving armies, AI clocks, and statistics.
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

Modes are explicit: main menu, options, level card, playing, pause, result,
score, and campaign complete. Transitions are direct assignments with a reset
mode clock. ImGui is a developer overlay only; shipping screens are drawn by the
game renderer.

Files remain split by concrete domain:

- `level.*`: JSON decoding and authored validation.
- `camera.*`: tile-world framing, manual movement, zoom, and interpolation.
- `ai.*`: weighted enemy decisions, soft objectives, threats, and supply movement.
- `game.*`: campaign/match creation, commands, movement, combat, and outcomes.
- `input.*`: SDL event translation and pointer gestures.
- `draw.*`: tiles, routes, nodes, troops, screens, and interpolation.
- `debug.*`: optional ImGui inspection and rule tuning.
- `main.cpp`: SDL ownership and the fixed-step loop.
