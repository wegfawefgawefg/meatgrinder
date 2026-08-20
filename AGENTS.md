# Meatgrinder contributor guide

Keep the game direct and inspectable. Use C++20 plain structs, enums, vectors,
fixed arrays where appropriate, and free functions operating on visible state.
Prefer small domain files over framework layers. Do not introduce an ECS,
service locator, event bus, scene graph, inheritance hierarchy, or generic
utility package without a demonstrated need in the current game.

Authored `Level` data is immutable after loading. Mutable playthrough data lives
in `Match` and `State`. Simulation runs only in fixed 60 Hz steps; floats used
for drawing may interpolate completed states but must not feed decisions back
into the simulation.

Comments are short paragraphs describing what a section is doing. Keep rule
constants beside the rules they control or in the small `Rules` struct exposed
to developer tools. ImGui is for inspection and tuning, not shipping menu or
game-state ownership.

Run `./scripts/test.sh` before committing gameplay changes. Never add source
IPA files, extracted HRR assets, dialogue, audio, or executable material to this
repository. Level imports stay limited to the provenance-tracked topology fields
documented in `docs/LEVELS.md`.
