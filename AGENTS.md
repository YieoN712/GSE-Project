# GSE Project Instructions

## Project identity

This repository is a single-player, open-world RPG presented as a quarter-view 2.5D game. Preserve these pillars unless the user explicitly changes them:

- Single-player narrative experience.
- Dense open world built around familiar places and recurring NPC routines.
- Quarter-view 2.5D presentation.
- Modern everyday setting with a hidden supernatural layer.
- A bright, welcoming surface with brief signs of darkness underneath it.
- A protagonist who begins with ordinary life, gradually gains a personal purpose, and discovers the value of everyday relationships and moments.
- Central theme: an ordinary day is precious because the people, places, and memories we care about are still present.

Do not silently turn the project into a multiplayer game, a mission-only linear RPG, a conventional heroic world-saving fantasy, a persistently bleak horror game, or a full third-person 3D game.

## Canon and design decisions

Before making gameplay, narrative, world, quest, art-direction, camera, or architecture decisions, read [docs/GAME_DEVELOPMENT_GUIDE.md](docs/GAME_DEVELOPMENT_GUIDE.md). Treat its `Fixed pillars` as requirements and its working names/details as editable canon.

When a new request conflicts with the guide, follow the user's latest explicit instruction and update the guide as part of the same change when appropriate. Do not change a fixed pillar merely because an implementation shortcut is easier.

## Development priorities

Use this order when tradeoffs are necessary:

1. Preserve the emotional theme and the player's attachment to everyday life.
2. Keep story, NPC routines, exploration, and game systems connected.
3. Favor a dense, changing world over empty map size.
4. Preserve readable quarter-view gameplay and coherent 2.5D depth.
5. Build maintainable foundations that can grow beyond the current rendering prototype.
6. Optimize only after measuring, except for architectural choices that would obviously block an open world.

## Narrative and content rules

- Let darkness appear through absence, repetition, altered memories, and subtle inconsistencies before using overt horror.
- Establish affection for NPCs and locations before threatening or removing them.
- Keep the protagonist's initial motivation personal. Expand it gradually from one missing person to the wider city.
- Avoid a simple good-versus-evil explanation. The memory-removal system should provide real comfort while exacting an unacceptable cost.
- Avoid a single morality meter. Track concrete actions: time spent, promises kept, people remembered, and places revisited.
- Make ordinary activities meaningful to later exploration, combat, relationships, or endings.
- Keep companions as authored NPCs with their own schedules and agency; they are not substitutes for other players.

## Technical direction

- The current code is an early C++/OpenGL/FreeGLUT/GLEW rendering prototype, not the final game architecture.
- Treat `DrawSolidRect` as prototype functionality. Do not build the full RPG directly around its pixel-coordinate API.
- Default world convention: `X-Z` is the ground plane and `Y` is height.
- Prefer a quarter-view camera, depth-tested world rendering, world/screen coordinate conversion, picking, scalable entity ownership, and streamed or partitioned world data.
- Keep simulation state independent from rendering state so NPC schedules, quests, saves, and world changes can be tested without drawing a frame.
- Use time-based updates rather than frame-dependent game logic.
- Keep authored world coordinates separate from navigation grids and screen-space UI coordinates.
- Account for window resizing, resource lifetime, shader errors, asset paths, and clean-clone dependency reproducibility.
- Do not add online services, networking, accounts, or multiplayer synchronization without an explicit user request.

## Working method

- Format project-owned C++ and shader code using the root `.clang-format`: four spaces, Allman braces, one statement per line, expanded short functions/conditionals, and blank lines between definitions and logical steps.
- Keep the existing naming conventions when doing formatting-only work. Preserve include order (especially `stdafx.h`), string contents, and file encoding; exclude bundled `SimpleGame/Dependencies` headers.
- Build and interactive result checks are normally performed by the user. Do not automatically build or launch the game unless requested or genuinely required by an exceptional task.

- Inspect the relevant current code and the design guide before implementing a feature.
- State assumptions when a request leaves a design choice open, then choose the option that best preserves the pillars.
- Keep documentation synchronized when a change establishes or revises canon, controls, save format, world conventions, or a major system contract.
- Verify changes in proportion to risk. The user retains responsibility for final interactive gameplay and visual acceptance testing.
