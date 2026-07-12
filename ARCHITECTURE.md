# Architecture

This document records the observed Akalabeth architecture and the target modernization boundaries.

## Legacy Source

- `AKLABETH.TXT` is the current working Applesoft BASIC listing.
- `AKLABETH-org.TXT` appears to be an archive-preserved wrapped version of the listing.
- The program is organized by BASIC line-number ranges rather than modules.
- Apple II platform behavior is embedded directly in game flow through graphics, keyboard, screen, memory, and random-number commands.

## Observed Line Ranges

| Range | Responsibility |
| --- | --- |
| `0-99` | startup, world initialization, perspective geometry tables |
| `100-190` | overworld view drawing |
| `200-490` | dungeon view drawing and monster/chest drawing |
| `500-590` | dungeon generation |
| `1000-1700` | command loop, movement, dungeon entry/exit, attacks, magic amulet |
| `2000-2090` | monster placement |
| `3087-3089` | Balrog drawing continuation |
| `4000-4999` | monster movement and monster attacks |
| `6000-6060` | death screen |
| `7000-7990` | Lord British quest flow and win condition |
| `60000-60250` | startup prompts, character generation, store/status screen |

## Target Boundaries

- Portable core: source parsing helpers, deterministic game-state rules, generation, movement, combat, store, quest, and render-command contracts.
- Adapters: Apple II-compatible random-number behavior, keyboard command mapping, rendering backend, persistence, and asset/screenshot reference loading.
- App shell: future platform lifecycle, windows, menus, save locations, and user-facing settings.

## Current Harness

- `Core/include/ak_basic_listing.h` and `Core/src/ak_basic_listing.c` provide a tiny parser for line-numbered BASIC listings.
- `Core/include/ak_game.h` and `Core/src/ak_game.c` provide the portable state, command, result, and event model for extracted startup, store, overworld, dungeon, combat, magic, death, and quest rules.
- `Core/include/ak_random.h` and `Core/src/ak_random.c` provide a deterministic portable `RND` adapter for future generated-map, dungeon, combat, and loot rules.
- `Core/include/ak_render.h` and `Core/src/ak_render.c` provide a portable Apple II-style render command contract for text, high-resolution line art, status panels, prompts, tiles, and creature sprites.
- `harness/basic_listing_tests.c` characterizes high-value structural facts about `AKLABETH.TXT` and `AKLABETH-org.TXT`.
- `harness/game_model_tests.c` validates deterministic game-state initialization and command/result behavior before detailed Applesoft rules are extracted.
- `harness/random_tests.c` validates the portable random adapter contract and representative source expression shapes.
- `harness/render_tests.c` validates representative render command streams for startup, town, overworld, dungeon, quest, and victory screens.
