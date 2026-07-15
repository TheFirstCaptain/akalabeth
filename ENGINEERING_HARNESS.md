# Engineering Harness

This document is the operating guide for modernizing this repository. The goal is to preserve observable Akalabeth behavior while moving the Applesoft BASIC listing and historical assets toward a modern, testable implementation in small steps.

## Current Reality

- The repository currently contains the Akalabeth Applesoft BASIC listing, historical notes, and GIF screenshots.
- `AKLABETH.TXT` is the current working source listing.
- There is a runnable native macOS AppKit shell built through SwiftPM.
- There is no full BASIC interpreter harness yet.
- The listing uses Apple II-specific graphics, keyboard, memory, and screen commands such as `HGR`, `HPLOT`, `PEEK`, `POKE`, `CALL`, `VTAB`, and `HTAB`.

## Modernization Principles

- Characterize behavior before rewriting it.
- Treat the BASIC listing and screenshots as the reference until a replacement is proven.
- Separate portable game rules from Apple II concerns such as graphics, keyboard input, screen layout, memory calls, and random-number behavior.
- Keep new code in clearly named harness and core directories rather than mixing it into historical files.
- Make changes narrow, reversible, and tied to a named porting objective.
- Preserve original behavior unless an intentional change is documented.
- State uncertainty when behavior is inferred from source text rather than executed.

## Working Boundaries

- `AKLABETH.TXT` is the historical source/reference listing. Avoid formatting churn or direct edits unless the change is explicitly about source preservation.
- GIF files, `README.md`, `RPG.TXT`, and `udic.txt` are historical assets and notes. Modify them only for a documented archival reason.
- `Core/` is for portable extracted behavior and source-inspection helpers.
- `harness/` is for command-line characterization tests that run on a modern compiler.
- `docs/` is for modernization inventory, feature planning, bugs, and validation notes.

## Project Documentation

- `ARCHITECTURE.md`: observed legacy architecture and target modernization boundaries.
- `BOUNDARIES.md`: quick checklist for keeping portable code separate from platform code.
- `DECISION_LOG.md`: durable architecture, tooling, workflow, compatibility, and behavior decisions.
- `AGENT_TASK_TEMPLATE.md`: standard shape for future agent tasks.
- `docs/modernization/`: legacy inventory, porting strategy, and validation strategy.
- `docs/features/`: feature templates, feature tracker, and feature-specific planning docs.
- `docs/bugs/`: bug templates, bug tracker, and defect or blocker docs.

## Validation Loop

Early modernization work may not have a runnable game. Use the strongest practical validation for the task:

```bash
make -C harness test
```

As behavior is extracted from the BASIC listing, add focused harness tests before changing implementation logic. For UI, rendering, audio, input, or gameplay changes, include manual verification notes with exact steps.

## Development Workflow

1. Read the relevant BASIC lines, screenshots, and harness docs before editing.
2. Identify the affected behavior and boundary: inventory, characterization, extraction, adapter, replacement, or modern UI work.
3. Before starting feature or milestone work, identify durable decisions that should be made first. Record accepted decisions before implementation; if no decision is needed, state that and proceed.
4. Prefer characterization tests or small harness executables before changing game logic.
5. Implement the smallest coherent change.
6. Run applicable validation and record skipped checks.
7. Update feature, bug, modernization, architecture, or decision documentation when the work changes status, boundaries, or strategy.

## Initial Inventory Targets

- Startup, character creation, and store flow.
- Overworld generation, tiles, movement, and location entry.
- Dungeon generation, navigation, traps, ladders, chests, and monster placement.
- Combat, weapon rules, magic amulet behavior, monster AI, theft, food loss, and death.
- Lord British quest progression and win condition.
- Apple II graphics, text layout, keyboard, memory calls, and random-number behavior.
- Historical screenshots and how they map to source behavior.

## Definition of Done

- Scope and affected legacy behavior are clear.
- Behavior preservation or intentional behavior change is documented.
- Relevant harness tests, fixtures, or manual checks are added where practical.
- Validation has passed, or failures and skipped checks are explicitly recorded.
- Historical source and assets are not changed without a documented reason.
- New assumptions or durable decisions are captured in project documentation.
