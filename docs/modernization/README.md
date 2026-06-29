# Modernization Docs

This folder contains working documents for converting the Akalabeth Applesoft BASIC listing into a modern, portable implementation.

## Documents

- `legacy-inventory.md`: source responsibilities, platform dependencies, data formats, and asset inventory.
- `basic-behavior-map.md`: source-backed behavior boundaries, fixture targets, dependencies, and extraction readiness.
- `rnd-call-sites.md`: `RND` call-site inventory, adapter contract, and exact-compatibility open questions.
- `porting-strategy.md`: sequencing notes and target modernization boundaries.
- `validation-strategy.md`: how behavior preservation and intentional changes will be verified.

## Working Rules

- Keep findings factual and tied to inspected files or line numbers.
- State uncertainty when behavior is inferred.
- Prefer concise tables and references over long narrative notes.
- Do not record feature status here; use `docs/features/FEATURE_TRACKER.md`.
- Do not record defects here; use `docs/bugs/BUG_TRACKER.md`.
