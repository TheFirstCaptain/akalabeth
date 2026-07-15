# RND Call Sites

This document characterizes every observed `RND` use in `AKLABETH.TXT`. The portable adapter in `Core/include/ak_random.h` preserves the command contract needed by extracted rules: negative arguments reseed, zero repeats the last generated value, and positive arguments advance the sequence.

The current adapter is deterministic and source-shaped, but not bit-for-bit proven identical to Applesoft BASIC. F-014 locks representative startup, overworld, dungeon, combat, loot, and quest fixtures against the current portable contract. Exact Apple II numeric compatibility remains an external validation task until an execution reference is available.

## Source Uses

| Lines | Expression Shape | Behavior Area | Adapter Use |
| --- | --- | --- | --- |
| `8`, `60010` | `RND(-ABS(LN))` | lucky-number seeding | `ak_random_rnd(random, -abs_lucky_number)` |
| `40` | `INT(RND(1) ^ 5 * 4.5)` | overworld terrain generation | raw `ak_random_rnd` plus extracted terrain expression |
| `41` | `RND(1) > .5` | overworld terrain thinning | `ak_random_chance_greater_than(random, .5)` |
| `50` | `INT(RND(1) * 19 + 1)` | castle/town placement and start position | `ak_random_int(random, 19.0, 1.0)` |
| `500` | `RND(-ABS(LN) - TX * 10 - TY * 1000 + INOUT * 31.4)` | dungeon seeding | `ak_random_rnd(random, dungeon_seed_expression)` |
| `540-549` | `RND(1) > threshold` | dungeon cell features | `ak_random_chance_greater_than` |
| `1160` | `INT(RND(1) * INOUT + 3)` | trap damage | `ak_random_int(random, INOUT, 3.0)` |
| `1170` | `INT(RND(1) * 5 * INOUT + INOUT)` | chest gold | `ak_random_int(random, 5 * INOUT, INOUT)` |
| `1175` | `INT(RND(1) * 6)` | chest item index | `ak_random_int(random, 6.0, 0.0)` |
| `1662` | `RND(1) * 25` | player hit/miss | raw `ak_random_rnd` in combat predicate |
| `1663` | `RND(1) * DAM + C(1) / 5` | player damage | raw `ak_random_rnd` in damage expression |
| `1681` | `INT(RND(1) * 4 + 1)` | fighter amulet random choice | `ak_random_int(random, 4.0, 1.0)` |
| `1683` | `RND(1) > .75` | amulet charge loss | `ak_random_chance_greater_than(random, .75)` |
| `1692` | `INT(RND(1) * 3 + 1)` | bad amulet outcome | `ak_random_int(random, 3.0, 1.0)` |
| `2010` | `RND(1) > .4` | monster placement chance | `ak_random_chance_greater_than(random, .4)` |
| `2020` | `INT(RND(1) * 9 + 1)` | monster coordinates | `ak_random_int(random, 9.0, 1.0)` |
| `4510` | `RND(1) * 20` | monster attack hit/miss | raw `ak_random_rnd` in combat predicate |
| `4520` | `INT(RND(1) * MM + IN)` | monster damage | `ak_random_int(random, MM, IN)` |
| `4600` | `RND(1) < .5` | thief/gremlin special branch | raw `ak_random_rnd` or inverted chance helper |
| `4620` | `INT(RND(1) * 6)` | thief stolen item index | `ak_random_int(random, 6.0, 0.0)` |
| `60050` | `INT(SQR(RND(1)) * 21 + 4)` | character qualities | raw `ak_random_rnd` plus extracted stat expression |

## Adapter Contract

- `ak_random_rnd(random, negative_value)` reseeds from the numeric argument and returns the first value from the new sequence.
- `ak_random_rnd(random, 0.0)` returns the previous value without advancing.
- `ak_random_rnd(random, positive_value)` advances and returns the next value. The magnitude of positive values is ignored, matching the way Akalabeth uses `RND(1)`.
- `ak_random_int` implements the positive `INT(RND(1) * multiplier + offset)` shape used by placement, loot, amulet, and damage rules.
- `ak_random_chance_greater_than` implements the common `RND(1) > threshold` shape.
- Harness coverage pins the default sequence, integer negative seeds, zero-repeat behavior, and the fractional dungeon seed shape `-ABS(LN) - TX * 10 - TY * 1000 + INOUT * 31.4`.

## Open Questions

- The adapter has not been validated against a real Applesoft BASIC runtime, so generated maps and combat results are portable golden fixtures rather than Apple II bit-exact reference outputs.
- Applesoft floating-point precision and `RND` seed encoding may differ from the current deterministic seed mixer, especially for fractional dungeon seeds.
- `INT(SQR(RND(1)) * 21 + 4)` intentionally remains a game-rule expression rather than an RNG helper because it belongs to character generation, not the random source itself.
