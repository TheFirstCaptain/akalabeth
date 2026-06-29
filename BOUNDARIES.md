# Implementation Boundary Checklist

Use this checklist before implementing or reviewing modern core, harness, adapter, or app-shell changes.

## Reference Boundary

- `AKLABETH.TXT` remains the working reference listing.
- `AKLABETH-org.TXT` may be used to cross-check source preservation and archive wrapping.
- Historical screenshots are visual references, not implementation assets for the portable core.

## Core Boundary

- `Core/` public headers must not expose AppKit, SwiftUI, SDL, Metal, AVFoundation, Foundation filesystem, browser, terminal UI, or other platform framework types.
- Portable game rules must not call macOS UI, graphics, audio, filesystem, application lifecycle, or Apple II emulator APIs directly.
- Core-facing APIs should use portable C data, result structs, command buffers, or explicit state transitions.
- Apple II concepts may be modeled as data, but direct Apple II screen, keyboard, and memory operations should remain behind adapters or test fixtures.

## Adapter Boundary

- Renderer, input, audio, persistence, random-number, and source-loading implementations should remain behind adapters.
- Platform adapters should translate modern app events and services into core-facing interfaces.
- A future app shell may own bundle paths, save locations, windows, menus, and event intake, but portable rules and serialization should stay outside the shell.

## Pre-Review Checks

Run these checks before final review when a change touches `Core/`, a modern shell, or adapters:

```bash
rg -n "AppKit|SwiftUI|Metal|AVFoundation|Foundation|SDL|NS[A-Z][A-Za-z]+\\b|NSURL|Sandbox" Core
make -C harness test
```

If a match in `Core/` is intentional, document why it does not violate the portable-core boundary before handoff.

