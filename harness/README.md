# Modernization Harness

This directory contains command-line characterization checks for bounded Akalabeth source behavior. It is intentionally separate from the historical BASIC listing and assets so early modernization work does not require an Apple II emulator, BASIC interpreter, or modern app shell.

## Commands

```bash
make -C harness test
```

## Current Scope

- `basic_listing_tests.c` characterizes structural facts about `AKLABETH.TXT` and `AKLABETH-org.TXT`: parsed logical line counts, important line numbers, key data tables, and major line ranges.

These harnesses compile small portable implementations from `Core/src/` and keep tests tied to source line numbers. This keeps validation executable on a modern compiler while avoiding broad platform shims for Apple II-specific behavior.

