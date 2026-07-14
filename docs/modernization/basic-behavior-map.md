# BASIC Behavior Map

This map records the source-backed behavior boundaries that should be characterized before extracting portable rules from `AKLABETH.TXT`. It is intentionally factual: line references name observed source responsibilities, likely inputs and outputs, Apple II dependencies, and extraction readiness.

## Status Terms

- Ready for fixture extraction: enough source anchors exist to write deterministic harness checks before implementation.
- Needs characterization: source intent is visible, but exact state transitions or data dependencies need fixtures first.
- Adapter blocked: behavior depends directly on Apple II screen, keyboard, memory, or random-number semantics that must be isolated before rule extraction.

## Behavior Areas

| Area | BASIC Lines | Source Anchors | Inputs And State | Outputs | Apple II Dependencies | Extraction Status |
| --- | --- | --- | --- | --- | --- | --- |
| Startup, world initialization, and perspective geometry | `0-99` | `RND ( - ABS (LN))`, `DIM DN%`, `TE%`, `PER%`, `LD%`, `FT%`, `LAD%`, welcome text | `LN`, `LP`, terrain arrays, perspective arrays, `TX`, `TY` | world map, castle/town/dungeon placement, geometry tables, initial overworld draw | `TEXT`, `HOME`, `VTAB`, `POKE`, `HCOLOR`, `RND` | Needs characterization |
| Overworld view drawing | `100-190` | `HGR`, 3x3 loops around `TX`,`TY`, `HPLOT` tile drawings | `TER%`/`TE%`, `TX`, `TY` | Apple II high-resolution line drawing for local overworld view | `HGR`, `HPLOT` | Adapter blocked |
| Dungeon view drawing and encounter art | `200-490`, `3087-3089` | `DNG%`, `PER%`, `CD%`, `LD%`, `FT%`, monster `ON MC GOTO`, Balrog continuation | `PX`, `PY`, `DX`, `DY`, `DNG%`, `M$`, monster health/location arrays | Apple II high-resolution first-person dungeon view, chest/monster labels | `HGR`, `HCOLOR`, `HPLOT`, `INVERSE`, `NORMAL`, `CALL -868` | Adapter blocked |
| Dungeon generation | `500-590` | seeded `RND`, `DNG%` boundaries, traps, secret doors, ladders, chests, `GOSUB 2000` | `LN`, `TX`, `TY`, `INOUT`, dungeon arrays | deterministic dungeon grid and monster placement call | `RND` | Ready for fixture extraction after RNG characterization |
| Command loop, movement, entry, food, and status refresh | `1000-1587` | `COMMAND?`, keyboard codes, food decrement, starvation, mountain blocking, town/dungeon/castle entry, ladder transitions | keyboard byte, `INOUT`, `TX`, `TY`, `PX`, `PY`, `DX`, `DY`, `PW`, `C`, `LK` | movement, turns, location transitions, status text, death transition, redraw requests | `PEEK`, `POKE`, `CALL -868`, `VTAB`, `HTAB` | Needs characterization |
| Player attack and magic | `1650-1697` | weapon prompt, weapon restrictions, hit/miss predicate, amulet choices, toad/lizard/backfire | `PT$`, `PW`, `C`, `DNG%`, `MZ%`, `ML%`, `TASK`, `INOUT`, `IN` | monster damage/death, gold reward, quest progress, inventory changes, player stat changes | `GET`, `RND`, pause prompt | Needs characterization |
| Monster placement | `2000-2090` | `MZ%`, `ML%`, `DNG%`, `X - 2 > INO`, random empty-cell placement | `INOUT`/`INO`, `IN`, `LP`, `PX`, `PY`, `DNG%` | active monster flags, locations, hit points, dungeon cell markers | `RND` | Needs characterization |
| Monster turns and monster attacks | `4000-4999` | range checks, pursuit/flee movement, thief/gremlin special behavior, attack hit predicate | `MZ%`, `ML%`, `PX`, `PY`, `PW`, `C`, `IN`, `LP`, `DNG%` | monster movement, player damage, stolen food/items, status messages | `RND`, pause prompt | Needs characterization |
| Death screen and resurrection loop | `6000-6060` | memorial text, name normalization, ESC resurrection to line `1` | `PN$`, keyboard byte | death text, optional restart path | `POKE`, `HTAB`, `PEEK` | Adapter blocked |
| Lord British quests and win condition | `7000-7990` | name prompt, adventure consent, `TASK`, attribute increase, quest completion, `LORD` title, final phone message | `PN$`, `TASK`, `C`, `M$`, `LP` | quest assignment, quest escalation, victory text, stat increases | `TEXT`, `HOME`, `CALL 62450`, `GET` | Ready for fixture extraction |
| Startup prompts, character generation, store, and status screen | `60000-60250` | lucky number, level validation, stat/monster/item data tables, `SQR(RND(1))`, class choice, prices, mage restrictions | `LN`, `LP`, `C`, `PW`, `PT$`, `C$`, `M$`, `W$`, purchase key | character attributes, class, inventory, gold changes, store/status text | `TEXT`, `HOME`, `VTAB`, `HTAB`, `GET`, `CALL 62450`, `RND` | Ready for fixture extraction after RNG characterization |

## Extraction Fixtures

| Fixture | Source Lines | Purpose | Minimum Checks |
| --- | --- | --- | --- |
| source anchors | all major ranges | Keep behavior-map references tied to actual listing lines | `harness/basic_listing_tests.c` verifies key line numbers and strings |
| data tables | `60020`, `60042`, `60070`, `60110-60120`, `60220-60225` | Preserve stats, monsters, items, prices, and damage labels | table names, ordering, prices, mage-restricted items |
| seeded startup | `60000-60075`, `0-99` | Characterize lucky number, level, stats, world placement | fixed `LN` and `LP` produce repeatable stats and terrain once RNG is modeled |
| dungeon generation | `500-590`, `2000-2090` | Characterize dungeon cell semantics and monster placement | boundaries, ladders, traps, chests, active monster slots |
| command transitions | `1000-1587` | Characterize movement, turns, entry/exit, starvation, and status updates | command byte to state transition with deterministic pre-state |
| combat and quest | `1650-1697`, `4000-4999`, `7000-7990` | Characterize combat, loot, quest target changes, and win condition | hit/miss branches, kill reward, `TASK` sign flip, quest escalation |
| rendering contract | `100-490`, `3087-3089`, `60080-60130`, `7000-7990` | Convert direct Apple II drawing/text to portable render commands | screen mode, text positions, line drawing commands, labels |

## Open Questions

- Several source identifiers appear inconsistent or mistyped in the listing, including `TER%` versus `TE%`, `PE%` versus `PER%`, `LEF`/`RIG`, `DNG` versus `DNG%`, `DI` versus `DIS`, `IN` versus `INOUT`, and `INO` versus `INOUT`. Treat these as listing ambiguities until characterized against an execution reference.
- Exact Applesoft numeric behavior is required before seeded fixtures can assert full generated maps, dungeon layouts, or combat results.
- `CALL 62450`, `CALL -868`, `POKE 33/34/35`, `PR#`, `IN#`, and keyboard `PEEK` behavior must remain adapter concerns.
- `README.1ST` describes no save system, so persistence belongs to a later Mac-specific feature rather than historical behavior preservation.
