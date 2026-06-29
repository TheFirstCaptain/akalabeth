# Porting Strategy

## Sequence

1. Preserve and characterize the source listing structure.
2. Build source-inspection helpers that can identify line ranges, tokens, and references without rewriting the listing.
3. Extract small portable rules with tests, starting with deterministic data tables and simple state transitions.
4. Add adapters for Applesoft-compatible random behavior, input commands, and render commands.
5. Build a minimal modern shell only after portable core contracts exist.

## Early Extraction Candidates

- Static names and labels: attributes, weapons, monsters, and prices.
- Store purchase rules.
- Overworld movement and mountain blocking.
- Dungeon turn left/right/around and forward movement.
- Trap, ladder, chest, and Lord British quest state transitions.
- Combat hit/miss predicates and damage calculations.

## Deferred Work

- Final UI technology choice.
- Full Apple II BASIC interpreter or emulator integration.
- Pixel-perfect rendering.
- Save-game design, since the historical notes identify save as missing.

